#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "FDialogueNodeHandle.h"
#include "NovelDialogueBranchData.h"
#include "NovelIntentBase.h"
#include "NovelChapterAsset.h"
#include "NovelStoryTypes.h"
#include "NovelValue.h"
#include "NovelRuntimeSnapshot.h"
#include "NovelSaveIndex.h"
#include "TimerManager.h"
#include "NovelStorySubsystem.generated.h"

class USoundBase;
class UAudioComponent;
class UNovelDialogueBranchData;
class UNovelSaveGame;
class UNovelActionContext;
class UNovelStoryAsset;
class USaveGame;
class UDataTable;
class UTexture2D;

/** -------------------------------------------------------------------------- *
 *  System Delegates
 * --------------------------------------------------------------------------- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineChanged, const FText&, Speaker, const FText&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveLoadUIRequested, bool, bIsSaveMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNovelRuntimeStateChanged, ENovelRuntimeState, NewState, ENovelRuntimeState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNovelStoryError, const FText&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSaveLoadOperationFinished, bool, bSuccess, const FString&, SlotName, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHistoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNovelLoadingStateChanged, bool, bIsLoading, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNovelStoryCompleted, int32, ChapterID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNovelMusicChanged, TSoftObjectPtr<USoundBase>, NewMusic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNovelVariableChanged, FName, VariableName, FNovelValue, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNovelVariablesReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNovelSaveIndexChanged);

/** -------------------------------------------------------------------------- *
 *  Visual & UI Delegates
 * --------------------------------------------------------------------------- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueOptionsShow, const TArray<FDialogueOption>&, Options);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueOptionsHide);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackgroundChanged, TSoftObjectPtr<UTexture2D>, NewBackground);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCharacterShown, FName, CharacterID, TSoftObjectPtr<UTexture2D>, CharacterSprite, float, XPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterHidden, FName, CharacterID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScreenFade, FLinearColor, FadeColor, float, FadeDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueResetRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsUIRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHistoryUIRequested);

UCLASS()
class NOVELRUNTIME_API UNovelStorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Story")
    void Advance();

    UFUNCTION(BlueprintCallable, Category = "Story", meta = (DeprecatedFunction, DeprecationMessage = "Use Advance."))
    void NextDialogue();

    UFUNCTION(BlueprintCallable, Category = "Story")
    void StartStory();

    UFUNCTION(BlueprintCallable, Category = "Story")
    void StopStory();

    UFUNCTION(BlueprintCallable, Category = "Story")
    void StartStoryAsset(TSoftObjectPtr<UNovelStoryAsset> StoryAsset);

    UFUNCTION(BlueprintCallable, Category = "Story", meta = (DeprecatedFunction, DeprecationMessage = "Use StartStory."))
    void StartNewGame();

    UFUNCTION(BlueprintCallable, Category = "Story")
    void ContinueLastGame();

    UFUNCTION(BlueprintCallable, Category = "Story", meta = (DeprecatedFunction, DeprecationMessage = "Quit behavior belongs to the host application."))
    void QuitGame();

    UFUNCTION(BlueprintPure, Category = "Story")
    ENovelRuntimeState GetRuntimeState() const { return RuntimeState; }

    UFUNCTION(BlueprintPure, Category = "Story")
    FDialogueNodeHandle GetCurrentNodeHandle() const { return CurrentNode; }

    UFUNCTION(BlueprintPure, Category = "Story")
    FNovelNodeRef GetCurrentNodeRef() const { return CurrentNodeRef; }

    UFUNCTION(BlueprintPure, Category = "Story|Snapshot")
    FNovelRuntimeSnapshot CreateSnapshot() const;

    UFUNCTION(BlueprintCallable, Category = "Story|Snapshot")
    bool RestoreSnapshot(const FNovelRuntimeSnapshot& Snapshot, FText& OutError);
    UFUNCTION(BlueprintPure, Category = "Story|History")
    TArray<FNovelHistoryEntry> GetHistory() const { return DialogueHistory; }

    const TArray<FNovelHistoryEntry>& GetHistoryView() const { return DialogueHistory; }

    UPROPERTY(BlueprintAssignable, Category = "Story|Variables")
    FOnNovelVariableChanged OnVariableChangedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Story|Variables")
    FOnNovelVariablesReset OnVariablesResetEvent;

    UFUNCTION(BlueprintPure, Category = "Story|Variables")
    bool HasVariable(FName VariableName) const;

    UFUNCTION(BlueprintPure, Category = "Story|Variables")
    bool GetVariable(FName VariableName, FNovelValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    bool SetVariable(FName VariableName, FNovelValue Value, FText& OutError);

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    bool RemoveVariable(FName VariableName);

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    void ClearVariables();

    UFUNCTION(BlueprintPure, Category = "Story|Variables")
    bool GetBool(FName VariableName, bool& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    bool SetBool(FName VariableName, bool Value, FText& OutError);

    UFUNCTION(BlueprintPure, Category = "Story|Variables")
    bool GetInteger(FName VariableName, int64& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    bool SetInteger(FName VariableName, int64 Value, FText& OutError);

    UFUNCTION(BlueprintPure, Category = "Story|Variables")
    bool GetFloat(FName VariableName, double& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    bool SetFloat(FName VariableName, double Value, FText& OutError);

    UFUNCTION(BlueprintPure, Category = "Story|Variables")
    bool GetName(FName VariableName, FName& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    bool SetName(FName VariableName, FName Value, FText& OutError);

    UFUNCTION(BlueprintPure, Category = "Story|Variables")
    bool GetString(FName VariableName, FString& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Story|Variables")
    bool SetString(FName VariableName, const FString& Value, FText& OutError);

    const TMap<FName, FNovelValue>& GetVariablesView() const { return Variables; }
    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnNovelRuntimeStateChanged OnRuntimeStateChangedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnNovelStoryError OnStoryErrorEvent;

    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnNovelLoadingStateChanged OnStoryLoadingStateChangedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnSaveLoadOperationFinished OnSaveLoadOperationFinishedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnHistoryChanged OnHistoryChangedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnNovelStoryCompleted OnStoryCompletedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Story|Presentation")
    FOnNovelMusicChanged OnMusicChangedEvent;

    UPROPERTY(BlueprintAssignable, meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSubsystem navigation events."))
    FOnHistoryUIRequested OnHistoryUIRequestedEvent;

    UPROPERTY(BlueprintAssignable)
    FOnBackgroundChanged OnBackgroundChangedEvent;

    UPROPERTY(BlueprintAssignable)
    FOnCharacterShown OnCharacterShownEvent;

    UPROPERTY(BlueprintAssignable)
    FOnCharacterHidden OnCharacterHiddenEvent;

    UPROPERTY(BlueprintAssignable)
    FOnScreenFade OnScreenFadeEvent;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueLineChanged OnDialogueLineChanged;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueOptionsShow OnDialogueOptionsShowEvent;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueOptionsHide OnDialogueOptionsHideEvent;

    UPROPERTY(BlueprintAssignable, meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSubsystem navigation events."))
    FOnSaveLoadUIRequested OnSaveLoadUIRequestedEvent;

    UPROPERTY(BlueprintAssignable)
    FOnGameStarted OnGameStartedEvent;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueResetRequested OnDialogueResetRequestedEvent;

    UPROPERTY(BlueprintAssignable, meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSubsystem navigation events."))
    FOnSettingsUIRequested OnSettingsUIRequestedEvent;

    UPROPERTY()
    UNovelDialogueBranchData* BranchData = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Story")
    int32 CurrentChapterID = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, Category = "Story")
    TSoftObjectPtr<UTexture2D> CurrentBackground;

    UFUNCTION(BlueprintCallable, Category = "Story", meta = (DeprecatedFunction, DeprecationMessage = "Use UNovelPresentationSubsystem::RequestSaveLoadScreen."))
    void ShowSaveLoadMenu(bool bIsSaveMode);

    UFUNCTION(BlueprintCallable, Category = "Story", meta = (DeprecatedFunction, DeprecationMessage = "Use UNovelPresentationSubsystem::RequestHistoryScreen."))
    void ShowHistory();

    UFUNCTION(BlueprintCallable, Category = "Story", meta = (DeprecatedFunction, DeprecationMessage = "Use UNovelPresentationSubsystem::RequestSettingsScreen."))
    void ShowSettingsMenu();

    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    TArray<FString> GetAllSaveSlotNames() const;

    UFUNCTION(BlueprintPure, Category = "Story|SaveLoad")
    bool GetSaveSlotMetadata(const FString& SlotName, FNovelSaveSlotMetadata& OutMetadata) const;

    UFUNCTION(BlueprintPure, Category = "Story|SaveLoad")
    TArray<FNovelSaveSlotMetadata> GetSaveSlotMetadataList() const;

    UPROPERTY(BlueprintAssignable, Category = "Story|SaveLoad")
    FOnNovelSaveIndexChanged OnSaveIndexChangedEvent;

    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    void SaveGame(FString SlotNameOverride = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    void LoadGame(FString SlotName);

    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    bool DeleteSaveSlot(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Story|Intent")
    void JumpToNode(FDialogueNodeHandle TargetNode);

    UFUNCTION(BlueprintCallable, Category = "Story|Intent")
    void RequestNodeTransition(FDialogueNodeHandle TargetNode, bool bClearScreen = false);

    UFUNCTION(BlueprintCallable, Category = "Story|Action")
    void RequestNodeTransitionByRef(FNovelNodeRef TargetNode, bool bClearScreen = false);

    UFUNCTION(BlueprintCallable, Category = "Story|Intent")
    void ProcessIntents(const TArray<UNovelIntentBase*>& Intents);

    UFUNCTION(BlueprintCallable, Category = "Story|Choice")
    void SelectChoice(int32 ChoiceIndex);

    UFUNCTION(BlueprintCallable, Category = "Story|Choice")
    void SelectChoiceForNode(int32 ChoiceIndex, FDialogueNodeHandle SourceNode);

    UFUNCTION(BlueprintCallable, Category = "Story|Visuals")
    void ResetVisualState();

    UFUNCTION(BlueprintCallable, Category = "Story|Visuals")
    void SetBackground(TSoftObjectPtr<UTexture2D> NewBackground);

    UFUNCTION(BlueprintCallable, Category = "Story|Visuals")
    void ShowCharacter(FName CharacterID, TSoftObjectPtr<UTexture2D> CharacterSprite, float XPosition);

    UFUNCTION(BlueprintCallable, Category = "Story|Visuals")
    void HideCharacter(FName CharacterID);

    void RequestScreenFade(FLinearColor FadeColor, float FadeDuration, const FNovelIntentCompleted& Completion, bool bWaitUntilFinished);

    UFUNCTION(BlueprintCallable, Category = "Story|Visuals")
    void NotifyScreenFadeFinished();

    UFUNCTION(BlueprintCallable, Category = "Story|Audio")
    void PlayBGM(TSoftObjectPtr<USoundBase> NewBGM, float FadeInTime = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Story|Audio")
    void StopBGM(float FadeOutTime = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Story|Audio")
    void PlaySFX(FName SFXKey);

    UFUNCTION(BlueprintCallable, Category = "Story|Audio")
    void ApplyAudioSettingsFromData(float Master, float BGM, float SFX);

    void CompleteIntentFromProxy(int32 CompletionSessionId, int32 CompletionNodeExecutionId, int32 CompletionIntentExecutionId);
    void FailActionFromContext(int32 CompletionSessionId, int32 CompletionNodeExecutionId, int32 CompletionIntentExecutionId, const FText& ErrorMessage);

private:
    static constexpr int32 CurrentSaveSchemaVersion = 4;
    static constexpr float IntentCompletionTimeoutSeconds = 30.0f;

    UPROPERTY()
    ENovelRuntimeState RuntimeState = ENovelRuntimeState::Idle;

    UPROPERTY()
    ENovelRuntimeState LastRecoverableState = ENovelRuntimeState::Idle;

    UPROPERTY()
    FDialogueNodeHandle CurrentNode;

    UPROPERTY()
    UDataTable* ActiveDialogueTable = nullptr;

    UPROPERTY()
    TObjectPtr<UNovelStoryAsset> ActiveStoryAsset;

    UPROPERTY()
    TObjectPtr<UNovelChapterAsset> ActiveChapterAsset;

    UPROPERTY()
    TMap<FName, FNovelNode> ActiveNodes;

    UPROPERTY()
    TArray<UNovelIntentBase*> CurrentIntentQueue;

    UPROPERTY()
    UAudioComponent* CurrentBGMComponent = nullptr;

    UPROPERTY()
    TSoftObjectPtr<USoundBase> CurrentBGM;

    UPROPERTY()
    TArray<FNovelHistoryEntry> DialogueHistory;

    UPROPERTY()
    TMap<FName, FNovelValue> Variables;

    UPROPERTY()
    TArray<int32> ActiveChoiceIndices;

    UPROPERTY()
    TMap<FName, FNovelCharacterPresentationState> VisibleCharacters;

    UPROPERTY()
    UNovelSaveGame* PendingLoadedSave = nullptr;

    UPROPERTY()
    TObjectPtr<UNovelSaveIndex> SaveIndex;

    UPROPERTY()
    TObjectPtr<UNovelActionContext> ActiveActionContext;

    TSharedPtr<FStreamableHandle> CurrentBGMLoadHandle;
    FNovelIntentCompleted PendingFadeCompletion;
    FTimerHandle IntentTimeoutTimerHandle;
    FTimerHandle FadeFallbackTimerHandle;

    int32 StorySessionId = 0;
    int32 ChapterLoadSerial = 0;
    int32 PendingLoadingSubsystemRequestId = INDEX_NONE;
    int32 NodeExecutionId = 0;
    int32 IntentExecutionId = 0;
    int32 ActiveIntentExecutionId = INDEX_NONE;
    int32 CurrentIntentIndex = 0;
    int32 NextHistorySequenceIndex = 0;
    int32 BGMLoadSerial = 0;
    int32 FadeSerial = 0;
    int32 ActiveFadeSerial = INDEX_NONE;

    bool bShowChoicesAfterIntentExecution = false;
    bool bHasPendingTransition = false;
    bool bHasPendingNodeRefTransition = false;
    bool bPendingTransitionClearScreen = false;

    FDialogueNodeHandle LastPublishedNode;
    FNovelNodeRef CurrentNodeRef;
    FPrimaryAssetId ActiveChapterId;
    FName ActiveEntryNodeId = NAME_None;
    bool bHasPostIntentTransition = false;
    bool bPendingUnifiedClearScreen = false;
    FNovelNodeRef PostIntentTransition;
    FDialogueNodeHandle PendingTransitionNode;
    FNovelNodeRef PendingNodeRefTransition;
    ENovelRuntimeState StateBeforeSaveOrLoad = ENovelRuntimeState::Idle;
    FNovelSaveSlotMetadata PendingSaveMetadata;
    bool bSaveIndexInitialized = false;
    bool bSaveIndexPersistenceBlocked = false;

    bool SetRuntimeState(ENovelRuntimeState NewState, const TCHAR* Reason);
    void RestoreRuntimeStateAfterAsyncFailure(ENovelRuntimeState PreviousState, const FText& ErrorMessage);
    void ReportError(const FString& Operation, const FText& ErrorMessage, ENovelRuntimeState ErrorState = ENovelRuntimeState::Error);
    bool CanStartBlockingOperation(const TCHAR* Operation) const;
    void RejectCommand(const TCHAR* Operation, ENovelRuntimeState ExpectedState) const;
    FString RuntimeStateToString(ENovelRuntimeState State) const;

    bool BeginChapterLoad(int32 ChapterIndex, FDialogueNodeHandle OptionalStartNode, bool bFromSave, ENovelRuntimeState PreviousState);
    void OnChapterLoadedAsync(int32 ChapterIndex, FDialogueNodeHandle OptionalStartNode, int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState, bool bFromSave);
    bool RetainLoadedChapter(int32 ChapterIndex, FText& OutErrorMessage);
    bool BuildLegacyActiveNodes(int32 ChapterIndex, FText& OutErrorMessage);
    bool RetainUnifiedChapter(UNovelChapterAsset* ChapterAsset, FText& OutErrorMessage);
    bool BeginStoryAssetLoad(TSoftObjectPtr<UNovelStoryAsset> StoryAsset, ENovelRuntimeState PreviousState);
    void OnStoryAssetLoaded(TSoftObjectPtr<UNovelStoryAsset> StoryAsset, int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState);
    bool BeginUnifiedChapterLoad(FNovelNodeRef TargetNode, ENovelRuntimeState PreviousState, bool bBroadcastStoryStarted);
    void OnUnifiedChapterLoaded(TSoftObjectPtr<UNovelChapterAsset> ChapterAsset, FNovelNodeRef TargetNode, int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState, bool bBroadcastStoryStarted);

    void OnAsyncLoadGameComplete(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);
    void OnAsyncSaveGameComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess);
    void OnAsyncSaveIndexComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess, FString StorySlotName);
    void InitializeSaveIndex();
    void RepairSaveIndex();
    void SortSaveIndex();
    void UpsertSaveIndexEntry(const FNovelSaveSlotMetadata& Metadata);
    FString GetSaveIndexSlotName() const;
    bool BeginUnifiedSaveAssetsLoad(UNovelSaveGame* SaveGame, ENovelRuntimeState PreviousState);
    void OnUnifiedSaveAssetsLoaded(int32 ExpectedSessionId, int32 ExpectedLoadSerial, ENovelRuntimeState PreviousState);
    bool ValidateSaveGameForLoad(const UNovelSaveGame* SaveGame, const FString& SlotName, FText& OutErrorMessage) const;
    bool TryReadSaveMetadata(const FString& SlotName, FNovelSaveMetadata& OutMetadata) const;
    FDateTime GetSaveFileTimestamp(const FString& SlotName) const;
    FString GetSaveSlotFilePath(const FString& SlotName) const;
    void ApplyLoadedSave();

    bool EnterNode(const FDialogueNodeHandle& NodeHandle);
    bool EnterNodeRef(const FNovelNodeRef& NodeRef, bool bAddHistoryEntry = true);
    bool RequestNodeRefTransition(const FNovelNodeRef& TargetNode, bool bClearScreen);
    const FNovelNode* FindActiveNode(FName NodeId) const;
    const FNovelNode* GetCurrentNodeData() const;
    bool RestoreLoadedNode(const FDialogueNodeHandle& NodeHandle, bool bRestoreChoice);
    bool ValidateNodeHandle(const FDialogueNodeHandle& NodeHandle, FText& OutErrorMessage) const;
    void BroadcastCurrentLine(bool bAddHistoryEntry);
    void AdvanceToNextRow();
    void ExecuteCurrentNode();
    void PlayNextIntent();
    void FinishIntentExecution();
    void HandleIntentTimeout(int32 ExpectedSessionId, int32 ExpectedNodeExecutionId, int32 ExpectedIntentExecutionId);
    void ApplyPendingTransitionIfAny();
    void ShowCurrentChoices(const FNovelNode& Node);
    void ClearIntentExecutionState();

    void AppendHistoryEntry(const FText& Speaker, const FText& Text);
    void RestorePresentationFromSave(const UNovelSaveGame& SaveGame);
    void CompletePendingFade(int32 ExpectedFadeSerial);
    void OnBGMAssetLoaded(TSoftObjectPtr<USoundBase> RequestedBGM, float FadeInTime, int32 ExpectedBGMLoadSerial);
};