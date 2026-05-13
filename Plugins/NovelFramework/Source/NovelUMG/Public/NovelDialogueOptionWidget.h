#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDialogueNodeHandle.h"
#include "NovelDialogueOptionWidget.generated.h"

class UButton;
class UTextBlock;
class UNovelStorySubsystem;

UCLASS()
class NOVELUMG_API UNovelDialogueOptionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitOption(const FText& InOptionText, int32 InChoiceIndex, FDialogueNodeHandle InSourceNode);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Choice;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ChoiceText;

    UFUNCTION()
    void OnChoiceClicked();

    UPROPERTY()
    UNovelStorySubsystem* StorySys;

private:
    int32 ChoiceIndex = INDEX_NONE;
    FDialogueNodeHandle SourceNode;
};