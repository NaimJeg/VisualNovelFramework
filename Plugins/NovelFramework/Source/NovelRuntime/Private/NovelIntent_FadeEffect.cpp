// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntent_FadeEffect.h"
#include "NovelStorySubsystem.h"

void UNovelIntent_FadeEffect::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    if (StorySys)
    {
        StorySys->RequestScreenFade(FadeColor, FadeDuration, Completion, bWaitUntilFinished);
        return;
    }

    Completion.ExecuteIfBound();
}