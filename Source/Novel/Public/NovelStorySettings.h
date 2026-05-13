#pragma once

#include "CoreMinimal.h"
#include "NovelDialogueBranchData.h"
#include "NovelRootScreen.h"
#include "Sound/SoundBase.h"
#include "Engine/DeveloperSettings.h"
#include "NovelStorySettings.generated.h"

/** -------------------------------------------------------------------------- *
 *  Audio Configuration
 * --------------------------------------------------------------------------- */

/**
 * @struct FNovelSoundConfig
 * @brief Configuration for playing a sound effect or BGM, including start offsets.
 */
USTRUCT(BlueprintType)
struct FNovelSoundConfig
{
    GENERATED_BODY()

    /// The soft reference to the actual sound asset (SoundWave, SoundCue, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SoundAsset;

    /// The offset (in seconds) to skip leading silence or start at a specific point
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float StartTimeOffset = 0.0f;
};

/** -------------------------------------------------------------------------- *
 *  Chapter Data Configuration
 * --------------------------------------------------------------------------- */

/**
 * @struct FChapterData
 * @brief Holds the necessary data tables and branching assets for a specific chapter.
 */
USTRUCT(BlueprintType)
struct FChapterData
{
    GENERATED_BODY()

    /// The Data Table containing the sequential dialogue lines for this chapter
    UPROPERTY(EditAnywhere, Category = "Chapter")
    TSoftObjectPtr<UDataTable> DialogueTable;

    /// The Data Asset containing branching logic and choices for this chapter
    UPROPERTY(EditAnywhere, Category = "Chapter")
    TSoftObjectPtr<UNovelDialogueBranchData> BranchData;
};

/** -------------------------------------------------------------------------- *
 *  Global Project Settings
 * --------------------------------------------------------------------------- */

/**
 * @class UNovelStorySettings
 * @brief Global developer settings for the Visual Novel framework.
 *        Accessible in the Unreal Editor under Project Settings -> Game -> Novel Story.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Novel Story Settings"))
class NOVEL_API UNovelStorySettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    /** ---------------------------------------------------------------------- *
     *  Settings Menu Registration
     * ----------------------------------------------------------------------- */

    /// Defines the overarching category in Project Settings (e.g., "Game")
    virtual FName GetCategoryName() const override { return FName("Game"); }

    /// Defines the specific section name in Project Settings (e.g., "Novel Story")
    virtual FName GetSectionName() const override { return FName("Novel Story"); }

    /** ---------------------------------------------------------------------- *
     *  UI Class References
     * ----------------------------------------------------------------------- */

    /// The master screen that handles UI layering and routing
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<class UNovelRootScreen> RootScreenClass;

    /// The main gameplay screen displaying characters, backgrounds, and text
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> DialogueScreenWidgetClass;

    /// The main menu screen displayed when the game boots
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> TitleScreenWidgetClass;

    /// The configuration screen for adjusting volumes, window modes, etc.
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> SettingsScreenWidgetClass;

    /// The interface used for creating, overriding, and loading save slots
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> SaveLoadScreenWidgetClass;

    /// The transition screen displayed during asynchronous asset loading
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass;

    /// The dynamically generated widget class representing a single dialogue choice
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> OptionWidgetClass;

    /** ---------------------------------------------------------------------- *
     *  Story & Chapter Management
     * ----------------------------------------------------------------------- */

    /// Maps a Chapter ID (int32) to its corresponding Dialogue and Branch data
    UPROPERTY(Config, EditAnywhere, Category = "Data")
    TMap<int32, FChapterData> ChapterMap;

    /** ---------------------------------------------------------------------- *
     *  Audio Management
     * ----------------------------------------------------------------------- */

    /// Global dictionary for Sound Effects, mapping string keys to actual sound configs
    UPROPERTY(Config, EditAnywhere, Category = "Audio Bank")
    TMap<FName, FNovelSoundConfig> SFXBank;
};