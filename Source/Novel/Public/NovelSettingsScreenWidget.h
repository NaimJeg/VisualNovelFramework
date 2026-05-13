// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelSettingsScreenWidget.generated.h"

class UComboBoxString;
class USlider;
class UNovelGameUserSettings; 
class UNovelStorySubsystem;

UCLASS()
class NOVEL_API UNovelSettingsScreenWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    UComboBoxString* CB_WindowMode;

    UPROPERTY(meta = (BindWidget))
    UComboBoxString* CB_Resolution;

    UPROPERTY(meta = (BindWidget))
    USlider* Slider_MasterVolume;

    UPROPERTY(meta = (BindWidget))
    USlider* Slider_BGMVolume;

    UPROPERTY(meta = (BindWidget))
    USlider* Slider_SFXVolume;


    UFUNCTION()
    void OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnMasterVolumeChanged(float Value);

    UFUNCTION()
    void OnBGMVolumeChanged(float Value);

    UFUNCTION()
    void OnSFXVolumeChanged(float Value);

private:

    UFUNCTION()
    void OnSliderReleased();

    UPROPERTY()
    UNovelGameUserSettings* UserSettings;

    UPROPERTY()
    UNovelStorySubsystem* StorySys;
};