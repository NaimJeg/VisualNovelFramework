#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NovelSaveGame.h"
#include "NovelSaveIndex.h"
#include "NovelStorySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNovelSnapshotIndexTest,
    "Novel.Persistence.SnapshotAndIndex",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNovelSnapshotIndexTest::RunTest(const FString& Parameters)
{
    UNovelSaveIndex* Index = NewObject<UNovelSaveIndex>();
    FNovelSaveSlotMetadata Older;
    Older.SlotId = TEXT("SlotB");
    Older.Timestamp = FDateTime(2025, 1, 1);
    FNovelSaveSlotMetadata NewerB;
    NewerB.SlotId = TEXT("SlotC");
    NewerB.Timestamp = FDateTime(2026, 1, 1);
    FNovelSaveSlotMetadata NewerA;
    NewerA.SlotId = TEXT("SlotA");
    NewerA.Timestamp = FDateTime(2026, 1, 1);
    Index->Upsert(Older);
    Index->Upsert(NewerB);
    Index->Upsert(NewerA);

    TestEqual(TEXT("Newest timestamp first with slot-ID tie break"), Index->Slots[0].SlotId, FString(TEXT("SlotA")));
    TestEqual(TEXT("Second equal timestamp ordered by slot ID"), Index->Slots[1].SlotId, FString(TEXT("SlotC")));
    TestEqual(TEXT("Oldest last"), Index->Slots[2].SlotId, FString(TEXT("SlotB")));

    NewerA.DisplayName = FText::FromString(TEXT("Updated"));
    Index->Upsert(NewerA);
    TestEqual(TEXT("Upsert does not duplicate slot"), Index->Slots.Num(), 3);
    TestEqual(TEXT("Upsert replaces metadata"), Index->Slots[0].DisplayName.ToString(), FString(TEXT("Updated")));

    UNovelSaveGame* Save = NewObject<UNovelSaveGame>();
    Save->SaveSchemaVersion = 4;
    Save->RuntimeSnapshot.SchemaVersion = 1;
    Save->RuntimeSnapshot.CurrentNode.ChapterId = FPrimaryAssetId(TEXT("NovelChapter:ChapterA"));
    Save->RuntimeSnapshot.CurrentNode.NodeId = TEXT("NodeA");
    Save->RuntimeSnapshot.Variables.Add(TEXT("Score"), FNovelValue::MakeInteger(42));
    FNovelHistoryEntry& History = Save->RuntimeSnapshot.History.AddDefaulted_GetRef();
    History.Text = FText::FromString(TEXT("Snapshot line"));
    History.SequenceIndex = 7;

    TArray<uint8> Data;
    TestTrue(TEXT("Serialize save to memory"), UGameplayStatics::SaveGameToMemory(Save, Data));
    UNovelSaveGame* Loaded = Cast<UNovelSaveGame>(UGameplayStatics::LoadGameFromMemory(Data));
    TestNotNull(TEXT("Deserialize save from memory"), Loaded);
    if (Loaded)
    {
        TestEqual(TEXT("Snapshot node survives round trip"), Loaded->RuntimeSnapshot.CurrentNode.NodeId, FName(TEXT("NodeA")));
        const FNovelValue* Score = Loaded->RuntimeSnapshot.Variables.Find(TEXT("Score"));
        TestTrue(TEXT("Snapshot variable survives round trip"), Score && Score->IntegerValue == 42);
        TestEqual(TEXT("Snapshot history survives round trip"), Loaded->RuntimeSnapshot.History.Num(), 1);
    }

    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UNovelStorySubsystem* Story = NewObject<UNovelStorySubsystem>(GameInstance);
    FNovelRuntimeSnapshot NewerSnapshot;
    NewerSnapshot.SchemaVersion = 99;
    FText Error;
    TestFalse(TEXT("Reject newer snapshot schema"), Story->RestoreSnapshot(NewerSnapshot, Error));
    return true;
}

#endif
