// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntent_PlaySFX.h"
#include "NovelStorySubsystem.h"

void UNovelIntent_PlaySFX::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    if (StorySys)
    {
        StorySys->PlaySFX(SFXKey);
    }

    Completion.ExecuteIfBound();
}