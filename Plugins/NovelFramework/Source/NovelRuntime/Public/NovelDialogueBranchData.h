#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NovelIntentBase.h"
#include "NovelDialogueBranchData.generated.h"

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FDialogueOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Option")
    FText OptionText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Intents")
    TArray<UNovelIntentBase*> Intents;
};

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FBranchData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Branch Config")
    bool bIsChoiceNode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice", meta = (EditCondition = "bIsChoiceNode", EditConditionHides))
    TArray<FDialogueOption> Options;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Auto Execute", meta = (EditCondition = "!bIsChoiceNode", EditConditionHides))
    TArray<UNovelIntentBase*> AutoIntents;
};

UCLASS()
class NOVELRUNTIME_API UNovelDialogueBranchData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Branch Data")
    TMap<FName, FBranchData> BranchMap;
};