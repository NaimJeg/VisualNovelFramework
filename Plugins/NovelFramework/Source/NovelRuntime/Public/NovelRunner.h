#pragma once

#include "CoreMinimal.h"
#include "NovelChapterAsset.h"
#include "NovelRuntimeSnapshot.h"

class UNovelIntentBase;

struct NOVELRUNTIME_API FNovelRunnerActionResult
{
    bool bSuccess = true;
    FText Error;
    bool bRequestTransition = false;
    FNovelNodeRef Transition;
    bool bClearScreen = false;

    static FNovelRunnerActionResult Success();
    static FNovelRunnerActionResult Failure(const FText& ErrorMessage);
    static FNovelRunnerActionResult TransitionTo(const FNovelNodeRef& Target, bool bClearScreen = false);
};

using FNovelRunnerActionCompletion = TFunction<void(const FNovelRunnerActionResult&)>;
using FNovelRunnerActionExecutor = TFunction<void(UNovelIntentBase*, FNovelRunnerActionCompletion)>;

/**
 * Presentation-free deterministic progression core. Registered chapter assets must
 * outlive the runner. Async action callbacks are generation checked and become inert
 * when the runner is destroyed or restarted.
 */
class NOVELRUNTIME_API FNovelRunner
{
public:
    FNovelRunner();
    ~FNovelRunner();
    FNovelRunner(const FNovelRunner&) = delete;
    FNovelRunner& operator=(const FNovelRunner&) = delete;

    void RegisterChapter(const UNovelChapterAsset* Chapter);
    void ClearChapters();
    void SetActionExecutor(FNovelRunnerActionExecutor InExecutor);
    void SetActionTimeout(double InSeconds);
    void SetStoryId(FPrimaryAssetId InStoryId) { StoryId = InStoryId; }

    bool Start(const FNovelNodeRef& EntryNode, FText& OutError);
    bool Advance(FText& OutError);
    bool SelectChoice(int32 PresentedChoiceIndex, uint64 ExpectedChoiceGeneration, FText& OutError);
    void Tick(double NowSeconds);
    void Stop();

    ENovelRuntimeState GetState() const { return State; }
    bool IsComplete() const { return bComplete; }
    const FNovelNodeRef& GetCurrentNodeRef() const { return CurrentNode; }
    const FNovelNode* GetCurrentNode() const;
    const TArray<int32>& GetAvailableChoiceIndices() const { return AvailableChoiceIndices; }
    uint64 GetChoiceGeneration() const { return ChoiceGeneration; }
    const TArray<FNovelHistoryEntry>& GetHistory() const { return History; }
    const TMap<FName, FNovelValue>& GetVariables() const { return Variables; }
    const FText& GetLastError() const { return LastError; }
    bool ConsumeClearScreenRequest();

    bool SetVariable(FName Name, const FNovelValue& Value, FText& OutError);
    bool RemoveVariable(FName Name);
    bool GetVariable(FName Name, FNovelValue& OutValue) const;

    FNovelRuntimeSnapshot CreateSnapshot() const;
    bool RestoreSnapshot(const FNovelRuntimeSnapshot& Snapshot, FText& OutError);

private:
    struct FLifetimeToken;

    const FNovelNode* FindNode(const FNovelNodeRef& Ref) const;
    const UNovelChapterAsset* FindChapter(FPrimaryAssetId ChapterId) const;
    bool EnterNode(const FNovelNodeRef& Ref, bool bRecordHistory, FText& OutError);
    bool BuildAvailableChoices(FText& OutError);
    void BeginActions(const TArray<TObjectPtr<UNovelIntentBase>>& Actions, const FNovelNodeRef* TransitionAfterActions = nullptr);
    void PumpActions();
    void CompleteAction(uint64 ExpectedSession, uint64 ExpectedNode, uint64 ExpectedAction, const FNovelRunnerActionResult& Result);
    void FinishNodeActions();
    void Fail(const FText& ErrorMessage);
    void InvalidateCallbacks();

    TMap<FPrimaryAssetId, TWeakObjectPtr<const UNovelChapterAsset>> Chapters;
    FNovelRunnerActionExecutor ActionExecutor;
    TSharedPtr<FLifetimeToken> LifetimeToken;
    FPrimaryAssetId StoryId;
    FNovelNodeRef CurrentNode;
    ENovelRuntimeState State = ENovelRuntimeState::Idle;
    FText LastError;
    TMap<FName, FNovelValue> Variables;
    TArray<FNovelHistoryEntry> History;
    TArray<int32> AvailableChoiceIndices;
    TArray<TObjectPtr<UNovelIntentBase>> ActionQueue;
    FNovelNodeRef TransitionAfterActions;
    FNovelNodeRef RequestedTransition;
    int32 ActionCursor = 0;
    int32 NextHistorySequence = 0;
    uint64 SessionGeneration = 0;
    uint64 NodeGeneration = 0;
    uint64 ActionGeneration = 0;
    uint64 ChoiceGeneration = 0;
    double CurrentTimeSeconds = 0.0;
    double ActionDeadlineSeconds = 0.0;
    double ActionTimeoutSeconds = 10.0;
    bool bWaitingForAction = false;
    bool bPumpingActions = false;
    bool bComplete = false;
    bool bRequestedClearScreen = false;
    bool bTransitionAfterActions = false;
    bool bRequestedTransition = false;
};