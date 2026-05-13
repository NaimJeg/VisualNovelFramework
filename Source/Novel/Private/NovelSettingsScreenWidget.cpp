
#include "NovelSettingsScreenWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UNovelSettingsScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CB_WindowMode)
    {
        CB_WindowMode->ClearOptions();
        CB_WindowMode->AddOption(TEXT("Fullscreen"));
        CB_WindowMode->AddOption(TEXT("Windowed Fullscreen"));
        CB_WindowMode->AddOption(TEXT("Windowed"));

        UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings();
        if (UserSettings)
        {
            EWindowMode::Type CurrentMode = UserSettings->GetFullscreenMode();
            switch (CurrentMode)
            {
                case EWindowMode::Fullscreen: CB_WindowMode->SetSelectedIndex(0); break;
                case EWindowMode::WindowedFullscreen: CB_WindowMode->SetSelectedIndex(1); break;
                case EWindowMode::Windowed: CB_WindowMode->SetSelectedIndex(2); break;
            }
        }

        CB_WindowMode->OnSelectionChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnWindowModeChanged);
    }

    if (GlobalSoundMix)
    {
        UGameplayStatics::PushSoundMixModifier(this, GlobalSoundMix);
    }

    if (Slider_MasterVolume)
    {
        // 实际项目中这里应该从存档(SaveGame)中读取上一次的音量值并设置
        // Slider_MasterVolume->SetValue(SavedMasterVolume);
        Slider_MasterVolume->OnValueChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnMasterVolumeChanged);
    }
    
    if (Slider_BGMVolume) Slider_BGMVolume->OnValueChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnBGMVolumeChanged);
    if (Slider_SFXVolume) Slider_SFXVolume->OnValueChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnSFXVolumeChanged);
}

void UNovelSettingsScreenWidget::OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings();
    if (!UserSettings) return;

    if (SelectedItem == TEXT("Fullscreen"))
    {
        UserSettings->SetFullscreenMode(EWindowMode::Fullscreen);
    }
    else if (SelectedItem == TEXT("Windowed Fullscreen"))
    {
        UserSettings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
    }
    else if (SelectedItem == TEXT("Windowed"))
    {
        UserSettings->SetFullscreenMode(EWindowMode::Windowed);
    }

    UserSettings->ApplySettings(true);
}

void UNovelSettingsScreenWidget::OnMasterVolumeChanged(float Value)
{
    SetVolume(MasterSoundClass, Value);
}

void UNovelSettingsScreenWidget::OnBGMVolumeChanged(float Value)
{
    SetVolume(BGMSoundClass, Value);
}

void UNovelSettingsScreenWidget::OnSFXVolumeChanged(float Value)
{
    SetVolume(SFXSoundClass, Value);
}

void UNovelSettingsScreenWidget::SetVolume(USoundClass* TargetClass, float Volume)
{
    if (TargetClass && GlobalSoundMix)
    {
        UGameplayStatics::SetSoundMixClassOverride(this, GlobalSoundMix, TargetClass, Volume, 1.0f, 0.0f, true);
    }
}