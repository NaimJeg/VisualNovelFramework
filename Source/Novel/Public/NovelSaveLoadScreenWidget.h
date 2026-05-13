// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelStorySettings.h"
#include "NovelStorySubsystem.h"
#include "NovelSaveSlotEntryWidget.h"
#include "NovelSaveLoadScreenWidget.generated.h"

/**
 * 
 */
UCLASS()
class NOVEL_API UNovelSaveLoadScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:

    UFUNCTION()
    void RefreshSaveSlots();
    void SetModeAndRefresh(bool bInIsSaveMode);
	
protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UPanelWidget* SlotsContainer;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UNovelSaveSlotEntryWidget> SlotEntryClass;

    UPROPERTY(BlueprintReadOnly, Category = "SaveLoad UI")
    bool bIsSaveMode = false;

private:

    UPROPERTY()
    UNovelStorySubsystem* StorySys;

};
