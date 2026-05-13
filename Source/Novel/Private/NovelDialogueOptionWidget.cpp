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
        Btn_Choice->OnClicked.AddDynamic(this, &UNovelDialogueOptionWidget::OnChoiceClicked);
    }
}

void UNovelDialogueOptionWidget::InitOption(const FText& InOptionText, const TArray<UNovelIntentBase*>& InIntents)
{
    if (Txt_ChoiceText)
    {
        Txt_ChoiceText->SetText(InOptionText);
    }
    
    // Copy the intents array into this widget
    OptionIntents = InIntents;
}

void UNovelDialogueOptionWidget::OnChoiceClicked()
{
    if (StorySys)
    {
        StorySys->OnDialogueOptionsHideEvent.Broadcast();
        
        StorySys->ProcessIntents(OptionIntents);
    }
}