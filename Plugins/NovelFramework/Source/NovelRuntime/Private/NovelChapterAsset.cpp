#include "NovelChapterAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId UNovelChapterAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(FPrimaryAssetType(TEXT("NovelChapter")), GetFName());
}

void UNovelChapterAsset::PostLoad()
{
    Super::PostLoad();
    RebuildNodeLookup();
}

const FNovelNode* UNovelChapterAsset::FindNode(FName NodeId) const
{
    if (NodeLookup.Num() != Nodes.Num())
    {
        RebuildNodeLookup();
    }

    const int32* Index = NodeLookup.Find(NodeId);
    return Index && Nodes.IsValidIndex(*Index) ? &Nodes[*Index] : nullptr;
}

FNovelNode* UNovelChapterAsset::FindMutableNode(FName NodeId)
{
    return const_cast<FNovelNode*>(const_cast<const UNovelChapterAsset*>(this)->FindNode(NodeId));
}

FNovelNodeRef UNovelChapterAsset::MakeNodeRef(FName NodeId) const
{
    FNovelNodeRef Ref;
    Ref.ChapterId = GetPrimaryAssetId();
    Ref.NodeId = NodeId;
    return Ref;
}

void UNovelChapterAsset::RebuildNodeLookup() const
{
    NodeLookup.Reset();
    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        if (!Nodes[Index].NodeId.IsNone() && !NodeLookup.Contains(Nodes[Index].NodeId))
        {
            NodeLookup.Add(Nodes[Index].NodeId, Index);
        }
    }
}

#if WITH_EDITOR
void UNovelChapterAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    RebuildNodeLookup();
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

EDataValidationResult UNovelChapterAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    bool bInvalid = Result == EDataValidationResult::Invalid;
    TSet<FName> NodeIds;

    if (EntryNodeId.IsNone())
    {
        Context.AddError(FText::FromString(TEXT("Chapter entry node is not set.")));
        bInvalid = true;
    }

    for (const FNovelNode& Node : Nodes)
    {
        if (Node.NodeId.IsNone())
        {
            Context.AddError(FText::FromString(TEXT("Chapter contains a node with an empty ID.")));
            bInvalid = true;
            continue;
        }
        if (NodeIds.Contains(Node.NodeId))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate node ID: %s"), *Node.NodeId.ToString())));
            bInvalid = true;
        }
        NodeIds.Add(Node.NodeId);

        if (Node.Speaker.IsEmpty() && Node.Text.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT("Node %s has empty speaker and text."), *Node.NodeId.ToString())));
        }

        for (const UNovelIntentBase* Action : Node.EntryActions)
        {
            if (!Action)
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Node %s contains a null entry action."), *Node.NodeId.ToString())));
                bInvalid = true;
            }
        }
        for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
        {
            const FNovelChoice& Choice = Node.Choices[ChoiceIndex];
            if (Choice.Condition)
            {
                if (static_cast<const UNovelExpression*>(Choice.Condition.Get())->IsDataValid(Context) == EDataValidationResult::Invalid)
                {
                    bInvalid = true;
                }
                const ENovelValueType StaticType = Choice.Condition->GetStaticResultType();
                if (StaticType != ENovelValueType::None && StaticType != ENovelValueType::Bool)
                {
                    Context.AddError(FText::FromString(FString::Printf(TEXT("Node %s choice %d condition has non-Bool result type %s."), *Node.NodeId.ToString(), ChoiceIndex, *StaticEnum<ENovelValueType>()->GetNameStringByValue(static_cast<int64>(StaticType)))));
                    bInvalid = true;
                }
            }
            if (!Choice.Target.IsValid())
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Node %s choice %d has no target."), *Node.NodeId.ToString(), ChoiceIndex)));
                bInvalid = true;
            }
            for (const UNovelIntentBase* Action : Choice.Actions)
            {
                if (!Action)
                {
                    Context.AddError(FText::FromString(FString::Printf(TEXT("Node %s choice %d contains a null action."), *Node.NodeId.ToString(), ChoiceIndex)));
                    bInvalid = true;
                }
            }
        }
    }

    if (!EntryNodeId.IsNone() && !NodeIds.Contains(EntryNodeId))
    {
        Context.AddError(FText::FromString(FString::Printf(TEXT("Entry node %s does not exist."), *EntryNodeId.ToString())));
        bInvalid = true;
    }

    const FPrimaryAssetId ThisChapterId = GetPrimaryAssetId();
    for (const FNovelNode& Node : Nodes)
    {
        if (Node.Next.IsValid() && Node.Next.ChapterId == ThisChapterId && !NodeIds.Contains(Node.Next.NodeId))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Node %s has missing Next target %s."), *Node.NodeId.ToString(), *Node.Next.NodeId.ToString())));
            bInvalid = true;
        }
        for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
        {
            const FNovelNodeRef& Target = Node.Choices[ChoiceIndex].Target;
            if (Target.IsValid() && Target.ChapterId == ThisChapterId && !NodeIds.Contains(Target.NodeId))
            {
                Context.AddError(FText::FromString(FString::Printf(TEXT("Node %s choice %d targets missing node %s."), *Node.NodeId.ToString(), ChoiceIndex, *Target.NodeId.ToString())));
                bInvalid = true;
            }
        }
    }

    if (NodeIds.Contains(EntryNodeId))
    {
        TSet<FName> Reachable;
        TArray<FName> Pending;
        Pending.Add(EntryNodeId);
        while (!Pending.IsEmpty())
        {
            const FName NodeId = Pending.Pop(EAllowShrinking::No);
            if (Reachable.Contains(NodeId))
            {
                continue;
            }
            Reachable.Add(NodeId);
            const FNovelNode* Node = FindNode(NodeId);
            if (!Node)
            {
                continue;
            }
            if (Node->Next.IsValid() && Node->Next.ChapterId == ThisChapterId)
            {
                Pending.Add(Node->Next.NodeId);
            }
            for (const FNovelChoice& Choice : Node->Choices)
            {
                if (Choice.Target.IsValid() && Choice.Target.ChapterId == ThisChapterId)
                {
                    Pending.Add(Choice.Target.NodeId);
                }
            }
        }

        for (const FName NodeId : NodeIds)
        {
            if (!Reachable.Contains(NodeId))
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT("Node %s is unreachable from the chapter entry."), *NodeId.ToString())));
            }
        }
    }

    return bInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
