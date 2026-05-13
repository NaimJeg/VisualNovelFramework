#pragma once

#include "CoreMinimal.h"
#include "NovelChapterAsset.h"
#include "NovelValue.h"
#include "NovelActionContext.generated.h"

class UNovelStorySubsystem;
class USoundBase;
class UTexture2D;

UCLASS(BlueprintType)
class NOVELRUNTIME_API UNovelActionContext : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UNovelStorySubsystem* InStorySubsystem, int32 InSessionId, int32 InNodeExecutionId, int32 InActionExecutionId);
    UNovelStorySubsystem* GetStorySubsystemForLegacy() const { return StorySubsystem; }
    virtual UWorld* GetWorld() const override;

    UFUNCTION(BlueprintCallable, Category = "Novel|Action")
    void Complete();

    UFUNCTION(BlueprintCallable, Category = "Novel|Action")
    void Fail(const FText& ErrorMessage);

    UFUNCTION(BlueprintCallable, Category = "Novel|Action")
    void RequestTransition(FNovelNodeRef TargetNode, bool bClearScreen = false);

    UFUNCTION(BlueprintPure, Category = "Novel|Variables")
    bool GetVariable(FName VariableName, FNovelValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Novel|Variables")
    bool SetVariable(FName VariableName, FNovelValue Value, FText& OutError);

    UFUNCTION(BlueprintCallable, Category = "Novel|Variables")
    bool RemoveVariable(FName VariableName);
    UFUNCTION(BlueprintCallable, Category = "Novel|Presentation")
    void SetBackground(TSoftObjectPtr<UTexture2D> Background);

    UFUNCTION(BlueprintCallable, Category = "Novel|Presentation")
    void ShowCharacter(FName CharacterID, TSoftObjectPtr<UTexture2D> Sprite, float XPosition);

    UFUNCTION(BlueprintCallable, Category = "Novel|Presentation")
    void HideCharacter(FName CharacterID);

    UFUNCTION(BlueprintCallable, Category = "Novel|Audio")
    void PlayMusic(TSoftObjectPtr<USoundBase> Music, float FadeInTime = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Novel|Audio")
    void StopMusic(float FadeOutTime = 1.0f);

private:
    UPROPERTY(Transient)
    TObjectPtr<UNovelStorySubsystem> StorySubsystem;

    int32 SessionId = INDEX_NONE;
    int32 NodeExecutionId = INDEX_NONE;
    int32 ActionExecutionId = INDEX_NONE;
    bool bFinished = false;
};
