#include "NovelRuntimeSettings.h"
#include "FDialogueNode.h"
#include "NovelIntent_Jump.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UNovelRuntimeSettings::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    bool bHasError = Result == EDataValidationResult::Invalid;

    if (EntryStory.IsNull() && !ChapterMap.Contains(0))
    {
        Context.AddError(FText::FromString(TEXT("Entry chapter 0 is required because StartNewGame starts chapter 0.")));
        bHasError = true;
    }

    for (const TPair<int32, FChapterData>& ChapterPair : ChapterMap)
    {
        const int32 ChapterID = ChapterPair.Key;
        const FChapterData& ChapterData = ChapterPair.Value;

        if (!ChapterData.ChapterAsset.IsNull())
        {
            if (!ChapterData.ChapterAsset.LoadSynchronous())
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Chapter %d unified asset failed to load."), ChapterID)));
                bHasError = true;
            }
            continue;
        }

        if (ChapterData.DialogueTable.IsNull())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Chapter %d has no dialogue table."), ChapterID)));
            bHasError = true;
            continue;
        }

        UDataTable* DialogueTable = ChapterData.DialogueTable.LoadSynchronous();
        if (!DialogueTable)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Chapter %d dialogue table failed to load."), ChapterID)));
            bHasError = true;
            continue;
        }

        const TArray<FName> RowNames = DialogueTable->GetRowNames();
        if (RowNames.IsEmpty())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Chapter %d dialogue table has no rows."), ChapterID)));
            bHasError = true;
        }

        for (const FName RowName : RowNames)
        {
            const FDialogueRow* Row = DialogueTable->FindRow<FDialogueRow>(RowName, TEXT("NovelStorySettingsValidation"), false);
            if (!Row)
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Chapter %d row %s cannot be read as FDialogueRow."), ChapterID, *RowName.ToString())));
                bHasError = true;
                continue;
            }

            if (Row->Text.IsEmpty() && Row->Speaker.IsEmpty())
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT("Chapter %d row %s has empty speaker and dialogue text."), ChapterID, *RowName.ToString())));
            }
        }

        UNovelDialogueBranchData* LoadedBranchData = ChapterData.BranchData.IsNull() ? nullptr : ChapterData.BranchData.LoadSynchronous();
        if (!LoadedBranchData)
        {
            continue;
        }

        auto ValidateIntent = [&](const UNovelIntentBase* Intent, const FString& SourceDescription)
        {
            if (!Intent)
            {
                Context.AddError(FText::FromString(SourceDescription + TEXT(" has a null intent.")));
                bHasError = true;
                return;
            }

            if (const UNovelIntent_Jump* JumpIntent = Cast<UNovelIntent_Jump>(Intent))
            {
                const FDialogueNodeHandle& Target = JumpIntent->TargetNode;
                UDataTable* TargetTable = Target.Table.IsNull() ? DialogueTable : Target.Table.LoadSynchronous();
                if (!TargetTable || Target.RowName.IsNone())
                {
                    Context.AddError(FText::FromString(SourceDescription + TEXT(" has an invalid jump target.")));
                    bHasError = true;
                    return;
                }

                if (!TargetTable->FindRow<FDialogueRow>(Target.RowName, TEXT("NovelStorySettingsValidation"), false))
                {
                    Context.AddError(FText::FromString(SourceDescription + FString::Printf(TEXT(" jumps to missing row %s."), *Target.RowName.ToString())));
                    bHasError = true;
                }
            }
        };

        for (const TPair<FName, FBranchData>& BranchPair : LoadedBranchData->BranchMap)
        {
            const FName BranchNode = BranchPair.Key;
            const FBranchData& Branch = BranchPair.Value;

            if (!DialogueTable->FindRow<FDialogueRow>(BranchNode, TEXT("NovelStorySettingsValidation"), false))
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Chapter %d branch %s does not match a dialogue row."), ChapterID, *BranchNode.ToString())));
                bHasError = true;
            }

            if (Branch.bIsChoiceNode)
            {
                for (int32 OptionIndex = 0; OptionIndex < Branch.Options.Num(); ++OptionIndex)
                {
                    const FDialogueOption& Option = Branch.Options[OptionIndex];
                    if (Option.Intents.IsEmpty())
                    {
                        Context.AddWarning(FText::FromString(FString::Printf(TEXT("Chapter %d branch %s option %d has no intents; it will fall back to AwaitingAdvance."), ChapterID, *BranchNode.ToString(), OptionIndex)));
                    }

                    for (int32 IntentIndex = 0; IntentIndex < Option.Intents.Num(); ++IntentIndex)
                    {
                        ValidateIntent(Option.Intents[IntentIndex], FString::Printf(TEXT("Chapter %d branch %s option %d intent %d"), ChapterID, *BranchNode.ToString(), OptionIndex, IntentIndex));
                    }
                }
            }
            else
            {
                for (int32 IntentIndex = 0; IntentIndex < Branch.AutoIntents.Num(); ++IntentIndex)
                {
                    ValidateIntent(Branch.AutoIntents[IntentIndex], FString::Printf(TEXT("Chapter %d branch %s auto intent %d"), ChapterID, *BranchNode.ToString(), IntentIndex));
                }
            }
        }
    }

    return bHasError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
