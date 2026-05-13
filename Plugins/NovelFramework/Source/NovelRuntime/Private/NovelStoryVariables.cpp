#include "NovelStorySubsystem.h"

bool UNovelStorySubsystem::HasVariable(FName VariableName) const
{
    return !VariableName.IsNone() && Variables.Contains(VariableName);
}

bool UNovelStorySubsystem::GetVariable(FName VariableName, FNovelValue& OutValue) const
{
    if (const FNovelValue* Value = Variables.Find(VariableName))
    {
        OutValue = *Value;
        return true;
    }
    OutValue = FNovelValue();
    return false;
}

bool UNovelStorySubsystem::SetVariable(FName VariableName, FNovelValue Value, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (VariableName.IsNone())
    {
        OutError = FText::FromString(TEXT("Variable name cannot be empty."));
        return false;
    }
    if (Value.Type == ENovelValueType::None)
    {
        OutError = FText::FromString(FString::Printf(TEXT("Variable %s cannot be assigned a None value."), *VariableName.ToString()));
        return false;
    }

    if (const FNovelValue* Existing = Variables.Find(VariableName))
    {
        if (Existing->Type != Value.Type)
        {
            if (Existing->Type == ENovelValueType::Float && Value.Type == ENovelValueType::Integer)
            {
                Value = FNovelValue::MakeFloat(static_cast<double>(Value.IntegerValue));
            }
            else if (Existing->Type == ENovelValueType::Integer && Value.Type == ENovelValueType::Float)
            {
                // The variable is promoted from Integer to Float; no reverse numeric coercion is allowed.
            }
            else
            {
                OutError = FText::FromString(FString::Printf(TEXT("Variable %s has type %s and cannot be assigned %s."),
                    *VariableName.ToString(),
                    *Existing->GetTypeName(),
                    *Value.GetTypeName()));
                return false;
            }
        }
    }

    Variables.Add(VariableName, Value);
    OnVariableChangedEvent.Broadcast(VariableName, Value);
    return true;
}

bool UNovelStorySubsystem::RemoveVariable(FName VariableName)
{
    if (Variables.Remove(VariableName) <= 0)
    {
        return false;
    }
    OnVariableChangedEvent.Broadcast(VariableName, FNovelValue());
    return true;
}

void UNovelStorySubsystem::ClearVariables()
{
    if (Variables.IsEmpty())
    {
        return;
    }
    Variables.Empty();
    OnVariablesResetEvent.Broadcast();
}

bool UNovelStorySubsystem::GetBool(FName VariableName, bool& OutValue) const
{
    const FNovelValue* Value = Variables.Find(VariableName);
    if (!Value || Value->Type != ENovelValueType::Bool)
    {
        return false;
    }
    OutValue = Value->BoolValue;
    return true;
}

bool UNovelStorySubsystem::SetBool(FName VariableName, bool Value, FText& OutError)
{
    return SetVariable(VariableName, FNovelValue::MakeBool(Value), OutError);
}

bool UNovelStorySubsystem::GetInteger(FName VariableName, int64& OutValue) const
{
    const FNovelValue* Value = Variables.Find(VariableName);
    if (!Value || Value->Type != ENovelValueType::Integer)
    {
        return false;
    }
    OutValue = Value->IntegerValue;
    return true;
}

bool UNovelStorySubsystem::SetInteger(FName VariableName, int64 Value, FText& OutError)
{
    return SetVariable(VariableName, FNovelValue::MakeInteger(Value), OutError);
}

bool UNovelStorySubsystem::GetFloat(FName VariableName, double& OutValue) const
{
    const FNovelValue* Value = Variables.Find(VariableName);
    return Value && Value->TryGetFloat(OutValue);
}

bool UNovelStorySubsystem::SetFloat(FName VariableName, double Value, FText& OutError)
{
    return SetVariable(VariableName, FNovelValue::MakeFloat(Value), OutError);
}

bool UNovelStorySubsystem::GetName(FName VariableName, FName& OutValue) const
{
    const FNovelValue* Value = Variables.Find(VariableName);
    if (!Value || Value->Type != ENovelValueType::Name)
    {
        return false;
    }
    OutValue = Value->NameValue;
    return true;
}

bool UNovelStorySubsystem::SetName(FName VariableName, FName Value, FText& OutError)
{
    return SetVariable(VariableName, FNovelValue::MakeName(Value), OutError);
}

bool UNovelStorySubsystem::GetString(FName VariableName, FString& OutValue) const
{
    const FNovelValue* Value = Variables.Find(VariableName);
    if (!Value || Value->Type != ENovelValueType::String)
    {
        return false;
    }
    OutValue = Value->StringValue;
    return true;
}

bool UNovelStorySubsystem::SetString(FName VariableName, const FString& Value, FText& OutError)
{
    return SetVariable(VariableName, FNovelValue::MakeString(Value), OutError);
}
