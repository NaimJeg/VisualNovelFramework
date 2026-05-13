#pragma once

#include "CoreMinimal.h"
#include "NovelValue.generated.h"

UENUM(BlueprintType)
enum class ENovelValueType : uint8
{
    None,
    Bool,
    Integer,
    Float,
    Name,
    String
};

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    ENovelValueType Type = ENovelValueType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    bool BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    int64 IntegerValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    double FloatValue = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    FName NameValue = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    FString StringValue;

    static FNovelValue MakeBool(bool Value);
    static FNovelValue MakeInteger(int64 Value);
    static FNovelValue MakeFloat(double Value);
    static FNovelValue MakeName(FName Value);
    static FNovelValue MakeString(const FString& Value);

    bool IsNumeric() const { return Type == ENovelValueType::Integer || Type == ENovelValueType::Float; }
    bool TryGetFloat(double& OutValue) const;
    FString GetTypeName() const;
};
