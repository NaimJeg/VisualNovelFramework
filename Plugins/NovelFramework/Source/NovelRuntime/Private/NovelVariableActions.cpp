#include "NovelVariableActions.h"

#include "NovelActionContext.h"

namespace
{
void FailOrCompleteSet(UNovelActionContext* Context, FName VariableName, const FNovelValue& Value)
{
    if (!Context)
    {
        return;
    }
    FText Error;
    if (!Context->SetVariable(VariableName, Value, Error))
    {
        Context->Fail(Error);
        return;
    }
    Context->Complete();
}
}

void UNovelIntent_SetVariable::ExecuteAction_Implementation(UNovelActionContext* Context)
{
    FailOrCompleteSet(Context, VariableName, Value);
}

void UNovelIntent_AddToVariable::ExecuteAction_Implementation(UNovelActionContext* Context)
{
    if (!Context)
    {
        return;
    }
    FNovelValue Existing;
    if (!Context->GetVariable(VariableName, Existing))
    {
        Context->Fail(FText::FromString(FString::Printf(TEXT("Variable %s does not exist."), *VariableName.ToString())));
        return;
    }
    if (!Existing.IsNumeric() || !Amount.IsNumeric())
    {
        Context->Fail(FText::FromString(FString::Printf(TEXT("AddToVariable requires numeric values, received %s and %s."), *Existing.GetTypeName(), *Amount.GetTypeName())));
        return;
    }

    FNovelValue Result;
    if (Existing.Type == ENovelValueType::Integer && Amount.Type == ENovelValueType::Integer)
    {
        Result = FNovelValue::MakeInteger(Existing.IntegerValue + Amount.IntegerValue);
    }
    else
    {
        double ExistingFloat = 0.0;
        double AmountFloat = 0.0;
        Existing.TryGetFloat(ExistingFloat);
        Amount.TryGetFloat(AmountFloat);
        Result = FNovelValue::MakeFloat(ExistingFloat + AmountFloat);
    }
    FailOrCompleteSet(Context, VariableName, Result);
}

void UNovelIntent_RemoveVariable::ExecuteAction_Implementation(UNovelActionContext* Context)
{
    if (!Context)
    {
        return;
    }
    if (!Context->RemoveVariable(VariableName))
    {
        Context->Fail(FText::FromString(FString::Printf(TEXT("Variable %s does not exist."), *VariableName.ToString())));
        return;
    }
    Context->Complete();
}

void UNovelIntent_CopyVariable::ExecuteAction_Implementation(UNovelActionContext* Context)
{
    if (!Context)
    {
        return;
    }
    FNovelValue SourceValue;
    if (!Context->GetVariable(SourceVariable, SourceValue))
    {
        Context->Fail(FText::FromString(FString::Printf(TEXT("Source variable %s does not exist."), *SourceVariable.ToString())));
        return;
    }
    FailOrCompleteSet(Context, DestinationVariable, SourceValue);
}
