#include "NovelRootScreen.h"
#include "NovelStorySubsystem.h"
#include "NovelSaveLoadScreenWidget.h"
#include "Components/WidgetSwitcher.h"
#include "NovelStorySettings.h"

void UNovelRootScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (MenuSwitcher)
	{
		const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();

		if (!Settings->TitleScreenWidgetClass.IsNull())
		{
			if (UClass* Class = Settings->TitleScreenWidgetClass.LoadSynchronous())
			{
				TitleScreenWBP = CreateWidget(this, Class);
				if (TitleScreenWBP)
				{
					MenuSwitcher->AddChild(TitleScreenWBP);
				}
			}
		}

		if (!Settings->TitleScreenWidgetClass.IsNull())
		{
			if (UClass* Class = Settings->TitleScreenWidgetClass.LoadSynchronous())
			{
				SettingsScreenWBP = CreateWidget(this, Class);
				if (SettingsScreenWBP)
				{
					MenuSwitcher->AddChild(SettingsScreenWBP);
				}
			}
		}

		if (!Settings->SaveLoadScreenWidgetClass.IsNull())
		{
			if (UClass* Class = Settings->SaveLoadScreenWidgetClass.LoadSynchronous())
			{
				SaveLoadScreenWBP = CreateWidget(this, Class);
				if (SaveLoadScreenWBP)
				{
					MenuSwitcher->AddChild(SaveLoadScreenWBP);
				}
			}
		}

		if (!Settings->DialogueScreenWidgetClass.IsNull())
		{
			if (UClass* Class = Settings->DialogueScreenWidgetClass.LoadSynchronous())
			{
				DialogueScreenWBP = CreateWidget(this, Class);
				if (DialogueScreenWBP)
				{
					MenuSwitcher->AddChild(DialogueScreenWBP);
				}
			}
		}

		SwitchToTitleScreen();
	}

	if (UNovelStorySubsystem* StorySys = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
	{
		StorySys->OnGameStartedEvent.AddDynamic(this, &UNovelRootScreen::SwitchToDialogueScreen);
		StorySys->OnSaveLoadUIRequestedEvent.AddDynamic(this, &UNovelRootScreen::SwitchToSaveLoadScreen);

	}
}

FReply UNovelRootScreen::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		PopMenu();

		return FReply::Handled();
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UNovelRootScreen::SwitchToTitleScreen()
{
	PushMenu(TitleScreenWBP);
}

void UNovelRootScreen::SwitchToSettingsScreen()
{
	PushMenu(SettingsScreenWBP);
}

void UNovelRootScreen::SwitchToSaveLoadScreen(bool bIsSaveMode)
{
	if (SaveLoadScreenWBP)
	{
		if (UNovelSaveLoadScreenWidget* Screen = Cast<UNovelSaveLoadScreenWidget>(SaveLoadScreenWBP))
		{
			Screen->SetModeAndRefresh(bIsSaveMode);
		}
		PushMenu(SaveLoadScreenWBP);
	}
}

void UNovelRootScreen::SwitchToDialogueScreen()
{
	//MenuHistory.Empty();
	//if (MenuSwitcher && DialogueScreenWBP)
	//{
	//	MenuSwitcher->SetActiveWidget(DialogueScreenWBP);
	//}
	PushMenu(DialogueScreenWBP);
}

void UNovelRootScreen::PushMenu(UWidget* NewWidget)
{
	if (!MenuSwitcher || !NewWidget) return;

	UWidget* CurrentWidget = MenuSwitcher->GetActiveWidget();

	if (CurrentWidget != NewWidget)
	{
		if (CurrentWidget)
		{
			MenuHistory.Push(CurrentWidget);
		}

		MenuSwitcher->SetActiveWidget(NewWidget);
	}
}

void UNovelRootScreen::PopMenu()
{

	UE_LOG(LogTemp, Warning, TEXT("MasterMenu: Pop Requested"));
	if (!MenuSwitcher) return;

	if (MenuHistory.IsEmpty())
	{
		return;
	}

	UWidget* PreviousWidget = MenuHistory.Pop();
	MenuSwitcher->SetActiveWidget(PreviousWidget);
}