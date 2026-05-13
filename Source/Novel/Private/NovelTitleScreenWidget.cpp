// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelTitleScreenWidget.h"
#include "NovelStorySubsystem.h"
#include "NovelPresentationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/Button.h"

void UNovelTitleScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Continue) { Btn_Continue->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::ContinueLastGame); Btn_Continue->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::ContinueLastGame); }
    if (Btn_Start) { Btn_Start->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::StartNewGame); Btn_Start->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::StartNewGame); }
    if (Btn_Quit) { Btn_Quit->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::QuitGame); Btn_Quit->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::QuitGame); }
    if (Btn_Load) { Btn_Load->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::OpenSaveLoadMenu); Btn_Load->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::OpenSaveLoadMenu); }
    if (Btn_Settings) { Btn_Settings->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::OpenSettingsMenu); Btn_Settings->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::OpenSettingsMenu); }
}

void UNovelTitleScreenWidget::NativeDestruct()
{
    if (Btn_Continue) Btn_Continue->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::ContinueLastGame);
    if (Btn_Start) Btn_Start->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::StartNewGame);
    if (Btn_Quit) Btn_Quit->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::QuitGame);
    if (Btn_Load) Btn_Load->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::OpenSaveLoadMenu);
    if (Btn_Settings) Btn_Settings->OnClicked.RemoveDynamic(this, &UNovelTitleScreenWidget::OpenSettingsMenu);

    Super::NativeDestruct();
}
void UNovelTitleScreenWidget::StartNewGame()
{
    if (UNovelStorySubsystem* SS = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    {
        SS->StartStory();
    }
}

void UNovelTitleScreenWidget::ContinueLastGame()
{
    if (UNovelStorySubsystem* SS = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    {
        SS->ContinueLastGame();
    }
}

void UNovelTitleScreenWidget::QuitGame()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UNovelTitleScreenWidget::OpenSaveLoadMenu()
{
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UNovelPresentationSubsystem* Presentation = LocalPlayer->GetSubsystem<UNovelPresentationSubsystem>())
        {
            Presentation->RequestSaveLoadScreen(false);
        }
    }
}

void UNovelTitleScreenWidget::OpenSettingsMenu()
{
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UNovelPresentationSubsystem* Presentation = LocalPlayer->GetSubsystem<UNovelPresentationSubsystem>())
        {
            Presentation->RequestSettingsScreen();
        }
    }
}