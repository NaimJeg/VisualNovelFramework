#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "NovelRootScreen.generated.h"

class UBorder;
class UOverlay;
class UWidgetSwitcher;
class UNovelHistoryScreenWidget;
class UNovelPresentationSubsystem;
class UNovelStorySubsystem;

UCLASS()
class NOVEL_API UNovelRootScreen : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidget))
    UWidgetSwitcher* MenuSwitcher;

    UPROPERTY(meta = (BindWidgetOptional))
    UOverlay* Overlay_Popups;

    UPROPERTY()
    UUserWidget* SettingsScreenWBP;

    UPROPERTY()
    UUserWidget* SaveLoadScreenWBP;

    UPROPERTY()
    UUserWidget* TitleScreenWBP;

    UPROPERTY()
    UUserWidget* DialogueScreenWBP;

    UPROPERTY()
    TObjectPtr<UNovelHistoryScreenWidget> HistoryScreenWBP;

    UPROPERTY()
    UNovelStorySubsystem* StorySys;

    UPROPERTY()
    TObjectPtr<UNovelPresentationSubsystem> PresentationSys;

    UPROPERTY()
    UBorder* RuntimeFadeOverlay;

    UPROPERTY()
    TArray<UWidget*> MenuHistory;

    FTimerHandle FadeTimerHandle;
    FLinearColor ActiveFadeColor = FLinearColor::Black;
    float ActiveFadeDuration = 0.0f;
    float ActiveFadeElapsed = 0.0f;
    bool bFadeCompletionSent = true;

    void PushMenu(UWidget* NewWidget);
    void PopMenu();
    void EnsureFadeOverlay();
    void TickFade();
    void FinishFade();

    UFUNCTION()
    void HandleScreenFade(FLinearColor FadeColor, float FadeDuration);

public:
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void SwitchToSettingsScreen();

    UFUNCTION(BlueprintCallable, Category = "Menu")
    void SwitchToSaveLoadScreen(bool bIsSaveMode);

    UFUNCTION(BlueprintCallable, Category = "Menu")
    void SwitchToTitleScreen();

    UFUNCTION(BlueprintCallable, Category = "Menu")
    void SwitchToDialogueScreen();

    UFUNCTION(BlueprintCallable, Category = "Menu")
    void ShowHistoryScreen();

    UFUNCTION(BlueprintCallable, Category = "Menu")
    void HideHistoryScreen();
};
