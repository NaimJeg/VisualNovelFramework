#include "NovelSaveSlotEntryWidget.h"
#include "NovelSaveLoadScreenWidget.h"
#include "NovelStorySubsystem.h"
#include "NovelSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UNovelSaveSlotEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Slot)
    {
        Btn_Slot->OnClicked.AddDynamic(this, &UNovelSaveSlotEntryWidget::OnSlotClicked);
    }

    if (Btn_Delete)
    {
        Btn_Delete->OnClicked.AddDynamic(this, &UNovelSaveSlotEntryWidget::OnDeleteClicked);
    }
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

    if (Txt_SlotName)
    {
        FText DisplayText = bIsNewSaveSlot ? FText::FromString(TEXT("+ Create New Save Slot")) : FText::FromString(SlotName);
        Txt_SlotName->SetText(DisplayText);
    }

    if (Img_Screenshot)
    {
        if (bIsNewSaveSlot)
        {
            Img_Screenshot->SetBrushFromTexture(nullptr);
        }
        else
        {
            if (UNovelSaveGame* SaveData = Cast<UNovelSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
            {
                if (!SaveData->SaveBackground.IsNull())
                {
                    Img_Screenshot->SetBrushFromSoftTexture(SaveData->SaveBackground);
                    Img_Screenshot->SetVisibility(ESlateVisibility::Visible);
                }
                else
                {
                    Img_Screenshot->SetBrushFromTexture(nullptr);
                }
            }
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

            if (ParentScreen)
            {
                ParentScreen->RefreshSaveSlots();
            }
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

        if (ParentScreen)
        {
            ParentScreen->RefreshSaveSlots();
        }
    }
}