#pragma once

#include "CoreMinimal.h"
#include "FDialogueNodeHandle.h"
#include "NovelChapterAsset.h"
#include "NovelStoryTypes.h"
#include "NovelValue.h"
#include "NovelRuntimeSnapshot.generated.h"

class UNovelChapterAsset;
class UNovelStoryAsset;
class USoundBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelRuntimeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    int32 SchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    FPrimaryAssetId StoryId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TSoftObjectPtr<UNovelStoryAsset> StoryAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TSoftObjectPtr<UNovelChapterAsset> ChapterAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    FNovelNodeRef CurrentNode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot|Legacy")
    int32 LegacyChapterId = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot|Legacy")
    FDialogueNodeHandle LegacyNode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    ENovelRuntimeState RestoredState = ENovelRuntimeState::AwaitingAdvance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TSoftObjectPtr<UTexture2D> Background;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TArray<FNovelCharacterPresentationState> VisibleCharacters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TSoftObjectPtr<USoundBase> Music;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TArray<FNovelHistoryEntry> History;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TMap<FName, FNovelValue> Variables;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    bool bAtChoice = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    int32 NextHistorySequenceIndex = 0;

    bool HasUnifiedIdentity() const { return CurrentNode.IsValid() && !ChapterAsset.IsNull(); }
};
