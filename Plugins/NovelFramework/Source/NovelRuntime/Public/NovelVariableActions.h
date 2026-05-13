#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelValue.h"
#include "NovelVariableActions.generated.h"

UCLASS(DisplayName = "Action: Set Variable")
class NOVELRUNTIME_API UNovelIntent_SetVariable : public UNovelIntentBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable") FName VariableName = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable") FNovelValue Value;
    virtual void ExecuteAction_Implementation(UNovelActionContext* Context) override;
};

UCLASS(DisplayName = "Action: Add To Variable")
class NOVELRUNTIME_API UNovelIntent_AddToVariable : public UNovelIntentBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable") FName VariableName = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable") FNovelValue Amount;
    virtual void ExecuteAction_Implementation(UNovelActionContext* Context) override;
};

UCLASS(DisplayName = "Action: Remove Variable")
class NOVELRUNTIME_API UNovelIntent_RemoveVariable : public UNovelIntentBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable") FName VariableName = NAME_None;
    virtual void ExecuteAction_Implementation(UNovelActionContext* Context) override;
};

UCLASS(DisplayName = "Action: Copy Variable")
class NOVELRUNTIME_API UNovelIntent_CopyVariable : public UNovelIntentBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable") FName SourceVariable = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable") FName DestinationVariable = NAME_None;
    virtual void ExecuteAction_Implementation(UNovelActionContext* Context) override;
};
