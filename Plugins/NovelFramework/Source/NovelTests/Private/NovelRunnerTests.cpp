#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "NovelExpression.h"
#include "NovelIntent_HideCharacter.h"
#include "NovelIntent_SetBackground.h"
#include "NovelRunner.h"

namespace NovelRunnerTests
{
    FNovelNodeRef Ref(const UNovelChapterAsset* Chapter, const TCHAR* NodeId)
    {
        return Chapter->MakeNodeRef(FName(NodeId));
    }

    UNovelChapterAsset* MakeProgressionChapter()
    {
        UNovelChapterAsset* Chapter = NewObject<UNovelChapterAsset>(GetTransientPackage(), TEXT("RunnerProgressionChapter"));
        Chapter->EntryNodeId = TEXT("A");

        FNovelNode& A = Chapter->Nodes.AddDefaulted_GetRef();
        A.NodeId = TEXT("A");
        A.Speaker = FText::FromString(TEXT("Speaker A"));
        A.Text = FText::FromString(TEXT("Line A"));
        A.EntryActions.Add(NewObject<UNovelIntent_SetBackground>(Chapter));
        A.EntryActions.Add(NewObject<UNovelIntent_HideCharacter>(Chapter));
        A.Next = Ref(Chapter, TEXT("B"));

        FNovelNode& B = Chapter->Nodes.AddDefaulted_GetRef();
        B.NodeId = TEXT("B");
        B.Text = FText::FromString(TEXT("Line B"));
        FNovelChoice& HiddenChoice = B.Choices.AddDefaulted_GetRef();
        HiddenChoice.Text = FText::FromString(TEXT("Hidden"));
        HiddenChoice.Target = Ref(Chapter, TEXT("A"));
        UNovelExpression_Literal* FalseCondition = NewObject<UNovelExpression_Literal>(Chapter);
        FalseCondition->Value = FNovelValue::MakeBool(false);
        HiddenChoice.Condition = FalseCondition;
        FNovelChoice& VisibleChoice = B.Choices.AddDefaulted_GetRef();
        VisibleChoice.Text = FText::FromString(TEXT("Continue"));
        VisibleChoice.Target = Ref(Chapter, TEXT("C"));
        UNovelExpression_Literal* TrueCondition = NewObject<UNovelExpression_Literal>(Chapter);
        TrueCondition->Value = FNovelValue::MakeBool(true);
        VisibleChoice.Condition = TrueCondition;

        FNovelNode& C = Chapter->Nodes.AddDefaulted_GetRef();
        C.NodeId = TEXT("C");
        C.Text = FText::FromString(TEXT("Line C"));
        Chapter->RebuildNodeLookup();
        return Chapter;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNovelRunnerProgressionTest,
    "Novel.Runner.ProgressionChoicesHistory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNovelRunnerProgressionTest::RunTest(const FString& Parameters)
{
    using namespace NovelRunnerTests;
    UNovelChapterAsset* Chapter = MakeProgressionChapter();
    FNovelRunner Runner;
    Runner.RegisterChapter(Chapter);

    TArray<UNovelIntentBase*> Executed;
    Runner.SetActionExecutor([&Executed](UNovelIntentBase* Action, FNovelRunnerActionCompletion Completion)
    {
        Executed.Add(Action);
        Completion(FNovelRunnerActionResult::Success());
        Completion(FNovelRunnerActionResult::Success());
    });

    FText Error;
    TestTrue(TEXT("Runner starts at explicit entry"), Runner.Start(Ref(Chapter, TEXT("A")), Error));
    TestEqual(TEXT("Sequential actions execute once each"), Executed.Num(), 2);
    TestEqual(TEXT("Entry actions preserve order"), Executed[0], Chapter->Nodes[0].EntryActions[0].Get());
    TestEqual(TEXT("Runner awaits advance after actions"), Runner.GetState(), ENovelRuntimeState::AwaitingAdvance);
    TestEqual(TEXT("First line recorded once"), Runner.GetHistory().Num(), 1);

    TestTrue(TEXT("Explicit Next advances"), Runner.Advance(Error));
    TestEqual(TEXT("Choice state reached"), Runner.GetState(), ENovelRuntimeState::AwaitingChoice);
    TestEqual(TEXT("False choice filtered"), Runner.GetAvailableChoiceIndices().Num(), 1);
    TestEqual(TEXT("Presented choice maps to authored choice"), Runner.GetAvailableChoiceIndices()[0], 1);
    const uint64 ChoiceGeneration = Runner.GetChoiceGeneration();
    TestFalse(TEXT("Stale choice rejected"), Runner.SelectChoice(0, ChoiceGeneration - 1, Error));
    TestFalse(TEXT("Invalid choice index rejected"), Runner.SelectChoice(3, ChoiceGeneration, Error));
    TestTrue(TEXT("Valid choice transitions"), Runner.SelectChoice(0, ChoiceGeneration, Error));
    TestEqual(TEXT("Choice reaches C"), Runner.GetCurrentNodeRef().NodeId, FName(TEXT("C")));
    TestEqual(TEXT("One history row per presented line"), Runner.GetHistory().Num(), 3);
    TestEqual(TEXT("History sequence is monotonic"), Runner.GetHistory()[2].SequenceIndex, 2);

    const FNovelRuntimeSnapshot Snapshot = Runner.CreateSnapshot();
    FNovelRunner Restored;
    Restored.RegisterChapter(Chapter);
    TestTrue(TEXT("Snapshot restores without assets or viewport"), Restored.RestoreSnapshot(Snapshot, Error));
    TestEqual(TEXT("Restore does not duplicate current history line"), Restored.GetHistory().Num(), 3);
    TestEqual(TEXT("Restore returns to awaiting advance"), Restored.GetState(), ENovelRuntimeState::AwaitingAdvance);

    FNovelRunner RevisitRunner;
    RevisitRunner.RegisterChapter(Chapter);
    RevisitRunner.SetActionExecutor([](UNovelIntentBase* Action, FNovelRunnerActionCompletion Completion)
    {
        Completion(FNovelRunnerActionResult::Success());
    });
    Chapter->FindMutableNode(TEXT("C"))->Next = Ref(Chapter, TEXT("A"));
    TestTrue(TEXT("Revisit runner starts"), RevisitRunner.Start(Ref(Chapter, TEXT("C")), Error));
    TestTrue(TEXT("Explicit loop revisits a node"), RevisitRunner.Advance(Error));
    TestEqual(TEXT("Revisited node creates another history entry"), RevisitRunner.GetHistory().Num(), 2);
    TestEqual(TEXT("Revisit receives a new sequence index"), RevisitRunner.GetHistory()[1].SequenceIndex, 1);
    Chapter->FindMutableNode(TEXT("C"))->Next = FNovelNodeRef();

    TestTrue(TEXT("Terminal node completes deterministically"), Restored.Advance(Error));
    TestTrue(TEXT("Runner reports completion"), Restored.IsComplete());
    TestEqual(TEXT("Completed runner is idle"), Restored.GetState(), ENovelRuntimeState::Idle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNovelRunnerActionSafetyTest,
    "Novel.Runner.ActionSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNovelRunnerActionSafetyTest::RunTest(const FString& Parameters)
{
    using namespace NovelRunnerTests;
    UNovelChapterAsset* Chapter = MakeProgressionChapter();
    FText Error;

    FNovelRunner DeferredRunner;
    DeferredRunner.RegisterChapter(Chapter);
    int32 DeferredExecutionCount = 0;
    DeferredRunner.SetActionExecutor([&](UNovelIntentBase* Action, FNovelRunnerActionCompletion Completion)
    {
        ++DeferredExecutionCount;
        Completion(FNovelRunnerActionResult::TransitionTo(Ref(Chapter, TEXT("C")), true));
    });
    TestTrue(TEXT("Deferred runner starts"), DeferredRunner.Start(Ref(Chapter, TEXT("A")), Error));
    TestEqual(TEXT("Deferred transition terminates old action chain"), DeferredExecutionCount, 1);
    TestEqual(TEXT("Deferred transition reaches target without recursion"), DeferredRunner.GetCurrentNodeRef().NodeId, FName(TEXT("C")));
    TestTrue(TEXT("Clear screen is applied at transition"), DeferredRunner.ConsumeClearScreenRequest());
    TestFalse(TEXT("Clear screen request is consumed once"), DeferredRunner.ConsumeClearScreenRequest());

    FNovelRunnerActionCompletion StaleCompletion;
    FNovelRunner StaleRunner;
    StaleRunner.RegisterChapter(Chapter);
    StaleRunner.SetActionExecutor([&](UNovelIntentBase* Action, FNovelRunnerActionCompletion Completion)
    {
        StaleCompletion = MoveTemp(Completion);
    });
    TestTrue(TEXT("Async action starts"), StaleRunner.Start(Ref(Chapter, TEXT("A")), Error));
    TestEqual(TEXT("Runner waits for action"), StaleRunner.GetState(), ENovelRuntimeState::ExecutingIntents);
    TestTrue(TEXT("Restart invalidates old action"), StaleRunner.Start(Ref(Chapter, TEXT("C")), Error));
    StaleCompletion(FNovelRunnerActionResult::TransitionTo(Ref(Chapter, TEXT("B"))));
    TestEqual(TEXT("Stale completion cannot change restarted runner"), StaleRunner.GetCurrentNodeRef().NodeId, FName(TEXT("C")));

    FNovelRunner TimeoutRunner;
    TimeoutRunner.RegisterChapter(Chapter);
    TimeoutRunner.SetActionTimeout(0.25);
    TimeoutRunner.SetActionExecutor([](UNovelIntentBase* Action, FNovelRunnerActionCompletion Completion) {});
    TestTrue(TEXT("Timeout runner starts"), TimeoutRunner.Start(Ref(Chapter, TEXT("A")), Error));
    TimeoutRunner.Tick(0.3);
    TestEqual(TEXT("Missing completion fails deterministically"), TimeoutRunner.GetState(), ENovelRuntimeState::Error);

    FNovelRunner MissingTargetRunner;
    MissingTargetRunner.RegisterChapter(Chapter);
    FNovelNode* C = Chapter->FindMutableNode(TEXT("C"));
    C->Next = Ref(Chapter, TEXT("Missing"));
    TestTrue(TEXT("Missing target runner enters valid node"), MissingTargetRunner.Start(Ref(Chapter, TEXT("C")), Error));
    TestFalse(TEXT("Missing explicit target is rejected"), MissingTargetRunner.Advance(Error));
    TestEqual(TEXT("Missing target enters error state"), MissingTargetRunner.GetState(), ENovelRuntimeState::Error);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNovelChapterValidationTest,
    "Novel.Validation.ChapterFailures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNovelChapterValidationTest::RunTest(const FString& Parameters)
{
    UNovelChapterAsset* Chapter = NewObject<UNovelChapterAsset>(GetTransientPackage(), TEXT("InvalidValidationChapter"));
    Chapter->EntryNodeId = TEXT("MissingEntry");
    FNovelNode& First = Chapter->Nodes.AddDefaulted_GetRef();
    First.NodeId = TEXT("Duplicate");
    First.Text = FText::FromString(TEXT("Valid text"));
    First.Next = Chapter->MakeNodeRef(TEXT("MissingTarget"));
    First.EntryActions.Add(nullptr);
    FNovelChoice& Choice = First.Choices.AddDefaulted_GetRef();
    Choice.Target = Chapter->MakeNodeRef(TEXT("Duplicate"));
    UNovelExpression_Literal* IntegerCondition = NewObject<UNovelExpression_Literal>(Chapter);
    IntegerCondition->Value = FNovelValue::MakeInteger(1);
    Choice.Condition = IntegerCondition;
    FNovelNode& Duplicate = Chapter->Nodes.AddDefaulted_GetRef();
    Duplicate.NodeId = TEXT("Duplicate");
    Duplicate.Text = FText::FromString(TEXT("Duplicate"));
    Chapter->RebuildNodeLookup();

    FDataValidationContext Context;
    TestEqual(TEXT("Invalid chapter fails validation"), Chapter->IsDataValid(Context), EDataValidationResult::Invalid);
    TestTrue(TEXT("Validation reports entry, duplicate, target, action, and expression failures"), Context.GetNumErrors() >= 5);
    return true;
}

#endif