#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NovelDialogueBranchData.h"
#include "NovelStoryAsset.h"
#include "Sound/SoundBase.h"
#include "NovelRuntimeSettings.generated.h"

class UDataTable;
class USoundClass;
class USoundMix;

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelSoundConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SoundAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float StartTimeOffset = 0.0f;
};

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FChapterData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Chapter")
    TSoftObjectPtr<UNovelChapterAsset> ChapterAsset;

    UPROPERTY(EditAnywhere, Category = "Chapter", meta = (DeprecatedProperty, DeprecationMessage = "Convert to UNovelChapterAsset."))
    TSoftObjectPtr<UDataTable> DialogueTable;

    UPROPERTY(EditAnywhere, Category = "Chapter")
    TSoftObjectPtr<UNovelDialogueBranchData> BranchData;
};

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Novel Runtime Settings"))
class NOVELRUNTIME_API UNovelRuntimeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName("Plugins"); }
    virtual FName GetSectionName() const override { return FName("Novel Runtime"); }

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    UPROPERTY(Config, EditAnywhere, Category = "Data")
    TSoftObjectPtr<UNovelStoryAsset> EntryStory;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy Data", meta = (DeprecatedProperty, DeprecationMessage = "Use EntryStory."))
    TMap<int32, FChapterData> ChapterMap;

    UPROPERTY(Config, EditAnywhere, Category = "Audio Bank")
    TMap<FName, FNovelSoundConfig> SFXBank;

    UPROPERTY(Config, EditAnywhere, Category = "Audio Mix")
    TSoftObjectPtr<USoundMix> GlobalSoundMix;

    UPROPERTY(Config, EditAnywhere, Category = "Audio Mix")
    TSoftObjectPtr<USoundClass> MasterSoundClass;

    UPROPERTY(Config, EditAnywhere, Category = "Audio Mix")
    TSoftObjectPtr<USoundClass> BGMSoundClass;

    UPROPERTY(Config, EditAnywhere, Category = "Audio Mix")
    TSoftObjectPtr<USoundClass> SFXSoundClass;

    UPROPERTY(Config, EditAnywhere, Category = "Runtime", meta = (ClampMin = "0"))
    int32 MaximumHistoryEntries = 0;

    UPROPERTY(Config, EditAnywhere, Category = "Save")
    FString DefaultSaveSlotPrefix = TEXT("Novel_");

    UPROPERTY(Config, EditAnywhere, Category = "Save")
    FString SaveIndexSlotName = TEXT("__NovelSaveIndex");
};
