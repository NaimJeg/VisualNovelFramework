#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NovelChapterAsset.h"
#include "NovelStoryAsset.generated.h"

UCLASS(BlueprintType)
class NOVELRUNTIME_API UNovelStoryAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
    FNovelNodeRef EntryNode;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
    TArray<TSoftObjectPtr<UNovelChapterAsset>> Chapters;

    TSoftObjectPtr<UNovelChapterAsset> FindChapter(FPrimaryAssetId ChapterId) const;
};
