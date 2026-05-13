#include "NovelAssetFactories.h"

#include "NovelChapterAsset.h"
#include "NovelStoryAsset.h"

UNovelChapterAssetFactory::UNovelChapterAssetFactory()
{
    SupportedClass = UNovelChapterAsset::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UNovelChapterAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UNovelChapterAsset>(InParent, Class, Name, Flags | RF_Transactional);
}

UNovelStoryAssetFactory::UNovelStoryAssetFactory()
{
    SupportedClass = UNovelStoryAsset::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UNovelStoryAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UNovelStoryAsset>(InParent, Class, Name, Flags | RF_Transactional);
}