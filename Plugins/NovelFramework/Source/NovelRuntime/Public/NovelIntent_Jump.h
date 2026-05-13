#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "FDialogueNodeHandle.h"
#include "NovelIntent_Jump.generated.h"

UCLASS(DisplayName = "Intent: Jump To Node")
class NOVELRUNTIME_API UNovelIntent_Jump : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Jump")
    FDialogueNodeHandle TargetNode;

    UPROPERTY(EditAnywhere, Category = "Jump")
    bool bClearScreen = false;

    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion) override;
};