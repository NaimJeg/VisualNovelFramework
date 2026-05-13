#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "Sound/SoundBase.h"
#include "NovelIntent_PlayBGM.generated.h"

UCLASS(DisplayName = "Intent: Play BGM")
class NOVELRUNTIME_API UNovelIntent_PlayBGM : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Audio")
    TSoftObjectPtr<USoundBase> BGMTexture;

    UPROPERTY(EditAnywhere, Category = "Audio")
    float FadeInTime = 1.0f;

    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion) override;
};