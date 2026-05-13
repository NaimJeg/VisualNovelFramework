#include "NovelExpression.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
bool RequireBool(const FNovelValue& Value, const TCHAR* Operation, bool& OutBool, FText& OutError)
{
    if (Value.Type != ENovelValueType::Bool)
    {
        OutError = FText::FromString(FString::Printf(TEXT("%s requires Bool, received %s."), Operation, *Value.GetTypeName()));
        return false;
    }
    OutBool = Value.BoolValue;
    return true;
}

bool CompareNumeric(const FNovelValue& Left, const FNovelValue& Right, const TCHAR* Operation, double& OutLeft, double& OutRight, FText& OutError)
{
    if (!Left.TryGetFloat(OutLeft) || !Right.TryGetFloat(OutRight))
    {
        OutError = FText::FromString(FString::Printf(TEXT("%s requires numeric operands, received %s and %s."), Operation, *Left.GetTypeName(), *Right.GetTypeName()));
        return false;
    }
    return true;
}

bool ValuesEqual(const FNovelValue& Left, const FNovelValue& Right, bool& OutEqual, FText& OutError)
{
    if (Left.IsNumeric() && Right.IsNumeric())
    {
        double L = 0.0;
        double R = 0.0;
        Left.TryGetFloat(L);
        Right.TryGetFloat(R);
        OutEqual = FMath::IsNearlyEqual(L, R);
        return true;
    }
    if (Left.Type != Right.Type || Left.Type == ENovelValueType::None)
    {
        OutError = FText::FromString(FString::Printf(TEXT("Equals cannot compare %s with %s."), *Left.GetTypeName(), *Right.GetTypeName()));
        return false;
    }
    switch (Left.Type)
    {
    case ENovelValueType::Bool: OutEqual = Left.BoolValue == Right.BoolValue; return true;
    case ENovelValueType::Name: OutEqual = Left.NameValue == Right.NameValue; return true;
    case ENovelValueType::String: OutEqual = Left.StringValue == Right.StringValue; return true;
    default: break;
    }
    OutError = FText::FromString(TEXT("Unsupported equality operands."));
    return false;
}

ENovelValueType ArithmeticStaticType(const UNovelBinaryExpression* Expression, bool bAlwaysFloat)
{
    if (!Expression || !Expression->Left || !Expression->Right)
    {
        return ENovelValueType::None;
    }
    const ENovelValueType LeftType = Expression->Left->GetStaticResultType();
    const ENovelValueType RightType = Expression->Right->GetStaticResultType();
    if (LeftType == ENovelValueType::None || RightType == ENovelValueType::None)
    {
        return ENovelValueType::None;
    }
    const bool bLeftNumeric = LeftType == ENovelValueType::Integer || LeftType == ENovelValueType::Float;
    const bool bRightNumeric = RightType == ENovelValueType::Integer || RightType == ENovelValueType::Float;
    if (!bLeftNumeric || !bRightNumeric)
    {
        return ENovelValueType::None;
    }
    return bAlwaysFloat || LeftType == ENovelValueType::Float || RightType == ENovelValueType::Float
        ? ENovelValueType::Float
        : ENovelValueType::Integer;
}

template <typename IntegerOp, typename FloatOp>
bool EvaluateArithmetic(const UNovelBinaryExpression* Expression, const UNovelExpressionContext* Context, const TCHAR* Operation, IntegerOp IntOperation, FloatOp FloatingOperation, FNovelValue& OutValue, FText& OutError)
{
    FNovelValue Left;
    FNovelValue Right;
    if (!Expression->EvaluateOperands(Context, Left, Right, OutError))
    {
        return false;
    }
    if (!Left.IsNumeric() || !Right.IsNumeric())
    {
        OutError = FText::FromString(FString::Printf(TEXT("%s requires numeric operands, received %s and %s."), Operation, *Left.GetTypeName(), *Right.GetTypeName()));
        return false;
    }
    if (Left.Type == ENovelValueType::Integer && Right.Type == ENovelValueType::Integer)
    {
        OutValue = FNovelValue::MakeInteger(IntOperation(Left.IntegerValue, Right.IntegerValue));
        return true;
    }
    double L = 0.0;
    double R = 0.0;
    Left.TryGetFloat(L);
    Right.TryGetFloat(R);
    OutValue = FNovelValue::MakeFloat(FloatingOperation(L, R));
    return true;
}
}

bool UNovelExpressionContext::GetVariable(FName VariableName, FNovelValue& OutValue) const
{
    if (const FNovelValue* Value = Variables.Find(VariableName))
    {
        OutValue = *Value;
        return true;
    }
    OutValue = FNovelValue();
    return false;
}

bool UNovelExpression::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    OutValue = FNovelValue();
    OutError = FText::FromString(TEXT("Expression has no implementation."));
    return false;
}

bool UNovelExpression_Literal::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    if (Value.Type == ENovelValueType::None)
    {
        OutError = FText::FromString(TEXT("Literal value cannot be None."));
        return false;
    }
    OutValue = Value;
    OutError = FText::GetEmpty();
    return true;
}

bool UNovelExpression_ReadVariable::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    if (!Context || VariableName.IsNone() || !Context->GetVariable(VariableName, OutValue))
    {
        OutError = FText::FromString(FString::Printf(TEXT("Variable %s does not exist."), *VariableName.ToString()));
        return false;
    }
    OutError = FText::GetEmpty();
    return true;
}

bool UNovelBinaryExpression::EvaluateOperands(const UNovelExpressionContext* Context, FNovelValue& OutLeft, FNovelValue& OutRight, FText& OutError) const
{
    if (!Left || !Right)
    {
        OutError = FText::FromString(TEXT("Binary expression is missing an operand."));
        return false;
    }
    return Left->Evaluate(Context, OutLeft, OutError) && Right->Evaluate(Context, OutRight, OutError);
}

#if WITH_EDITOR
EDataValidationResult UNovelBinaryExpression::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    if (!Left || !Right)
    {
        Context.AddError(FText::FromString(TEXT("Binary expression requires both operands.")));
        return EDataValidationResult::Invalid;
    }
    const UNovelExpression* LeftExpression = Left.Get();
    const UNovelExpression* RightExpression = Right.Get();
    bool bInvalid = LeftExpression->IsDataValid(Context) == EDataValidationResult::Invalid || RightExpression->IsDataValid(Context) == EDataValidationResult::Invalid;
    const ENovelValueType LeftType = LeftExpression->GetStaticResultType();
    const ENovelValueType RightType = RightExpression->GetStaticResultType();
    const bool bKnownLeft = LeftType != ENovelValueType::None;
    const bool bKnownRight = RightType != ENovelValueType::None;
    const bool bLeftNumeric = LeftType == ENovelValueType::Integer || LeftType == ENovelValueType::Float;
    const bool bRightNumeric = RightType == ENovelValueType::Integer || RightType == ENovelValueType::Float;

    if ((IsA<UNovelExpression_And>() || IsA<UNovelExpression_Or>()) &&
        ((bKnownLeft && LeftType != ENovelValueType::Bool) || (bKnownRight && RightType != ENovelValueType::Bool)))
    {
        Context.AddError(FText::FromString(TEXT("Logical expression operands must be Bool.")));
        bInvalid = true;
    }
    else if ((IsA<UNovelExpression_LessThan>() || IsA<UNovelExpression_LessThanOrEqual>() || IsA<UNovelExpression_GreaterThan>() || IsA<UNovelExpression_GreaterThanOrEqual>() ||
        IsA<UNovelExpression_Add>() || IsA<UNovelExpression_Subtract>() || IsA<UNovelExpression_Multiply>() || IsA<UNovelExpression_Divide>()) &&
        ((bKnownLeft && !bLeftNumeric) || (bKnownRight && !bRightNumeric)))
    {
        Context.AddError(FText::FromString(TEXT("Numeric expression has a known non-numeric operand.")));
        bInvalid = true;
    }
    else if ((IsA<UNovelExpression_Equals>() || IsA<UNovelExpression_NotEquals>()) && bKnownLeft && bKnownRight &&
        LeftType != RightType && !(bLeftNumeric && bRightNumeric))
    {
        Context.AddError(FText::FromString(TEXT("Equality expression has incompatible operand types.")));
        bInvalid = true;
    }
    return bInvalid ? EDataValidationResult::Invalid : Result;
}
#endif

bool UNovelExpression_Equals::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    FNovelValue L, R; bool bEqual = false;
    if (!EvaluateOperands(Context, L, R, OutError) || !ValuesEqual(L, R, bEqual, OutError)) return false;
    OutValue = FNovelValue::MakeBool(bEqual); return true;
}

bool UNovelExpression_NotEquals::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    FNovelValue L, R; bool bEqual = false;
    if (!EvaluateOperands(Context, L, R, OutError) || !ValuesEqual(L, R, bEqual, OutError)) return false;
    OutValue = FNovelValue::MakeBool(!bEqual); return true;
}

#define NOVEL_COMPARE_IMPL(ClassName, Symbol, Label) \
bool ClassName::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const \
{ FNovelValue L, R; double LV = 0.0, RV = 0.0; if (!EvaluateOperands(Context, L, R, OutError) || !CompareNumeric(L, R, TEXT(Label), LV, RV, OutError)) return false; OutValue = FNovelValue::MakeBool(LV Symbol RV); return true; }

NOVEL_COMPARE_IMPL(UNovelExpression_LessThan, <, "LessThan")
NOVEL_COMPARE_IMPL(UNovelExpression_LessThanOrEqual, <=, "LessThanOrEqual")
NOVEL_COMPARE_IMPL(UNovelExpression_GreaterThan, >, "GreaterThan")
NOVEL_COMPARE_IMPL(UNovelExpression_GreaterThanOrEqual, >=, "GreaterThanOrEqual")
#undef NOVEL_COMPARE_IMPL

bool UNovelExpression_And::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    if (!Left || !Right) { OutError = FText::FromString(TEXT("And is missing an operand.")); return false; }
    FNovelValue L; bool LB = false;
    if (!Left->Evaluate(Context, L, OutError) || !RequireBool(L, TEXT("And"), LB, OutError)) return false;
    if (!LB) { OutValue = FNovelValue::MakeBool(false); return true; }
    FNovelValue R; bool RB = false;
    if (!Right->Evaluate(Context, R, OutError) || !RequireBool(R, TEXT("And"), RB, OutError)) return false;
    OutValue = FNovelValue::MakeBool(RB); return true;
}

bool UNovelExpression_Or::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    if (!Left || !Right) { OutError = FText::FromString(TEXT("Or is missing an operand.")); return false; }
    FNovelValue L; bool LB = false;
    if (!Left->Evaluate(Context, L, OutError) || !RequireBool(L, TEXT("Or"), LB, OutError)) return false;
    if (LB) { OutValue = FNovelValue::MakeBool(true); return true; }
    FNovelValue R; bool RB = false;
    if (!Right->Evaluate(Context, R, OutError) || !RequireBool(R, TEXT("Or"), RB, OutError)) return false;
    OutValue = FNovelValue::MakeBool(RB); return true;
}

bool UNovelExpression_Not::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    if (!Operand) { OutError = FText::FromString(TEXT("Not is missing its operand.")); return false; }
    FNovelValue Value; bool bValue = false;
    if (!Operand->Evaluate(Context, Value, OutError) || !RequireBool(Value, TEXT("Not"), bValue, OutError)) return false;
    OutValue = FNovelValue::MakeBool(!bValue); return true;
}

#if WITH_EDITOR
EDataValidationResult UNovelExpression_Not::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    if (!Operand)
    {
        Context.AddError(FText::FromString(TEXT("Not expression requires an operand.")));
        return EDataValidationResult::Invalid;
    }
    const UNovelExpression* OperandExpression = Operand.Get();
    if (OperandExpression->IsDataValid(Context) == EDataValidationResult::Invalid)
    {
        return EDataValidationResult::Invalid;
    }
    const ENovelValueType StaticType = OperandExpression->GetStaticResultType();
    if (StaticType != ENovelValueType::None && StaticType != ENovelValueType::Bool)
    {
        Context.AddError(FText::FromString(TEXT("Not expression operand must be Bool.")));
        return EDataValidationResult::Invalid;
    }
    return Result;
}
#endif

bool UNovelExpression_Add::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{ return EvaluateArithmetic(this, Context, TEXT("Add"), [](int64 A, int64 B){ return A + B; }, [](double A, double B){ return A + B; }, OutValue, OutError); }
bool UNovelExpression_Subtract::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{ return EvaluateArithmetic(this, Context, TEXT("Subtract"), [](int64 A, int64 B){ return A - B; }, [](double A, double B){ return A - B; }, OutValue, OutError); }
bool UNovelExpression_Multiply::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{ return EvaluateArithmetic(this, Context, TEXT("Multiply"), [](int64 A, int64 B){ return A * B; }, [](double A, double B){ return A * B; }, OutValue, OutError); }
bool UNovelExpression_Divide::Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const
{
    FNovelValue L, R; double LV = 0.0, RV = 0.0;
    if (!EvaluateOperands(Context, L, R, OutError) || !CompareNumeric(L, R, TEXT("Divide"), LV, RV, OutError)) return false;
    if (FMath::IsNearlyZero(RV)) { OutError = FText::FromString(TEXT("Division by zero.")); return false; }
    OutValue = FNovelValue::MakeFloat(LV / RV); return true;
}

ENovelValueType UNovelExpression_Add::GetStaticResultType() const { return ArithmeticStaticType(this, false); }
ENovelValueType UNovelExpression_Subtract::GetStaticResultType() const { return ArithmeticStaticType(this, false); }
ENovelValueType UNovelExpression_Multiply::GetStaticResultType() const { return ArithmeticStaticType(this, false); }
ENovelValueType UNovelExpression_Divide::GetStaticResultType() const { return ArithmeticStaticType(this, true); }
