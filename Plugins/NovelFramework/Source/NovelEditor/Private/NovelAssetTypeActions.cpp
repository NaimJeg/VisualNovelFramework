#include "NovelAssetTypeActions.h"

#include "NovelChapterAsset.h"
#include "NovelChapterEditor.h"
#include "NovelStoryAsset.h"

#define LOCTEXT_NAMESPACE "NovelAssetTypeActions"

FText FNovelAssetTypeActions_Chapter::GetName() const
{
    return LOCTEXT("NovelChapterAssetName", "Novel Chapter");
}

FColor FNovelAssetTypeActions_Chapter::GetTypeColor() const
{
    return FColor(35, 145, 175);
}

UClass* FNovelAssetTypeActions_Chapter::GetSupportedClass() const
{
    return UNovelChapterAsset::StaticClass();
}

uint32 FNovelAssetTypeActions_Chapter::GetCategories()
{
    return EAssetTypeCategories::Misc;
}

void FNovelAssetTypeActions_Chapter::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
    for (UObject* Object : InObjects)
    {
        if (UNovelChapterAsset* Chapter = Cast<UNovelChapterAsset>(Object))
        {
            TSharedRef<FNovelChapterEditor> Editor = MakeShared<FNovelChapterEditor>();
            Editor->InitChapterEditor(Mode, EditWithinLevelEditor, Chapter);
        }
    }
}

FText FNovelAssetTypeActions_Story::GetName() const
{
    return LOCTEXT("NovelStoryAssetName", "Novel Story");
}

FColor FNovelAssetTypeActions_Story::GetTypeColor() const
{
    return FColor(65, 175, 120);
}

UClass* FNovelAssetTypeActions_Story::GetSupportedClass() const
{
    return UNovelStoryAsset::StaticClass();
}

uint32 FNovelAssetTypeActions_Story::GetCategories()
{
    return EAssetTypeCategories::Misc;
}

#undef LOCTEXT_NAMESPACE