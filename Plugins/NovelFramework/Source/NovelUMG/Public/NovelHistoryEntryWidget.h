#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "NovelHistoryEntryWidget.generated.h"

class UTextBlock;

UCLASS(Abstract, Blueprintable)
class NOVELUMG_API UNovelHistoryEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SpeakerText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DialogueText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MetadataText;
};
