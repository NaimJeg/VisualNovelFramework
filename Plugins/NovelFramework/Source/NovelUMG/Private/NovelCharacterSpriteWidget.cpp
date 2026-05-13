// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelCharacterSpriteWidget.h"
#include "Components/Image.h"

void UNovelCharacterSpriteWidget::SetSprite(TSoftObjectPtr<UTexture2D> CharacterSprite)
{
    if (Img_Sprite)
    {
        if (CharacterSprite.IsNull())
        {
            Img_Sprite->SetVisibility(ESlateVisibility::Hidden);
            Img_Sprite->SetBrushFromTexture(nullptr);
        }
        else
        {
            Img_Sprite->SetBrushFromSoftTexture(CharacterSprite);
            Img_Sprite->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }
}