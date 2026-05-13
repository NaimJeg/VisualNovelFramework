// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntent_ShowCharacter.h"
#include "NovelStorySubsystem.h"

void UNovelIntent_ShowCharacter::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    if (StorySys && CharacterData)
    {
        if (TSoftObjectPtr<UTexture2D>* FoundSprite = CharacterData->Expressions.Find(Expression))
        {
            StorySys->ShowCharacter(CharacterID, *FoundSprite, PositionX);
        }
    }

    Completion.ExecuteIfBound();
}