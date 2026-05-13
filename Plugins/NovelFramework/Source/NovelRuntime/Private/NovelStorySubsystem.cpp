// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelStorySubsystem.h"
#include "NovelLog.h"
#include "NovelActionContext.h"
#include "FDialogueNode.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "NovelLoadingSubsystem.h"
#include "NovelIntent_Jump.h"
#include "NovelSaveGame.h"
#include "NovelStoryAsset.h"
#include "NovelRuntimeSettings.h"
#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UNovelStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SetRuntimeState(ENovelRuntimeState::Idle, TEXT("Initialize"));
    InitializeSaveIndex();
}

void UNovelStorySubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(IntentTimeoutTimerHandle);
        World->GetTimerManager().ClearTimer(FadeFallbackTimerHandle);
    }

    if (CurrentBGMLoadHandle.IsValid())
    {
        CurrentBGMLoadHandle->CancelHandle();
        CurrentBGMLoadHandle.Reset();
    }

    ClearIntentExecutionState();
    PendingLoadedSave = nullptr;
    SaveIndex = nullptr;
    Super::Deinitialize();
}

bool UNovelStorySubsystem::SetRuntimeState(ENovelRuntimeState NewState, const TCHAR* Reason)
{
    if (RuntimeState == NewState)
    {
        return false;
    }

    const ENovelRuntimeState PreviousState = RuntimeState;
    RuntimeState = NewState;

    if (NewState != ENovelRuntimeState::LoadingChapter &&
        NewState != ENovelRuntimeState::LoadingSave &&
        NewState != ENovelRuntimeState::Saving &&
        NewState != ENovelRuntimeState::Error)
    {
        LastRecoverableState = NewState;
    }

    UE_LOG(LogNovel, Verbose, TEXT("RuntimeState: %s -> %s (%s)"), *RuntimeStateToString(PreviousState), *RuntimeStateToString(NewState), Reason ? Reason : TEXT("No reason"));
    OnRuntimeStateChangedEvent.Broadcast(NewState, PreviousState);
    return true;
}

void UNovelStorySubsystem::RestoreRuntimeStateAfterAsyncFailure(ENovelRuntimeState PreviousState, const FText& ErrorMessage)
{
    const ENovelRuntimeState RestoreState = PreviousState == ENovelRuntimeState::Saving ||
        PreviousState == ENovelRuntimeState::LoadingSave ||
        PreviousState == ENovelRuntimeState::LoadingChapter
        ? LastRecoverableState
        : PreviousState;

    SetRuntimeState(RestoreState, TEXT("Async failure recovery"));
    OnStoryErrorEvent.Broadcast(ErrorMessage);
}

void UNovelStorySubsystem::ReportError(const FString& Operation, const FText& ErrorMessage, ENovelRuntimeState ErrorState)
{
    UE_LOG(LogNovel, Error, TEXT("%s failed. Chapter=%d Node=%s State=%s Reason=%s"),
        *Operation,
        CurrentChapterID,
        *CurrentNode.RowName.ToString(),
        *RuntimeStateToString(RuntimeState),
        *ErrorMessage.ToString());

    SetRuntimeState(ErrorState, TEXT("ReportError"));
    OnStoryErrorEvent.Broadcast(ErrorMessage);
}

bool UNovelStorySubsystem::CanStartBlockingOperation(const TCHAR* Operation) const
{
    const bool bBlocked = RuntimeState == ENovelRuntimeState::LoadingChapter ||
        RuntimeState == ENovelRuntimeState::LoadingSave ||
        RuntimeState == ENovelRuntimeState::Saving;

    if (bBlocked)
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected %s. Expected non-blocking state, actual state=%s"), Operation, *RuntimeStateToString(RuntimeState));
        return false;
    }

    return true;
}

void UNovelStorySubsystem::RejectCommand(const TCHAR* Operation, ENovelRuntimeState ExpectedState) const
{
    UE_LOG(LogNovel, Verbose, TEXT("Rejected %s. Expected state=%s actual state=%s Chapter=%d Node=%s"),
        Operation,
        *RuntimeStateToString(ExpectedState),
        *RuntimeStateToString(RuntimeState),
        CurrentChapterID,
        *CurrentNode.RowName.ToString());
}

FString UNovelStorySubsystem::RuntimeStateToString(ENovelRuntimeState State) const
{
    switch (State)
    {
    case ENovelRuntimeState::Idle: return TEXT("Idle");
    case ENovelRuntimeState::LoadingChapter: return TEXT("LoadingChapter");
    case ENovelRuntimeState::PresentingDialogue: return TEXT("PresentingDialogue");
    case ENovelRuntimeState::ExecutingIntents: return TEXT("ExecutingIntents");
    case ENovelRuntimeState::AwaitingAdvance: return TEXT("AwaitingAdvance");
    case ENovelRuntimeState::AwaitingChoice: return TEXT("AwaitingChoice");
    case ENovelRuntimeState::Saving: return TEXT("Saving");
    case ENovelRuntimeState::LoadingSave: return TEXT("LoadingSave");
    case ENovelRuntimeState::Error: return TEXT("Error");
    default: return TEXT("Unknown");
    }
}

void UNovelStorySubsystem::StartStory()
{
    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    if (Settings && !Settings->EntryStory.IsNull())
    {
        StartStoryAsset(Settings->EntryStory);
        return;
    }

    StartNewGame();
}

void UNovelStorySubsystem::StartStoryAsset(TSoftObjectPtr<UNovelStoryAsset> StoryAsset)
{
    if (!CanStartBlockingOperation(TEXT("StartStoryAsset")))
    {
        return;
    }
    if (StoryAsset.IsNull())
    {
        ReportError(TEXT("StartStoryAsset"), FText::FromString(TEXT("Story asset is null.")));
        return;
    }

    const ENovelRuntimeState PreviousState = RuntimeState;
    ++StorySessionId;
    ++ChapterLoadSerial;
    PendingLoadedSave = nullptr;
    ActiveStoryAsset = nullptr;
    ActiveChapterAsset = nullptr;
    ActiveDialogueTable = nullptr;
    BranchData = nullptr;
    ActiveNodes.Empty();
    ActiveChapterId = FPrimaryAssetId();
    ActiveEntryNodeId = NAME_None;
    CurrentChapterID = INDEX_NONE;
    CurrentNode = FDialogueNodeHandle();
    CurrentNodeRef = FNovelNodeRef();
    LastPublishedNode = FDialogueNodeHandle();
    DialogueHistory.Empty();
    ClearVariables();
    NextHistorySequenceIndex = 0;
    VisibleCharacters.Empty();
    ClearIntentExecutionState();
    StopBGM(0.0f);
    ResetVisualState();

    if (!BeginStoryAssetLoad(StoryAsset, PreviousState))
    {
        ReportError(TEXT("StartStoryAsset"), FText::FromString(FString::Printf(TEXT("Could not request story asset %s."), *StoryAsset.ToString())));
    }
}

void UNovelStorySubsystem::StopStory()
{
    if (RuntimeState == ENovelRuntimeState::Saving || RuntimeState == ENovelRuntimeState::LoadingSave)
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected StopStory while persistence is active. State=%s"), *RuntimeStateToString(RuntimeState));
        return;
    }

    ++StorySessionId;
    ++ChapterLoadSerial;

    if (PendingLoadingSubsystemRequestId != INDEX_NONE)
    {
        if (UNovelLoadingSubsystem* LoadingSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelLoadingSubsystem>() : nullptr)
        {
            LoadingSubsystem->CancelLoad(PendingLoadingSubsystemRequestId);
        }
        PendingLoadingSubsystemRequestId = INDEX_NONE;
    }

    PendingLoadedSave = nullptr;
    ClearIntentExecutionState();
    bHasPendingTransition = false;
    bPendingTransitionClearScreen = false;
    PendingTransitionNode = FDialogueNodeHandle();
    CurrentNode = FDialogueNodeHandle();
    LastPublishedNode = FDialogueNodeHandle();
    CurrentChapterID = INDEX_NONE;
    CurrentNodeRef = FNovelNodeRef();
    ActiveStoryAsset = nullptr;
    ActiveChapterAsset = nullptr;
    ActiveDialogueTable = nullptr;
    BranchData = nullptr;
    ActiveNodes.Empty();
    ActiveChapterId = FPrimaryAssetId();
    ActiveEntryNodeId = NAME_None;
    bHasPostIntentTransition = false;
    bPendingUnifiedClearScreen = false;
    PostIntentTransition = FNovelNodeRef();
    DialogueHistory.Empty();
    ClearVariables();
    NextHistorySequenceIndex = 0;
    StopBGM(0.0f);
    ResetVisualState();
    OnHistoryChangedEvent.Broadcast();
    OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
    SetRuntimeState(ENovelRuntimeState::Idle, TEXT("StopStory"));
}

void UNovelStorySubsystem::StartNewGame()
{
    if (!CanStartBlockingOperation(TEXT("StartNewGame")))
    {
        return;
    }

    ++StorySessionId;
    ++ChapterLoadSerial;
    PendingLoadedSave = nullptr;
    CurrentChapterID = INDEX_NONE;
    CurrentNode = FDialogueNodeHandle();
    LastPublishedNode = FDialogueNodeHandle();
    DialogueHistory.Empty();
    ClearVariables();
    NextHistorySequenceIndex = 0;
    BranchData = nullptr;
    ActiveStoryAsset = nullptr;
    ActiveChapterAsset = nullptr;
    ActiveDialogueTable = nullptr;
    ActiveNodes.Empty();
    ActiveChapterId = FPrimaryAssetId();
    ActiveEntryNodeId = NAME_None;
    CurrentNodeRef = FNovelNodeRef();
    StopBGM(0.0f);
    VisibleCharacters.Empty();
    ClearIntentExecutionState();
    ResetVisualState();

    if (!BeginChapterLoad(0, FDialogueNodeHandle(), false, RuntimeState))
    {
        ReportError(TEXT("StartNewGame"), FText::FromString(TEXT("Chapter 0 is not configured or could not be requested.")));
    }
}

void UNovelStorySubsystem::ContinueLastGame()
{
    if (!CanStartBlockingOperation(TEXT("ContinueLastGame")))
    {
        return;
    }

    TArray<FString> SaveSlots = GetAllSaveSlotNames();
    if (SaveSlots.IsEmpty())
    {
        const FText Message = FText::FromString(TEXT("No valid save slots were found."));
        UE_LOG(LogNovel, Warning, TEXT("ContinueLastGame failed. Reason=%s"), *Message.ToString());
        OnSaveLoadOperationFinishedEvent.Broadcast(false, FString(), Message);
        OnStoryErrorEvent.Broadcast(Message);
        return;
    }

    LoadGame(SaveSlots[0]);
}

void UNovelStorySubsystem::QuitGame()
{
    if (UWorld* World = GetWorld())
    {
        UKismetSystemLibrary::QuitGame(World, World->GetFirstPlayerController(), EQuitPreference::Quit, false);
    }
}
bool UNovelStorySubsystem::BeginStoryAssetLoad(TSoftObjectPtr<UNovelStoryAsset> StoryAsset, ENovelRuntimeState PreviousState)
{
    UNovelLoadingSubsystem* LoadingSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelLoadingSubsystem>() : nullptr;
    if (!LoadingSubsystem)
    {
        return false;
    }

    if (PendingLoadingSubsystemRequestId != INDEX_NONE)
    {
        LoadingSubsystem->CancelLoad(PendingLoadingSubsystemRequestId);
        PendingLoadingSubsystemRequestId = INDEX_NONE;
    }

    const int32 ExpectedSessionId = StorySessionId;
    const int32 ExpectedLoadSerial = ChapterLoadSerial;
    SetRuntimeState(ENovelRuntimeState::LoadingChapter, TEXT("BeginStoryAssetLoad"));
    OnStoryLoadingStateChangedEvent.Broadcast(true, FText::FromString(TEXT("Loading story")));
    PendingLoadingSubsystemRequestId = LoadingSubsystem->RequestLoad(
        { StoryAsset.ToSoftObjectPath() },
        FOnLoadCompleted::CreateUObject(this, &UNovelStorySubsystem::OnStoryAssetLoaded, StoryAsset, ExpectedSessionId, ExpectedLoadSerial, PreviousState));
    return true;
}

void UNovelStorySubsystem::OnStoryAssetLoaded(TSoftObjectPtr<UNovelStoryAsset> StoryAsset, int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState)
{
    if (ExpectedSessionId != StorySessionId || ExpectedLoadSerial != ChapterLoadSerial)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignoring stale story load. Story=%s"), *StoryAsset.ToString());
        return;
    }

    PendingLoadingSubsystemRequestId = INDEX_NONE;
    ActiveStoryAsset = StoryAsset.Get();
    if (!ActiveStoryAsset || !ActiveStoryAsset->EntryNode.IsValid())
    {
        OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
        RestoreRuntimeStateAfterAsyncFailure(PreviousState, FText::FromString(FString::Printf(TEXT("Story %s failed to load or has no valid entry node."), *StoryAsset.ToString())));
        return;
    }

    if (!BeginUnifiedChapterLoad(ActiveStoryAsset->EntryNode, PreviousState, true))
    {
        OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
        RestoreRuntimeStateAfterAsyncFailure(PreviousState, FText::FromString(FString::Printf(TEXT("Entry chapter %s could not be requested."), *ActiveStoryAsset->EntryNode.ChapterId.ToString())));
    }
}

bool UNovelStorySubsystem::BeginUnifiedChapterLoad(FNovelNodeRef TargetNode, ENovelRuntimeState PreviousState, bool bBroadcastStoryStarted)
{
    if (!ActiveStoryAsset || !TargetNode.IsValid())
    {
        return false;
    }

    TSoftObjectPtr<UNovelChapterAsset> ChapterAsset = ActiveStoryAsset->FindChapter(TargetNode.ChapterId);
    if (ChapterAsset.IsNull())
    {
        UE_LOG(LogNovel, Error, TEXT("Unified chapter load failed. Target=%s Reason=Chapter not in active story"), *TargetNode.ToString());
        return false;
    }

    UNovelLoadingSubsystem* LoadingSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelLoadingSubsystem>() : nullptr;
    if (!LoadingSubsystem)
    {
        return false;
    }

    if (PendingLoadingSubsystemRequestId != INDEX_NONE)
    {
        LoadingSubsystem->CancelLoad(PendingLoadingSubsystemRequestId);
        PendingLoadingSubsystemRequestId = INDEX_NONE;
    }

    const int32 ExpectedSessionId = StorySessionId;
    const int32 ExpectedLoadSerial = ChapterLoadSerial;
    SetRuntimeState(ENovelRuntimeState::LoadingChapter, TEXT("BeginUnifiedChapterLoad"));
    OnStoryLoadingStateChangedEvent.Broadcast(true, FText::FromString(FString::Printf(TEXT("Loading chapter %s"), *TargetNode.ChapterId.ToString())));
    PendingLoadingSubsystemRequestId = LoadingSubsystem->RequestLoad(
        { ChapterAsset.ToSoftObjectPath() },
        FOnLoadCompleted::CreateUObject(this, &UNovelStorySubsystem::OnUnifiedChapterLoaded, ChapterAsset, TargetNode, ExpectedSessionId, ExpectedLoadSerial, PreviousState, bBroadcastStoryStarted));
    return true;
}

void UNovelStorySubsystem::OnUnifiedChapterLoaded(TSoftObjectPtr<UNovelChapterAsset> ChapterAsset, FNovelNodeRef TargetNode, int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState, bool bBroadcastStoryStarted)
{
    if (ExpectedSessionId != StorySessionId || ExpectedLoadSerial != ChapterLoadSerial)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignoring stale unified chapter load. Target=%s"), *TargetNode.ToString());
        return;
    }

    PendingLoadingSubsystemRequestId = INDEX_NONE;
    OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());

    FText ErrorMessage;
    if (!RetainUnifiedChapter(ChapterAsset.Get(), ErrorMessage) || !FindActiveNode(TargetNode.NodeId))
    {
        if (ErrorMessage.IsEmpty())
        {
            ErrorMessage = FText::FromString(FString::Printf(TEXT("Target node %s is missing."), *TargetNode.ToString()));
        }
        RestoreRuntimeStateAfterAsyncFailure(PreviousState, ErrorMessage);
        return;
    }

    CurrentChapterID = INDEX_NONE;
    if (bPendingUnifiedClearScreen)
    {
        bPendingUnifiedClearScreen = false;
        ResetVisualState();
    }
    if (bBroadcastStoryStarted)
    {
        OnGameStartedEvent.Broadcast();
    }
    EnterNodeRef(TargetNode);
}

bool UNovelStorySubsystem::BeginChapterLoad(int32 ChapterIndex, FDialogueNodeHandle OptionalStartNode, bool bFromSave, ENovelRuntimeState PreviousState)
{
    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    const FChapterData* ChapterData = Settings ? Settings->ChapterMap.Find(ChapterIndex) : nullptr;
    if (!ChapterData || (ChapterData->ChapterAsset.IsNull() && ChapterData->DialogueTable.IsNull()))
    {
        UE_LOG(LogNovel, Error, TEXT("BeginChapterLoad failed. Chapter=%d Reason=Missing chapter asset and dialogue table"), ChapterIndex);
        return false;
    }

    UNovelLoadingSubsystem* LoadingSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelLoadingSubsystem>() : nullptr;
    if (!LoadingSubsystem)
    {
        UE_LOG(LogNovel, Error, TEXT("BeginChapterLoad failed. Chapter=%d Reason=Loading subsystem unavailable"), ChapterIndex);
        return false;
    }

    if (PendingLoadingSubsystemRequestId != INDEX_NONE)
    {
        LoadingSubsystem->CancelLoad(PendingLoadingSubsystemRequestId);
        PendingLoadingSubsystemRequestId = INDEX_NONE;
    }

    TArray<FSoftObjectPath> PathsToLoad;
    if (!ChapterData->ChapterAsset.IsNull())
    {
        PathsToLoad.Add(ChapterData->ChapterAsset.ToSoftObjectPath());
    }
    else
    {
        PathsToLoad.Add(ChapterData->DialogueTable.ToSoftObjectPath());
        if (!ChapterData->BranchData.IsNull())
        {
            PathsToLoad.Add(ChapterData->BranchData.ToSoftObjectPath());
        }
    }

    const int32 ExpectedSessionId = StorySessionId;
    const int32 ExpectedLoadSerial = ChapterLoadSerial;
    SetRuntimeState(bFromSave ? ENovelRuntimeState::LoadingSave : ENovelRuntimeState::LoadingChapter, TEXT("BeginChapterLoad"));
    OnStoryLoadingStateChangedEvent.Broadcast(true, FText::FromString(FString::Printf(TEXT("Loading chapter %d"), ChapterIndex)));
    PendingLoadingSubsystemRequestId = LoadingSubsystem->RequestLoad(
        PathsToLoad,
        FOnLoadCompleted::CreateUObject(this, &UNovelStorySubsystem::OnChapterLoadedAsync, ChapterIndex, OptionalStartNode, ExpectedSessionId, ExpectedLoadSerial, PreviousState, bFromSave));
    return true;
}

void UNovelStorySubsystem::OnChapterLoadedAsync(int32 ChapterIndex, FDialogueNodeHandle OptionalStartNode, int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState, bool bFromSave)
{
    if (ExpectedSessionId != StorySessionId || ExpectedLoadSerial != ChapterLoadSerial)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignoring stale chapter load. Chapter=%d"), ChapterIndex);
        return;
    }

    PendingLoadingSubsystemRequestId = INDEX_NONE;
    OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());

    FText ErrorMessage;
    if (!RetainLoadedChapter(ChapterIndex, ErrorMessage))
    {
        RestoreRuntimeStateAfterAsyncFailure(PreviousState, ErrorMessage);
        return;
    }

    FDialogueNodeHandle TargetNode = OptionalStartNode;
    if (TargetNode.RowName.IsNone())
    {
        TargetNode = FDialogueNodeHandle(ActiveDialogueTable, ActiveEntryNodeId);
    }
    else
    {
        TargetNode.Table = ActiveDialogueTable;
    }

    if (!ValidateNodeHandle(TargetNode, ErrorMessage))
    {
        RestoreRuntimeStateAfterAsyncFailure(PreviousState, ErrorMessage);
        return;
    }

    CurrentChapterID = ChapterIndex;
    if (bFromSave)
    {
        ApplyLoadedSave();
        return;
    }

    OnGameStartedEvent.Broadcast();
    EnterNode(TargetNode);
}

bool UNovelStorySubsystem::RetainLoadedChapter(int32 ChapterIndex, FText& OutErrorMessage)
{
    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    const FChapterData* ChapterData = Settings ? Settings->ChapterMap.Find(ChapterIndex) : nullptr;
    if (!ChapterData)
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Chapter %d is not configured."), ChapterIndex));
        return false;
    }

    ActiveStoryAsset = nullptr;
    if (!ChapterData->ChapterAsset.IsNull())
    {
        return RetainUnifiedChapter(ChapterData->ChapterAsset.Get(), OutErrorMessage);
    }

    ActiveChapterAsset = nullptr;
    ActiveDialogueTable = ChapterData->DialogueTable.Get();
    BranchData = ChapterData->BranchData.Get();
    if (!ActiveDialogueTable)
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Chapter %d dialogue table failed to load."), ChapterIndex));
        return false;
    }
    return BuildLegacyActiveNodes(ChapterIndex, OutErrorMessage);
}

bool UNovelStorySubsystem::BuildLegacyActiveNodes(int32 ChapterIndex, FText& OutErrorMessage)
{
    ActiveNodes.Reset();
    ActiveChapterId = FPrimaryAssetId(FPrimaryAssetType(TEXT("NovelLegacyChapter")), FName(*FString::Printf(TEXT("Chapter_%d"), ChapterIndex)));
    const TArray<FName> RowNames = ActiveDialogueTable ? ActiveDialogueTable->GetRowNames() : TArray<FName>();
    if (RowNames.IsEmpty())
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Legacy chapter %d has no rows."), ChapterIndex));
        return false;
    }

    ActiveEntryNodeId = RowNames[0];
    UE_LOG(LogNovel, Warning, TEXT("Legacy chapter %d is normalized from DataTable row order. Convert it to UNovelChapterAsset to persist explicit transitions."), ChapterIndex);

    for (int32 Index = 0; Index < RowNames.Num(); ++Index)
    {
        const FName RowName = RowNames[Index];
        const FDialogueRow* Row = ActiveDialogueTable->FindRow<FDialogueRow>(RowName, TEXT("BuildLegacyActiveNodes"), false);
        if (!Row)
        {
            OutErrorMessage = FText::FromString(FString::Printf(TEXT("Legacy row %s could not be read."), *RowName.ToString()));
            return false;
        }

        FNovelNode Node;
        Node.NodeId = RowName;
        Node.Speaker = FText::FromString(Row->Speaker);
        Node.Text = FText::FromString(Row->Text);
        if (Index + 1 < RowNames.Num())
        {
            Node.Next.ChapterId = ActiveChapterId;
            Node.Next.NodeId = RowNames[Index + 1];
        }

        if (const FBranchData* Branch = BranchData ? BranchData->BranchMap.Find(RowName) : nullptr)
        {
            for (UNovelIntentBase* Intent : Branch->AutoIntents)
            {
                Node.EntryActions.Add(Intent);
            }
            if (Branch->bIsChoiceNode)
            {
                for (const FDialogueOption& LegacyOption : Branch->Options)
                {
                    FNovelChoice Choice;
                    Choice.Text = LegacyOption.OptionText;
                    for (UNovelIntentBase* Intent : LegacyOption.Intents)
                    {
                        Choice.Actions.Add(Intent);
                        if (!Choice.Target.IsValid())
                        {
                            if (const UNovelIntent_Jump* Jump = Cast<UNovelIntent_Jump>(Intent))
                            {
                                Choice.Target.ChapterId = ActiveChapterId;
                                Choice.Target.NodeId = Jump->TargetNode.RowName;
                            }
                        }
                    }
                    Node.Choices.Add(MoveTemp(Choice));
                }
            }
        }
        ActiveNodes.Add(Node.NodeId, MoveTemp(Node));
    }
    return true;
}

bool UNovelStorySubsystem::RetainUnifiedChapter(UNovelChapterAsset* ChapterAsset, FText& OutErrorMessage)
{
    if (!ChapterAsset)
    {
        OutErrorMessage = FText::FromString(TEXT("Unified chapter asset failed to load."));
        return false;
    }

    ActiveChapterAsset = ChapterAsset;
    ActiveDialogueTable = nullptr;
    BranchData = nullptr;
    ActiveChapterId = ChapterAsset->GetPrimaryAssetId();
    ActiveEntryNodeId = ChapterAsset->EntryNodeId;
    ActiveNodes.Reset();
    for (const FNovelNode& Node : ChapterAsset->Nodes)
    {
        if (Node.NodeId.IsNone() || ActiveNodes.Contains(Node.NodeId))
        {
            OutErrorMessage = FText::FromString(FString::Printf(TEXT("Unified chapter %s contains an empty or duplicate node ID %s."), *ActiveChapterId.ToString(), *Node.NodeId.ToString()));
            return false;
        }
        ActiveNodes.Add(Node.NodeId, Node);
    }

    if (!ActiveNodes.Contains(ActiveEntryNodeId))
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Unified chapter %s has missing entry node %s."), *ActiveChapterId.ToString(), *ActiveEntryNodeId.ToString()));
        return false;
    }
    return true;
}

const FNovelNode* UNovelStorySubsystem::FindActiveNode(FName NodeId) const
{
    return ActiveNodes.Find(NodeId);
}

const FNovelNode* UNovelStorySubsystem::GetCurrentNodeData() const
{
    return FindActiveNode(CurrentNode.RowName);
}
void UNovelStorySubsystem::Advance()
{
    AdvanceToNextRow();
}

void UNovelStorySubsystem::NextDialogue()
{
    Advance();
}

void UNovelStorySubsystem::AdvanceToNextRow()
{
    if (RuntimeState != ENovelRuntimeState::AwaitingAdvance)
    {
        RejectCommand(TEXT("AdvanceDialogue"), ENovelRuntimeState::AwaitingAdvance);
        return;
    }

    const FNovelNode* Node = GetCurrentNodeData();
    if (!Node)
    {
        ReportError(TEXT("AdvanceDialogue"), FText::FromString(TEXT("Current normalized node is missing.")));
        return;
    }

    if (!Node->Next.IsValid())
    {
        UE_LOG(LogNovel, Log, TEXT("Story chapter ended. Chapter=%s LastNode=%s"), *ActiveChapterId.ToString(), *CurrentNode.RowName.ToString());
        SetRuntimeState(ENovelRuntimeState::Idle, TEXT("Chapter ended"));
        OnStoryCompletedEvent.Broadcast(CurrentChapterID);
        return;
    }

    RequestNodeRefTransition(Node->Next, false);
}

void UNovelStorySubsystem::ExecuteCurrentNode()
{
    EnterNodeRef(CurrentNodeRef);
}

bool UNovelStorySubsystem::EnterNode(const FDialogueNodeHandle& NodeHandle)
{
    FNovelNodeRef NodeRef;
    NodeRef.ChapterId = ActiveChapterId;
    NodeRef.NodeId = NodeHandle.RowName;
    return EnterNodeRef(NodeRef);
}

bool UNovelStorySubsystem::EnterNodeRef(const FNovelNodeRef& NodeRef, bool bAddHistoryEntry)
{
    if (!NodeRef.IsValid() || NodeRef.ChapterId != ActiveChapterId)
    {
        ReportError(TEXT("EnterNodeRef"), FText::FromString(FString::Printf(TEXT("Node reference %s is not in the loaded chapter %s."), *NodeRef.ToString(), *ActiveChapterId.ToString())));
        return false;
    }

    const FNovelNode* Node = FindActiveNode(NodeRef.NodeId);
    if (!Node)
    {
        ReportError(TEXT("EnterNodeRef"), FText::FromString(FString::Printf(TEXT("Node %s does not exist in chapter %s."), *NodeRef.NodeId.ToString(), *ActiveChapterId.ToString())));
        return false;
    }

    ClearIntentExecutionState();
    bHasPostIntentTransition = false;
    PostIntentTransition = FNovelNodeRef();
    CurrentNode = FDialogueNodeHandle(ActiveDialogueTable, NodeRef.NodeId);
    CurrentNodeRef = NodeRef;
    ++NodeExecutionId;
    LastPublishedNode = FDialogueNodeHandle();

    SetRuntimeState(ENovelRuntimeState::PresentingDialogue, TEXT("EnterNodeRef"));
    ActiveChoiceIndices.Reset();
    OnDialogueOptionsHideEvent.Broadcast();
    BroadcastCurrentLine(bAddHistoryEntry);

    bShowChoicesAfterIntentExecution = !Node->Choices.IsEmpty();
    CurrentIntentQueue.Empty();
    CurrentIntentIndex = 0;
    for (UNovelIntentBase* Action : Node->EntryActions)
    {
        CurrentIntentQueue.Add(Action);
    }

    if (!CurrentIntentQueue.IsEmpty())
    {
        SetRuntimeState(ENovelRuntimeState::ExecutingIntents, TEXT("Node actions"));
        PlayNextIntent();
    }
    else
    {
        FinishIntentExecution();
    }
    return true;
}

bool UNovelStorySubsystem::RestoreLoadedNode(const FDialogueNodeHandle& NodeHandle, bool bRestoreChoice)
{
    FNovelNodeRef NodeRef;
    NodeRef.ChapterId = ActiveChapterId;
    NodeRef.NodeId = NodeHandle.RowName;
    const FNovelNode* Node = FindActiveNode(NodeRef.NodeId);
    if (!Node)
    {
        ReportError(TEXT("RestoreLoadedNode"), FText::FromString(FString::Printf(TEXT("Saved node %s is missing."), *NodeRef.ToString())));
        return false;
    }

    ClearIntentExecutionState();
    bHasPendingTransition = false;
    bPendingTransitionClearScreen = false;
    bHasPostIntentTransition = false;
    PendingTransitionNode = FDialogueNodeHandle();
    PostIntentTransition = FNovelNodeRef();
    CurrentNode = FDialogueNodeHandle(ActiveDialogueTable, NodeRef.NodeId);
    CurrentNodeRef = NodeRef;
    ++NodeExecutionId;
    LastPublishedNode = FDialogueNodeHandle();

    SetRuntimeState(ENovelRuntimeState::PresentingDialogue, TEXT("RestoreLoadedNode"));
    OnDialogueOptionsHideEvent.Broadcast();
    BroadcastCurrentLine(false);

    if (bRestoreChoice)
    {
        if (Node->Choices.IsEmpty())
        {
            ReportError(TEXT("RestoreLoadedNode"), FText::FromString(FString::Printf(TEXT("Save expects choices at %s, but none are available."), *NodeRef.ToString())));
            return false;
        }
        ShowCurrentChoices(*Node);
    }
    else
    {
        SetRuntimeState(ENovelRuntimeState::AwaitingAdvance, TEXT("RestoreLoadedNode complete"));
    }
    return true;
}

bool UNovelStorySubsystem::ValidateNodeHandle(const FDialogueNodeHandle& NodeHandle, FText& OutErrorMessage) const
{
    if (NodeHandle.RowName.IsNone())
    {
        OutErrorMessage = FText::FromString(TEXT("Dialogue node ID is empty."));
        return false;
    }
    if (!FindActiveNode(NodeHandle.RowName))
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Node %s does not exist in active chapter %s."), *NodeHandle.RowName.ToString(), *ActiveChapterId.ToString()));
        return false;
    }
    return true;
}

bool UNovelStorySubsystem::RequestNodeRefTransition(const FNovelNodeRef& TargetNode, bool bClearScreen)
{
    if (!TargetNode.IsValid())
    {
        ReportError(TEXT("RequestNodeRefTransition"), FText::FromString(TEXT("Target node reference is invalid.")));
        return false;
    }

    if (TargetNode.ChapterId != ActiveChapterId)
    {
        if (!ActiveStoryAsset)
        {
            ReportError(TEXT("RequestNodeRefTransition"), FText::FromString(FString::Printf(TEXT("Cross-chapter target %s requires an active story asset."), *TargetNode.ToString())));
            return false;
        }

        ++ChapterLoadSerial;
        bPendingUnifiedClearScreen = bClearScreen;
        return BeginUnifiedChapterLoad(TargetNode, RuntimeState, false);
    }

    if (!FindActiveNode(TargetNode.NodeId))
    {
        ReportError(TEXT("RequestNodeRefTransition"), FText::FromString(FString::Printf(TEXT("Target node %s does not exist."), *TargetNode.ToString())));
        return false;
    }

    if (bClearScreen)
    {
        ResetVisualState();
    }
    return EnterNodeRef(TargetNode);
}

void UNovelStorySubsystem::BroadcastCurrentLine(bool bAddHistoryEntry)
{
    if (LastPublishedNode == CurrentNode)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Skipped duplicate line broadcast. Chapter=%s Node=%s"), *ActiveChapterId.ToString(), *CurrentNode.RowName.ToString());
        return;
    }

    const FNovelNode* Node = GetCurrentNodeData();
    if (!Node)
    {
        ReportError(TEXT("BroadcastCurrentLine"), FText::FromString(FString::Printf(TEXT("Node %s was not found."), *CurrentNode.RowName.ToString())));
        return;
    }

    LastPublishedNode = CurrentNode;
    OnDialogueLineChanged.Broadcast(Node->Speaker, Node->Text);
    if (bAddHistoryEntry)
    {
        AppendHistoryEntry(Node->Speaker, Node->Text);
    }
}
void UNovelStorySubsystem::PlayNextIntent()
{
    if (RuntimeState != ENovelRuntimeState::ExecutingIntents)
    {
        return;
    }

    if (bHasPendingTransition || bHasPendingNodeRefTransition)
    {
        ApplyPendingTransitionIfAny();
        return;
    }

    while (CurrentIntentIndex < CurrentIntentQueue.Num())
    {
        UNovelIntentBase* NextIntent = CurrentIntentQueue[CurrentIntentIndex];
        if (!NextIntent)
        {
            UE_LOG(LogNovel, Warning, TEXT("Skipping null intent. Chapter=%d Node=%s IntentIndex=%d"), CurrentChapterID, *CurrentNode.RowName.ToString(), CurrentIntentIndex);
            ++CurrentIntentIndex;
            continue;
        }

        ActiveIntentExecutionId = ++IntentExecutionId;
        ActiveActionContext = NewObject<UNovelActionContext>(this);
        ActiveActionContext->Initialize(this, StorySessionId, NodeExecutionId, ActiveIntentExecutionId);

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                IntentTimeoutTimerHandle,
                FTimerDelegate::CreateUObject(this, &UNovelStorySubsystem::HandleIntentTimeout, StorySessionId, NodeExecutionId, ActiveIntentExecutionId),
                IntentCompletionTimeoutSeconds,
                false);
        }

        NextIntent->ExecuteAction(ActiveActionContext);
        return;
    }

    FinishIntentExecution();
}

void UNovelStorySubsystem::CompleteIntentFromProxy(int32 CompletionSessionId, int32 CompletionNodeExecutionId, int32 CompletionIntentExecutionId)
{
    if (CompletionSessionId != StorySessionId || CompletionNodeExecutionId != NodeExecutionId || CompletionIntentExecutionId != ActiveIntentExecutionId)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignored stale intent completion. CompletionSession=%d ActiveSession=%d CompletionNodeExec=%d ActiveNodeExec=%d CompletionIntent=%d ActiveIntent=%d"),
            CompletionSessionId,
            StorySessionId,
            CompletionNodeExecutionId,
            NodeExecutionId,
            CompletionIntentExecutionId,
            ActiveIntentExecutionId);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(IntentTimeoutTimerHandle);
    }

    ActiveIntentExecutionId = INDEX_NONE;
    ActiveActionContext = nullptr;
    ++CurrentIntentIndex;

    if (bHasPendingTransition || bHasPendingNodeRefTransition)
    {
        ApplyPendingTransitionIfAny();
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UNovelStorySubsystem::PlayNextIntent));
    }
    else
    {
        PlayNextIntent();
    }
}

void UNovelStorySubsystem::FailActionFromContext(int32 CompletionSessionId, int32 CompletionNodeExecutionId, int32 CompletionIntentExecutionId, const FText& ErrorMessage)
{
    if (CompletionSessionId != StorySessionId || CompletionNodeExecutionId != NodeExecutionId || CompletionIntentExecutionId != ActiveIntentExecutionId)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignored stale action failure. Session=%d NodeExecution=%d ActionExecution=%d"), CompletionSessionId, CompletionNodeExecutionId, CompletionIntentExecutionId);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(IntentTimeoutTimerHandle);
    }
    ActiveIntentExecutionId = INDEX_NONE;
    ActiveActionContext = nullptr;
    CurrentIntentQueue.Empty();
    ReportError(TEXT("ExecuteAction"), ErrorMessage);
}

void UNovelStorySubsystem::HandleIntentTimeout(int32 ExpectedSessionId, int32 ExpectedNodeExecutionId, int32 ExpectedIntentExecutionId)
{
    if (ExpectedSessionId != StorySessionId || ExpectedNodeExecutionId != NodeExecutionId || ExpectedIntentExecutionId != ActiveIntentExecutionId)
    {
        return;
    }

    UE_LOG(LogNovel, Warning, TEXT("Intent timed out without completion. Chapter=%d Node=%s IntentIndex=%d Timeout=%.2fs"),
        CurrentChapterID,
        *CurrentNode.RowName.ToString(),
        CurrentIntentIndex,
        IntentCompletionTimeoutSeconds);

    CompleteIntentFromProxy(ExpectedSessionId, ExpectedNodeExecutionId, ExpectedIntentExecutionId);
}

void UNovelStorySubsystem::FinishIntentExecution()
{
    if (bHasPendingTransition || bHasPendingNodeRefTransition)
    {
        ApplyPendingTransitionIfAny();
        return;
    }

    const bool bShouldShowChoices = bShowChoicesAfterIntentExecution;
    const bool bApplyPostTransition = bHasPostIntentTransition;
    const FNovelNodeRef PostTransition = PostIntentTransition;
    ClearIntentExecutionState();
    bHasPostIntentTransition = false;
    PostIntentTransition = FNovelNodeRef();

    if (bApplyPostTransition)
    {
        RequestNodeRefTransition(PostTransition, false);
        return;
    }

    const FNovelNode* Node = GetCurrentNodeData();
    if (Node && bShouldShowChoices && !Node->Choices.IsEmpty())
    {
        ShowCurrentChoices(*Node);
        return;
    }

    SetRuntimeState(ENovelRuntimeState::AwaitingAdvance, TEXT("Node complete"));
}

void UNovelStorySubsystem::ApplyPendingTransitionIfAny()
{
    if (!bHasPendingTransition && !bHasPendingNodeRefTransition)
    {
        return;
    }

    const bool bUseNodeRef = bHasPendingNodeRefTransition;
    const FDialogueNodeHandle LegacyTarget = PendingTransitionNode;
    const FNovelNodeRef NodeRefTarget = PendingNodeRefTransition;
    const bool bClearScreen = bPendingTransitionClearScreen;
    bHasPendingTransition = false;
    bHasPendingNodeRefTransition = false;
    bPendingTransitionClearScreen = false;
    bHasPostIntentTransition = false;
    PendingTransitionNode = FDialogueNodeHandle();
    PendingNodeRefTransition = FNovelNodeRef();
    PostIntentTransition = FNovelNodeRef();
    ClearIntentExecutionState();

    if (bUseNodeRef)
    {
        RequestNodeRefTransition(NodeRefTarget, bClearScreen);
        return;
    }

    if (bClearScreen)
    {
        ResetVisualState();
    }
    EnterNode(LegacyTarget);
}
void UNovelStorySubsystem::ShowCurrentChoices(const FNovelNode& Node)
{
    if (Node.Choices.IsEmpty())
    {
        SetRuntimeState(ENovelRuntimeState::AwaitingAdvance, TEXT("Empty choice node"));
        return;
    }

    UNovelExpressionContext* ExpressionContext = NewObject<UNovelExpressionContext>(this);
    ExpressionContext->Initialize(Variables);
    TArray<FDialogueOption> CompatibilityOptions;
    ActiveChoiceIndices.Reset();

    for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
    {
        const FNovelChoice& Choice = Node.Choices[ChoiceIndex];
        if (Choice.Condition)
        {
            FNovelValue ConditionValue;
            FText Error;
            if (!Choice.Condition->Evaluate(ExpressionContext, ConditionValue, Error))
            {
                ReportError(TEXT("EvaluateChoiceCondition"), FText::FromString(FString::Printf(TEXT("Choice %d at %s failed: %s"), ChoiceIndex, *CurrentNodeRef.ToString(), *Error.ToString())));
                return;
            }
            if (ConditionValue.Type != ENovelValueType::Bool)
            {
                ReportError(TEXT("EvaluateChoiceCondition"), FText::FromString(FString::Printf(TEXT("Choice %d at %s returned %s instead of Bool."), ChoiceIndex, *CurrentNodeRef.ToString(), *ConditionValue.GetTypeName())));
                return;
            }
            if (!ConditionValue.BoolValue)
            {
                continue;
            }
        }

        ActiveChoiceIndices.Add(ChoiceIndex);
        FDialogueOption& Option = CompatibilityOptions.AddDefaulted_GetRef();
        Option.OptionText = Choice.Text;
        for (UNovelIntentBase* Action : Choice.Actions)
        {
            Option.Intents.Add(Action);
        }
    }

    if (CompatibilityOptions.IsEmpty())
    {
        UE_LOG(LogNovel, Log, TEXT("No choices passed conditions. Node=%s"), *CurrentNodeRef.ToString());
        SetRuntimeState(ENovelRuntimeState::AwaitingAdvance, TEXT("No available choices"));
        OnDialogueOptionsHideEvent.Broadcast();
        return;
    }

    SetRuntimeState(ENovelRuntimeState::AwaitingChoice, TEXT("Awaiting choice"));
    OnDialogueOptionsShowEvent.Broadcast(CompatibilityOptions);
}
void UNovelStorySubsystem::ClearIntentExecutionState()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(IntentTimeoutTimerHandle);
    }

    CurrentIntentQueue.Empty();
    CurrentIntentIndex = 0;
    ActiveIntentExecutionId = INDEX_NONE;
    ActiveActionContext = nullptr;
    bShowChoicesAfterIntentExecution = false;
}

void UNovelStorySubsystem::ProcessIntents(const TArray<UNovelIntentBase*>& Intents)
{
    if (RuntimeState != ENovelRuntimeState::AwaitingChoice)
    {
        RejectCommand(TEXT("ProcessIntents"), ENovelRuntimeState::AwaitingChoice);
        return;
    }

    OnDialogueOptionsHideEvent.Broadcast();
    CurrentIntentQueue = Intents;
    CurrentIntentIndex = 0;
    bShowChoicesAfterIntentExecution = false;
    bHasPostIntentTransition = false;
    PostIntentTransition = FNovelNodeRef();
    SetRuntimeState(ENovelRuntimeState::ExecutingIntents, TEXT("ProcessIntents"));
    PlayNextIntent();
}

void UNovelStorySubsystem::JumpToNode(FDialogueNodeHandle TargetNode)
{
    RequestNodeTransition(TargetNode, false);
}

void UNovelStorySubsystem::RequestNodeTransition(FDialogueNodeHandle TargetNode, bool bClearScreen)
{
    FText ErrorMessage;
    if (!ValidateNodeHandle(TargetNode, ErrorMessage))
    {
        ReportError(TEXT("RequestNodeTransition"), ErrorMessage);
        return;
    }

    TargetNode.Table = ActiveDialogueTable;

    if (RuntimeState == ENovelRuntimeState::ExecutingIntents)
    {
        if (bHasPendingTransition || bHasPendingNodeRefTransition)
        {
            UE_LOG(LogNovel, Warning, TEXT("Rejected duplicate node transition. Chapter=%d FromNode=%s ExistingTarget=%s RequestedTarget=%s"),
                CurrentChapterID,
                *CurrentNode.RowName.ToString(),
                *PendingTransitionNode.RowName.ToString(),
                *TargetNode.RowName.ToString());
            return;
        }

        bHasPendingTransition = true;
        bPendingTransitionClearScreen = bClearScreen;
        PendingTransitionNode = TargetNode;
        UE_LOG(LogNovel, Verbose, TEXT("Deferred node transition requested. Chapter=%d FromNode=%s TargetNode=%s ClearScreen=%s"),
            CurrentChapterID,
            *CurrentNode.RowName.ToString(),
            *TargetNode.RowName.ToString(),
            bClearScreen ? TEXT("true") : TEXT("false"));
        return;
    }

    if (RuntimeState == ENovelRuntimeState::LoadingChapter || RuntimeState == ENovelRuntimeState::LoadingSave || RuntimeState == ENovelRuntimeState::Saving)
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected RequestNodeTransition while busy. TargetNode=%s State=%s"), *TargetNode.RowName.ToString(), *RuntimeStateToString(RuntimeState));
        return;
    }

    if (bClearScreen)
    {
        ResetVisualState();
    }
    EnterNode(TargetNode);
}

void UNovelStorySubsystem::RequestNodeTransitionByRef(FNovelNodeRef TargetNode, bool bClearScreen)
{
    if (!TargetNode.IsValid())
    {
        ReportError(TEXT("RequestNodeTransitionByRef"), FText::FromString(TEXT("Target node reference is invalid.")));
        return;
    }

    const bool bTargetCanLoad = TargetNode.ChapterId == ActiveChapterId
        ? FindActiveNode(TargetNode.NodeId) != nullptr
        : ActiveStoryAsset && !ActiveStoryAsset->FindChapter(TargetNode.ChapterId).IsNull();
    if (!bTargetCanLoad)
    {
        ReportError(TEXT("RequestNodeTransitionByRef"), FText::FromString(FString::Printf(TEXT("Target %s cannot be resolved."), *TargetNode.ToString())));
        return;
    }

    if (RuntimeState == ENovelRuntimeState::ExecutingIntents)
    {
        if (bHasPendingTransition || bHasPendingNodeRefTransition)
        {
            UE_LOG(LogNovel, Warning, TEXT("Rejected duplicate action transition. ExistingLegacy=%s ExistingRef=%s Requested=%s"),
                *PendingTransitionNode.RowName.ToString(),
                *PendingNodeRefTransition.ToString(),
                *TargetNode.ToString());
            return;
        }
        bHasPendingNodeRefTransition = true;
        bPendingTransitionClearScreen = bClearScreen;
        PendingNodeRefTransition = TargetNode;
        return;
    }

    if (RuntimeState == ENovelRuntimeState::LoadingChapter || RuntimeState == ENovelRuntimeState::LoadingSave || RuntimeState == ENovelRuntimeState::Saving)
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected RequestNodeTransitionByRef while busy. Target=%s State=%s"), *TargetNode.ToString(), *RuntimeStateToString(RuntimeState));
        return;
    }
    RequestNodeRefTransition(TargetNode, bClearScreen);
}
void UNovelStorySubsystem::SelectChoice(int32 ChoiceIndex)
{
    SelectChoiceForNode(ChoiceIndex, CurrentNode);
}

void UNovelStorySubsystem::SelectChoiceForNode(int32 ChoiceIndex, FDialogueNodeHandle SourceNode)
{
    if (RuntimeState != ENovelRuntimeState::AwaitingChoice)
    {
        RejectCommand(TEXT("SelectChoice"), ENovelRuntimeState::AwaitingChoice);
        return;
    }
    if (SourceNode != CurrentNode)
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected stale choice. SourceNode=%s CurrentNode=%s Chapter=%s"), *SourceNode.RowName.ToString(), *CurrentNode.RowName.ToString(), *ActiveChapterId.ToString());
        return;
    }

    const FNovelNode* Node = GetCurrentNodeData();
    if (!Node || Node->Choices.IsEmpty())
    {
        ReportError(TEXT("SelectChoice"), FText::FromString(FString::Printf(TEXT("Current node %s has no choices."), *CurrentNode.RowName.ToString())));
        return;
    }
    if (!ActiveChoiceIndices.IsValidIndex(ChoiceIndex))
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected invalid presented choice index. Chapter=%s Node=%s Index=%d PresentedCount=%d"), *ActiveChapterId.ToString(), *CurrentNode.RowName.ToString(), ChoiceIndex, ActiveChoiceIndices.Num());
        return;
    }

    const int32 AuthoredChoiceIndex = ActiveChoiceIndices[ChoiceIndex];
    if (!Node->Choices.IsValidIndex(AuthoredChoiceIndex))
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected stale choice mapping. Node=%s PresentedIndex=%d AuthoredIndex=%d"), *CurrentNodeRef.ToString(), ChoiceIndex, AuthoredChoiceIndex);
        return;
    }

    ActiveChoiceIndices.Reset();
    OnDialogueOptionsHideEvent.Broadcast();
    const FNovelChoice& Choice = Node->Choices[AuthoredChoiceIndex];
    CurrentIntentQueue.Empty();
    for (UNovelIntentBase* Action : Choice.Actions)
    {
        CurrentIntentQueue.Add(Action);
    }
    CurrentIntentIndex = 0;
    bShowChoicesAfterIntentExecution = false;
    bHasPostIntentTransition = Choice.Target.IsValid();
    PostIntentTransition = Choice.Target;

    if (CurrentIntentQueue.IsEmpty())
    {
        if (bHasPostIntentTransition)
        {
            bHasPostIntentTransition = false;
            const FNovelNodeRef Target = PostIntentTransition;
            PostIntentTransition = FNovelNodeRef();
            RequestNodeRefTransition(Target, false);
        }
        else
        {
            UE_LOG(LogNovel, Warning, TEXT("Choice has no actions or target. Chapter=%s Node=%s Index=%d"), *ActiveChapterId.ToString(), *CurrentNode.RowName.ToString(), ChoiceIndex);
            SetRuntimeState(ENovelRuntimeState::AwaitingAdvance, TEXT("Empty choice"));
        }
        return;
    }

    SetRuntimeState(ENovelRuntimeState::ExecutingIntents, TEXT("SelectChoice"));
    PlayNextIntent();
}
TArray<FString> UNovelStorySubsystem::GetAllSaveSlotNames() const
{
    TArray<FString> SlotNames;
    if (!SaveIndex)
    {
        return SlotNames;
    }
    SlotNames.Reserve(SaveIndex->Slots.Num());
    for (const FNovelSaveSlotMetadata& Metadata : SaveIndex->Slots)
    {
        SlotNames.Add(Metadata.SlotId);
    }
    return SlotNames;
}
void UNovelStorySubsystem::SaveGame(FString SlotNameOverride)
{
    if (RuntimeState == ENovelRuntimeState::Saving || RuntimeState == ENovelRuntimeState::LoadingSave || RuntimeState == ENovelRuntimeState::LoadingChapter)
    {
        UE_LOG(LogNovel, Warning, TEXT("Rejected SaveGame. Slot=%s State=%s"), *SlotNameOverride, *RuntimeStateToString(RuntimeState));
        return;
    }

    if (CurrentNode.RowName.IsNone())
    {
        const FText Message = FText::FromString(TEXT("Cannot save because no story node is active."));
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotNameOverride, Message);
        OnStoryErrorEvent.Broadcast(Message);
        return;
    }

    UNovelSaveGame* SaveGameInstance = Cast<UNovelSaveGame>(UGameplayStatics::CreateSaveGameObject(UNovelSaveGame::StaticClass()));
    if (!SaveGameInstance)
    {
        const FText Message = FText::FromString(TEXT("Could not create save object."));
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotNameOverride, Message);
        OnStoryErrorEvent.Broadcast(Message);
        return;
    }

    const UNovelRuntimeSettings* RuntimeSettings = GetDefault<UNovelRuntimeSettings>();
    const FString Prefix = RuntimeSettings ? RuntimeSettings->DefaultSaveSlotPrefix : TEXT("Novel_");
    const FString SlotName = SlotNameOverride.IsEmpty() ? Prefix + FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")) : SlotNameOverride;
    SaveGameInstance->SaveSchemaVersion = CurrentSaveSchemaVersion;
    SaveGameInstance->RuntimeSnapshot = CreateSnapshot();
    SaveGameInstance->SaveTimestamp = FDateTime::UtcNow();
    SaveGameInstance->SaveSlotName = SlotName;
    SaveGameInstance->SaveNode = CurrentNode;
    SaveGameInstance->SavedChapterID = CurrentChapterID;
    SaveGameInstance->SavedStoryID = ActiveStoryAsset ? ActiveStoryAsset->GetPrimaryAssetId() : FPrimaryAssetId();
    SaveGameInstance->SaveNodeRef = CurrentNodeRef;
    SaveGameInstance->SavedStoryAsset = ActiveStoryAsset;
    SaveGameInstance->SavedChapterAsset = ActiveChapterAsset;
    SaveGameInstance->SaveBackground = CurrentBackground;
    SaveGameInstance->SaveBGM = CurrentBGM;
    SaveGameInstance->SaveHistory = DialogueHistory;
    SaveGameInstance->SaveVariables = Variables;
    SaveGameInstance->bSavedAtChoice = RuntimeState == ENovelRuntimeState::AwaitingChoice;

    PendingSaveMetadata = FNovelSaveSlotMetadata();
    PendingSaveMetadata.SlotId = SlotName;
    PendingSaveMetadata.DisplayName = FText::FromString(SlotName);
    PendingSaveMetadata.Timestamp = SaveGameInstance->SaveTimestamp;
    PendingSaveMetadata.StoryId = SaveGameInstance->RuntimeSnapshot.StoryId;
    PendingSaveMetadata.Node = SaveGameInstance->RuntimeSnapshot.CurrentNode;
    PendingSaveMetadata.SnapshotSchemaVersion = SaveGameInstance->RuntimeSnapshot.SchemaVersion;
    PendingSaveMetadata.Background = CurrentBackground;
    if (const FNovelNode* Node = GetCurrentNodeData())
    {
        PendingSaveMetadata.LinePreview = Node->Text;
    }

    SaveGameInstance->SaveVisibleCharacters.Empty();
    for (const TPair<FName, FNovelCharacterPresentationState>& Pair : VisibleCharacters)
    {
        SaveGameInstance->SaveVisibleCharacters.Add(Pair.Value);
    }

    StateBeforeSaveOrLoad = RuntimeState;
    SetRuntimeState(ENovelRuntimeState::Saving, TEXT("SaveGame"));

    UGameplayStatics::AsyncSaveGameToSlot(
        SaveGameInstance,
        SlotName,
        0,
        FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UNovelStorySubsystem::OnAsyncSaveGameComplete));
}

void UNovelStorySubsystem::OnAsyncSaveGameComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
    if (!bSuccess)
    {
        const FText Message = FText::FromString(FString::Printf(TEXT("Failed to save slot %s."), *SlotName));
        UE_LOG(LogNovel, Error, TEXT("Story save failed. Slot=%s UserIndex=%d"), *SlotName, UserIndex);
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, Message);
        OnStoryErrorEvent.Broadcast(Message);
        SetRuntimeState(StateBeforeSaveOrLoad, TEXT("Story save failed"));
        return;
    }

    UpsertSaveIndexEntry(PendingSaveMetadata);
    if (bSaveIndexPersistenceBlocked)
    {
        OnAsyncSaveIndexComplete(GetSaveIndexSlotName(), 0, false, SlotName);
        return;
    }
    UGameplayStatics::AsyncSaveGameToSlot(
        SaveIndex,
        GetSaveIndexSlotName(),
        0,
        FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UNovelStorySubsystem::OnAsyncSaveIndexComplete, SlotName));
}

void UNovelStorySubsystem::OnAsyncSaveIndexComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess, FString StorySlotName)
{
    const FText Message = bSuccess
        ? FText::FromString(FString::Printf(TEXT("Saved slot %s."), *StorySlotName))
        : FText::FromString(FString::Printf(TEXT("Saved slot %s, but the save index could not be persisted and will require repair."), *StorySlotName));

    if (bSuccess)
    {
        UE_LOG(LogNovel, Log, TEXT("Save transaction complete. Slot=%s IndexSlot=%s"), *StorySlotName, *SlotName);
    }
    else
    {
        UE_LOG(LogNovel, Error, TEXT("Save index persistence failed after story save. Slot=%s IndexSlot=%s"), *StorySlotName, *SlotName);
        OnStoryErrorEvent.Broadcast(Message);
    }
    OnSaveLoadOperationFinishedEvent.Broadcast(true, StorySlotName, Message);
    SetRuntimeState(StateBeforeSaveOrLoad, TEXT("Save transaction complete"));
}
void UNovelStorySubsystem::LoadGame(FString SlotName)
{
    if (SlotName.IsEmpty())
    {
        const FText Message = FText::FromString(TEXT("Cannot load an empty save slot name."));
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, Message);
        OnStoryErrorEvent.Broadcast(Message);
        return;
    }

    if (!CanStartBlockingOperation(TEXT("LoadGame")))
    {
        return;
    }

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        const FText Message = FText::FromString(FString::Printf(TEXT("Save slot %s does not exist."), *SlotName));
        UE_LOG(LogNovel, Warning, TEXT("LoadGame failed. Slot=%s Reason=Missing save file"), *SlotName);
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, Message);
        OnStoryErrorEvent.Broadcast(Message);
        return;
    }

    StateBeforeSaveOrLoad = RuntimeState;
    SetRuntimeState(ENovelRuntimeState::LoadingSave, TEXT("LoadGame"));
    OnStoryLoadingStateChangedEvent.Broadcast(true, FText::FromString(FString::Printf(TEXT("Loading save %s"), *SlotName)));

    UGameplayStatics::AsyncLoadGameFromSlot(
        SlotName,
        0,
        FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &UNovelStorySubsystem::OnAsyncLoadGameComplete));
}

void UNovelStorySubsystem::OnAsyncLoadGameComplete(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData)
{
    UNovelSaveGame* LoadedGame = Cast<UNovelSaveGame>(LoadedGameData);
    if (LoadedGame && LoadedGame->RuntimeSnapshot.SchemaVersion == 1)
    {
        const FNovelRuntimeSnapshot& Snapshot = LoadedGame->RuntimeSnapshot;
        LoadedGame->SavedStoryID = Snapshot.StoryId;
        LoadedGame->SavedStoryAsset = Snapshot.StoryAsset;
        LoadedGame->SavedChapterAsset = Snapshot.ChapterAsset;
        LoadedGame->SaveNodeRef = Snapshot.CurrentNode;
        LoadedGame->SavedChapterID = Snapshot.LegacyChapterId;
        LoadedGame->SaveNode = Snapshot.LegacyNode;
        LoadedGame->SaveBackground = Snapshot.Background;
        LoadedGame->SaveVisibleCharacters = Snapshot.VisibleCharacters;
        LoadedGame->SaveBGM = Snapshot.Music;
        LoadedGame->SaveHistory = Snapshot.History;
        LoadedGame->SaveVariables = Snapshot.Variables;
        LoadedGame->bSavedAtChoice = Snapshot.bAtChoice;
    }
    FText ErrorMessage;
    if (!ValidateSaveGameForLoad(LoadedGame, SlotName, ErrorMessage))
    {
        OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
        RestoreRuntimeStateAfterAsyncFailure(StateBeforeSaveOrLoad, ErrorMessage);
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, ErrorMessage);
        return;
    }

    ++StorySessionId;
    ++ChapterLoadSerial;
    if (LoadedGame->SaveSlotName.IsEmpty())
    {
        LoadedGame->SaveSlotName = SlotName;
    }
    PendingLoadedSave = LoadedGame;

    if (LoadedGame->SaveNodeRef.IsValid() && !LoadedGame->SavedChapterAsset.IsNull())
    {
        if (!BeginUnifiedSaveAssetsLoad(LoadedGame, StateBeforeSaveOrLoad))
        {
            const FText Message = FText::FromString(FString::Printf(TEXT("Could not request unified save assets for slot %s."), *SlotName));
            OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
            RestoreRuntimeStateAfterAsyncFailure(StateBeforeSaveOrLoad, Message);
            OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, Message);
        }
        return;
    }

    FDialogueNodeHandle TargetNode = LoadedGame->SaveNode;
    if (!BeginChapterLoad(LoadedGame->SavedChapterID, TargetNode, true, StateBeforeSaveOrLoad))
    {
        const FText Message = FText::FromString(FString::Printf(TEXT("Could not request legacy chapter %d for save slot %s."), LoadedGame->SavedChapterID, *SlotName));
        OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
        RestoreRuntimeStateAfterAsyncFailure(StateBeforeSaveOrLoad, Message);
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, Message);
    }
}

bool UNovelStorySubsystem::BeginUnifiedSaveAssetsLoad(UNovelSaveGame* SaveGame, ENovelRuntimeState PreviousState)
{
    UNovelLoadingSubsystem* LoadingSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelLoadingSubsystem>() : nullptr;
    if (!LoadingSubsystem || !SaveGame || SaveGame->SavedChapterAsset.IsNull())
    {
        return false;
    }

    TArray<FSoftObjectPath> Paths;
    Paths.Add(SaveGame->SavedChapterAsset.ToSoftObjectPath());
    if (!SaveGame->SavedStoryAsset.IsNull())
    {
        Paths.Add(SaveGame->SavedStoryAsset.ToSoftObjectPath());
    }

    const int32 ExpectedSessionId = StorySessionId;
    const int32 ExpectedLoadSerial = ChapterLoadSerial;
    PendingLoadingSubsystemRequestId = LoadingSubsystem->RequestLoad(
        Paths,
        FOnLoadCompleted::CreateUObject(this, &UNovelStorySubsystem::OnUnifiedSaveAssetsLoaded, ExpectedSessionId, ExpectedLoadSerial, PreviousState));
    return true;
}

void UNovelStorySubsystem::OnUnifiedSaveAssetsLoaded(int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState)
{
    if (ExpectedSessionId != StorySessionId || ExpectedLoadSerial != ChapterLoadSerial || !PendingLoadedSave)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignoring stale unified save asset callback."));
        return;
    }

    PendingLoadingSubsystemRequestId = INDEX_NONE;
    OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
    ActiveStoryAsset = PendingLoadedSave->SavedStoryAsset.Get();

    FText ErrorMessage;
    if (!RetainUnifiedChapter(PendingLoadedSave->SavedChapterAsset.Get(), ErrorMessage) ||
        PendingLoadedSave->SaveNodeRef.ChapterId != ActiveChapterId ||
        !FindActiveNode(PendingLoadedSave->SaveNodeRef.NodeId))
    {
        if (ErrorMessage.IsEmpty())
        {
            ErrorMessage = FText::FromString(FString::Printf(TEXT("Unified save target %s is invalid."), *PendingLoadedSave->SaveNodeRef.ToString()));
        }
        const FString SlotName = PendingLoadedSave->SaveSlotName;
        PendingLoadedSave = nullptr;
        RestoreRuntimeStateAfterAsyncFailure(PreviousState, ErrorMessage);
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, ErrorMessage);
        return;
    }

    CurrentChapterID = INDEX_NONE;
    ApplyLoadedSave();
}
bool UNovelStorySubsystem::ValidateSaveGameForLoad(const UNovelSaveGame* SaveGame, const FString& SlotName, FText& OutErrorMessage) const
{
    if (!SaveGame)
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Save slot %s is missing, corrupt, or not a Novel save."), *SlotName));
        return false;
    }

    if (SaveGame->SaveSchemaVersion > CurrentSaveSchemaVersion)
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Save slot %s uses unsupported schema %d."), *SlotName, SaveGame->SaveSchemaVersion));
        return false;
    }
    if (SaveGame->RuntimeSnapshot.SchemaVersion > 1)
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Save slot %s uses unsupported snapshot schema %d."), *SlotName, SaveGame->RuntimeSnapshot.SchemaVersion));
        return false;
    }

    if (SaveGame->SaveNodeRef.IsValid() && !SaveGame->SavedChapterAsset.IsNull())
    {
        return true;
    }

    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    const FChapterData* ChapterData = Settings ? Settings->ChapterMap.Find(SaveGame->SavedChapterID) : nullptr;
    if (!ChapterData || (ChapterData->ChapterAsset.IsNull() && ChapterData->DialogueTable.IsNull()))
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Save slot %s references missing legacy chapter %d."), *SlotName, SaveGame->SavedChapterID));
        return false;
    }

    if (SaveGame->SaveNode.RowName.IsNone())
    {
        OutErrorMessage = FText::FromString(FString::Printf(TEXT("Save slot %s has no saved node identity."), *SlotName));
        return false;
    }

    return true;
}

bool UNovelStorySubsystem::TryReadSaveMetadata(const FString& SlotName, FNovelSaveMetadata& OutMetadata) const
{
    UNovelSaveGame* SaveGame = Cast<UNovelSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    FText ErrorMessage;
    if (!ValidateSaveGameForLoad(SaveGame, SlotName, ErrorMessage))
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignoring invalid save slot. Slot=%s Reason=%s"), *SlotName, *ErrorMessage.ToString());
        return false;
    }

    OutMetadata.SlotName = SlotName;
    OutMetadata.SchemaVersion = SaveGame->SaveSchemaVersion;
    OutMetadata.Timestamp = SaveGame->SaveTimestamp.GetTicks() > 0 ? SaveGame->SaveTimestamp : GetSaveFileTimestamp(SlotName);
    OutMetadata.ChapterID = SaveGame->SavedChapterID;
    OutMetadata.NodeID = SaveGame->SaveNodeRef.IsValid() ? SaveGame->SaveNodeRef.NodeId : SaveGame->SaveNode.RowName;
    return true;
}

FDateTime UNovelStorySubsystem::GetSaveFileTimestamp(const FString& SlotName) const
{
    const FDateTime FileTime = IFileManager::Get().GetTimeStamp(*GetSaveSlotFilePath(SlotName));
    return FileTime.GetTicks() > 0 ? FileTime : FDateTime::MinValue();
}

FString UNovelStorySubsystem::GetSaveSlotFilePath(const FString& SlotName) const
{
    return FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".sav"));
}

void UNovelStorySubsystem::ApplyLoadedSave()
{
    if (!PendingLoadedSave)
    {
        RestoreRuntimeStateAfterAsyncFailure(StateBeforeSaveOrLoad, FText::FromString(TEXT("Loaded save data was lost before chapter load completed.")));
        return;
    }

    FText ErrorMessage;
    FDialogueNodeHandle LoadedNode = PendingLoadedSave->SaveNode;
    if (PendingLoadedSave->SaveNodeRef.IsValid() && ActiveChapterAsset)
    {
        LoadedNode.RowName = PendingLoadedSave->SaveNodeRef.NodeId;
    }
    LoadedNode.Table = ActiveDialogueTable;
    if (!ValidateNodeHandle(LoadedNode, ErrorMessage))
    {
        const FString SlotName = PendingLoadedSave->SaveSlotName;
        PendingLoadedSave = nullptr;
        RestoreRuntimeStateAfterAsyncFailure(StateBeforeSaveOrLoad, ErrorMessage);
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, ErrorMessage);
        return;
    }

    const FString SlotName = PendingLoadedSave->SaveSlotName;
    DialogueHistory = PendingLoadedSave->SaveHistory;
    Variables = PendingLoadedSave->SaveVariables;
    OnVariablesResetEvent.Broadcast();
    NextHistorySequenceIndex = 0;
    for (const FNovelHistoryEntry& Entry : DialogueHistory)
    {
        NextHistorySequenceIndex = FMath::Max(NextHistorySequenceIndex, Entry.SequenceIndex + 1);
    }
    OnHistoryChangedEvent.Broadcast();

    ResetVisualState();
    RestorePresentationFromSave(*PendingLoadedSave);
    OnGameStartedEvent.Broadcast();
    if (!RestoreLoadedNode(LoadedNode, PendingLoadedSave->bSavedAtChoice))
    {
        const FText RestoreError = FText::FromString(FString::Printf(TEXT("Could not restore node %s from slot %s without replaying its actions."), *LoadedNode.RowName.ToString(), *SlotName));
        PendingLoadedSave = nullptr;
        RestoreRuntimeStateAfterAsyncFailure(StateBeforeSaveOrLoad, RestoreError);
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, RestoreError);
        return;
    }

    const FText Message = FText::FromString(FString::Printf(TEXT("Loaded slot %s."), *SlotName));
    OnSaveLoadOperationFinishedEvent.Broadcast(true, SlotName, Message);
    UE_LOG(LogNovel, Log, TEXT("LoadGame complete. Slot=%s Chapter=%d Node=%s"), *SlotName, CurrentChapterID, *CurrentNode.RowName.ToString());
    PendingLoadedSave = nullptr;
}

bool UNovelStorySubsystem::DeleteSaveSlot(const FString& SlotName)
{
    if (SlotName.IsEmpty() || SlotName == GetSaveIndexSlotName() || !SaveIndex)
    {
        return false;
    }

    const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);
    if (!bDeleted)
    {
        const FText Message = FText::FromString(FString::Printf(TEXT("Failed to delete slot %s."), *SlotName));
        OnSaveLoadOperationFinishedEvent.Broadcast(false, SlotName, Message);
        return false;
    }

    SaveIndex->Slots.RemoveAll([&SlotName](const FNovelSaveSlotMetadata& Entry) { return Entry.SlotId == SlotName; });
    SortSaveIndex();
    const bool bIndexSaved = !bSaveIndexPersistenceBlocked && UGameplayStatics::SaveGameToSlot(SaveIndex, GetSaveIndexSlotName(), 0);
    OnSaveIndexChangedEvent.Broadcast();
    const FText Message = bIndexSaved
        ? FText::FromString(FString::Printf(TEXT("Deleted slot %s."), *SlotName))
        : FText::FromString(FString::Printf(TEXT("Deleted slot %s, but the save index update failed."), *SlotName));
    OnSaveLoadOperationFinishedEvent.Broadcast(bIndexSaved, SlotName, Message);
    if (bIndexSaved)
    {
        UE_LOG(LogNovel, Log, TEXT("Delete transaction complete. Slot=%s"), *SlotName);
    }
    else
    {
        UE_LOG(LogNovel, Error, TEXT("Delete transaction index update failed. Slot=%s"), *SlotName);
    }
    return true;
}
void UNovelStorySubsystem::ShowSaveLoadMenu(bool bIsSaveMode)
{
    OnSaveLoadUIRequestedEvent.Broadcast(bIsSaveMode);
}

void UNovelStorySubsystem::ShowHistory()
{
    OnHistoryUIRequestedEvent.Broadcast();
}

void UNovelStorySubsystem::ShowSettingsMenu()
{
    OnSettingsUIRequestedEvent.Broadcast();
}

void UNovelStorySubsystem::ResetVisualState()
{
    CurrentBackground = nullptr;
    VisibleCharacters.Empty();
    OnDialogueResetRequestedEvent.Broadcast();
    OnDialogueOptionsHideEvent.Broadcast();
}

void UNovelStorySubsystem::SetBackground(TSoftObjectPtr<UTexture2D> NewBackground)
{
    CurrentBackground = NewBackground;
    OnBackgroundChangedEvent.Broadcast(NewBackground);
}

void UNovelStorySubsystem::ShowCharacter(FName CharacterID, TSoftObjectPtr<UTexture2D> CharacterSprite, float XPosition)
{
    if (CharacterID.IsNone())
    {
        UE_LOG(LogNovel, Warning, TEXT("ShowCharacter rejected. Reason=CharacterID is None"));
        return;
    }

    if (CharacterSprite.IsNull())
    {
        UE_LOG(LogNovel, Warning, TEXT("ShowCharacter rejected. CharacterID=%s Reason=Sprite is null"), *CharacterID.ToString());
        return;
    }

    FNovelCharacterPresentationState& State = VisibleCharacters.FindOrAdd(CharacterID);
    State.CharacterID = CharacterID;
    State.Sprite = CharacterSprite;
    State.XPosition = FMath::Clamp(XPosition, 0.0f, 1.0f);
    OnCharacterShownEvent.Broadcast(CharacterID, CharacterSprite, State.XPosition);
}

void UNovelStorySubsystem::HideCharacter(FName CharacterID)
{
    if (CharacterID.IsNone())
    {
        UE_LOG(LogNovel, Warning, TEXT("HideCharacter rejected. Reason=CharacterID is None"));
        return;
    }

    VisibleCharacters.Remove(CharacterID);
    OnCharacterHiddenEvent.Broadcast(CharacterID);
}

void UNovelStorySubsystem::RequestScreenFade(FLinearColor FadeColor, float FadeDuration, const FNovelIntentCompleted& Completion, bool bWaitUntilFinished)
{
    if (ActiveFadeSerial != INDEX_NONE)
    {
        UE_LOG(LogNovel, Warning, TEXT("Superseding active fade. PreviousFadeSerial=%d"), ActiveFadeSerial);
        CompletePendingFade(ActiveFadeSerial);
    }

    const float SafeDuration = FMath::Max(0.0f, FadeDuration);
    OnScreenFadeEvent.Broadcast(FadeColor, SafeDuration);

    if (!bWaitUntilFinished || SafeDuration <= 0.0f)
    {
        Completion.ExecuteIfBound();
        return;
    }

    PendingFadeCompletion = Completion;
    ActiveFadeSerial = ++FadeSerial;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FadeFallbackTimerHandle,
            FTimerDelegate::CreateUObject(this, &UNovelStorySubsystem::CompletePendingFade, ActiveFadeSerial),
            SafeDuration + 0.1f,
            false);
    }
    else
    {
        CompletePendingFade(ActiveFadeSerial);
    }
}

void UNovelStorySubsystem::NotifyScreenFadeFinished()
{
    if (ActiveFadeSerial == INDEX_NONE)
    {
        return;
    }

    CompletePendingFade(ActiveFadeSerial);
}

void UNovelStorySubsystem::CompletePendingFade(int32 ExpectedFadeSerial)
{
    if (ExpectedFadeSerial != ActiveFadeSerial)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FadeFallbackTimerHandle);
    }

    ActiveFadeSerial = INDEX_NONE;
    FNovelIntentCompleted Completion = PendingFadeCompletion;
    PendingFadeCompletion.Unbind();
    Completion.ExecuteIfBound();
}

void UNovelStorySubsystem::AppendHistoryEntry(const FText& Speaker, const FText& Text)
{
    FNovelHistoryEntry Entry;
    Entry.Speaker = Speaker;
    Entry.Text = Text;
    Entry.ChapterID = CurrentChapterID;
    Entry.StoryAssetId = ActiveStoryAsset ? ActiveStoryAsset->GetPrimaryAssetId() : FPrimaryAssetId();
    Entry.ChapterAssetId = ActiveChapterId;
    Entry.NodeID = CurrentNode.RowName;
    Entry.SequenceIndex = NextHistorySequenceIndex++;
    Entry.Timestamp = FDateTime::UtcNow();
    DialogueHistory.Add(Entry);

    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    const int32 MaximumEntries = Settings ? Settings->MaximumHistoryEntries : 0;
    if (MaximumEntries > 0 && DialogueHistory.Num() > MaximumEntries)
    {
        DialogueHistory.RemoveAt(0, DialogueHistory.Num() - MaximumEntries, EAllowShrinking::No);
    }

    OnHistoryChangedEvent.Broadcast();
}

void UNovelStorySubsystem::RestorePresentationFromSave(const UNovelSaveGame& SaveGame)
{
    SetBackground(SaveGame.SaveBackground);

    VisibleCharacters.Empty();
    for (const FNovelCharacterPresentationState& CharacterState : SaveGame.SaveVisibleCharacters)
    {
        ShowCharacter(CharacterState.CharacterID, CharacterState.Sprite, CharacterState.XPosition);
    }

    if (!SaveGame.SaveBGM.IsNull())
    {
        PlayBGM(SaveGame.SaveBGM, 0.0f);
    }
    else
    {
        StopBGM(0.0f);
    }
}

void UNovelStorySubsystem::PlayBGM(TSoftObjectPtr<USoundBase> NewBGM, float FadeInTime)
{
    if (NewBGM.IsNull())
    {
        UE_LOG(LogNovel, Warning, TEXT("PlayBGM ignored. Reason=Null BGM asset"));
        return;
    }

    if (CurrentBGM == NewBGM && CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        UE_LOG(LogNovel, Verbose, TEXT("PlayBGM ignored duplicate track. Track=%s"), *NewBGM.ToSoftObjectPath().ToString());
        return;
    }

    CurrentBGM = NewBGM;
    OnMusicChangedEvent.Broadcast(CurrentBGM);
    const int32 ExpectedBGMLoadSerial = ++BGMLoadSerial;

    if (CurrentBGMLoadHandle.IsValid())
    {
        CurrentBGMLoadHandle->CancelHandle();
        CurrentBGMLoadHandle.Reset();
    }

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    CurrentBGMLoadHandle = Streamable.RequestAsyncLoad(
        NewBGM.ToSoftObjectPath(),
        FStreamableDelegate::CreateUObject(this, &UNovelStorySubsystem::OnBGMAssetLoaded, NewBGM, FadeInTime, ExpectedBGMLoadSerial));
}

void UNovelStorySubsystem::OnBGMAssetLoaded(TSoftObjectPtr<USoundBase> RequestedBGM, float FadeInTime, int32 ExpectedBGMLoadSerial)
{
    if (ExpectedBGMLoadSerial != BGMLoadSerial || RequestedBGM != CurrentBGM)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignoring stale BGM load. Requested=%s"), *RequestedBGM.ToSoftObjectPath().ToString());
        return;
    }

    USoundBase* LoadedSound = RequestedBGM.Get();
    if (!LoadedSound)
    {
        UE_LOG(LogNovel, Warning, TEXT("PlayBGM failed. Track=%s Reason=Asset load returned null"), *RequestedBGM.ToSoftObjectPath().ToString());
        CurrentBGM = nullptr;
        OnMusicChangedEvent.Broadcast(CurrentBGM);
        CurrentBGMLoadHandle.Reset();
        return;
    }

    if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        CurrentBGMComponent->FadeOut(FadeInTime, 0.0f);
    }

    CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, LoadedSound);
    if (CurrentBGMComponent)
    {
        const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
        if (Settings && !Settings->BGMSoundClass.IsNull())
        {
            CurrentBGMComponent->SoundClassOverride = Settings->BGMSoundClass.LoadSynchronous();
        }

        CurrentBGMComponent->FadeIn(FMath::Max(0.0f, FadeInTime));
    }

    CurrentBGMLoadHandle.Reset();
}

void UNovelStorySubsystem::StopBGM(float FadeOutTime)
{
    ++BGMLoadSerial;
    CurrentBGM = nullptr;
    OnMusicChangedEvent.Broadcast(CurrentBGM);

    if (CurrentBGMLoadHandle.IsValid())
    {
        CurrentBGMLoadHandle->CancelHandle();
        CurrentBGMLoadHandle.Reset();
    }

    if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        CurrentBGMComponent->FadeOut(FMath::Max(0.0f, FadeOutTime), 0.0f);
    }

    CurrentBGMComponent = nullptr;
}

void UNovelStorySubsystem::PlaySFX(FName SFXKey)
{
    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    if (!Settings || !Settings->SFXBank.Contains(SFXKey))
    {
        UE_LOG(LogNovel, Warning, TEXT("PlaySFX failed. SFXKey=%s Reason=Missing SFX bank entry"), *SFXKey.ToString());
        return;
    }

    const FNovelSoundConfig& SoundConfig = Settings->SFXBank[SFXKey];
    if (SoundConfig.SoundAsset.IsNull())
    {
        UE_LOG(LogNovel, Warning, TEXT("PlaySFX failed. SFXKey=%s Reason=Null sound asset"), *SFXKey.ToString());
        return;
    }

    USoundBase* LoadedSound = SoundConfig.SoundAsset.LoadSynchronous();
    if (!LoadedSound)
    {
        UE_LOG(LogNovel, Warning, TEXT("PlaySFX failed. SFXKey=%s Reason=Sound load returned null"), *SFXKey.ToString());
        return;
    }

    UAudioComponent* SFXComponent = UGameplayStatics::CreateSound2D(this, LoadedSound, 1.0f, 1.0f, SoundConfig.StartTimeOffset);
    if (!SFXComponent)
    {
        return;
    }

    if (!Settings->SFXSoundClass.IsNull())
    {
        SFXComponent->SoundClassOverride = Settings->SFXSoundClass.LoadSynchronous();
    }

    SFXComponent->bAutoDestroy = true;
    SFXComponent->Play(SoundConfig.StartTimeOffset);
}

void UNovelStorySubsystem::ApplyAudioSettingsFromData(float Master, float BGM, float SFX)
{
    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    if (!Settings)
    {
        return;
    }

    USoundMix* LoadedMix = Settings->GlobalSoundMix.LoadSynchronous();
    if (!LoadedMix)
    {
        return;
    }

    UGameplayStatics::PushSoundMixModifier(this, LoadedMix);

    if (USoundClass* LoadedMaster = Settings->MasterSoundClass.LoadSynchronous())
    {
        UGameplayStatics::SetSoundMixClassOverride(this, LoadedMix, LoadedMaster, Master, 1.0f, 0.0f, true);
    }

    if (USoundClass* LoadedBGM = Settings->BGMSoundClass.LoadSynchronous())
    {
        UGameplayStatics::SetSoundMixClassOverride(this, LoadedMix, LoadedBGM, BGM, 1.0f, 0.0f, true);
    }

    if (USoundClass* LoadedSFX = Settings->SFXSoundClass.LoadSynchronous())
    {
        UGameplayStatics::SetSoundMixClassOverride(this, LoadedMix, LoadedSFX, SFX, 1.0f, 0.0f, true);
    }
}
