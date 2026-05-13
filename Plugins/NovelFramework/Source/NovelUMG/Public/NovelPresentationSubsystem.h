#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "NovelPresentationSubsystem.generated.h"

class UNovelLoadingSubsystem;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNovelSaveLoadScreenRequest, bool, bIsSaveMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNovelHistoryScreenRequest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNovelSettingsScreenRequest);

UCLASS()
class NOVELUMG_API UNovelPresentationSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable, Category = "Novel|Navigation")
    FOnNovelSaveLoadScreenRequest OnSaveLoadScreenRequested;

    UPROPERTY(BlueprintAssignable, Category = "Novel|Navigation")
    FOnNovelHistoryScreenRequest OnHistoryScreenRequested;

    UPROPERTY(BlueprintAssignable, Category = "Novel|Navigation")
    FOnNovelSettingsScreenRequest OnSettingsScreenRequested;

    UFUNCTION(BlueprintCallable, Category = "Novel|Navigation")
    void RequestSaveLoadScreen(bool bIsSaveMode);

    UFUNCTION(BlueprintCallable, Category = "Novel|Navigation")
    void RequestHistoryScreen();

    UFUNCTION(BlueprintCallable, Category = "Novel|Navigation")
    void RequestSettingsScreen();

    UFUNCTION(BlueprintCallable, Category = "Novel|Presentation")
    bool CreateDefaultPresentation();

    UFUNCTION(BlueprintCallable, Category = "Novel|Presentation")
    void RemoveDefaultPresentation();

    UFUNCTION(BlueprintPure, Category = "Novel|Presentation")
    UUserWidget* GetDefaultRootWidget() const { return DefaultRootWidget; }

private:
    UPROPERTY(Transient)
    TObjectPtr<UNovelLoadingSubsystem> LoadingSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> LoadingWidget;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> DefaultRootWidget;

    UFUNCTION()
    void HandleLoadingStateChanged(bool bIsLoading);

    void ShowLoadingWidget();
    void HideLoadingWidget();
};
