#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NovelChapterAsset.h"
#include "NovelSaveIndex.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelSaveSlotMetadata
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString SlotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FDateTime Timestamp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FPrimaryAssetId StoryId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FNovelNodeRef Node;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FText LinePreview;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    double PlayTimeSeconds = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 SnapshotSchemaVersion = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TSoftObjectPtr<UTexture2D> Background;
};

UCLASS()
class NOVELRUNTIME_API UNovelSaveIndex : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 IndexSchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TArray<FNovelSaveSlotMetadata> Slots;

    void SortSlots();
    void Upsert(const FNovelSaveSlotMetadata& Metadata);
};
