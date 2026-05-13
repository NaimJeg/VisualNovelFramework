// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelSettingsScreenWidget.h"
#include "Components/ComboBoxString.h"
#include "NovelGameUserSettings.h"
#include "NovelStorySubsystem.h"
#include "Components/Slider.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

/** -------------------------------------------------------------------------- *
 *  Initialization & Data Binding
 * --------------------------------------------------------------------------- */

void UNovelSettingsScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UserSettings = UNovelGameUserSettings::GetNovelGameUserSettings();
    StorySys = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>();

    if (!UserSettings) return;

    /// Hydrate the UI Sliders with the data loaded from the .ini file
    if (Slider_MasterVolume) Slider_MasterVolume->SetValue(UserSettings->MasterVolume);
    if (Slider_BGMVolume)    Slider_BGMVolume->SetValue(UserSettings->BGMVolume);
    if (Slider_SFXVolume)    Slider_SFXVolume->SetValue(UserSettings->SFXVolume);

    /// 1. Bind real-time updates: Modifies engine audio memory instantly for immediate auditory feedback
    if (Slider_MasterVolume) { Slider_MasterVolume->OnValueChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnMasterVolumeChanged); Slider_MasterVolume->OnValueChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnMasterVolumeChanged); }
    if (Slider_BGMVolume) { Slider_BGMVolume->OnValueChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnBGMVolumeChanged); Slider_BGMVolume->OnValueChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnBGMVolumeChanged); }
    if (Slider_SFXVolume) { Slider_SFXVolume->OnValueChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSFXVolumeChanged); Slider_SFXVolume->OnValueChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnSFXVolumeChanged); }

    /// 2. Bind deferred persistence: Triggers only when the user lets go of the mouse button.
    /// This prevents massive disk I/O stuttering that would occur if we saved on every single tick of the drag.
    if (Slider_MasterVolume) { Slider_MasterVolume->OnMouseCaptureEnd.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased); Slider_MasterVolume->OnMouseCaptureEnd.AddDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased); }
    if (Slider_BGMVolume) { Slider_BGMVolume->OnMouseCaptureEnd.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased); Slider_BGMVolume->OnMouseCaptureEnd.AddDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased); }
    if (Slider_SFXVolume) { Slider_SFXVolume->OnMouseCaptureEnd.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased); Slider_SFXVolume->OnMouseCaptureEnd.AddDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased); }

    /// 3. Establish the dropdown options for display modes. 
    if (CB_WindowMode)
    {
        CB_WindowMode->ClearOptions();
        CB_WindowMode->AddOption(TEXT("Fullscreen"));
        CB_WindowMode->AddOption(TEXT("Windowed Fullscreen"));
        CB_WindowMode->AddOption(TEXT("Windowed"));

        /// Poll the OS/Engine to reflect the actual current window state in the UI
        EWindowMode::Type CurrentMode = UserSettings->GetFullscreenMode();
        switch (CurrentMode)
        {
            case EWindowMode::Fullscreen: CB_WindowMode->SetSelectedIndex(0); break;
            case EWindowMode::WindowedFullscreen: CB_WindowMode->SetSelectedIndex(1); break;
            case EWindowMode::Windowed: CB_WindowMode->SetSelectedIndex(2); break;
        }

        CB_WindowMode->OnSelectionChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnWindowModeChanged);
        CB_WindowMode->OnSelectionChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnWindowModeChanged);
    }

    /// 4. Establish the dropdown options for screen resolutions dynamically.
    if (CB_Resolution)
    {
        CB_Resolution->ClearOptions();

        /// Ask the Engine's hardware interface (RHI) for a clean list of all resolutions natively supported by the user's current monitor.
        TArray<FIntPoint> SupportedResolutions;
        UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions);

        /// Fallback safety: If the engine fails to query the monitor (e.g., in some headless servers or weird window modes), provide a standard default.
        if (SupportedResolutions.IsEmpty())
        {
            SupportedResolutions.Add(FIntPoint(1920, 1080));
        }

        /// Poll current engine resolution to set the correct default dropdown state
        FIntPoint CurrentRes = UserSettings->GetScreenResolution();
        int32 TargetIndex = 0; 

        for (int32 i = 0; i < SupportedResolutions.Num(); ++i)
        {
            /// Format as "1920x1080"
            FString ResString = FString::Printf(TEXT("%dx%d"), SupportedResolutions[i].X, SupportedResolutions[i].Y);
            CB_Resolution->AddOption(ResString);

            /// Sync the UI state with the actual system state
            if (SupportedResolutions[i] == CurrentRes)
            {
                TargetIndex = i;
            }
        }

        /// Initialize the visual dropdown position without triggering the 'OnValueChanged' broadcast prematurely
        CB_Resolution->SetSelectedIndex(TargetIndex);
        
        /// Bind the execution event AFTER setting the default index to prevent accidental resolution resetting on boot
        CB_Resolution->OnSelectionChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnResolutionChanged);
        CB_Resolution->OnSelectionChanged.AddDynamic(this, &UNovelSettingsScreenWidget::OnResolutionChanged);
    }

}

void UNovelSettingsScreenWidget::NativeDestruct()
{
    if (Slider_MasterVolume) Slider_MasterVolume->OnValueChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnMasterVolumeChanged);
    if (Slider_BGMVolume) Slider_BGMVolume->OnValueChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnBGMVolumeChanged);
    if (Slider_SFXVolume) Slider_SFXVolume->OnValueChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSFXVolumeChanged);
    if (Slider_MasterVolume) Slider_MasterVolume->OnMouseCaptureEnd.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased);
    if (Slider_BGMVolume) Slider_BGMVolume->OnMouseCaptureEnd.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased);
    if (Slider_SFXVolume) Slider_SFXVolume->OnMouseCaptureEnd.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnSliderReleased);
    if (CB_WindowMode) CB_WindowMode->OnSelectionChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnWindowModeChanged);
    if (CB_Resolution) CB_Resolution->OnSelectionChanged.RemoveDynamic(this, &UNovelSettingsScreenWidget::OnResolutionChanged);

    StorySys = nullptr;
    UserSettings = nullptr;
    Super::NativeDestruct();
}
/** -------------------------------------------------------------------------- *
 *  Display & Graphics Execution
 * --------------------------------------------------------------------------- */

void UNovelSettingsScreenWidget::OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!UserSettings) return;

    /// Map the UI string representation to the underlying Engine Enum
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

    UserSettings->ApplySettings(false);
    UserSettings->SaveSettings();
}

void UNovelSettingsScreenWidget::OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!UserSettings) return;

    FString LeftStr, RightStr;
    
    /// Parse the "1920x1080" string back into integers for the engine API
    if (SelectedItem.Split(TEXT("x"), &LeftStr, &RightStr))
    {
        FIntPoint NewResolution(FCString::Atoi(*LeftStr), FCString::Atoi(*RightStr));
        
        UserSettings->SetScreenResolution(NewResolution);

        /// IMMEDIATE EXECUTION & PERSISTENCE
        UserSettings->ApplySettings(false);
        UserSettings->SaveSettings();
    }
}

/** -------------------------------------------------------------------------- *
 *  Audio Mixing & Real-Time Application
 * --------------------------------------------------------------------------- */

void UNovelSettingsScreenWidget::OnMasterVolumeChanged(float Value)
{
    if (UserSettings && StorySys)
    {
        UserSettings->MasterVolume = Value;
        StorySys->ApplyAudioSettingsFromData(UserSettings->MasterVolume, UserSettings->BGMVolume, UserSettings->SFXVolume);
    }
}

void UNovelSettingsScreenWidget::OnBGMVolumeChanged(float Value)
{
    if (UserSettings && StorySys)
    {
        UserSettings->BGMVolume = Value;
        StorySys->ApplyAudioSettingsFromData(UserSettings->MasterVolume, UserSettings->BGMVolume, UserSettings->SFXVolume);
    }
}

void UNovelSettingsScreenWidget::OnSFXVolumeChanged(float Value)
{
    if (UserSettings && StorySys)
    {
        UserSettings->SFXVolume = Value;
        StorySys->ApplyAudioSettingsFromData(UserSettings->MasterVolume, UserSettings->BGMVolume, UserSettings->SFXVolume);
    }
}

void UNovelSettingsScreenWidget::OnSliderReleased()
{
    if (UserSettings)
    {
        /// Safely flush the purely data-driven variables to GameUserSettings.ini
        /// now that the high-frequency dragging action has entirely concluded.
        UserSettings->SaveSettings();
    }
}