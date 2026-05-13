#include "NovelHistoryEntryWidget.h"

#include "Components/TextBlock.h"
#include "NovelHistoryListItem.h"

void UNovelHistoryEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

    const UNovelHistoryListItem* HistoryItem = Cast<UNovelHistoryListItem>(ListItemObject);
    if (!HistoryItem)
    {
        if (SpeakerText)
        {
            SpeakerText->SetText(FText::GetEmpty());
        }
        if (DialogueText)
        {
            DialogueText->SetText(FText::GetEmpty());
        }
        if (MetadataText)
        {
            MetadataText->SetText(FText::GetEmpty());
        }
        return;
    }

    const FNovelHistoryEntry& Entry = HistoryItem->Entry;
    if (SpeakerText)
    {
        SpeakerText->SetText(Entry.Speaker);
    }
    if (DialogueText)
    {
        DialogueText->SetText(Entry.Text);
    }

    if (MetadataText)
    {
        TArray<FString> MetadataParts;
        if (Entry.ChapterID != INDEX_NONE)
        {
            MetadataParts.Add(FString::Printf(TEXT("Chapter %d"), Entry.ChapterID));
        }
        if (!Entry.NodeID.IsNone())
        {
            MetadataParts.Add(Entry.NodeID.ToString());
        }
        if (Entry.Timestamp.GetTicks() > 0)
        {
            MetadataParts.Add(Entry.Timestamp.ToString(TEXT("%Y-%m-%d %H:%M:%S")));
        }

        MetadataText->SetText(FText::FromString(FString::Join(MetadataParts, TEXT(" | "))));
    }
}
