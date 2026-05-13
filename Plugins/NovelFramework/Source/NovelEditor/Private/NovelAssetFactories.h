#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "NovelAssetFactories.generated.h"

UCLASS()
class NOVELEDITOR_API UNovelChapterAssetFactory : public UFactory
{
    GENERATED_BODY()

public:
    UNovelChapterAssetFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class NOVELEDITOR_API UNovelStoryAssetFactory : public UFactory
{
    GENERATED_BODY()

public:
    UNovelStoryAssetFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};