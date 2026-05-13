// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NovelCharacterData.generated.h"

UCLASS()
class NOVEL_API UNovelCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    FText CharacterName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprites")
    TMap<FName, TSoftObjectPtr<UTexture2D>> Expressions;
};