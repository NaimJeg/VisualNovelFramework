#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NovelRootScreen.h"
#include "NovelRuntimeSettings.h"
#include "NovelStorySettings.generated.h"

class USoundClass;
class USoundMix;
class UUserWidget;

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Novel Story Settings (Legacy)", DeprecationMessage = "Use Novel Runtime Settings and Novel Presentation Settings."))
class NOVEL_API UNovelStorySettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName("Game"); }
    virtual FName GetSectionName() const override { return FName("Novel Story (Legacy)"); }

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UNovelRootScreen> RootScreenClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UUserWidget> DialogueScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UUserWidget> TitleScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UUserWidget> SettingsScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UUserWidget> SaveLoadScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSubclassOf<UUserWidget> OptionWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UUserWidget> CharacterSpriteWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy UI", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelPresentationSettings."))
    TSoftClassPtr<UUserWidget> HistoryScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy Runtime", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelRuntimeSettings."))
    TMap<int32, FChapterData> ChapterMap;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy Runtime", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelRuntimeSettings."))
    TMap<FName, FNovelSoundConfig> SFXBank;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy Runtime", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelRuntimeSettings."))
    TSoftObjectPtr<USoundMix> GlobalSoundMix;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy Runtime", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelRuntimeSettings."))
    TSoftObjectPtr<USoundClass> MasterSoundClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy Runtime", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelRuntimeSettings."))
    TSoftObjectPtr<USoundClass> BGMSoundClass;

    UPROPERTY(Config, EditAnywhere, Category = "Legacy Runtime", meta = (DeprecatedProperty, DeprecationMessage = "Use UNovelRuntimeSettings."))
    TSoftObjectPtr<USoundClass> SFXSoundClass;
};
