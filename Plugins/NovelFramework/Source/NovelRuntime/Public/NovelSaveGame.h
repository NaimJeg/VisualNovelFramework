#pragma once

#include "CoreMinimal.h"
#include "FDialogueNodeHandle.h"
#include "GameFramework/SaveGame.h"
#include "NovelStoryTypes.h"
#include "NovelValue.h"
#include "NovelRuntimeSnapshot.h"
#include "NovelChapterAsset.h"
#include "NovelSaveGame.generated.h"

class USoundBase;
class UTexture2D;
class UNovelStoryAsset;
class UNovelChapterAsset;

UCLASS()
class NOVELRUNTIME_API UNovelSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 SaveSchemaVersion = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FNovelRuntimeSnapshot RuntimeSnapshot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FDateTime SaveTimestamp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString SaveSlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FDialogueNodeHandle SaveNode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FPrimaryAssetId SavedStoryID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FNovelNodeRef SaveNodeRef;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TSoftObjectPtr<UNovelStoryAsset> SavedStoryAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TSoftObjectPtr<UNovelChapterAsset> SavedChapterAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 SavedChapterID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TSoftObjectPtr<UTexture2D> SaveBackground;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TArray<FNovelCharacterPresentationState> SaveVisibleCharacters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TSoftObjectPtr<USoundBase> SaveBGM;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TArray<FNovelHistoryEntry> SaveHistory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TMap<FName, FNovelValue> SaveVariables;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    bool bSavedAtChoice = false;

};