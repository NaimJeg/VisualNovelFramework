#include "NovelRunner.h"

#include "NovelExpression.h"
#include "NovelIntentBase.h"

FNovelRunnerActionResult FNovelRunnerActionResult::Success()
{
    return FNovelRunnerActionResult();
}

FNovelRunnerActionResult FNovelRunnerActionResult::Failure(const FText& ErrorMessage)
{
    FNovelRunnerActionResult Result;
    Result.bSuccess = false;
    Result.Error = ErrorMessage;
    return Result;
}

FNovelRunnerActionResult FNovelRunnerActionResult::TransitionTo(const FNovelNodeRef& Target, const bool bInClearScreen)
{
    FNovelRunnerActionResult Result;
    Result.bRequestTransition = true;
    Result.Transition = Target;
    Result.bClearScreen = bInClearScreen;
    return Result;
}

struct FNovelRunner::FLifetimeToken
{
    FNovelRunner* Owner = nullptr;
};

FNovelRunner::FNovelRunner()
    : LifetimeToken(MakeShared<FLifetimeToken>())
{
    LifetimeToken->Owner = this;
}

FNovelRunner::~FNovelRunner()
{
    LifetimeToken->Owner = nullptr;
    LifetimeToken.Reset();
}

void FNovelRunner::RegisterChapter(const UNovelChapterAsset* Chapter)
{
    if (Chapter)
    {
        Chapters.Add(Chapter->GetPrimaryAssetId(), Chapter);
    }
}

void FNovelRunner::ClearChapters()
{
    Stop();
    Chapters.Reset();
}

void FNovelRunner::SetActionExecutor(FNovelRunnerActionExecutor InExecutor)
{
    ActionExecutor = MoveTemp(InExecutor);
}

void FNovelRunner::SetActionTimeout(const double InSeconds)
{
    ActionTimeoutSeconds = FMath::Max(0.001, InSeconds);
}

bool FNovelRunner::Start(const FNovelNodeRef& EntryNode, FText& OutError)
{
    InvalidateCallbacks();
    State = ENovelRuntimeState::Idle;
    bComplete = false;
    LastError = FText::GetEmpty();
    History.Reset();
    Variables.Reset();
    NextHistorySequence = 0;
    CurrentTimeSeconds = 0.0;
    return EnterNode(EntryNode, true, OutError);
}

bool FNovelRunner::Advance(FText& OutError)
{
    OutError = FText::GetEmpty();
    if (State != ENovelRuntimeState::AwaitingAdvance)
    {
        OutError = FText::FromString(TEXT("Advance is only valid while awaiting advance."));
        return false;
    }

    const FNovelNode* Node = GetCurrentNode();
    if (!Node)
    {
        OutError = FText::FromString(TEXT("Current node no longer exists."));
        Fail(OutError);
        return false;
    }
    if (!Node->Next.IsValid())
    {
        State = ENovelRuntimeState::Idle;
        bComplete = true;
        return true;
    }
    if (!FindNode(Node->Next))
    {
        OutError = FText::FromString(FString::Printf(TEXT("Next target %s does not exist."), *Node->Next.ToString()));
        Fail(OutError);
        return false;
    }
    return EnterNode(Node->Next, true, OutError);
}

bool FNovelRunner::SelectChoice(const int32 PresentedChoiceIndex, const uint64 ExpectedChoiceGeneration, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (State != ENovelRuntimeState::AwaitingChoice)
    {
        OutError = FText::FromString(TEXT("Choice selection is only valid while awaiting a choice."));
        return false;
    }
    if (ExpectedChoiceGeneration != ChoiceGeneration)
    {
        OutError = FText::FromString(TEXT("Choice selection is stale."));
        return false;
    }
    if (!AvailableChoiceIndices.IsValidIndex(PresentedChoiceIndex))
    {
        OutError = FText::FromString(FString::Printf(TEXT("Choice index %d is invalid."), PresentedChoiceIndex));
        return false;
    }

    const FNovelNode* Node = GetCurrentNode();
    const int32 AuthoredIndex = AvailableChoiceIndices[PresentedChoiceIndex];
    if (!Node || !Node->Choices.IsValidIndex(AuthoredIndex))
    {
        OutError = FText::FromString(TEXT("Authored choice no longer exists."));
        Fail(OutError);
        return false;
    }

    const FNovelChoice& Choice = Node->Choices[AuthoredIndex];
    if (!Choice.Target.IsValid() || !FindNode(Choice.Target))
    {
        OutError = FText::FromString(FString::Printf(TEXT("Choice target %s does not exist."), *Choice.Target.ToString()));
        return false;
    }

    AvailableChoiceIndices.Reset();
    ++ChoiceGeneration;
    if (Choice.Actions.IsEmpty())
    {
        return EnterNode(Choice.Target, true, OutError);
    }
    BeginActions(Choice.Actions, &Choice.Target);
    return State != ENovelRuntimeState::Error;
}

void FNovelRunner::Tick(const double NowSeconds)
{
    CurrentTimeSeconds = FMath::Max(CurrentTimeSeconds, NowSeconds);
    if (State == ENovelRuntimeState::ExecutingIntents && bWaitingForAction && CurrentTimeSeconds >= ActionDeadlineSeconds)
    {
        Fail(FText::FromString(FString::Printf(TEXT("Action %llu timed out after %.3f seconds."), ActionGeneration, ActionTimeoutSeconds)));
    }
}

void FNovelRunner::Stop()
{
    InvalidateCallbacks();
    State = ENovelRuntimeState::Idle;
    CurrentNode = FNovelNodeRef();
    AvailableChoiceIndices.Reset();
    ActionQueue.Reset();
    bComplete = false;
    LastError = FText::GetEmpty();
}

const FNovelNode* FNovelRunner::GetCurrentNode() const
{
    return FindNode(CurrentNode);
}

bool FNovelRunner::ConsumeClearScreenRequest()
{
    const bool bResult = bRequestedClearScreen;
    bRequestedClearScreen = false;
    return bResult;
}

bool FNovelRunner::SetVariable(const FName Name, const FNovelValue& Value, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (Name.IsNone() || Value.Type == ENovelValueType::None)
    {
        OutError = FText::FromString(TEXT("Variable name and value type must be valid."));
        return false;
    }
    if (const FNovelValue* Existing = Variables.Find(Name))
    {
        if (Existing->Type == ENovelValueType::Float && Value.Type == ENovelValueType::Integer)
        {
            Variables.Add(Name, FNovelValue::MakeFloat(static_cast<double>(Value.IntegerValue)));
            return true;
        }
        if (Existing->Type != Value.Type)
        {
            OutError = FText::FromString(FString::Printf(TEXT("Variable %s cannot change from %s to %s."), *Name.ToString(), *Existing->GetTypeName(), *Value.GetTypeName()));
            return false;
        }
    }
    Variables.Add(Name, Value);
    return true;
}

bool FNovelRunner::RemoveVariable(const FName Name)
{
    return Variables.Remove(Name) > 0;
}

bool FNovelRunner::GetVariable(const FName Name, FNovelValue& OutValue) const
{
    const FNovelValue* Value = Variables.Find(Name);
    if (!Value)
    {
        return false;
    }
    OutValue = *Value;
    return true;
}

FNovelRuntimeSnapshot FNovelRunner::CreateSnapshot() const
{
    FNovelRuntimeSnapshot Snapshot;
    Snapshot.StoryId = StoryId;
    Snapshot.CurrentNode = CurrentNode;
    if (const UNovelChapterAsset* Chapter = FindChapter(CurrentNode.ChapterId))
    {
        Snapshot.ChapterAsset = const_cast<UNovelChapterAsset*>(Chapter);
    }
    Snapshot.RestoredState = State;
    Snapshot.History = History;
    Snapshot.Variables = Variables;
    Snapshot.bAtChoice = State == ENovelRuntimeState::AwaitingChoice;
    Snapshot.NextHistorySequenceIndex = NextHistorySequence;
    return Snapshot;
}

bool FNovelRunner::RestoreSnapshot(const FNovelRuntimeSnapshot& Snapshot, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (Snapshot.SchemaVersion != 1)
    {
        OutError = FText::FromString(FString::Printf(TEXT("Unsupported snapshot schema %d."), Snapshot.SchemaVersion));
        return false;
    }
    if (!Snapshot.CurrentNode.IsValid() || !FindNode(Snapshot.CurrentNode))
    {
        OutError = FText::FromString(TEXT("Snapshot current node does not exist in registered chapters."));
        return false;
    }

    InvalidateCallbacks();
    StoryId = Snapshot.StoryId;
    CurrentNode = Snapshot.CurrentNode;
    History = Snapshot.History;
    Variables = Snapshot.Variables;
    NextHistorySequence = FMath::Max(Snapshot.NextHistorySequenceIndex, History.IsEmpty() ? 0 : History.Last().SequenceIndex + 1);
    bComplete = false;
    LastError = FText::GetEmpty();
    if (Snapshot.bAtChoice)
    {
        if (!BuildAvailableChoices(OutError))
        {
            Fail(OutError);
            return false;
        }
    }
    else
    {
        State = ENovelRuntimeState::AwaitingAdvance;
        AvailableChoiceIndices.Reset();
    }
    return true;
}

const FNovelNode* FNovelRunner::FindNode(const FNovelNodeRef& Ref) const
{
    const UNovelChapterAsset* Chapter = FindChapter(Ref.ChapterId);
    return Chapter && Ref.IsValid() ? Chapter->FindNode(Ref.NodeId) : nullptr;
}

const UNovelChapterAsset* FNovelRunner::FindChapter(const FPrimaryAssetId ChapterId) const
{
    const TWeakObjectPtr<const UNovelChapterAsset>* Chapter = Chapters.Find(ChapterId);
    return Chapter ? Chapter->Get() : nullptr;
}

bool FNovelRunner::EnterNode(const FNovelNodeRef& Ref, const bool bRecordHistory, FText& OutError)
{
    OutError = FText::GetEmpty();
    const FNovelNode* Node = FindNode(Ref);
    if (!Node)
    {
        OutError = FText::FromString(FString::Printf(TEXT("Node %s does not exist."), *Ref.ToString()));
        Fail(OutError);
        return false;
    }

    ++NodeGeneration;
    CurrentNode = Ref;
    State = ENovelRuntimeState::PresentingDialogue;
    AvailableChoiceIndices.Reset();
    if (bRecordHistory)
    {
        FNovelHistoryEntry& Entry = History.AddDefaulted_GetRef();
        Entry.Speaker = Node->Speaker;
        Entry.Text = Node->Text;
        Entry.StoryAssetId = StoryId;
        Entry.ChapterAssetId = Ref.ChapterId;
        Entry.NodeID = Ref.NodeId;
        Entry.SequenceIndex = NextHistorySequence++;
        Entry.Timestamp = FDateTime::FromUnixTimestamp(FMath::FloorToInt64(CurrentTimeSeconds));
    }

    if (Node->EntryActions.IsEmpty())
    {
        FinishNodeActions();
    }
    else
    {
        BeginActions(Node->EntryActions);
    }
    return State != ENovelRuntimeState::Error;
}

bool FNovelRunner::BuildAvailableChoices(FText& OutError)
{
    OutError = FText::GetEmpty();
    const FNovelNode* Node = GetCurrentNode();
    if (!Node)
    {
        OutError = FText::FromString(TEXT("Current node does not exist while evaluating choices."));
        return false;
    }

    AvailableChoiceIndices.Reset();
    UNovelExpressionContext* Context = NewObject<UNovelExpressionContext>();
    Context->Initialize(Variables);
    for (int32 ChoiceIndex = 0; ChoiceIndex < Node->Choices.Num(); ++ChoiceIndex)
    {
        const FNovelChoice& Choice = Node->Choices[ChoiceIndex];
        if (!Choice.Target.IsValid() || !FindNode(Choice.Target))
        {
            OutError = FText::FromString(FString::Printf(TEXT("Choice %d target %s does not exist."), ChoiceIndex, *Choice.Target.ToString()));
            return false;
        }
        if (Choice.Condition)
        {
            FNovelValue Value;
            if (!Choice.Condition->Evaluate(Context, Value, OutError))
            {
                return false;
            }
            if (Value.Type != ENovelValueType::Bool)
            {
                OutError = FText::FromString(FString::Printf(TEXT("Choice %d condition did not return Bool."), ChoiceIndex));
                return false;
            }
            if (!Value.BoolValue)
            {
                continue;
            }
        }
        AvailableChoiceIndices.Add(ChoiceIndex);
    }

    if (AvailableChoiceIndices.IsEmpty())
    {
        State = ENovelRuntimeState::AwaitingAdvance;
    }
    else
    {
        ++ChoiceGeneration;
        State = ENovelRuntimeState::AwaitingChoice;
    }
    return true;
}

void FNovelRunner::BeginActions(const TArray<TObjectPtr<UNovelIntentBase>>& Actions, const FNovelNodeRef* InTransitionAfterActions)
{
    ActionQueue = Actions;
    ActionCursor = 0;
    bTransitionAfterActions = InTransitionAfterActions != nullptr;
    TransitionAfterActions = InTransitionAfterActions ? *InTransitionAfterActions : FNovelNodeRef();
    bRequestedTransition = false;
    RequestedTransition = FNovelNodeRef();
    State = ENovelRuntimeState::ExecutingIntents;
    PumpActions();
}

void FNovelRunner::PumpActions()
{
    if (bPumpingActions || State != ENovelRuntimeState::ExecutingIntents)
    {
        return;
    }

    TGuardValue<bool> PumpGuard(bPumpingActions, true);
    while (State == ENovelRuntimeState::ExecutingIntents && !bWaitingForAction)
    {
        if (bRequestedTransition)
        {
            const FNovelNodeRef Target = RequestedTransition;
            bRequestedTransition = false;
            ActionQueue.Reset();
            FText Error;
            EnterNode(Target, true, Error);
            return;
        }
        if (!ActionQueue.IsValidIndex(ActionCursor))
        {
            FinishNodeActions();
            return;
        }

        UNovelIntentBase* Action = ActionQueue[ActionCursor++];
        if (!Action)
        {
            Fail(FText::FromString(FString::Printf(TEXT("Action %d is null."), ActionCursor - 1)));
            return;
        }
        if (!ActionExecutor)
        {
            Fail(FText::FromString(TEXT("No action executor is configured.")));
            return;
        }

        bWaitingForAction = true;
        ActionDeadlineSeconds = CurrentTimeSeconds + ActionTimeoutSeconds;
        const uint64 ExpectedSession = SessionGeneration;
        const uint64 ExpectedNode = NodeGeneration;
        const uint64 ExpectedAction = ++ActionGeneration;
        TWeakPtr<FLifetimeToken> WeakLifetime = LifetimeToken;
        ActionExecutor(Action, [WeakLifetime, ExpectedSession, ExpectedNode, ExpectedAction](const FNovelRunnerActionResult& Result)
        {
            const TSharedPtr<FLifetimeToken> Lifetime = WeakLifetime.Pin();
            if (Lifetime && Lifetime->Owner)
            {
                Lifetime->Owner->CompleteAction(ExpectedSession, ExpectedNode, ExpectedAction, Result);
            }
        });
        if (bWaitingForAction)
        {
            return;
        }
    }
}

void FNovelRunner::CompleteAction(const uint64 ExpectedSession, const uint64 ExpectedNode, const uint64 ExpectedAction, const FNovelRunnerActionResult& Result)
{
    if (State != ENovelRuntimeState::ExecutingIntents || !bWaitingForAction
        || ExpectedSession != SessionGeneration || ExpectedNode != NodeGeneration || ExpectedAction != ActionGeneration)
    {
        return;
    }
    bWaitingForAction = false;
    if (!Result.bSuccess)
    {
        Fail(Result.Error.IsEmpty() ? FText::FromString(TEXT("Action failed without an error message.")) : Result.Error);
        return;
    }
    if (Result.bRequestTransition)
    {
        if (!Result.Transition.IsValid() || !FindNode(Result.Transition))
        {
            Fail(FText::FromString(FString::Printf(TEXT("Action transition target %s does not exist."), *Result.Transition.ToString())));
            return;
        }
        bRequestedTransition = true;
        RequestedTransition = Result.Transition;
        bRequestedClearScreen |= Result.bClearScreen;
    }
    if (!bPumpingActions)
    {
        PumpActions();
    }
}

void FNovelRunner::FinishNodeActions()
{
    ActionQueue.Reset();
    bWaitingForAction = false;
    if (bTransitionAfterActions)
    {
        const FNovelNodeRef Target = TransitionAfterActions;
        bTransitionAfterActions = false;
        TransitionAfterActions = FNovelNodeRef();
        FText Error;
        EnterNode(Target, true, Error);
        return;
    }

    FText Error;
    if (!BuildAvailableChoices(Error))
    {
        Fail(Error);
    }
}

void FNovelRunner::Fail(const FText& ErrorMessage)
{
    LastError = ErrorMessage;
    State = ENovelRuntimeState::Error;
    bWaitingForAction = false;
    ActionQueue.Reset();
    AvailableChoiceIndices.Reset();
}

void FNovelRunner::InvalidateCallbacks()
{
    ++SessionGeneration;
    ++NodeGeneration;
    ++ActionGeneration;
    ++ChoiceGeneration;
    bWaitingForAction = false;
    bPumpingActions = false;
    bTransitionAfterActions = false;
    bRequestedTransition = false;
    bRequestedClearScreen = false;
    ActionQueue.Reset();
    AvailableChoiceIndices.Reset();
}