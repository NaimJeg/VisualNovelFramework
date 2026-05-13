#include "NovelSaveIndex.h"

void UNovelSaveIndex::SortSlots()
{
    Slots.Sort([](const FNovelSaveSlotMetadata& A, const FNovelSaveSlotMetadata& B)
    {
        if (A.Timestamp == B.Timestamp)
        {
            return A.SlotId < B.SlotId;
        }
        return A.Timestamp > B.Timestamp;
    });
}

void UNovelSaveIndex::Upsert(const FNovelSaveSlotMetadata& Metadata)
{
    const int32 ExistingIndex = Slots.IndexOfByPredicate([&Metadata](const FNovelSaveSlotMetadata& Entry)
    {
        return Entry.SlotId == Metadata.SlotId;
    });
    if (ExistingIndex == INDEX_NONE)
    {
        Slots.Add(Metadata);
    }
    else
    {
        Slots[ExistingIndex] = Metadata;
    }
    SortSlots();
}
