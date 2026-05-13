#include "NovelLoadingSubsystem.h"
#include "NovelStorySettings.h"
#include "Engine/AssetManager.h"

void UNovelLoadingSubsystem::RequestLoad(const TArray<FSoftObjectPath>& PathsToLoad, FOnLoadCompleted OnCompleteCallback)
{
    if (PathsToLoad.IsEmpty())
    {
        OnCompleteCallback.ExecuteIfBound();
        return;
    }

    const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
    if (!Settings->LoadingScreenWidgetClass.IsNull())
    {
        UClass* WidgetClass = Settings->LoadingScreenWidgetClass.LoadSynchronous();
        if (WidgetClass && !LoadingWidgetInstance)
        {
            LoadingWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
        }

        if (LoadingWidgetInstance && !LoadingWidgetInstance->IsInViewport())
        {
            LoadingWidgetInstance->AddToViewport(9999);

            LoadingWidgetInstance->SetKeyboardFocus();
        }
    }

    CurrentCallback = OnCompleteCallback;

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    CurrentLoadHandle = Streamable.RequestAsyncLoad(
        PathsToLoad,
        FStreamableDelegate::CreateUObject(this, &UNovelLoadingSubsystem::OnLoadFinished)
    );
}

void UNovelLoadingSubsystem::OnLoadFinished()
{
    if (CurrentLoadHandle)
    {
        CurrentLoadHandle.Reset();
    }

    CurrentCallback.ExecuteIfBound();
    CurrentCallback.Unbind();

    if (LoadingWidgetInstance && LoadingWidgetInstance->IsInViewport())
    {
        LoadingWidgetInstance->RemoveFromParent();

        // if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        // {
        //     PC->SetInputMode(FInputModeGameAndUI());
        // }
    }
}