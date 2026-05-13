#include "NovelHistoryScreenWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "NovelHistoryListItem.h"
#include "NovelLog.h"
#include "NovelStorySubsystem.h"

namespace
{
bool bLoggedLegacyHistoryFallback = false;
}

void UNovelHistoryScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);
    StorySys = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNovelStorySubsystem>() : nullptr;
    if (StorySys)
    {
        StorySys->OnHistoryChangedEvent.AddUniqueDynamic(this, &UNovelHistoryScreenWidget::RefreshHistory);
    }
    if (Btn_Close)
    {
        Btn_Close->OnClicked.AddUniqueDynamic(this, &UNovelHistoryScreenWidget::HandleCloseClicked);
    }

    RefreshHistory();
}

void UNovelHistoryScreenWidget::NativeDestruct()
{
    if (StorySys)
    {
        StorySys->OnHistoryChangedEvent.RemoveDynamic(this, &UNovelHistoryScreenWidget::RefreshHistory);
    }
    if (Btn_Close)
    {
        Btn_Close->OnClicked.RemoveDynamic(this, &UNovelHistoryScreenWidget::HandleCloseClicked);
    }
    if (HistoryList)
    {
        HistoryList->ClearListItems();
    }

    ListItems.Reset();
    StorySys = nullptr;
    Super::NativeDestruct();
}

void UNovelHistoryScreenWidget::RefreshHistory()
{
    if (!StorySys)
    {
        return;
    }

    const TArray<FNovelHistoryEntry>& Entries = StorySys->GetHistoryView();
    if (HistoryList)
    {
        HistoryList->ClearListItems();
        ListItems.Reset(Entries.Num());

        for (const FNovelHistoryEntry& Entry : Entries)
        {
            UNovelHistoryListItem* Item = NewObject<UNovelHistoryListItem>(this);
            Item->Initialize(Entry);
            ListItems.Add(Item);
            HistoryList->AddItem(Item);
        }

        if (!ListItems.IsEmpty())
        {
            HistoryList->ScrollIndexIntoView(ListItems.Num() - 1);
        }
        return;
    }

    if (!Txt_History)
    {
        return;
    }

    if (!bLoggedLegacyHistoryFallback)
    {
        bLoggedLegacyHistoryFallback = true;
        UE_LOG(LogNovel, Warning, TEXT("History screen is using deprecated Txt_History fallback. Bind HistoryList to enable virtualized history."));
    }

    FString RenderedHistory;
    for (const FNovelHistoryEntry& Entry : Entries)
    {
        if (!RenderedHistory.IsEmpty())
        {
            RenderedHistory += TEXT("\n\n");
        }

        const FString Speaker = Entry.Speaker.ToString();
        if (!Speaker.IsEmpty())
        {
            RenderedHistory += Speaker;
            RenderedHistory += TEXT(": ");
        }

        RenderedHistory += Entry.Text.ToString();
    }

    Txt_History->SetText(FText::FromString(RenderedHistory));
}

void UNovelHistoryScreenWidget::HandleCloseClicked()
{
    OnCloseRequested.Broadcast();
}

FReply UNovelHistoryScreenWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    return FReply::Handled();
}
