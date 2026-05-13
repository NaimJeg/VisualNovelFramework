#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "NovelIntent_HideCharacter.generated.h"

UCLASS(DisplayName = "Intent: Hide Character")
class NOVELRUNTIME_API UNovelIntent_HideCharacter : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Character")
    FName CharacterID = NAME_None;

    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion) override;
};