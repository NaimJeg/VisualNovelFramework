// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntent_PlayBGM.h"
#include "NovelStorySubsystem.h"

void UNovelIntent_PlayBGM::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    if (StorySys)
    {
        StorySys->PlayBGM(BGMTexture, FadeInTime);
    }

    Completion.ExecuteIfBound();
}