#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NovelLoadingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadingStateChanged, bool, bIsLoading);
DECLARE_DELEGATE(FOnLoadCompleted);

UCLASS()
class NOVELRUNTIME_API UNovelLoadingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable, Category = "Loading")
    FOnLoadingStateChanged OnLoadingStateChanged;

    int32 RequestLoad(const TArray<FSoftObjectPath>& PathsToLoad, FOnLoadCompleted OnCompleteCallback);
    void CancelLoad(int32 RequestId);

    UFUNCTION(BlueprintPure, Category = "Loading")
    bool IsLoading() const { return !ActiveLoadHandles.IsEmpty(); }

private:
    TMap<int32, TSharedPtr<FStreamableHandle>> ActiveLoadHandles;
    TMap<int32, FOnLoadCompleted> ActiveCallbacks;
    int32 NextRequestId = 0;
    bool bLastBroadcastLoadingState = false;

    void UpdateLoadingState();
    void OnLoadFinished(int32 RequestId);
};
