// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelCharacterSpriteWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS()
class NOVELUMG_API UNovelCharacterSpriteWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "Character")
    void SetSprite(TSoftObjectPtr<UTexture2D> CharacterSprite);

protected:
    UPROPERTY(meta = (BindWidget))
    UImage* Img_Sprite;
};