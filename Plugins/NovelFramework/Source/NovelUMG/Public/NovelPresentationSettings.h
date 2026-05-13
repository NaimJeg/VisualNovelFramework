#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NovelPresentationSettings.generated.h"

class UNovelDialogueOptionWidget;
class UUserWidget;

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Novel Presentation Settings"))
class NOVELUMG_API UNovelPresentationSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName("Plugins"); }
    virtual FName GetSectionName() const override { return FName("Novel Presentation"); }

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> RootScreenClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    bool bAutoCreateRootScreen = false;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> DialogueScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> TitleScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> SettingsScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> SaveLoadScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSubclassOf<UNovelDialogueOptionWidget> OptionWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> CharacterSpriteWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftClassPtr<UUserWidget> HistoryScreenWidgetClass;
};
