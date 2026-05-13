#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "Engine/Texture2D.h"
#include "NovelIntent_SetBackground.generated.h"

UCLASS(DisplayName = "Intent: Set Background")
class NOVELRUNTIME_API UNovelIntent_SetBackground : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Background")
    TSoftObjectPtr<UTexture2D> BackgroundTexture;

    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion) override;
};