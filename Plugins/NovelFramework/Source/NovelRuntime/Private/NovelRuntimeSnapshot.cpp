#include "NovelRuntimeSnapshot.h"

#include "NovelStorySubsystem.h"
#include "NovelStoryAsset.h"

#include "Kismet/GameplayStatics.h"
#include "NovelSaveGame.h"

FNovelRuntimeSnapshot UNovelStorySubsystem::CreateSnapshot() const
{
    FNovelRuntimeSnapshot Snapshot;
    Snapshot.StoryId = ActiveStoryAsset ? ActiveStoryAsset->GetPrimaryAssetId() : FPrimaryAssetId();
    Snapshot.StoryAsset = ActiveStoryAsset;
    Snapshot.ChapterAsset = ActiveChapterAsset;
    Snapshot.CurrentNode = CurrentNodeRef;
    Snapshot.LegacyChapterId = CurrentChapterID;
    Snapshot.LegacyNode = CurrentNode;
    Snapshot.RestoredState = RuntimeState == ENovelRuntimeState::AwaitingChoice ? ENovelRuntimeState::AwaitingChoice : ENovelRuntimeState::AwaitingAdvance;
    Snapshot.Background = CurrentBackground;
    Snapshot.Music = CurrentBGM;
    Snapshot.History = DialogueHistory;
    Snapshot.Variables = Variables;
    Snapshot.bAtChoice = RuntimeState == ENovelRuntimeState::AwaitingChoice;
    Snapshot.NextHistorySequenceIndex = NextHistorySequenceIndex;

    for (const TPair<FName, FNovelCharacterPresentationState>& Pair : VisibleCharacters)
    {
        Snapshot.VisibleCharacters.Add(Pair.Value);
    }
    Snapshot.VisibleCharacters.Sort([](const FNovelCharacterPresentationState& A, const FNovelCharacterPresentationState& B)
    {
        return A.CharacterID.LexicalLess(B.CharacterID);
    });
    return Snapshot;
}

bool UNovelStorySubsystem::RestoreSnapshot(const FNovelRuntimeSnapshot& Snapshot, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (Snapshot.SchemaVersion <= 0 || Snapshot.SchemaVersion > 1)
    {
        OutError = FText::FromString(FString::Printf(TEXT("Unsupported snapshot schema %d."), Snapshot.SchemaVersion));
        return false;
    }
    if (!Snapshot.HasUnifiedIdentity() && (Snapshot.LegacyChapterId == INDEX_NONE || Snapshot.LegacyNode.RowName.IsNone()))
    {
        OutError = FText::FromString(TEXT("Snapshot has no valid unified or legacy node identity."));
        return false;
    }
    if (!CanStartBlockingOperation(TEXT("RestoreSnapshot")))
    {
        OutError = FText::FromString(TEXT("Runtime is busy and cannot restore a snapshot."));
        return false;
    }

    UNovelSaveGame* SnapshotSave = NewObject<UNovelSaveGame>(this);
    SnapshotSave->SaveSchemaVersion = CurrentSaveSchemaVersion;
    SnapshotSave->SaveSlotName = TEXT("SnapshotRestore");
    SnapshotSave->RuntimeSnapshot = Snapshot;
    SnapshotSave->SavedStoryID = Snapshot.StoryId;
    SnapshotSave->SavedStoryAsset = Snapshot.StoryAsset;
    SnapshotSave->SavedChapterAsset = Snapshot.ChapterAsset;
    SnapshotSave->SaveNodeRef = Snapshot.CurrentNode;
    SnapshotSave->SavedChapterID = Snapshot.LegacyChapterId;
    SnapshotSave->SaveNode = Snapshot.LegacyNode;
    SnapshotSave->SaveBackground = Snapshot.Background;
    SnapshotSave->SaveVisibleCharacters = Snapshot.VisibleCharacters;
    SnapshotSave->SaveBGM = Snapshot.Music;
    SnapshotSave->SaveHistory = Snapshot.History;
    SnapshotSave->SaveVariables = Snapshot.Variables;
    SnapshotSave->bSavedAtChoice = Snapshot.bAtChoice;

    StateBeforeSaveOrLoad = RuntimeState;
    SetRuntimeState(ENovelRuntimeState::LoadingSave, TEXT("RestoreSnapshot"));
    OnStoryLoadingStateChangedEvent.Broadcast(true, FText::FromString(TEXT("Restoring snapshot")));
    ++StorySessionId;
    ++ChapterLoadSerial;
    PendingLoadedSave = SnapshotSave;

    if (Snapshot.HasUnifiedIdentity())
    {
        if (BeginUnifiedSaveAssetsLoad(SnapshotSave, StateBeforeSaveOrLoad))
        {
            return true;
        }
    }
    else if (BeginChapterLoad(Snapshot.LegacyChapterId, Snapshot.LegacyNode, true, StateBeforeSaveOrLoad))
    {
        return true;
    }

    PendingLoadedSave = nullptr;
    OnStoryLoadingStateChangedEvent.Broadcast(false, FText::GetEmpty());
    SetRuntimeState(StateBeforeSaveOrLoad, TEXT("RestoreSnapshot request failed"));
    OutError = FText::FromString(TEXT("Snapshot assets could not be requested."));
    return false;
}
