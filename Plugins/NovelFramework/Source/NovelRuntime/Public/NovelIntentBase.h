#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NovelIntentBase.generated.h"

class UNovelStorySubsystem;
class UNovelActionContext;

DECLARE_DYNAMIC_DELEGATE(FNovelIntentCompleted);

UCLASS()
class NOVELRUNTIME_API UNovelIntentCompletionProxy : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UNovelStorySubsystem* InStorySubsystem, int32 InSessionId, int32 InNodeExecutionId, int32 InIntentExecutionId);

    UFUNCTION()
    void Complete();

private:
    UPROPERTY()
    UNovelStorySubsystem* StorySubsystem = nullptr;

    int32 SessionId = INDEX_NONE;
    int32 NodeExecutionId = INDEX_NONE;
    int32 IntentExecutionId = INDEX_NONE;
    bool bCompleted = false;
};

UCLASS(Abstract, EditInlineNew, Blueprintable)
class NOVELRUNTIME_API UNovelIntentBase : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intent Settings")
    bool bWaitUntilFinished = true;

    UFUNCTION(BlueprintNativeEvent, Category = "Novel|Action")
    void ExecuteAction(UNovelActionContext* Context);
    virtual void ExecuteAction_Implementation(UNovelActionContext* Context);

    UFUNCTION(BlueprintNativeEvent, Category = "Novel", meta = (DeprecatedFunction, DeprecationMessage = "Override ExecuteAction for context-based execution."))
    void ExecuteIntent(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion);
    virtual void ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion);
};