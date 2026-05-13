#include "NovelStorySubsystem.h"

#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "NovelLog.h"
#include "NovelRuntimeSettings.h"
#include "NovelSaveGame.h"

FString UNovelStorySubsystem::GetSaveIndexSlotName() const
{
    const UNovelRuntimeSettings* Settings = GetDefault<UNovelRuntimeSettings>();
    return Settings && !Settings->SaveIndexSlotName.IsEmpty() ? Settings->SaveIndexSlotName : TEXT("__NovelSaveIndex");
}

void UNovelStorySubsystem::InitializeSaveIndex()
{
    const FString IndexSlot = GetSaveIndexSlotName();
    if (UGameplayStatics::DoesSaveGameExist(IndexSlot, 0))
    {
        SaveIndex = Cast<UNovelSaveIndex>(UGameplayStatics::LoadGameFromSlot(IndexSlot, 0));
        if (SaveIndex && SaveIndex->IndexSchemaVersion > 1)
        {
            UE_LOG(LogNovel, Warning, TEXT("Save index schema %d is newer than supported; using a read-only repaired view without overwriting it."), SaveIndex->IndexSchemaVersion);
            bSaveIndexPersistenceBlocked = true;
            SaveIndex = nullptr;
        }
    }

    if (!SaveIndex)
    {
        SaveIndex = NewObject<UNovelSaveIndex>(this);
    }
    RepairSaveIndex();
    bSaveIndexInitialized = true;
}

void UNovelStorySubsystem::SortSaveIndex()
{
    if (SaveIndex)
    {
        SaveIndex->SortSlots();
    }
}

void UNovelStorySubsystem::UpsertSaveIndexEntry(const FNovelSaveSlotMetadata& Metadata)
{
    if (!SaveIndex)
    {
        SaveIndex = NewObject<UNovelSaveIndex>(this);
    }
    SaveIndex->Upsert(Metadata);
    OnSaveIndexChangedEvent.Broadcast();
}
void UNovelStorySubsystem::RepairSaveIndex()
{
    if (!SaveIndex)
    {
        return;
    }

    bool bChanged = false;
    const FString IndexSlot = GetSaveIndexSlotName();
    bChanged |= SaveIndex->Slots.RemoveAll([&IndexSlot](const FNovelSaveSlotMetadata& Entry)
    {
        return Entry.SlotId.IsEmpty() || Entry.SlotId == IndexSlot || !UGameplayStatics::DoesSaveGameExist(Entry.SlotId, 0);
    }) > 0;

    TArray<FString> SaveFiles;
    IFileManager::Get().FindFiles(SaveFiles, *(FPaths::ProjectSavedDir() / TEXT("SaveGames")), TEXT(".sav"));
    for (const FString& File : SaveFiles)
    {
        const FString SlotName = FPaths::GetBaseFilename(File);
        if (SlotName == IndexSlot || SaveIndex->Slots.ContainsByPredicate([&SlotName](const FNovelSaveSlotMetadata& Entry) { return Entry.SlotId == SlotName; }))
        {
            continue;
        }

        UNovelSaveGame* Save = Cast<UNovelSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
        FText Error;
        if (!ValidateSaveGameForLoad(Save, SlotName, Error))
        {
            UE_LOG(LogNovel, Verbose, TEXT("Save index repair ignored slot %s: %s"), *SlotName, *Error.ToString());
            continue;
        }

        FNovelSaveSlotMetadata Metadata;
        Metadata.SlotId = SlotName;
        Metadata.DisplayName = FText::FromString(SlotName);
        Metadata.Timestamp = Save->SaveTimestamp.GetTicks() > 0 ? Save->SaveTimestamp : GetSaveFileTimestamp(SlotName);
        Metadata.StoryId = Save->RuntimeSnapshot.SchemaVersion > 0 ? Save->RuntimeSnapshot.StoryId : Save->SavedStoryID;
        Metadata.Node = Save->RuntimeSnapshot.SchemaVersion > 0 ? Save->RuntimeSnapshot.CurrentNode : Save->SaveNodeRef;
        Metadata.SnapshotSchemaVersion = Save->RuntimeSnapshot.SchemaVersion;
        Metadata.Background = Save->RuntimeSnapshot.SchemaVersion > 0 ? Save->RuntimeSnapshot.Background : Save->SaveBackground;
        const TArray<FNovelHistoryEntry>& History = Save->RuntimeSnapshot.SchemaVersion > 0 ? Save->RuntimeSnapshot.History : Save->SaveHistory;
        if (!History.IsEmpty())
        {
            Metadata.LinePreview = History.Last().Text;
        }
        SaveIndex->Slots.Add(Metadata);
        bChanged = true;
    }

    SortSaveIndex();
    if (bChanged && !bSaveIndexPersistenceBlocked)
    {
        UGameplayStatics::SaveGameToSlot(SaveIndex, IndexSlot, 0);
        OnSaveIndexChangedEvent.Broadcast();
    }
}

bool UNovelStorySubsystem::GetSaveSlotMetadata(const FString& SlotName, FNovelSaveSlotMetadata& OutMetadata) const
{
    if (!SaveIndex)
    {
        return false;
    }
    const FNovelSaveSlotMetadata* Metadata = SaveIndex->Slots.FindByPredicate([&SlotName](const FNovelSaveSlotMetadata& Entry)
    {
        return Entry.SlotId == SlotName;
    });
    if (!Metadata)
    {
        return false;
    }
    OutMetadata = *Metadata;
    return true;
}

TArray<FNovelSaveSlotMetadata> UNovelStorySubsystem::GetSaveSlotMetadataList() const
{
    return SaveIndex ? SaveIndex->Slots : TArray<FNovelSaveSlotMetadata>();
}
