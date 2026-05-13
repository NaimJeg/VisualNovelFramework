// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelStorySubsystem.h"
#include "NovelIntent_HideCharacter.generated.h"

UCLASS(DisplayName = "Intent: Hide Character")
class NOVEL_API UNovelIntent_HideCharacter : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Character")
    FName CharacterSlot = TEXT("");

    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished) override
    {
        if (StorySys)
        {
            StorySys->OnCharacterHiddenEvent.Broadcast(CharacterSlot);
        }
        OnFinished.ExecuteIfBound();
    }
};