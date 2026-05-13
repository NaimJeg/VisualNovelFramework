#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NovelIntentBase.h"
#include "NovelDialogueBranchData.generated.h"

USTRUCT(BlueprintType)
struct FDialogueOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Option")
    FText OptionText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Intents")
    TArray<UNovelIntentBase*> Intents;
};

USTRUCT(BlueprintType)
struct FBranchData
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
class NOVEL_API UNovelDialogueBranchData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Branch Data")
    TMap<FName, FBranchData> BranchMap;
};