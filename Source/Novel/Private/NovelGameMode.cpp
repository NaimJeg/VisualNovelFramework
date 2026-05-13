// Fill out your copyright notice in the Description page of Project Settings.


#include "NovelGameMode.h"
#include "NovelStorySettings.h"
#include "Blueprint/UserWidget.h"

ANovelGameMode::ANovelGameMode()
{
	RootScreenIns = nullptr;
}

void ANovelGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetInputMode(FInputModeUIOnly());
		PC->SetShowMouseCursor(true);

		if (const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>())
		{
			if (UClass* MenuClass = Settings->RootScreenClass.LoadSynchronous())
			{
				RootScreenIns = CreateWidget(PC, MenuClass);
				if (RootScreenIns)
				{
					RootScreenIns->AddToViewport();
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("GameMode: RootScreenClass is not set in Story Settings!"));
			}
		}
	}
}