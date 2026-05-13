#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelIntentBase.h" 
#include "NovelDialogueOptionWidget.generated.h"

class UButton;
class UTextBlock;
class UNovelStorySubsystem;

UCLASS()
class NOVEL_API UNovelDialogueOptionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitOption(const FText& InOptionText, const TArray<UNovelIntentBase*>& InIntents);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Choice;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ChoiceText;

    UPROPERTY()
    TArray<UNovelIntentBase*> OptionIntents;

    UFUNCTION()
    void OnChoiceClicked();

    UPROPERTY()
    UNovelStorySubsystem* StorySys;
};