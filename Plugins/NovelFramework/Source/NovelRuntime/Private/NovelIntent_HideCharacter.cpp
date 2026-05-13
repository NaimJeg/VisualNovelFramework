// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntent_HideCharacter.h"
#include "NovelStorySubsystem.h"

void UNovelIntent_HideCharacter::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    if (StorySys)
    {
        StorySys->HideCharacter(CharacterID);
    }

    Completion.ExecuteIfBound();
}