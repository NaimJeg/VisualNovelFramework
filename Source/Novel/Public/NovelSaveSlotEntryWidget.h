// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NovelSaveSlotEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UNovelSaveLoadScreenWidget;

UCLASS()
class NOVEL_API UNovelSaveSlotEntryWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Slot;

    UPROPERTY(meta = (BindWidget))
    UButton* Btn_Delete;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_SlotName;

    UPROPERTY(meta = (BindWidget))
    UImage* Img_Screenshot;

public:

    void InitSlot(const FString& InSlotName, bool bInIsNewSaveSlot, bool bInIsSaveMode, UNovelSaveLoadScreenWidget* InParentScreen);
    

private:
    FString SlotName;
    bool bIsNewSaveSlot = false;

    bool bIsSaveMode = false;

    UPROPERTY()
    UNovelSaveLoadScreenWidget* ParentScreen;

    UFUNCTION()
    void OnSlotClicked();

    UFUNCTION()
    void OnDeleteClicked();
};