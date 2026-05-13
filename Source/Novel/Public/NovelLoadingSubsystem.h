#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "NovelLoadingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadingStateChanged, bool, bIsLoading);

DECLARE_DELEGATE(FOnLoadCompleted);

UCLASS()
class NOVEL_API UNovelLoadingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnLoadingStateChanged OnLoadingStateChanged;

    void RequestLoad(const TArray<FSoftObjectPath>& PathsToLoad, FOnLoadCompleted OnCompleteCallback);

private:
    TSharedPtr<FStreamableHandle> CurrentLoadHandle;

    FOnLoadCompleted CurrentCallback;

    UPROPERTY()
    UUserWidget* LoadingWidgetInstance;

    void OnLoadFinished();
};