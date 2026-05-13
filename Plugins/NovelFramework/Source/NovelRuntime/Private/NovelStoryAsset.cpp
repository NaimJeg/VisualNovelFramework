#include "NovelStoryAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId UNovelStoryAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(FPrimaryAssetType(TEXT("NovelStory")), GetFName());
}

TSoftObjectPtr<UNovelChapterAsset> UNovelStoryAsset::FindChapter(FPrimaryAssetId ChapterId) const
{
    for (const TSoftObjectPtr<UNovelChapterAsset>& Chapter : Chapters)
    {
        if (const UNovelChapterAsset* LoadedChapter = Chapter.Get())
        {
            if (LoadedChapter->GetPrimaryAssetId() == ChapterId)
            {
                return Chapter;
            }
        }
        else if (ChapterId.IsValid() && Chapter.ToSoftObjectPath().GetAssetFName() == ChapterId.PrimaryAssetName)
        {
            return Chapter;
        }
    }
    return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UNovelStoryAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    bool bInvalid = Result == EDataValidationResult::Invalid;
    TMap<FPrimaryAssetId, UNovelChapterAsset*> LoadedChapters;

    if (!EntryNode.IsValid())
    {
        Context.AddError(FText::FromString(TEXT("Story entry node is not configured.")));
        bInvalid = true;
    }
    if (Chapters.IsEmpty())
    {
        Context.AddError(FText::FromString(TEXT("Story has no chapters.")));
        bInvalid = true;
    }

    for (const TSoftObjectPtr<UNovelChapterAsset>& ChapterRef : Chapters)
    {
        UNovelChapterAsset* Chapter = ChapterRef.LoadSynchronous();
        if (!Chapter)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Story chapter failed to load: %s"), *ChapterRef.ToString())));
            bInvalid = true;
            continue;
        }
        if (LoadedChapters.Contains(Chapter->GetPrimaryAssetId()))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Story contains duplicate chapter %s."), *Chapter->GetPrimaryAssetId().ToString())));
            bInvalid = true;
            continue;
        }
        LoadedChapters.Add(Chapter->GetPrimaryAssetId(), Chapter);
    }

    auto ValidateTarget = [&](const FNovelNodeRef& Target, const FString& ContextDescription)
    {
        if (!Target.IsValid())
        {
            return;
        }
        UNovelChapterAsset* const* TargetChapter = LoadedChapters.Find(Target.ChapterId);
        if (!TargetChapter)
        {
            Context.AddError(FText::FromString(ContextDescription + FString::Printf(TEXT(" targets missing chapter %s."), *Target.ChapterId.ToString())));
            bInvalid = true;
        }
        else if (!(*TargetChapter)->FindNode(Target.NodeId))
        {
            Context.AddError(FText::FromString(ContextDescription + FString::Printf(TEXT(" targets missing node %s."), *Target.ToString())));
            bInvalid = true;
        }
    };

    ValidateTarget(EntryNode, TEXT("Story entry"));
    for (const TPair<FPrimaryAssetId, UNovelChapterAsset*>& Pair : LoadedChapters)
    {
        for (const FNovelNode& Node : Pair.Value->Nodes)
        {
            ValidateTarget(Node.Next, FString::Printf(TEXT("Node %s"), *Pair.Value->MakeNodeRef(Node.NodeId).ToString()));
            for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
            {
                ValidateTarget(Node.Choices[ChoiceIndex].Target, FString::Printf(TEXT("Node %s choice %d"), *Pair.Value->MakeNodeRef(Node.NodeId).ToString(), ChoiceIndex));
            }
        }
    }

    if (EntryNode.IsValid() && LoadedChapters.Contains(EntryNode.ChapterId))
    {
        TSet<FNovelNodeRef> Reachable;
        TArray<FNovelNodeRef> Pending;
        Pending.Add(EntryNode);
        while (!Pending.IsEmpty())
        {
            const FNovelNodeRef Ref = Pending.Pop(EAllowShrinking::No);
            if (Reachable.Contains(Ref))
            {
                continue;
            }
            Reachable.Add(Ref);
            UNovelChapterAsset* const* Chapter = LoadedChapters.Find(Ref.ChapterId);
            const FNovelNode* Node = Chapter ? (*Chapter)->FindNode(Ref.NodeId) : nullptr;
            if (!Node)
            {
                continue;
            }
            if (Node->Next.IsValid())
            {
                Pending.Add(Node->Next);
            }
            for (const FNovelChoice& Choice : Node->Choices)
            {
                if (Choice.Target.IsValid())
                {
                    Pending.Add(Choice.Target);
                }
            }
        }

        for (const TPair<FPrimaryAssetId, UNovelChapterAsset*>& Pair : LoadedChapters)
        {
            for (const FNovelNode& Node : Pair.Value->Nodes)
            {
                const FNovelNodeRef Ref = Pair.Value->MakeNodeRef(Node.NodeId);
                if (!Reachable.Contains(Ref))
                {
                    Context.AddWarning(FText::FromString(FString::Printf(TEXT("Node %s is unreachable from the story entry."), *Ref.ToString())));
                }
            }
        }
    }

    return bInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif