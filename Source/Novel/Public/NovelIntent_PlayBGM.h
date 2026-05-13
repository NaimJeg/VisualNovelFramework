// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelStorySubsystem.h"
#include "NovelIntent_PlayBGM.generated.h"

UCLASS(DisplayName = "Intent: Play BGM")
class NOVEL_API UNovelIntent_PlayBGM : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Audio")
    TSoftObjectPtr<USoundBase> BGMTexture;

    UPROPERTY(EditAnywhere, Category = "Audio")
    float FadeInTime = 1.0f;

    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished) override
    {
        if (StorySys)
        {
            StorySys->PlayBGM(BGMTexture, FadeInTime);
        }
        OnFinished.ExecuteIfBound();
    }
};
