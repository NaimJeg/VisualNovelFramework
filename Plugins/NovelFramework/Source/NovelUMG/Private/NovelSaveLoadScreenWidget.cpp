#include "NovelSaveLoadScreenWidget.h"
#include "NovelStorySubsystem.h"
#include "Components\PanelWidget.h"
#include "NovelSaveSlotEntryWidget.h"

void UNovelSaveLoadScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    StorySys = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelStorySubsystem>() : nullptr;
    if (StorySys)
    {
        StorySys->OnSaveIndexChangedEvent.AddUniqueDynamic(this, &UNovelSaveLoadScreenWidget::RefreshSaveSlots);
    }

    RefreshSaveSlots();
}

void UNovelSaveLoadScreenWidget::NativeDestruct()
{
    if (StorySys)
    {
        StorySys->OnSaveIndexChangedEvent.RemoveDynamic(this, &UNovelSaveLoadScreenWidget::RefreshSaveSlots);
    }
    StorySys = nullptr;
    Super::NativeDestruct();
}
void UNovelSaveLoadScreenWidget::SetModeAndRefresh(bool bInIsSaveMode)
{
    bIsSaveMode = bInIsSaveMode;
    RefreshSaveSlots();
}

void UNovelSaveLoadScreenWidget::RefreshSaveSlots()
{
    if (!SlotsContainer || !SlotEntryClass) return;
    if (!StorySys) return;

    SlotsContainer->ClearChildren();

    if (bIsSaveMode)
    {
        if (UNovelSaveSlotEntryWidget* NewSaveSlot = CreateWidget<UNovelSaveSlotEntryWidget>(this, SlotEntryClass))
        {
            NewSaveSlot->InitSlot(TEXT(""), true, bIsSaveMode, this);
            SlotsContainer->AddChild(NewSaveSlot);
        }
    }

    TArray<FString> ExistingSaves = StorySys->GetAllSaveSlotNames();

    for (const FString& SaveName : ExistingSaves)
    {
        if (UNovelSaveSlotEntryWidget* ExistingSlot = CreateWidget<UNovelSaveSlotEntryWidget>(this, SlotEntryClass))
        {
            ExistingSlot->InitSlot(SaveName, false, bIsSaveMode, this);
            SlotsContainer->AddChild(ExistingSlot);
        }
    }
}