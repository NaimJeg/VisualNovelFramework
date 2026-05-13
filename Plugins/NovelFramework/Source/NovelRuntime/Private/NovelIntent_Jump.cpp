// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntent_Jump.h"
#include "NovelStorySubsystem.h"

void UNovelIntent_Jump::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    if (StorySys)
    {
        StorySys->RequestNodeTransition(TargetNode, bClearScreen);
    }

    Completion.ExecuteIfBound();
}