#pragma once

#include "CoreMinimal.h"
#include "NovelStoryTypes.h"
#include "NovelHistoryListItem.generated.h"

UCLASS(BlueprintType)
class NOVELUMG_API UNovelHistoryListItem : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "History")
    FNovelHistoryEntry Entry;

    void Initialize(const FNovelHistoryEntry& InEntry);
};
