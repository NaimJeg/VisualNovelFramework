#include "NovelSaveSlotEntryWidget.h"
#include "NovelSaveLoadScreenWidget.h"
#include "NovelStorySubsystem.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UNovelSaveSlotEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Slot)
    {
        Btn_Slot->OnClicked.RemoveDynamic(this, &UNovelSaveSlotEntryWidget::OnSlotClicked); Btn_Slot->OnClicked.AddDynamic(this, &UNovelSaveSlotEntryWidget::OnSlotClicked);
    }

    if (Btn_Delete)
    {
        Btn_Delete->OnClicked.RemoveDynamic(this, &UNovelSaveSlotEntryWidget::OnDeleteClicked); Btn_Delete->OnClicked.AddDynamic(this, &UNovelSaveSlotEntryWidget::OnDeleteClicked);
    }
}

void UNovelSaveSlotEntryWidget::NativeDestruct()
{
    if (Btn_Slot) Btn_Slot->OnClicked.RemoveDynamic(this, &UNovelSaveSlotEntryWidget::OnSlotClicked);
    if (Btn_Delete) Btn_Delete->OnClicked.RemoveDynamic(this, &UNovelSaveSlotEntryWidget::OnDeleteClicked);
    ParentScreen = nullptr;
    Super::NativeDestruct();
}
void UNovelSaveSlotEntryWidget::InitSlot(const FString& InSlotName, bool bInIsNewSaveSlot, bool bInIsSaveMode, UNovelSaveLoadScreenWidget* InParentScreen)
{
    SlotName = InSlotName;
    bIsNewSaveSlot = bInIsNewSaveSlot;
    bIsSaveMode = bInIsSaveMode;
    ParentScreen = InParentScreen;

    if (Btn_Delete)
    {
        Btn_Delete->SetVisibility(bIsNewSaveSlot ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    FNovelSaveSlotMetadata Metadata;
    const UNovelStorySubsystem* StorySubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelStorySubsystem>() : nullptr;
    const bool bHasMetadata = !bIsNewSaveSlot && StorySubsystem && StorySubsystem->GetSaveSlotMetadata(SlotName, Metadata);

    if (Txt_SlotName)
    {
        const FText DisplayText = bIsNewSaveSlot
            ? FText::FromString(TEXT("+ Create New Save Slot"))
            : bHasMetadata && !Metadata.DisplayName.IsEmpty() ? Metadata.DisplayName : FText::FromString(SlotName);
        Txt_SlotName->SetText(DisplayText);
    }

    if (Img_Screenshot)
    {
        if (bHasMetadata && !Metadata.Background.IsNull())
        {
            Img_Screenshot->SetBrushFromSoftTexture(Metadata.Background);
            Img_Screenshot->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            Img_Screenshot->SetBrushFromTexture(nullptr);
            Img_Screenshot->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UNovelSaveSlotEntryWidget::OnSlotClicked()
{
    if (UNovelStorySubsystem* StorySys = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    {
        if (bIsNewSaveSlot)
        {
            StorySys->SaveGame(TEXT(""));
        }
        else
        {
            StorySys->LoadGame(SlotName);
        }
    }
}

void UNovelSaveSlotEntryWidget::OnDeleteClicked()
{
    if (bIsNewSaveSlot) return;

    if (UNovelStorySubsystem* StorySys = GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    {
        StorySys->DeleteSaveSlot(SlotName);
    }
}