// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntent_SetBackground.h"
#include "NovelStorySubsystem.h"

void UNovelIntent_SetBackground::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    if (StorySys)
    {
        StorySys->SetBackground(BackgroundTexture);
    }

    Completion.ExecuteIfBound();
}