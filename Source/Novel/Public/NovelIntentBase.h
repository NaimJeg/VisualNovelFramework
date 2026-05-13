#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NovelIntentBase.generated.h"

class UNovelStorySubsystem;

DECLARE_DELEGATE(FOnIntentFinished);

UCLASS(Abstract, EditInlineNew, Blueprintable)
class NOVEL_API UNovelIntentBase : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Intent Settings")
    bool bWaitUntilFinished = true;

    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished)
    {
        OnFinished.ExecuteIfBound();
    }
};