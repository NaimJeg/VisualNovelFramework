#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NovelRuntimeSettings.h"
#include "NovelLegacyChapterConverter.generated.h"

class UNovelChapterAsset;

USTRUCT(BlueprintType)
struct NOVELEDITOR_API FNovelLegacyConversionReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Conversion")
    TArray<FString> ConvertedChapters;

    UPROPERTY(BlueprintReadOnly, Category = "Conversion")
    int32 ConvertedNodeCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Conversion")
    TArray<FString> Warnings;

    UPROPERTY(BlueprintReadOnly, Category = "Conversion")
    TArray<FString> MissingTargets;

    UPROPERTY(BlueprintReadOnly, Category = "Conversion")
    TArray<FString> UnsupportedConstructs;

    UPROPERTY(BlueprintReadOnly, Category = "Conversion")
    TArray<FString> AssetsRequiringManualReview;

    bool HasBlockingIssues() const { return !MissingTargets.IsEmpty(); }
};

UCLASS()
class NOVELEDITOR_API UNovelLegacyChapterConverter : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Novel|Conversion")
    static UNovelChapterAsset* ConvertLegacyChapter(
        int32 ChapterID,
        const FChapterData& LegacyChapter,
        const FString& DestinationPackagePath,
        FNovelLegacyConversionReport& OutReport);
};
