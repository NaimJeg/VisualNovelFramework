#pragma once

#include "CoreMinimal.h"
#include "NovelStoryTypes.generated.h"

class USoundBase;
class UTexture2D;

UENUM(BlueprintType)
enum class ENovelRuntimeState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    LoadingChapter UMETA(DisplayName = "Loading Chapter"),
    PresentingDialogue UMETA(DisplayName = "Presenting Dialogue"),
    ExecutingIntents UMETA(DisplayName = "Executing Intents"),
    AwaitingAdvance UMETA(DisplayName = "Awaiting Advance"),
    AwaitingChoice UMETA(DisplayName = "Awaiting Choice"),
    Saving UMETA(DisplayName = "Saving"),
    LoadingSave UMETA(DisplayName = "Loading Save"),
    Error UMETA(DisplayName = "Error")
};

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelHistoryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FText Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    int32 ChapterID = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FPrimaryAssetId StoryAssetId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FPrimaryAssetId ChapterAssetId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FName NodeID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    int32 SequenceIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FDateTime Timestamp;
};

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelCharacterPresentationState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    FName CharacterID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
    TSoftObjectPtr<UTexture2D> Sprite;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float XPosition = 0.5f;
};

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelSaveMetadata
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString SlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 SchemaVersion = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FDateTime Timestamp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 ChapterID = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FName NodeID = NAME_None;
};