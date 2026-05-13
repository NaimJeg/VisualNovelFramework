#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelCharacterData.h"
#include "NovelIntent_ShowCharacter.generated.h"

UCLASS(DisplayName = "Intent: Show Character")
class NOVELRUNTIME_API UNovelIntent_ShowCharacter : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Character")
    FName CharacterID = NAME_None;

    UPROPERTY(EditAnywhere, Category = "Character")
    UNovelCharacterData* CharacterData = nullptr;

    UPROPERTY(EditAnywhere, Category = "Character")
    FName Expression = NAME_None;

    UPROPERTY(EditAnywhere, Category = "Character", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PositionX = 0.5f;

    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion) override;
};