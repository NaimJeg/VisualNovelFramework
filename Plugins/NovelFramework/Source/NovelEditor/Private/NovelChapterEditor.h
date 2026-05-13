#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "GraphEditor.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class IDetailsView;
class SGraphEditor;
class UNovelChapterAsset;
class UEdGraph_NovelChapter;
class UEdGraphNode_NovelDialogue;

class FNovelChapterEditorCommands final : public TCommands<FNovelChapterEditorCommands>
{
public:
    FNovelChapterEditorCommands();
    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> ValidateChapter;
    TSharedPtr<FUICommandInfo> FocusTarget;
    TSharedPtr<FUICommandInfo> LegacyConversionHelp;
};

class FNovelChapterEditor final : public FAssetEditorToolkit, public FGCObject, public FEditorUndoClient
{
public:
    FNovelChapterEditor();
    virtual ~FNovelChapterEditor() override;

    void InitChapterEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UNovelChapterAsset* InChapterAsset);

    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    virtual void SaveAsset_Execute() override;
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override { return TEXT("FNovelChapterEditor"); }
    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

private:
    static const FName GraphTabId;
    static const FName DetailsTabId;

    TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);
    void CreateWidgets();
    void BindCommands();
    void ExtendToolbar();
    void OnSelectedNodesChanged(const TSet<UObject*>& NewSelection);
    void DeleteSelectedNodes();
    bool CanDeleteSelectedNodes() const;
    void DuplicateSelectedNodes();
    bool CanDuplicateSelectedNodes() const;
    void ValidateChapter();
    void FocusReferencedTarget();
    bool CanFocusReferencedTarget() const;
    void ShowLegacyConversionHelp();
    UEdGraphNode_NovelDialogue* GetSingleSelectedDialogueNode() const;

    TObjectPtr<UNovelChapterAsset> ChapterAsset;
    TObjectPtr<UEdGraph_NovelChapter> ChapterGraph;
    TSharedPtr<SGraphEditor> GraphEditor;
    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<FUICommandList> GraphCommands;
};