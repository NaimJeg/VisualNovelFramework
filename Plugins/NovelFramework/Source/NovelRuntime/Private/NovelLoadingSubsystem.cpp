#include "NovelLoadingSubsystem.h"

#include "Engine/AssetManager.h"
#include "NovelLog.h"

void UNovelLoadingSubsystem::Deinitialize()
{
    for (TPair<int32, TSharedPtr<FStreamableHandle>>& Pair : ActiveLoadHandles)
    {
        if (Pair.Value.IsValid())
        {
            Pair.Value->CancelHandle();
        }
    }

    ActiveLoadHandles.Empty();
    ActiveCallbacks.Empty();
    UpdateLoadingState();
    Super::Deinitialize();
}
int32 UNovelLoadingSubsystem::RequestLoad(const TArray<FSoftObjectPath>& PathsToLoad, FOnLoadCompleted OnCompleteCallback)
{
    const int32 RequestId = ++NextRequestId;
    if (PathsToLoad.IsEmpty())
    {
        OnCompleteCallback.ExecuteIfBound();
        return RequestId;
    }

    ActiveCallbacks.Add(RequestId, OnCompleteCallback);
    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TSharedPtr<FStreamableHandle> LoadHandle = Streamable.RequestAsyncLoad(
        PathsToLoad,
        FStreamableDelegate::CreateUObject(this, &UNovelLoadingSubsystem::OnLoadFinished, RequestId));

    if (!LoadHandle.IsValid())
    {
        FOnLoadCompleted Callback = ActiveCallbacks.FindRef(RequestId);
        ActiveCallbacks.Remove(RequestId);
        UE_LOG(LogNovel, Warning, TEXT("Asset load request could not be created. RequestId=%d PathCount=%d"), RequestId, PathsToLoad.Num());
        Callback.ExecuteIfBound();
        return RequestId;
    }

    ActiveLoadHandles.Add(RequestId, LoadHandle);
    UpdateLoadingState();
    return RequestId;
}

void UNovelLoadingSubsystem::CancelLoad(int32 RequestId)
{
    if (TSharedPtr<FStreamableHandle>* Handle = ActiveLoadHandles.Find(RequestId))
    {
        if (Handle->IsValid())
        {
            (*Handle)->CancelHandle();
        }
        ActiveLoadHandles.Remove(RequestId);
    }

    ActiveCallbacks.Remove(RequestId);
    UpdateLoadingState();
}

void UNovelLoadingSubsystem::UpdateLoadingState()
{
    const bool bIsLoading = !ActiveLoadHandles.IsEmpty();
    if (bIsLoading != bLastBroadcastLoadingState)
    {
        bLastBroadcastLoadingState = bIsLoading;
        OnLoadingStateChanged.Broadcast(bIsLoading);
    }
}

void UNovelLoadingSubsystem::OnLoadFinished(int32 RequestId)
{
    FOnLoadCompleted Callback;
    if (FOnLoadCompleted* FoundCallback = ActiveCallbacks.Find(RequestId))
    {
        Callback = *FoundCallback;
    }
    else
    {
        UE_LOG(LogNovel, Verbose, TEXT("Ignoring stale load callback. RequestId=%d"), RequestId);
        return;
    }

    Callback.ExecuteIfBound();
    ActiveCallbacks.Remove(RequestId);
    ActiveLoadHandles.Remove(RequestId);
    UpdateLoadingState();
}
