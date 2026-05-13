#include "NovelValue.h"

FNovelValue FNovelValue::MakeBool(bool Value)
{
    FNovelValue Result;
    Result.Type = ENovelValueType::Bool;
    Result.BoolValue = Value;
    return Result;
}

FNovelValue FNovelValue::MakeInteger(int64 Value)
{
    FNovelValue Result;
    Result.Type = ENovelValueType::Integer;
    Result.IntegerValue = Value;
    return Result;
}

FNovelValue FNovelValue::MakeFloat(double Value)
{
    FNovelValue Result;
    Result.Type = ENovelValueType::Float;
    Result.FloatValue = Value;
    return Result;
}

FNovelValue FNovelValue::MakeName(FName Value)
{
    FNovelValue Result;
    Result.Type = ENovelValueType::Name;
    Result.NameValue = Value;
    return Result;
}

FNovelValue FNovelValue::MakeString(const FString& Value)
{
    FNovelValue Result;
    Result.Type = ENovelValueType::String;
    Result.StringValue = Value;
    return Result;
}

bool FNovelValue::TryGetFloat(double& OutValue) const
{
    if (Type == ENovelValueType::Float)
    {
        OutValue = FloatValue;
        return true;
    }
    if (Type == ENovelValueType::Integer)
    {
        OutValue = static_cast<double>(IntegerValue);
        return true;
    }
    return false;
}

FString FNovelValue::GetTypeName() const
{
    switch (Type)
    {
    case ENovelValueType::None: return TEXT("None");
    case ENovelValueType::Bool: return TEXT("Bool");
    case ENovelValueType::Integer: return TEXT("Integer");
    case ENovelValueType::Float: return TEXT("Float");
    case ENovelValueType::Name: return TEXT("Name");
    case ENovelValueType::String: return TEXT("String");
    default: return TEXT("Unknown");
    }
}
