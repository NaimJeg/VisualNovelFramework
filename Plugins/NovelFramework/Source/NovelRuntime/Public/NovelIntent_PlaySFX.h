#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelIntent_PlaySFX.generated.h"

UCLASS(DisplayName = "Intent: Play SFX")
class NOVELRUNTIME_API UNovelIntent_PlaySFX : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Audio")
    FName SFXKey = NAME_None;

    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion) override;
};