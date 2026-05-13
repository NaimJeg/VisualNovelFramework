#pragma once

#include "AssetTypeActions_Base.h"

class FNovelAssetTypeActions_Chapter final : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;
    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
};

class FNovelAssetTypeActions_Story final : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;
};