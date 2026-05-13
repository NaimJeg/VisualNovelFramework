// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelCharacterData.h"
#include "NovelStorySubsystem.h"
#include "NovelIntent_ShowCharacter.generated.h"

UCLASS(DisplayName = "Intent: Show Character")
class NOVEL_API UNovelIntent_ShowCharacter : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Character")
    FName CharacterSlot = TEXT("");

    UPROPERTY(EditAnywhere, Category = "Character")
    UNovelCharacterData* CharacterData;

    UPROPERTY(EditAnywhere, Category = "Character")
    FName Expression = TEXT("");



    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished) override
    {
        if (StorySys && CharacterData)
        {
            if (TSoftObjectPtr<UTexture2D>* FoundSprite = CharacterData->Expressions.Find(Expression))
            {
                StorySys->OnCharacterShownEvent.Broadcast(CharacterSlot, *FoundSprite);
            }
        }
        OnFinished.ExecuteIfBound();
    }
};