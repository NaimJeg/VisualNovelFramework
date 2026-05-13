#include "NovelDialogueOptionWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "NovelStorySubsystem.h"

void UNovelDialogueOptionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        StorySys = GI->GetSubsystem<UNovelStorySubsystem>();
    }

    if (Btn_Choice)
    {
        Btn_Choice->OnClicked.RemoveDynamic(this, &UNovelDialogueOptionWidget::OnChoiceClicked);
        Btn_Choice->OnClicked.AddDynamic(this, &UNovelDialogueOptionWidget::OnChoiceClicked);
    }
}

void UNovelDialogueOptionWidget::NativeDestruct()
{
    if (Btn_Choice)
    {
        Btn_Choice->OnClicked.RemoveDynamic(this, &UNovelDialogueOptionWidget::OnChoiceClicked);
    }

    StorySys = nullptr;
    Super::NativeDestruct();
}

void UNovelDialogueOptionWidget::InitOption(const FText& InOptionText, int32 InChoiceIndex, FDialogueNodeHandle InSourceNode)
{
    ChoiceIndex = InChoiceIndex;
    SourceNode = InSourceNode;

    if (Txt_ChoiceText)
    {
        Txt_ChoiceText->SetText(InOptionText);
    }
}

void UNovelDialogueOptionWidget::OnChoiceClicked()
{
    if (StorySys)
    {
        StorySys->SelectChoiceForNode(ChoiceIndex, SourceNode);
    }
}