#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NovelIntentBase.h"
#include "NovelExpression.h"
#include "NovelChapterAsset.generated.h"

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelNodeRef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
    FPrimaryAssetId ChapterId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
    FName NodeId = NAME_None;

    bool IsValid() const { return ChapterId.IsValid() && !NodeId.IsNone(); }
    FString ToString() const { return FString::Printf(TEXT("%s:%s"), *ChapterId.ToString(), *NodeId.ToString()); }

    bool operator==(const FNovelNodeRef& Other) const
    {
        return ChapterId == Other.ChapterId && NodeId == Other.NodeId;
    }

    bool operator!=(const FNovelNodeRef& Other) const { return !(*this == Other); }
};

FORCEINLINE uint32 GetTypeHash(const FNovelNodeRef& Ref)
{
    return HashCombine(GetTypeHash(Ref.ChapterId), GetTypeHash(Ref.NodeId));
}

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
    FNovelNodeRef Target;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Choice")
    TObjectPtr<UNovelExpression> Condition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Choice")
    TArray<TObjectPtr<UNovelIntentBase>> Actions;
};

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FNovelNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
    FName NodeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
    FText Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node", meta = (MultiLine = "true"))
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Node")
    TArray<TObjectPtr<UNovelIntentBase>> EntryActions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
    FNovelNodeRef Next;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
    TArray<FNovelChoice> Choices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
    TArray<FName> Tags;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, Category = "Editor")
    FVector2D EditorPosition = FVector2D::ZeroVector;
#endif
};

UCLASS(BlueprintType)
class NOVELRUNTIME_API UNovelChapterAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chapter")
    FName EntryNodeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chapter")
    TArray<FNovelNode> Nodes;

    const FNovelNode* FindNode(FName NodeId) const;
    FNovelNode* FindMutableNode(FName NodeId);
    FNovelNodeRef MakeNodeRef(FName NodeId) const;
    void RebuildNodeLookup() const;

private:
    mutable TMap<FName, int32> NodeLookup;
};
