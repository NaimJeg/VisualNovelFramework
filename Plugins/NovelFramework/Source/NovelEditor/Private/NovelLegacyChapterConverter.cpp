#include "NovelLegacyChapterConverter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FDialogueNode.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "NovelChapterAsset.h"
#include "NovelDialogueBranchData.h"
#include "NovelIntent_Jump.h"
#include "UObject/Package.h"

namespace
{
UNovelIntentBase* DuplicateLegacyAction(UNovelIntentBase* Source, UNovelChapterAsset* Destination, const FString& Context, FNovelLegacyConversionReport& Report)
{
    if (!Source)
    {
        Report.Warnings.Add(Context + TEXT(" contains a null action."));
        return nullptr;
    }

    UNovelIntentBase* Duplicate = DuplicateObject<UNovelIntentBase>(Source, Destination);
    if (!Duplicate)
    {
        Report.UnsupportedConstructs.Add(Context + TEXT(" could not be duplicated."));
    }
    return Duplicate;
}
}

UNovelChapterAsset* UNovelLegacyChapterConverter::ConvertLegacyChapter(
    int32 ChapterID,
    const FChapterData& LegacyChapter,
    const FString& DestinationPackagePath,
    FNovelLegacyConversionReport& OutReport)
{
    OutReport = FNovelLegacyConversionReport();
    if (!FPackageName::IsValidLongPackageName(DestinationPackagePath))
    {
        OutReport.UnsupportedConstructs.Add(FString::Printf(TEXT("Invalid destination package path: %s"), *DestinationPackagePath));
        return nullptr;
    }

    UDataTable* DialogueTable = LegacyChapter.DialogueTable.LoadSynchronous();
    if (!DialogueTable)
    {
        OutReport.UnsupportedConstructs.Add(TEXT("Legacy dialogue table could not be loaded."));
        return nullptr;
    }

    const FString AssetName = FPackageName::GetLongPackageAssetName(DestinationPackagePath);
    UPackage* Package = CreatePackage(*DestinationPackagePath);
    if (!Package || FindObject<UNovelChapterAsset>(Package, *AssetName))
    {
        OutReport.UnsupportedConstructs.Add(FString::Printf(TEXT("Destination already exists or could not be created: %s"), *DestinationPackagePath));
        return nullptr;
    }

    UNovelChapterAsset* Chapter = NewObject<UNovelChapterAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
    if (!Chapter)
    {
        OutReport.UnsupportedConstructs.Add(TEXT("Could not allocate chapter asset."));
        return nullptr;
    }

    const TArray<FName> RowNames = DialogueTable->GetRowNames();
    if (RowNames.IsEmpty())
    {
        OutReport.UnsupportedConstructs.Add(TEXT("Legacy dialogue table has no rows."));
        return nullptr;
    }

    Chapter->EntryNodeId = RowNames[0];
    const FPrimaryAssetId ChapterAssetId = Chapter->GetPrimaryAssetId();
    UNovelDialogueBranchData* BranchData = LegacyChapter.BranchData.IsNull() ? nullptr : LegacyChapter.BranchData.LoadSynchronous();
    OutReport.Warnings.Add(FString::Printf(TEXT("Chapter %d was converted using current DataTable row order. Review every generated Next transition."), ChapterID));

    for (int32 RowIndex = 0; RowIndex < RowNames.Num(); ++RowIndex)
    {
        const FName RowName = RowNames[RowIndex];
        const FDialogueRow* Row = DialogueTable->FindRow<FDialogueRow>(RowName, TEXT("NovelLegacyChapterConverter"), false);
        if (!Row)
        {
            OutReport.UnsupportedConstructs.Add(FString::Printf(TEXT("Row %s could not be read as FDialogueRow."), *RowName.ToString()));
            continue;
        }

        FNovelNode& Node = Chapter->Nodes.AddDefaulted_GetRef();
        Node.NodeId = RowName;
        Node.Speaker = FText::FromString(Row->Speaker);
        Node.Text = FText::FromString(Row->Text);
        if (RowIndex + 1 < RowNames.Num())
        {
            Node.Next.ChapterId = ChapterAssetId;
            Node.Next.NodeId = RowNames[RowIndex + 1];
        }

        const FBranchData* Branch = BranchData ? BranchData->BranchMap.Find(RowName) : nullptr;
        if (!Branch)
        {
            continue;
        }

        for (int32 ActionIndex = 0; ActionIndex < Branch->AutoIntents.Num(); ++ActionIndex)
        {
            UNovelIntentBase* LegacyAction = Branch->AutoIntents[ActionIndex];
            if (UNovelIntentBase* Duplicate = DuplicateLegacyAction(LegacyAction, Chapter, FString::Printf(TEXT("Node %s entry action %d"), *RowName.ToString(), ActionIndex), OutReport))
            {
                Node.EntryActions.Add(Duplicate);
                if (Cast<UNovelIntent_Jump>(LegacyAction))
                {
                    OutReport.AssetsRequiringManualReview.Add(FString::Printf(TEXT("Node %s retains a legacy jump entry action."), *RowName.ToString()));
                }
            }
        }

        if (!Branch->bIsChoiceNode)
        {
            continue;
        }

        for (int32 ChoiceIndex = 0; ChoiceIndex < Branch->Options.Num(); ++ChoiceIndex)
        {
            const FDialogueOption& LegacyChoice = Branch->Options[ChoiceIndex];
            FNovelChoice& Choice = Node.Choices.AddDefaulted_GetRef();
            Choice.Text = LegacyChoice.OptionText;

            for (int32 ActionIndex = 0; ActionIndex < LegacyChoice.Intents.Num(); ++ActionIndex)
            {
                UNovelIntentBase* LegacyAction = LegacyChoice.Intents[ActionIndex];
                if (const UNovelIntent_Jump* Jump = Cast<UNovelIntent_Jump>(LegacyAction))
                {
                    if (!Choice.Target.IsValid())
                    {
                        Choice.Target.ChapterId = ChapterAssetId;
                        Choice.Target.NodeId = Jump->TargetNode.RowName;
                    }
                    else
                    {
                        OutReport.UnsupportedConstructs.Add(FString::Printf(TEXT("Node %s choice %d contains multiple jump actions."), *RowName.ToString(), ChoiceIndex));
                    }
                    continue;
                }

                if (UNovelIntentBase* Duplicate = DuplicateLegacyAction(LegacyAction, Chapter, FString::Printf(TEXT("Node %s choice %d action %d"), *RowName.ToString(), ChoiceIndex, ActionIndex), OutReport))
                {
                    Choice.Actions.Add(Duplicate);
                }
            }

            if (!Choice.Target.IsValid())
            {
                OutReport.MissingTargets.Add(FString::Printf(TEXT("Node %s choice %d has no jump target."), *RowName.ToString(), ChoiceIndex));
            }
            else if (!RowNames.Contains(Choice.Target.NodeId))
            {
                OutReport.MissingTargets.Add(FString::Printf(TEXT("Node %s choice %d targets missing row %s."), *RowName.ToString(), ChoiceIndex, *Choice.Target.NodeId.ToString()));
            }
        }
    }

    if (BranchData)
    {
        for (const TPair<FName, FBranchData>& Pair : BranchData->BranchMap)
        {
            if (!RowNames.Contains(Pair.Key))
            {
                OutReport.Warnings.Add(FString::Printf(TEXT("Branch key %s has no matching dialogue row."), *Pair.Key.ToString()));
            }
        }
    }

    Chapter->RebuildNodeLookup();
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Chapter);
    OutReport.ConvertedNodeCount = Chapter->Nodes.Num();
    OutReport.ConvertedChapters.Add(DestinationPackagePath);
    if (OutReport.HasBlockingIssues() || !OutReport.UnsupportedConstructs.IsEmpty())
    {
        OutReport.AssetsRequiringManualReview.Add(DestinationPackagePath);
    }
    return Chapter;
}
