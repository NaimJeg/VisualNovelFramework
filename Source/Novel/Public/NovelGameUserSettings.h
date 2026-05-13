#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "NovelGameUserSettings.generated.h"

/**
 * @class UNovelGameUserSettings
 * @brief Global, independent data class for player preferences. 
 *        Serializes automatically to GameUserSettings.ini.
 */
UCLASS()
class NOVEL_API UNovelGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:

    UNovelGameUserSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static UNovelGameUserSettings* GetNovelGameUserSettings();

    UPROPERTY(Config)
    float MasterVolume;

    UPROPERTY(Config)
    float BGMVolume;

    UPROPERTY(Config)
    float SFXVolume;

    UPROPERTY(Config)
    float TextSpeed;

    virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

};