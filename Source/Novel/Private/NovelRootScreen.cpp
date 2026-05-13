#include "NovelRootScreen.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/WidgetSwitcher.h"
#include "NovelGameUserSettings.h"
#include "NovelDialogueScreenWidget.h"
#include "NovelHistoryScreenWidget.h"
#include "NovelLog.h"
#include "NovelSaveLoadScreenWidget.h"
#include "NovelPresentationSettings.h"
#include "NovelPresentationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "NovelStorySubsystem.h"

void UNovelRootScreen::NativeConstruct()
{
    Super::NativeConstruct();

    StorySys = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelStorySubsystem>() : nullptr;
    PresentationSys = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UNovelPresentationSubsystem>() : nullptr;
    if (UNovelGameUserSettings* UserSettings = UNovelGameUserSettings::GetNovelGameUserSettings())
    {
        if (StorySys)
        {
            StorySys->ApplyAudioSettingsFromData(UserSettings->MasterVolume, UserSettings->BGMVolume, UserSettings->SFXVolume);
        }
    }

    if (MenuSwitcher)
    {
        const UNovelPresentationSettings* Settings = GetDefault<UNovelPresentationSettings>();

        if (Settings && !Settings->TitleScreenWidgetClass.IsNull() && !TitleScreenWBP)
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

        if (Settings && !Settings->SettingsScreenWidgetClass.IsNull() && !SettingsScreenWBP)
        {
            if (UClass* Class = Settings->SettingsScreenWidgetClass.LoadSynchronous())
            {
                SettingsScreenWBP = CreateWidget(this, Class);
                if (SettingsScreenWBP)
                {
                    MenuSwitcher->AddChild(SettingsScreenWBP);
                }
            }
        }

        if (Settings && !Settings->SaveLoadScreenWidgetClass.IsNull() && !SaveLoadScreenWBP)
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

        if (Settings && !Settings->DialogueScreenWidgetClass.IsNull() && !DialogueScreenWBP)
        {
            if (UClass* Class = Settings->DialogueScreenWidgetClass.LoadSynchronous())
            {
                DialogueScreenWBP = CreateWidget(this, Class);
                if (DialogueScreenWBP)
                {
                    MenuSwitcher->AddChild(DialogueScreenWBP);
                    if (UNovelDialogueScreenWidget* DialogueScreen = Cast<UNovelDialogueScreenWidget>(DialogueScreenWBP))
                    {
                        const UNovelGameUserSettings* UserSettings = UNovelGameUserSettings::GetNovelGameUserSettings();
                        DialogueScreen->SetTextSpeedMultiplier(UserSettings ? UserSettings->TextSpeed : 1.0f);
                    }
                }
            }
        }

        if (Settings && !Settings->HistoryScreenWidgetClass.IsNull() && !HistoryScreenWBP)
        {
            if (UClass* Class = Settings->HistoryScreenWidgetClass.LoadSynchronous())
            {
                HistoryScreenWBP = CreateWidget<UNovelHistoryScreenWidget>(this, Class);
                if (HistoryScreenWBP && Overlay_Popups)
                {
                    Overlay_Popups->AddChild(HistoryScreenWBP);
                    HistoryScreenWBP->SetVisibility(ESlateVisibility::Collapsed);
                }
                else if (!HistoryScreenWBP)
                {
                    UE_LOG(LogNovel, Error, TEXT("HistoryScreenWidgetClass must derive from UNovelHistoryScreenWidget."));
                }
            }
        }

        if (HistoryScreenWBP)
        {
            HistoryScreenWBP->OnCloseRequested.AddUniqueDynamic(this, &UNovelRootScreen::HideHistoryScreen);
        }

        SwitchToTitleScreen();
    }

    EnsureFadeOverlay();

    if (StorySys)
    {
        StorySys->OnGameStartedEvent.AddUniqueDynamic(this, &UNovelRootScreen::SwitchToDialogueScreen);
        StorySys->OnScreenFadeEvent.AddUniqueDynamic(this, &UNovelRootScreen::HandleScreenFade);
    }
    if (PresentationSys)
    {
        PresentationSys->OnSaveLoadScreenRequested.AddUniqueDynamic(this, &UNovelRootScreen::SwitchToSaveLoadScreen);
        PresentationSys->OnSettingsScreenRequested.AddUniqueDynamic(this, &UNovelRootScreen::SwitchToSettingsScreen);
        PresentationSys->OnHistoryScreenRequested.AddUniqueDynamic(this, &UNovelRootScreen::ShowHistoryScreen);
    }
}

void UNovelRootScreen::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FadeTimerHandle);
    }

    FinishFade();

    if (HistoryScreenWBP)
    {
        HistoryScreenWBP->OnCloseRequested.RemoveDynamic(this, &UNovelRootScreen::HideHistoryScreen);
    }

    if (StorySys)
    {
        StorySys->OnGameStartedEvent.RemoveDynamic(this, &UNovelRootScreen::SwitchToDialogueScreen);
        StorySys->OnScreenFadeEvent.RemoveDynamic(this, &UNovelRootScreen::HandleScreenFade);
    }

    if (PresentationSys)
    {
        PresentationSys->OnSaveLoadScreenRequested.RemoveDynamic(this, &UNovelRootScreen::SwitchToSaveLoadScreen);
        PresentationSys->OnSettingsScreenRequested.RemoveDynamic(this, &UNovelRootScreen::SwitchToSettingsScreen);
        PresentationSys->OnHistoryScreenRequested.RemoveDynamic(this, &UNovelRootScreen::ShowHistoryScreen);
    }

    PresentationSys = nullptr;
    StorySys = nullptr;
    Super::NativeDestruct();
}

FReply UNovelRootScreen::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (HistoryScreenWBP && HistoryScreenWBP->GetVisibility() == ESlateVisibility::Visible)
        {
            HideHistoryScreen();
            return FReply::Handled();
        }

        PopMenu();
        return FReply::Handled();
    }

    return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UNovelRootScreen::ShowHistoryScreen()
{
    if (HistoryScreenWBP)
    {
        HistoryScreenWBP->RefreshHistory();
        HistoryScreenWBP->SetIsEnabled(true);
        HistoryScreenWBP->SetVisibility(ESlateVisibility::Visible);
        HistoryScreenWBP->SetKeyboardFocus();
    }
}

void UNovelRootScreen::HideHistoryScreen()
{
    if (HistoryScreenWBP)
    {
        HistoryScreenWBP->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (MenuSwitcher && MenuSwitcher->GetActiveWidget())
    {
        MenuSwitcher->GetActiveWidget()->SetKeyboardFocus();
    }
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
    PushMenu(DialogueScreenWBP);
}

void UNovelRootScreen::PushMenu(UWidget* NewWidget)
{
    if (!MenuSwitcher || !NewWidget)
    {
        return;
    }

    UWidget* CurrentWidget = MenuSwitcher->GetActiveWidget();
    if (CurrentWidget == NewWidget)
    {
        return;
    }

    if (CurrentWidget)
    {
        MenuHistory.Push(CurrentWidget);
    }

    MenuSwitcher->SetActiveWidget(NewWidget);
}

void UNovelRootScreen::PopMenu()
{
    if (!MenuSwitcher || MenuHistory.IsEmpty())
    {
        return;
    }

    UWidget* PreviousWidget = MenuHistory.Pop();
    if (PreviousWidget)
    {
        MenuSwitcher->SetActiveWidget(PreviousWidget);
    }
}

void UNovelRootScreen::EnsureFadeOverlay()
{
    if (RuntimeFadeOverlay || !Overlay_Popups || !WidgetTree)
    {
        return;
    }

    RuntimeFadeOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeFadeOverlay"));
    if (!RuntimeFadeOverlay)
    {
        return;
    }

    RuntimeFadeOverlay->SetBrushColor(FLinearColor::Transparent);
    RuntimeFadeOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);

    UOverlaySlot* OverlaySlot = Overlay_Popups->AddChildToOverlay(RuntimeFadeOverlay);
    if (OverlaySlot)
    {
        OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
        OverlaySlot->SetVerticalAlignment(VAlign_Fill);
    }
}

void UNovelRootScreen::HandleScreenFade(FLinearColor FadeColor, float FadeDuration)
{
    EnsureFadeOverlay();
    ActiveFadeColor = FadeColor;
    ActiveFadeDuration = FMath::Max(0.0f, FadeDuration);
    ActiveFadeElapsed = 0.0f;
    bFadeCompletionSent = false;

    if (!RuntimeFadeOverlay || ActiveFadeDuration <= 0.0f)
    {
        FinishFade();
        return;
    }

    RuntimeFadeOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    RuntimeFadeOverlay->SetBrushColor(FLinearColor(ActiveFadeColor.R, ActiveFadeColor.G, ActiveFadeColor.B, 0.0f));

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FadeTimerHandle);
        World->GetTimerManager().SetTimer(FadeTimerHandle, this, &UNovelRootScreen::TickFade, 0.016f, true);
    }
    else
    {
        FinishFade();
    }
}

void UNovelRootScreen::TickFade()
{
    if (!RuntimeFadeOverlay || ActiveFadeDuration <= 0.0f)
    {
        FinishFade();
        return;
    }

    ActiveFadeElapsed += 0.016f;
    const float Progress = FMath::Clamp(ActiveFadeElapsed / ActiveFadeDuration, 0.0f, 1.0f);
    const float WaveAlpha = Progress <= 0.5f ? Progress * 2.0f : (1.0f - Progress) * 2.0f;
    const float TargetAlpha = ActiveFadeColor.A <= 0.0f ? 1.0f : ActiveFadeColor.A;
    RuntimeFadeOverlay->SetBrushColor(FLinearColor(ActiveFadeColor.R, ActiveFadeColor.G, ActiveFadeColor.B, TargetAlpha * WaveAlpha));

    if (Progress >= 1.0f)
    {
        FinishFade();
    }
}

void UNovelRootScreen::FinishFade()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FadeTimerHandle);
    }

    if (RuntimeFadeOverlay)
    {
        RuntimeFadeOverlay->SetBrushColor(FLinearColor::Transparent);
        RuntimeFadeOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    if (!bFadeCompletionSent)
    {
        bFadeCompletionSent = true;
        if (StorySys)
        {
            StorySys->NotifyScreenFadeFinished();
        }
    }
}