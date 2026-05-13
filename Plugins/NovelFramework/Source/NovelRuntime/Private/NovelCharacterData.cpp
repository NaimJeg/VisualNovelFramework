// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelCharacterData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UNovelCharacterData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    bool bHasError = Result == EDataValidationResult::Invalid;

    if (CharacterName.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("CharacterName is empty.")));
    }

    if (Expressions.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("Character has no expression sprites.")));
    }

    for (const TPair<FName, TSoftObjectPtr<UTexture2D>>& Pair : Expressions)
    {
        if (Pair.Key.IsNone())
        {
            Context.AddError(FText::FromString(TEXT("Character expression map contains an empty expression key.")));
            bHasError = true;
        }

        if (Pair.Value.IsNull())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Expression %s has no sprite."), *Pair.Key.ToString())));
            bHasError = true;
        }
    }

    return bHasError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif