#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelDialogueScreenWidget.generated.h"

class UNovelStorySubsystem;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
struct FDialogueOption;

UCLASS()
class NOVEL_API UNovelDialogueScreenWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnDialogueLineUpdated(const FText& Speaker, const FText& Text);

    UFUNCTION()
    void OnBackgroundChanged(TSoftObjectPtr<UTexture2D> NewBackground);

    UFUNCTION()
    void OnCharacterShown(FName CharacterSlot, TSoftObjectPtr<UTexture2D> CharacterSprite);

    UFUNCTION()
    void OnCharacterHidden(FName CharacterSlot);

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SpeakerName;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* DialogueText;

    UFUNCTION()
    void NextDialogue();
    
    UFUNCTION()
    void ShowSaveLoadScreen();
    
    UFUNCTION()
    void HideDialogue();
    
    UFUNCTION()
    void ShowHistory();

    UFUNCTION()
    void OnDialogueOptionsShow(const TArray<FDialogueOption>& Options);

    UFUNCTION()
    void OnDialogueOptionsHide();

    UFUNCTION()
    void OnDialogueResetRequested();

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Next;
    
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_SaveLoad;
    
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_History;
    
    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Hide;

    UPROPERTY(meta = (BindWidget))
    UImage* Img_Background;

    UPROPERTY(meta = (BindWidget))
    UImage* Img_CharLeft;

    UPROPERTY(meta = (BindWidget))
    UImage* Img_CharRight;

    UPROPERTY(meta = (BindWidget))
    UImage* Img_CharCenter;
    
    UPROPERTY(meta = (BindWidget))
    UPanelWidget* OptionLayer;

    UPROPERTY(BlueprintReadOnly, Category = "Dialogue UI")
    bool bIsUIHidden = false;

    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue UI")
    void OnDialogueUIVisibilityChanged(bool bIsVisible);

    UPROPERTY()
    UNovelStorySubsystem* StorySys;

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* Box_Options;
};