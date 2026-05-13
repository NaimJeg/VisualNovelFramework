// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelTitleScreenWidget.h"
#include "NovelStorySubsystem.h"
#include "Components/Button.h"

void UNovelTitleScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Continue) Btn_Continue->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::ContinueLastGame);
    if (Btn_Start) Btn_Start->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::StartNewGame);
    if (Btn_Quit) Btn_Quit->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::QuitGame);
    if (Btn_Load) Btn_Load->OnClicked.AddDynamic(this, &UNovelTitleScreenWidget::OpenSaveLoadMenu);
}

void UNovelTitleScreenWidget::StartNewGame()
{
    if (UNovelStorySubsystem* SS = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    {
        SS->StartNewGame();
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
    if (UNovelStorySubsystem* SS = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    {
        SS->QuitGame();
    }
}

void UNovelTitleScreenWidget::OpenSaveLoadMenu()
{
    if (UNovelStorySubsystem* SS = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    {
        SS->ShowSaveLoadMenu(false);
    }
}

void UNovelTitleScreenWidget::OpenSettingsMenu()
{
}