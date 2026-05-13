// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelStorySubsystem.h" 
#include "Engine/Texture2D.h"
#include "NovelIntent_SetBackground.generated.h"

UCLASS(DisplayName = "Intent: Set Background")
class NOVEL_API UNovelIntent_SetBackground : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Background")
    TSoftObjectPtr<UTexture2D> BackgroundTexture;

    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished) override
    {
        if (StorySys)
        {
            StorySys->CurrentBackground = BackgroundTexture;

            StorySys->OnBackgroundChangedEvent.Broadcast(BackgroundTexture);
        }

        OnFinished.ExecuteIfBound();
    }
};