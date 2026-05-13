#include "NovelChapterEditor.h"

#include "Editor.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailsView.h"
#include "Misc/DataValidation.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "NovelChapterAsset.h"
#include "NovelChapterGraph.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "NovelChapterEditor"

const FName FNovelChapterEditor::GraphTabId(TEXT("NovelChapterEditor_Graph"));
const FName FNovelChapterEditor::DetailsTabId(TEXT("NovelChapterEditor_Details"));

FNovelChapterEditorCommands::FNovelChapterEditorCommands()
    : TCommands<FNovelChapterEditorCommands>(TEXT("NovelChapterEditor"), LOCTEXT("CommandContext", "Novel Chapter Editor"), NAME_None, FAppStyle::GetAppStyleSetName())
{
}

void FNovelChapterEditorCommands::RegisterCommands()
{
    UI_COMMAND(ValidateChapter, "Validate", "Validate this chapter and display all errors and warnings.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(FocusTarget, "Focus Target", "Focus the selected node's local Next or first choice target.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(LegacyConversionHelp, "Legacy Conversion", "Show the legacy DataTable conversion entry point.", EUserInterfaceActionType::Button, FInputChord());
}

FNovelChapterEditor::FNovelChapterEditor() = default;

FNovelChapterEditor::~FNovelChapterEditor()
{
    if (ChapterGraph)
    {
        ChapterGraph->SyncAllToAsset();
    }
    if (GEditor)
    {
        GEditor->UnregisterForUndo(this);
    }
}

void FNovelChapterEditor::InitChapterEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UNovelChapterAsset* InChapterAsset)
{
    ChapterAsset = InChapterAsset;
    check(ChapterAsset);
    ChapterAsset->SetFlags(RF_Transactional);

    ChapterGraph = NewObject<UEdGraph_NovelChapter>(GetTransientPackage(), NAME_None, RF_Transient | RF_Transactional);
    ChapterGraph->Initialize(ChapterAsset);

    FNovelChapterEditorCommands::Register();
    BindCommands();
    CreateWidgets();
    if (GEditor)
    {
        GEditor->RegisterForUndo(this);
    }

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(TEXT("NovelChapterEditor_Layout_v1"))
        ->AddArea(
            FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
            ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.72f)->AddTab(GraphTabId, ETabState::OpenedTab))
            ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.28f)->AddTab(DetailsTabId, ETabState::OpenedTab)));

    InitAssetEditor(Mode, InitToolkitHost, TEXT("NovelChapterEditorApp"), Layout, true, true, ChapterAsset);
    ExtendToolbar();
    RegenerateMenusAndToolbars();
}

void FNovelChapterEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceCategory", "Novel Chapter Editor"));
    const TSharedRef<FWorkspaceItem> WorkspaceCategoryRef = WorkspaceMenuCategory.ToSharedRef();
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
    InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FNovelChapterEditor::SpawnGraphTab))
        .SetDisplayName(LOCTEXT("GraphTab", "Graph"))
        .SetGroup(WorkspaceCategoryRef)
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));
    InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FNovelChapterEditor::SpawnDetailsTab))
        .SetDisplayName(LOCTEXT("DetailsTab", "Details"))
        .SetGroup(WorkspaceCategoryRef)
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FNovelChapterEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    InTabManager->UnregisterTabSpawner(GraphTabId);
    InTabManager->UnregisterTabSpawner(DetailsTabId);
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

FName FNovelChapterEditor::GetToolkitFName() const
{
    return TEXT("NovelChapterEditor");
}

FText FNovelChapterEditor::GetBaseToolkitName() const
{
    return LOCTEXT("ToolkitName", "Novel Chapter");
}

FString FNovelChapterEditor::GetWorldCentricTabPrefix() const
{
    return TEXT("Novel Chapter ");
}

FLinearColor FNovelChapterEditor::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.12f, 0.42f, 0.55f);
}

void FNovelChapterEditor::SaveAsset_Execute()
{
    if (ChapterGraph)
    {
        ChapterGraph->SyncAllToAsset();
    }

    FDataValidationContext Context;
    if (ChapterAsset && ChapterAsset->IsDataValid(Context) == EDataValidationResult::Invalid)
    {
        FString Message(TEXT("This chapter has validation errors:\n"));
        for (const FDataValidationContext::FIssue& Issue : Context.GetIssues())
        {
            if (Issue.Severity == EMessageSeverity::Error)
            {
                Message += TEXT("\n- ") + Issue.Message.ToString();
            }
        }
        Message += TEXT("\n\nSave the invalid chapter anyway?");
        if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(Message), LOCTEXT("InvalidSaveTitle", "Novel Chapter Validation")) != EAppReturnType::Yes)
        {
            return;
        }
    }
    FAssetEditorToolkit::SaveAsset_Execute();
}

void FNovelChapterEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(ChapterAsset);
    Collector.AddReferencedObject(ChapterGraph);
}

void FNovelChapterEditor::PostUndo(bool bSuccess)
{
    if (bSuccess && ChapterGraph && ChapterAsset)
    {
        ChapterAsset->RebuildNodeLookup();
        ChapterGraph->RebuildFromAsset();
        DetailsView->SetObject(ChapterAsset);
    }
}

TSharedRef<SDockTab> FNovelChapterEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab).Label(LOCTEXT("GraphTabLabel", "Graph"))[GraphEditor.ToSharedRef()];
}

TSharedRef<SDockTab> FNovelChapterEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab).Label(LOCTEXT("DetailsTabLabel", "Details"))[DetailsView.ToSharedRef()];
}

void FNovelChapterEditor::CreateWidgets()
{
    FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.NotifyHook = nullptr;
    DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);
    DetailsView->SetObject(ChapterAsset);

    SGraphEditor::FGraphEditorEvents Events;
    Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FNovelChapterEditor::OnSelectedNodesChanged);
    FGraphAppearanceInfo Appearance;
    Appearance.CornerText = LOCTEXT("GraphCorner", "NOVEL CHAPTER");
    GraphEditor = SNew(SGraphEditor)
        .AdditionalCommands(GraphCommands)
        .IsEditable(true)
        .Appearance(Appearance)
        .GraphToEdit(ChapterGraph)
        .GraphEvents(Events)
        .ShowGraphStateOverlay(false);
}

void FNovelChapterEditor::BindCommands()
{
    GraphCommands = MakeShared<FUICommandList>();
    GraphCommands->MapAction(FGenericCommands::Get().Delete,
        FExecuteAction::CreateSP(this, &FNovelChapterEditor::DeleteSelectedNodes),
        FCanExecuteAction::CreateSP(this, &FNovelChapterEditor::CanDeleteSelectedNodes));
    GraphCommands->MapAction(FGenericCommands::Get().Duplicate,
        FExecuteAction::CreateSP(this, &FNovelChapterEditor::DuplicateSelectedNodes),
        FCanExecuteAction::CreateSP(this, &FNovelChapterEditor::CanDuplicateSelectedNodes));

    const FNovelChapterEditorCommands& Commands = FNovelChapterEditorCommands::Get();
    ToolkitCommands->MapAction(Commands.ValidateChapter, FExecuteAction::CreateSP(this, &FNovelChapterEditor::ValidateChapter));
    ToolkitCommands->MapAction(Commands.FocusTarget,
        FExecuteAction::CreateSP(this, &FNovelChapterEditor::FocusReferencedTarget),
        FCanExecuteAction::CreateSP(this, &FNovelChapterEditor::CanFocusReferencedTarget));
    ToolkitCommands->MapAction(Commands.LegacyConversionHelp, FExecuteAction::CreateSP(this, &FNovelChapterEditor::ShowLegacyConversionHelp));
}

void FNovelChapterEditor::ExtendToolbar()
{
    TSharedRef<FExtender> Extender = MakeShared<FExtender>();
    Extender->AddToolBarExtension(TEXT("Asset"), EExtensionHook::After, GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& ToolbarBuilder)
        {
            const FNovelChapterEditorCommands& Commands = FNovelChapterEditorCommands::Get();
            ToolbarBuilder.BeginSection(TEXT("NovelChapter"));
            ToolbarBuilder.AddToolBarButton(Commands.ValidateChapter);
            ToolbarBuilder.AddToolBarButton(Commands.FocusTarget);
            ToolbarBuilder.AddToolBarButton(Commands.LegacyConversionHelp);
            ToolbarBuilder.EndSection();
        }));
    AddToolbarExtender(Extender);
}

void FNovelChapterEditor::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
    TArray<UObject*> SelectedObjects;
    for (UObject* Object : NewSelection)
    {
        if (Object->IsA<UEdGraphNode_NovelDialogue>())
        {
            SelectedObjects.Add(Object);
        }
    }
    DetailsView->SetObjects(SelectedObjects.IsEmpty() ? TArray<UObject*>({ChapterAsset}) : SelectedObjects);
}

void FNovelChapterEditor::DeleteSelectedNodes()
{
    const FScopedTransaction Transaction(LOCTEXT("DeleteNodesTransaction", "Delete Novel Dialogue Nodes"));
    ChapterGraph->Modify();
    TArray<UEdGraphNode_NovelDialogue*> NodesToDelete;
    for (UObject* Object : GraphEditor->GetSelectedNodes())
    {
        if (UEdGraphNode_NovelDialogue* Node = Cast<UEdGraphNode_NovelDialogue>(Object))
        {
            NodesToDelete.Add(Node);
        }
    }
    GraphEditor->ClearSelectionSet();
    for (UEdGraphNode_NovelDialogue* Node : NodesToDelete)
    {
        Node->Modify();
        Node->DestroyNode();
    }
    ChapterGraph->SyncAllToAsset();
    DetailsView->SetObject(ChapterAsset);
}

bool FNovelChapterEditor::CanDeleteSelectedNodes() const
{
    if (!GraphEditor.IsValid())
    {
        return false;
    }
    for (UObject* Object : GraphEditor->GetSelectedNodes())
    {
        const UEdGraphNode_NovelDialogue* Node = Cast<UEdGraphNode_NovelDialogue>(Object);
        if (Node && Node->CanUserDeleteNode())
        {
            return true;
        }
    }
    return false;
}

void FNovelChapterEditor::DuplicateSelectedNodes()
{
    TArray<UEdGraphNode_NovelDialogue*> SourceNodes;
    for (UObject* Object : GraphEditor->GetSelectedNodes())
    {
        if (UEdGraphNode_NovelDialogue* Node = Cast<UEdGraphNode_NovelDialogue>(Object))
        {
            SourceNodes.Add(Node);
        }
    }
    GraphEditor->ClearSelectionSet();
    for (UEdGraphNode_NovelDialogue* Source : SourceNodes)
    {
        const FVector2f Position(Source->NodePosX + 80.0f, Source->NodePosY + 80.0f);
        if (UEdGraphNode_NovelDialogue* Duplicate = ChapterGraph->AddDialogueNode(Position, &Source->NodeData))
        {
            GraphEditor->SetNodeSelection(Duplicate, true);
        }
    }
}

bool FNovelChapterEditor::CanDuplicateSelectedNodes() const
{
    return GetSingleSelectedDialogueNode() != nullptr;
}

void FNovelChapterEditor::ValidateChapter()
{
    ChapterGraph->SyncAllToAsset();
    FDataValidationContext Context;
    const EDataValidationResult Result = ChapterAsset->IsDataValid(Context);
    FString Message = Result == EDataValidationResult::Invalid ? TEXT("Chapter validation failed.") : TEXT("Chapter validation passed.");
    for (const FDataValidationContext::FIssue& Issue : Context.GetIssues())
    {
        Message += FString::Printf(TEXT("\n\n%s: %s"), Issue.Severity == EMessageSeverity::Error ? TEXT("Error") : TEXT("Warning"), *Issue.Message.ToString());
    }
    FMessageDialog::Open(Result == EDataValidationResult::Invalid ? EAppMsgType::Ok : EAppMsgType::Ok, FText::FromString(Message), LOCTEXT("ValidationDialogTitle", "Novel Chapter Validation"));
}

void FNovelChapterEditor::FocusReferencedTarget()
{
    UEdGraphNode_NovelDialogue* Source = GetSingleSelectedDialogueNode();
    if (!Source)
    {
        return;
    }
    FNovelNodeRef Target = Source->NodeData.Next;
    if (!Target.IsValid() && !Source->NodeData.Choices.IsEmpty())
    {
        Target = Source->NodeData.Choices[0].Target;
    }
    if (UEdGraphNode_NovelDialogue* TargetNode = ChapterGraph->FindDialogueNode(Target.NodeId))
    {
        GraphEditor->JumpToNode(TargetNode, false, true);
    }
}

bool FNovelChapterEditor::CanFocusReferencedTarget() const
{
    const UEdGraphNode_NovelDialogue* Source = GetSingleSelectedDialogueNode();
    if (!Source || !ChapterAsset)
    {
        return false;
    }
    const FPrimaryAssetId ChapterId = ChapterAsset->GetPrimaryAssetId();
    if (Source->NodeData.Next.ChapterId == ChapterId && ChapterGraph->FindDialogueNode(Source->NodeData.Next.NodeId))
    {
        return true;
    }
    return !Source->NodeData.Choices.IsEmpty()
        && Source->NodeData.Choices[0].Target.ChapterId == ChapterId
        && ChapterGraph->FindDialogueNode(Source->NodeData.Choices[0].Target.NodeId);
}

void FNovelChapterEditor::ShowLegacyConversionHelp()
{
    FMessageDialog::Open(EAppMsgType::Ok,
        LOCTEXT("LegacyConversionMessage", "Use UNovelLegacyChapterConverter::ConvertLegacyChapter from an Editor Utility Blueprint. It creates a new chapter asset and returns a structured report; the current chapter is never overwritten."),
        LOCTEXT("LegacyConversionTitle", "Legacy Chapter Conversion"));
}

UEdGraphNode_NovelDialogue* FNovelChapterEditor::GetSingleSelectedDialogueNode() const
{
    if (!GraphEditor.IsValid() || GraphEditor->GetSelectedNodes().Num() != 1)
    {
        return nullptr;
    }
    for (UObject* Object : GraphEditor->GetSelectedNodes())
    {
        return Cast<UEdGraphNode_NovelDialogue>(Object);
    }
    return nullptr;
}

#undef LOCTEXT_NAMESPACE