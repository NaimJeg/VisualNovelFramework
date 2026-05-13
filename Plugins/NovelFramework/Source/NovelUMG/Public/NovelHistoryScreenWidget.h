#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelHistoryScreenWidget.generated.h"

class UButton;
class UListView;
class UTextBlock;
class UNovelHistoryListItem;
class UNovelStorySubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNovelHistoryCloseRequested);

UCLASS()
class NOVELUMG_API UNovelHistoryScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "History")
    void RefreshHistory();

    UPROPERTY(BlueprintAssignable, Category = "History")
    FOnNovelHistoryCloseRequested OnCloseRequested;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UListView> HistoryList;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Close;

    UPROPERTY(meta = (BindWidgetOptional, DeprecatedProperty, DeprecationMessage = "Use HistoryList with UNovelHistoryEntryWidget entries."))
    TObjectPtr<UTextBlock> Txt_History;

    UFUNCTION()
    void HandleCloseClicked();

private:
    UPROPERTY(Transient)
    TObjectPtr<UNovelStorySubsystem> StorySys;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UNovelHistoryListItem>> ListItems;
};
