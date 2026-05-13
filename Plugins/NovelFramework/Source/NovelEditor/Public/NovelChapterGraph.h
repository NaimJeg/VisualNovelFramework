#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "NovelChapterAsset.h"
#include "NovelChapterGraph.generated.h"

class UNovelChapterAsset;
class UEdGraphNode_NovelDialogue;

UCLASS()
class NOVELEDITOR_API UEdGraph_NovelChapter : public UEdGraph
{
    GENERATED_BODY()

public:
    void Initialize(UNovelChapterAsset* InChapterAsset);
    void RebuildFromAsset();
    UEdGraphNode_NovelDialogue* AddDialogueNode(const FVector2f& Location, const FNovelNode* SourceNode = nullptr);
    void RemoveDialogueNode(UEdGraphNode_NovelDialogue* Node);
    void SyncNodeToAsset(UEdGraphNode_NovelDialogue* Node);
    void SyncAllToAsset();
    void RebuildConnections();
    UEdGraphNode_NovelDialogue* FindDialogueNode(FName NodeId) const;
    FName MakeUniqueNodeId(FName BaseName = TEXT("Dialogue")) const;

    UPROPERTY(Transient)
    TObjectPtr<UNovelChapterAsset> ChapterAsset;

    bool IsRebuilding() const { return bRebuilding; }

private:
    bool bRebuilding = false;
};

UCLASS()
class NOVELEDITOR_API UEdGraphNode_NovelDialogue : public UEdGraphNode
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (ShowOnlyInnerProperties))
    FNovelNode NodeData;

    UPROPERTY(Transient)
    FName AssetNodeId = NAME_None;

    virtual void AllocateDefaultPins() override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void DestroyNode() override;
    virtual bool CanDuplicateNode() const override { return true; }
    virtual bool CanUserDeleteNode() const override { return true; }

    UEdGraphPin* GetInputPin() const;
    UEdGraphPin* GetNextPin() const;
    UEdGraphPin* GetChoicePin(int32 ChoiceIndex) const;
};

UCLASS()
class NOVELEDITOR_API UEdGraphSchema_NovelChapter : public UEdGraphSchema
{
    GENERATED_BODY()

public:
    virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
    virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
    virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
    virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
    virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;
    virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
};