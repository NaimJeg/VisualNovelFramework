#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelDialogueScreenWidget.generated.h"

class UNovelStorySubsystem;
class UButton;
class UNovelCharacterSpriteWidget;
class UTextBlock;
class UImage;
class UTexture2D;
class UVerticalBox;
class UCanvasPanel;
class UPanelWidget;
struct FDialogueOption;

UCLASS()
class NOVELUMG_API UNovelDialogueScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Dialogue UI")
    void SetTextSpeedMultiplier(float InMultiplier);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY()
    UNovelStorySubsystem* StorySys;

    UFUNCTION()
    void OnDialogueLineUpdated(const FText& Speaker, const FText& Text);

    UFUNCTION()
    void OnBackgroundChanged(TSoftObjectPtr<UTexture2D> NewBackground);

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

    // UPROPERTY(meta = (BindWidget))
    // UImage* Img_CharLeft;

    // UPROPERTY(meta = (BindWidget))
    // UImage* Img_CharRight;

    // UPROPERTY(meta = (BindWidget))
    // UImage* Img_CharCenter;

    UPROPERTY(meta = (BindWidget))
    UCanvasPanel* CharacterLayer;

    UPROPERTY()
    TMap<FName, UNovelCharacterSpriteWidget*> ActiveCharacters;

    UFUNCTION()
    void OnCharacterShown(FName CharacterID, TSoftObjectPtr<UTexture2D> CharacterSprite, float XPosition);

    UFUNCTION()
    void OnCharacterHidden(FName CharacterID);


    
    UPROPERTY(meta = (BindWidget))
    UPanelWidget* OptionLayer;

    UPROPERTY(BlueprintReadOnly, Category = "Dialogue UI")
    bool bIsUIHidden = false;

    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue UI")
    void OnDialogueUIVisibilityChanged(bool bIsVisible);

    

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* Box_Options;

    FTimerHandle TypewriterTimerHandle;

    FString TargetDialogueString;

    int32 CurrentCharacterIndex = 0;

    bool bIsTyping = false;

    float TextSpeedMultiplier = 1.0f;

    void OnTypewriterTick();

    void FinishTypewriter();

};