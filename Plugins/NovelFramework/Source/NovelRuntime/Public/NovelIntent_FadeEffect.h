#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelIntent_FadeEffect.generated.h"

UCLASS(DisplayName = "Intent: Fade Effect")
class NOVELRUNTIME_API UNovelIntent_FadeEffect : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Effect")
    FLinearColor FadeColor = FLinearColor::Black;

    UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "0.0"))
    float FadeDuration = 1.0f;

    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion) override;
};