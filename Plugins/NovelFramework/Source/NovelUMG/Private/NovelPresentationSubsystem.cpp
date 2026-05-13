#include "NovelPresentationSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "NovelLoadingSubsystem.h"
#include "NovelLog.h"
#include "NovelPresentationSettings.h"

void UNovelPresentationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
    LoadingSubsystem = GameInstance ? GameInstance->GetSubsystem<UNovelLoadingSubsystem>() : nullptr;
    if (LoadingSubsystem)
    {
        LoadingSubsystem->OnLoadingStateChanged.AddUniqueDynamic(this, &UNovelPresentationSubsystem::HandleLoadingStateChanged);
        HandleLoadingStateChanged(LoadingSubsystem->IsLoading());
    }

    const UNovelPresentationSettings* Settings = GetDefault<UNovelPresentationSettings>();
    if (Settings && Settings->bAutoCreateRootScreen)
    {
        CreateDefaultPresentation();
    }
}

void UNovelPresentationSubsystem::Deinitialize()
{
    if (LoadingSubsystem)
    {
        LoadingSubsystem->OnLoadingStateChanged.RemoveDynamic(this, &UNovelPresentationSubsystem::HandleLoadingStateChanged);
    }

    HideLoadingWidget();
    RemoveDefaultPresentation();
    LoadingSubsystem = nullptr;
    Super::Deinitialize();
}

void UNovelPresentationSubsystem::RequestSaveLoadScreen(bool bIsSaveMode)
{
    OnSaveLoadScreenRequested.Broadcast(bIsSaveMode);
}

void UNovelPresentationSubsystem::RequestHistoryScreen()
{
    OnHistoryScreenRequested.Broadcast();
}

void UNovelPresentationSubsystem::RequestSettingsScreen()
{
    OnSettingsScreenRequested.Broadcast();
}

bool UNovelPresentationSubsystem::CreateDefaultPresentation()
{
    if (DefaultRootWidget)
    {
        return true;
    }

    const UNovelPresentationSettings* Settings = GetDefault<UNovelPresentationSettings>();
    APlayerController* PlayerController = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
    UClass* RootClass = Settings && !Settings->RootScreenClass.IsNull() ? Settings->RootScreenClass.LoadSynchronous() : nullptr;
    if (!PlayerController || !RootClass)
    {
        UE_LOG(LogNovel, Verbose, TEXT("Default presentation was not created. RootClass=%s PlayerController=%s"),
            RootClass ? *RootClass->GetName() : TEXT("None"),
            PlayerController ? *PlayerController->GetName() : TEXT("None"));
        return false;
    }

    DefaultRootWidget = CreateWidget<UUserWidget>(PlayerController, RootClass);
    if (!DefaultRootWidget)
    {
        UE_LOG(LogNovel, Error, TEXT("Failed to create default root widget. Class=%s"), *RootClass->GetName());
        return false;
    }

    DefaultRootWidget->AddToViewport();
    return true;
}

void UNovelPresentationSubsystem::RemoveDefaultPresentation()
{
    if (DefaultRootWidget)
    {
        DefaultRootWidget->RemoveFromParent();
        DefaultRootWidget = nullptr;
    }
}

void UNovelPresentationSubsystem::HandleLoadingStateChanged(bool bIsLoading)
{
    if (bIsLoading)
    {
        ShowLoadingWidget();
    }
    else
    {
        HideLoadingWidget();
    }
}

void UNovelPresentationSubsystem::ShowLoadingWidget()
{
    const UNovelPresentationSettings* Settings = GetDefault<UNovelPresentationSettings>();
    if (!Settings || Settings->LoadingScreenWidgetClass.IsNull())
    {
        return;
    }

    if (!LoadingWidget)
    {
        UClass* WidgetClass = Settings->LoadingScreenWidgetClass.LoadSynchronous();
        APlayerController* PlayerController = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
        if (!WidgetClass || !PlayerController)
        {
            UE_LOG(LogNovel, Warning, TEXT("Could not create loading widget. WidgetClass=%s PlayerController=%s"),
                WidgetClass ? *WidgetClass->GetName() : TEXT("None"),
                PlayerController ? *PlayerController->GetName() : TEXT("None"));
            return;
        }
        LoadingWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
    }

    if (LoadingWidget && !LoadingWidget->IsInViewport())
    {
        LoadingWidget->AddToViewport(9999);
        LoadingWidget->SetKeyboardFocus();
    }
}

void UNovelPresentationSubsystem::HideLoadingWidget()
{
    if (LoadingWidget && LoadingWidget->IsInViewport())
    {
        LoadingWidget->RemoveFromParent();
    }
}
