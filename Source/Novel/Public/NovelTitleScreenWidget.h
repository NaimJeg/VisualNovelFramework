#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelTitleScreenWidget.generated.h"

class UButton;

UCLASS()
class NOVEL_API UNovelTitleScreenWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Continue;
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Start;
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Load;
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Settings;
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Quit;

private:
    UFUNCTION()
    void StartNewGame();

    UFUNCTION()
    void ContinueLastGame();

    UFUNCTION()
    void QuitGame();

    UFUNCTION()
    void OpenSaveLoadMenu();

    UFUNCTION()
    void OpenSettingsMenu();
};