#include "NovelChapterGraph.h"

#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "NovelChapterGraph"

namespace NovelChapterGraph
{
    const FName TransitionCategory(TEXT("NovelTransition"));
    const FName InputPinName(TEXT("In"));
    const FName NextPinName(TEXT("Next"));

    FName ChoicePinName(const int32 ChoiceIndex)
    {
        return *FString::Printf(TEXT("Choice_%d"), ChoiceIndex);
    }

    UEdGraphNode_NovelDialogue* MakeProjectionNode(UEdGraph_NovelChapter* Graph, const FNovelNode& Source)
    {
        UEdGraphNode_NovelDialogue* Node = NewObject<UEdGraphNode_NovelDialogue>(Graph, NAME_None, RF_Transactional);
        Node->NodeData = Source;
        Node->AssetNodeId = Source.NodeId;
        Node->CreateNewGuid();
        Node->PostPlacedNewNode();
        Node->AllocateDefaultPins();
#if WITH_EDITORONLY_DATA
        Node->NodePosX = FMath::RoundToInt(Source.EditorPosition.X);
        Node->NodePosY = FMath::RoundToInt(Source.EditorPosition.Y);
#endif
        Graph->AddNode(Node, false, false);
        return Node;
    }

    FNovelNode DuplicateNodeData(const FNovelNode& Source, UNovelChapterAsset* Owner)
    {
        FNovelNode Result = Source;
        for (TObjectPtr<UNovelIntentBase>& Action : Result.EntryActions)
        {
            Action = Action ? Cast<UNovelIntentBase>(StaticDuplicateObject(Action, Owner)) : nullptr;
        }
        for (FNovelChoice& Choice : Result.Choices)
        {
            Choice.Condition = Choice.Condition ? Cast<UNovelExpression>(StaticDuplicateObject(Choice.Condition, Owner)) : nullptr;
            for (TObjectPtr<UNovelIntentBase>& Action : Choice.Actions)
            {
                Action = Action ? Cast<UNovelIntentBase>(StaticDuplicateObject(Action, Owner)) : nullptr;
            }
        }
        return Result;
    }

    struct FAddDialogueAction final : FEdGraphSchemaAction
    {
        FAddDialogueAction()
            : FEdGraphSchemaAction(LOCTEXT("DialogueCategory", "Novel"), LOCTEXT("AddDialogueNode", "Add Dialogue Node"), LOCTEXT("AddDialogueNodeTooltip", "Adds a dialogue node to this chapter."), 0)
        {
        }

        virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode) override
        {
            UEdGraph_NovelChapter* Graph = Cast<UEdGraph_NovelChapter>(ParentGraph);
            if (!Graph)
            {
                return nullptr;
            }

            UEdGraphNode_NovelDialogue* NewNode = Graph->AddDialogueNode(Location);
            if (NewNode && FromPin && FromPin->Direction == EGPD_Output)
            {
                ParentGraph->GetSchema()->TryCreateConnection(FromPin, NewNode->GetInputPin());
            }
            return NewNode;
        }
    };
}

void UEdGraph_NovelChapter::Initialize(UNovelChapterAsset* InChapterAsset)
{
    ChapterAsset = InChapterAsset;
    Schema = UEdGraphSchema_NovelChapter::StaticClass();
    SetFlags(RF_Transactional);
    RebuildFromAsset();
}

void UEdGraph_NovelChapter::RebuildFromAsset()
{
    if (!ChapterAsset)
    {
        return;
    }

    TGuardValue<bool> RebuildGuard(bRebuilding, true);
    Nodes.Reset();
    for (const FNovelNode& Source : ChapterAsset->Nodes)
    {
        NovelChapterGraph::MakeProjectionNode(this, Source);
    }
    RebuildConnections();
    NotifyGraphChanged();
}

UEdGraphNode_NovelDialogue* UEdGraph_NovelChapter::AddDialogueNode(const FVector2f& Location, const FNovelNode* SourceNode)
{
    if (!ChapterAsset)
    {
        return nullptr;
    }

    const FScopedTransaction Transaction(LOCTEXT("AddDialogueTransaction", "Add Novel Dialogue Node"));
    Modify();
    ChapterAsset->Modify();

    FNovelNode NewData = SourceNode ? NovelChapterGraph::DuplicateNodeData(*SourceNode, ChapterAsset) : FNovelNode();
    NewData.NodeId = MakeUniqueNodeId(SourceNode ? SourceNode->NodeId : FName(TEXT("Dialogue")));
#if WITH_EDITORONLY_DATA
    NewData.EditorPosition = FVector2D(Location);
#endif
    ChapterAsset->Nodes.Add(NewData);
    if (ChapterAsset->EntryNodeId.IsNone())
    {
        ChapterAsset->EntryNodeId = NewData.NodeId;
    }
    ChapterAsset->RebuildNodeLookup();
    ChapterAsset->MarkPackageDirty();

    UEdGraphNode_NovelDialogue* NewNode = NovelChapterGraph::MakeProjectionNode(this, NewData);
    NotifyGraphChanged();
    return NewNode;
}

void UEdGraph_NovelChapter::RemoveDialogueNode(UEdGraphNode_NovelDialogue* Node)
{
    if (!ChapterAsset || !Node || bRebuilding)
    {
        return;
    }

    ChapterAsset->Modify();
    const FName RemovedId = Node->AssetNodeId.IsNone() ? Node->NodeData.NodeId : Node->AssetNodeId;
    ChapterAsset->Nodes.RemoveAll([RemovedId](const FNovelNode& Candidate)
    {
        return Candidate.NodeId == RemovedId;
    });
    if (ChapterAsset->EntryNodeId == RemovedId)
    {
        ChapterAsset->EntryNodeId = ChapterAsset->Nodes.IsEmpty() ? NAME_None : ChapterAsset->Nodes[0].NodeId;
    }
    ChapterAsset->RebuildNodeLookup();
    ChapterAsset->MarkPackageDirty();
}

void UEdGraph_NovelChapter::SyncNodeToAsset(UEdGraphNode_NovelDialogue* Node)
{
    if (!ChapterAsset || !Node || bRebuilding)
    {
        return;
    }

    ChapterAsset->Modify();
    FNovelNode* Existing = ChapterAsset->FindMutableNode(Node->AssetNodeId.IsNone() ? Node->NodeData.NodeId : Node->AssetNodeId);
    if (Existing)
    {
        *Existing = Node->NodeData;
    }
    else
    {
        ChapterAsset->Nodes.Add(Node->NodeData);
    }
#if WITH_EDITORONLY_DATA
    FNovelNode* Updated = ChapterAsset->FindMutableNode(Node->NodeData.NodeId);
    if (Updated)
    {
        Updated->EditorPosition = FVector2D(Node->NodePosX, Node->NodePosY);
    }
#endif
    Node->AssetNodeId = Node->NodeData.NodeId;
    ChapterAsset->RebuildNodeLookup();
    ChapterAsset->MarkPackageDirty();
}

void UEdGraph_NovelChapter::SyncAllToAsset()
{
    if (!ChapterAsset || bRebuilding)
    {
        return;
    }

    ChapterAsset->Modify();
    TArray<FNovelNode> UpdatedNodes;
    UpdatedNodes.Reserve(Nodes.Num());
    for (UEdGraphNode* RawNode : Nodes)
    {
        UEdGraphNode_NovelDialogue* Node = Cast<UEdGraphNode_NovelDialogue>(RawNode);
        if (!Node)
        {
            continue;
        }
#if WITH_EDITORONLY_DATA
        Node->NodeData.EditorPosition = FVector2D(Node->NodePosX, Node->NodePosY);
#endif
        UpdatedNodes.Add(Node->NodeData);
    }

    const FPrimaryAssetId ThisChapterId = ChapterAsset->GetPrimaryAssetId();
    for (UEdGraphNode* RawNode : Nodes)
    {
        UEdGraphNode_NovelDialogue* Node = Cast<UEdGraphNode_NovelDialogue>(RawNode);
        if (!Node)
        {
            continue;
        }

        FNovelNode* Updated = UpdatedNodes.FindByPredicate([Node](const FNovelNode& Candidate)
        {
            return Candidate.NodeId == Node->NodeData.NodeId;
        });
        if (!Updated)
        {
            continue;
        }

        auto ResolvePin = [ThisChapterId](UEdGraphPin* Pin, FNovelNodeRef& Target)
        {
            if (Pin && !Pin->LinkedTo.IsEmpty())
            {
                if (const UEdGraphNode_NovelDialogue* TargetNode = Cast<UEdGraphNode_NovelDialogue>(Pin->LinkedTo[0]->GetOwningNode()))
                {
                    Target.ChapterId = ThisChapterId;
                    Target.NodeId = TargetNode->NodeData.NodeId;
                }
            }
            else if (Target.ChapterId == ThisChapterId)
            {
                Target = FNovelNodeRef();
            }
        };

        ResolvePin(Node->GetNextPin(), Updated->Next);
        for (int32 ChoiceIndex = 0; ChoiceIndex < Updated->Choices.Num(); ++ChoiceIndex)
        {
            ResolvePin(Node->GetChoicePin(ChoiceIndex), Updated->Choices[ChoiceIndex].Target);
        }
        Node->NodeData = *Updated;
        Node->AssetNodeId = Node->NodeData.NodeId;
    }

    ChapterAsset->Nodes = MoveTemp(UpdatedNodes);
    ChapterAsset->RebuildNodeLookup();
    ChapterAsset->MarkPackageDirty();
}

void UEdGraph_NovelChapter::RebuildConnections()
{
    if (!ChapterAsset)
    {
        return;
    }

    TGuardValue<bool> RebuildGuard(bRebuilding, true);
    for (UEdGraphNode* Node : Nodes)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            Pin->BreakAllPinLinks(false);
        }
    }

    const FPrimaryAssetId ThisChapterId = ChapterAsset->GetPrimaryAssetId();
    for (UEdGraphNode* RawNode : Nodes)
    {
        UEdGraphNode_NovelDialogue* Node = Cast<UEdGraphNode_NovelDialogue>(RawNode);
        if (!Node)
        {
            continue;
        }

        auto LinkTarget = [this, ThisChapterId](UEdGraphPin* SourcePin, const FNovelNodeRef& Target)
        {
            if (!SourcePin || Target.ChapterId != ThisChapterId)
            {
                return;
            }
            if (UEdGraphNode_NovelDialogue* TargetNode = FindDialogueNode(Target.NodeId))
            {
                SourcePin->MakeLinkTo(TargetNode->GetInputPin());
            }
        };

        LinkTarget(Node->GetNextPin(), Node->NodeData.Next);
        for (int32 ChoiceIndex = 0; ChoiceIndex < Node->NodeData.Choices.Num(); ++ChoiceIndex)
        {
            LinkTarget(Node->GetChoicePin(ChoiceIndex), Node->NodeData.Choices[ChoiceIndex].Target);
        }
    }
}

UEdGraphNode_NovelDialogue* UEdGraph_NovelChapter::FindDialogueNode(const FName NodeId) const
{
    for (UEdGraphNode* Node : Nodes)
    {
        UEdGraphNode_NovelDialogue* DialogueNode = Cast<UEdGraphNode_NovelDialogue>(Node);
        if (DialogueNode && DialogueNode->NodeData.NodeId == NodeId)
        {
            return DialogueNode;
        }
    }
    return nullptr;
}

FName UEdGraph_NovelChapter::MakeUniqueNodeId(const FName BaseName) const
{
    const FString Base = BaseName.IsNone() ? TEXT("Dialogue") : BaseName.ToString();
    FName Candidate(*Base);
    int32 Suffix = 2;
    while (FindDialogueNode(Candidate) || (ChapterAsset && ChapterAsset->FindNode(Candidate)))
    {
        Candidate = *FString::Printf(TEXT("%s_%d"), *Base, Suffix++);
    }
    return Candidate;
}

void UEdGraphNode_NovelDialogue::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, NovelChapterGraph::TransitionCategory, NovelChapterGraph::InputPinName);
    CreatePin(EGPD_Output, NovelChapterGraph::TransitionCategory, NovelChapterGraph::NextPinName);
    for (int32 ChoiceIndex = 0; ChoiceIndex < NodeData.Choices.Num(); ++ChoiceIndex)
    {
        UEdGraphPin* ChoicePin = CreatePin(EGPD_Output, NovelChapterGraph::TransitionCategory, NovelChapterGraph::ChoicePinName(ChoiceIndex));
        ChoicePin->PinFriendlyName = NodeData.Choices[ChoiceIndex].Text.IsEmpty()
            ? FText::Format(LOCTEXT("ChoicePinFallback", "Choice {0}"), FText::AsNumber(ChoiceIndex + 1))
            : NodeData.Choices[ChoiceIndex].Text;
    }
}

FText UEdGraphNode_NovelDialogue::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (NodeData.Speaker.IsEmpty())
    {
        return FText::FromName(NodeData.NodeId);
    }
    return FText::Format(LOCTEXT("DialogueNodeTitle", "{0}: {1}"), FText::FromName(NodeData.NodeId), NodeData.Speaker);
}

FLinearColor UEdGraphNode_NovelDialogue::GetNodeTitleColor() const
{
    return FLinearColor(0.12f, 0.42f, 0.55f);
}

void UEdGraphNode_NovelDialogue::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    UEdGraph_NovelChapter* NovelGraph = Cast<UEdGraph_NovelChapter>(GetGraph());
    if (!NovelGraph || NovelGraph->IsRebuilding())
    {
        return;
    }

    NovelGraph->SyncNodeToAsset(this);
    ReconstructNode();
    NovelGraph->RebuildConnections();
    NovelGraph->NotifyGraphChanged();
}

void UEdGraphNode_NovelDialogue::DestroyNode()
{
    UEdGraph_NovelChapter* NovelGraph = Cast<UEdGraph_NovelChapter>(GetGraph());
    Super::DestroyNode();
    if (NovelGraph)
    {
        NovelGraph->RemoveDialogueNode(this);
        NovelGraph->SyncAllToAsset();
    }
}

UEdGraphPin* UEdGraphNode_NovelDialogue::GetInputPin() const
{
    return FindPin(NovelChapterGraph::InputPinName, EGPD_Input);
}

UEdGraphPin* UEdGraphNode_NovelDialogue::GetNextPin() const
{
    return FindPin(NovelChapterGraph::NextPinName, EGPD_Output);
}

UEdGraphPin* UEdGraphNode_NovelDialogue::GetChoicePin(const int32 ChoiceIndex) const
{
    return FindPin(NovelChapterGraph::ChoicePinName(ChoiceIndex), EGPD_Output);
}

void UEdGraphSchema_NovelChapter::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    ContextMenuBuilder.AddAction(MakeShared<NovelChapterGraph::FAddDialogueAction>());
}

const FPinConnectionResponse UEdGraphSchema_NovelChapter::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
    if (!A || !B || A == B || A->GetOwningNode() == B->GetOwningNode())
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("InvalidConnection", "Invalid transition."));
    }
    if (A->Direction == B->Direction)
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("DirectionsMustDiffer", "Connect an output to an input."));
    }
    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}

bool UEdGraphSchema_NovelChapter::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
    if (CanCreateConnection(A, B).Response == CONNECT_RESPONSE_DISALLOW)
    {
        return false;
    }

    UEdGraphPin* Output = A->Direction == EGPD_Output ? A : B;
    UEdGraphPin* Input = A->Direction == EGPD_Input ? A : B;
    const FScopedTransaction Transaction(LOCTEXT("ConnectTransitionTransaction", "Connect Novel Transition"));
    Output->Modify();
    Input->Modify();
    Output->BreakAllPinLinks(true);
    Output->MakeLinkTo(Input);
    if (UEdGraph_NovelChapter* Graph = Cast<UEdGraph_NovelChapter>(Output->GetOwningNode()->GetGraph()))
    {
        Graph->SyncAllToAsset();
        Graph->NotifyGraphChanged();
    }
    return true;
}

void UEdGraphSchema_NovelChapter::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
    Super::BreakNodeLinks(TargetNode);
    if (UEdGraph_NovelChapter* Graph = Cast<UEdGraph_NovelChapter>(TargetNode.GetGraph()))
    {
        Graph->SyncAllToAsset();
    }
}

void UEdGraphSchema_NovelChapter::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
    Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
    if (UEdGraph_NovelChapter* Graph = Cast<UEdGraph_NovelChapter>(TargetPin.GetOwningNode()->GetGraph()))
    {
        Graph->SyncAllToAsset();
    }
}

void UEdGraphSchema_NovelChapter::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
    Super::BreakSinglePinLink(SourcePin, TargetPin);
    if (SourcePin)
    {
        if (UEdGraph_NovelChapter* Graph = Cast<UEdGraph_NovelChapter>(SourcePin->GetOwningNode()->GetGraph()))
        {
            Graph->SyncAllToAsset();
        }
    }
}

FLinearColor UEdGraphSchema_NovelChapter::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
    return FLinearColor(0.3f, 0.75f, 0.85f);
}

#undef LOCTEXT_NAMESPACE