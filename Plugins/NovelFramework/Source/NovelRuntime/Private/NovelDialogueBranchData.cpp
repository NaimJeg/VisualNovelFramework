// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelDialogueBranchData.h"
#include "NovelLog.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UNovelDialogueBranchData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    bool bHasError = Result == EDataValidationResult::Invalid;

    for (const TPair<FName, FBranchData>& Pair : BranchMap)
    {
        const FName NodeID = Pair.Key;
        const FBranchData& Branch = Pair.Value;

        if (NodeID.IsNone())
        {
            Context.AddError(FText::FromString(TEXT("Branch map contains an empty node identifier.")));
            bHasError = true;
        }

        if (Branch.bIsChoiceNode)
        {
            if (Branch.Options.IsEmpty())
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Choice branch %s has no options."), *NodeID.ToString())));
                bHasError = true;
            }

            for (int32 OptionIndex = 0; OptionIndex < Branch.Options.Num(); ++OptionIndex)
            {
                const FDialogueOption& Option = Branch.Options[OptionIndex];
                if (Option.OptionText.IsEmpty())
                {
                    Context.AddWarning(FText::FromString(FString::Printf(TEXT("Choice branch %s option %d has empty display text."), *NodeID.ToString(), OptionIndex)));
                }

                for (int32 IntentIndex = 0; IntentIndex < Option.Intents.Num(); ++IntentIndex)
                {
                    if (!Option.Intents[IntentIndex])
                    {
                        Context.AddError(FText::FromString(FString::Printf(TEXT("Choice branch %s option %d has a null intent at index %d."), *NodeID.ToString(), OptionIndex, IntentIndex)));
                        bHasError = true;
                    }
                }
            }
        }
        else
        {
            for (int32 IntentIndex = 0; IntentIndex < Branch.AutoIntents.Num(); ++IntentIndex)
            {
                if (!Branch.AutoIntents[IntentIndex])
                {
                    Context.AddError(FText::FromString(FString::Printf(TEXT("Auto branch %s has a null intent at index %d."), *NodeID.ToString(), IntentIndex)));
                    bHasError = true;
                }
            }
        }
    }

    return bHasError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif