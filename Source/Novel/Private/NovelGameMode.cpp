// Fill out your copyright notice in the Description page of Project Settings.


#include "NovelGameMode.h"
#include "NovelLog.h"
#include "NovelPresentationSettings.h"
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

		if (const UNovelPresentationSettings* Settings = GetDefault<UNovelPresentationSettings>())
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
				UE_LOG(LogNovel, Error, TEXT("GameMode failed to create root screen. Reason=RootScreenClass is not set in Presentation Settings."));
			}
		}
	}
}