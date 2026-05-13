#pragma once
#include "NovelIntentBase.h"
#include "NovelStorySubsystem.h"
#include "FDialogueNodeHandle.h"
#include "NovelIntent_Jump.generated.h"

UCLASS(DisplayName = "Intent: Jump To Node")
class NOVEL_API UNovelIntent_Jump : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Jump")
    FDialogueNodeHandle TargetNode;

    UPROPERTY(EditAnywhere, Category = "Jump")
    bool bClearScreen = false;

    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished) override
    {
        if (StorySys)
        {
            // If it is a macro jump (time/space discontinuity), clear visuals first
            if (bClearScreen)
            {
                StorySys->ResetVisualState();
            }

            // Move the logic pointer
            StorySys->JumpToNode(TargetNode);
        }

        OnFinished.ExecuteIfBound();
    }
};