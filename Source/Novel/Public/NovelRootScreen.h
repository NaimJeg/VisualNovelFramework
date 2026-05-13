#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelRootScreen.generated.h"

class UWidgetSwitcher;

UCLASS()
class NOVEL_API UNovelRootScreen : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* MenuSwitcher;

	UPROPERTY()
	UUserWidget* SettingsScreenWBP;

	UPROPERTY()
	UUserWidget* SaveLoadScreenWBP;

	UPROPERTY()
	UUserWidget* TitleScreenWBP;

	UPROPERTY()
	UUserWidget* DialogueScreenWBP;

	UPROPERTY()
	TArray<UWidget*> MenuHistory;

	void PushMenu(UWidget* NewWidget);

	void PopMenu();

public:

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SwitchToSettingsScreen();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SwitchToSaveLoadScreen(bool bIsSaveMode);

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SwitchToTitleScreen();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SwitchToDialogueScreen();
};