// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelSettingsScreenWidget.generated.h"

class UComboBoxString;
class USlider;
class USoundMix;
class USoundClass;

UCLASS()
class NOVEL_API UNovelSettingsScreenWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UComboBoxString* CB_WindowMode;

    UPROPERTY(meta = (BindWidget))
    USlider* Slider_MasterVolume;

    UPROPERTY(meta = (BindWidget))
    USlider* Slider_BGMVolume;

    UPROPERTY(meta = (BindWidget))
    USlider* Slider_SFXVolume;

    
    UPROPERTY(EditAnywhere, Category = "Audio Settings")
    USoundMix* GlobalSoundMix;

    UPROPERTY(EditAnywhere, Category = "Audio Settings")
    USoundClass* MasterSoundClass;

    UPROPERTY(EditAnywhere, Category = "Audio Settings")
    USoundClass* BGMSoundClass;

    UPROPERTY(EditAnywhere, Category = "Audio Settings")
    USoundClass* SFXSoundClass;


    UFUNCTION()
    void OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnMasterVolumeChanged(float Value);

    UFUNCTION()
    void OnBGMVolumeChanged(float Value);

    UFUNCTION()
    void OnSFXVolumeChanged(float Value);

private:
    void SetVolume(USoundClass* TargetClass, float Volume);
};