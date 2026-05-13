#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "NovelExpression.h"
#include "NovelStorySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNovelValueExpressionTest,
    "Novel.Runtime.ValuesAndExpressions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNovelValueExpressionTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UNovelStorySubsystem* Story = NewObject<UNovelStorySubsystem>(GameInstance);
    FText Error;
    TestTrue(TEXT("Set Bool"), Story->SetBool(TEXT("Flag"), true, Error));
    TestTrue(TEXT("Set Integer"), Story->SetInteger(TEXT("Count"), 4, Error));
    TestTrue(TEXT("Set Float"), Story->SetFloat(TEXT("Ratio"), 2.5, Error));
    TestTrue(TEXT("Set Name"), Story->SetName(TEXT("Route"), TEXT("A"), Error));
    TestTrue(TEXT("Set String"), Story->SetString(TEXT("Label"), TEXT("Ready"), Error));

    TestFalse(TEXT("Reject Bool to Integer coercion"), Story->SetInteger(TEXT("Flag"), 1, Error));
    TestTrue(TEXT("Promote Integer variable to Float"), Story->SetFloat(TEXT("Count"), 4.5, Error));
    double Promoted = 0.0;
    TestTrue(TEXT("Read promoted Float"), Story->GetFloat(TEXT("Count"), Promoted));
    TestEqual(TEXT("Promoted value"), Promoted, 4.5);

    UNovelExpressionContext* Context = NewObject<UNovelExpressionContext>();
    Context->Initialize(Story->GetVariablesView());

    UNovelExpression_ReadVariable* ReadCount = NewObject<UNovelExpression_ReadVariable>();
    ReadCount->VariableName = TEXT("Count");
    UNovelExpression_Literal* Two = NewObject<UNovelExpression_Literal>();
    Two->Value = FNovelValue::MakeInteger(2);
    UNovelExpression_Add* Add = NewObject<UNovelExpression_Add>();
    Add->Left = ReadCount;
    Add->Right = Two;

    FNovelValue Result;
    TestTrue(TEXT("Evaluate mixed numeric add"), Add->Evaluate(Context, Result, Error));
    TestEqual(TEXT("Mixed add returns Float"), Result.Type, ENovelValueType::Float);
    TestEqual(TEXT("Mixed add value"), Result.FloatValue, 6.5);

    UNovelExpression_ReadVariable* Missing = NewObject<UNovelExpression_ReadVariable>();
    Missing->VariableName = TEXT("Missing");
    TestFalse(TEXT("Missing variable fails"), Missing->Evaluate(Context, Result, Error));

    UNovelExpression_Literal* Zero = NewObject<UNovelExpression_Literal>();
    Zero->Value = FNovelValue::MakeInteger(0);
    UNovelExpression_Divide* Divide = NewObject<UNovelExpression_Divide>();
    Divide->Left = Two;
    Divide->Right = Zero;
    TestFalse(TEXT("Division by zero fails"), Divide->Evaluate(Context, Result, Error));

    UNovelExpression_Literal* TrueLiteral = NewObject<UNovelExpression_Literal>();
    TrueLiteral->Value = FNovelValue::MakeBool(true);
    UNovelExpression_Not* Not = NewObject<UNovelExpression_Not>();
    Not->Operand = TrueLiteral;
    TestTrue(TEXT("Evaluate Not"), Not->Evaluate(Context, Result, Error));
    TestFalse(TEXT("Not true is false"), Result.BoolValue);
    return true;
}

#endif
