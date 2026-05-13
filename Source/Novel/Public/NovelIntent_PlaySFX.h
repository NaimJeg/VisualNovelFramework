// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelStorySubsystem.h"
#include "NovelIntent_PlaySFX.generated.h"

UCLASS(DisplayName = "Intent: Play SFX")
class NOVEL_API UNovelIntent_PlaySFX : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Audio")
    FName SFXKey;

    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished) override
    {
        if (StorySys)
        {
            StorySys->PlaySFX(SFXKey);
        }
        OnFinished.ExecuteIfBound();
    }
};