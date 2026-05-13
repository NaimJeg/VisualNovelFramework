#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NovelCharacterData.generated.h"

UCLASS()
class NOVELRUNTIME_API UNovelCharacterData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    FText CharacterName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprites")
    TMap<FName, TSoftObjectPtr<UTexture2D>> Expressions;
};