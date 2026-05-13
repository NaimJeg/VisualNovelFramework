#pragma once

#include "CoreMinimal.h"
#include "FDialogueNodeHandle.h"
#include "GameFramework/SaveGame.h"
#include "NovelSaveGame.generated.h"

UCLASS()
class NOVEL_API UNovelSaveGame : public USaveGame
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDialogueNodeHandle SaveNode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SavedChapterID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> SaveBackground;
};