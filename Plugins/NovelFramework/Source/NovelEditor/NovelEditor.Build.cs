using UnrealBuildTool;

public class NovelEditor : ModuleRules
{
    public NovelEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "NovelRuntime"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd",
            "AssetRegistry",
            "AssetTools",
            "GraphEditor",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "EditorFramework",
            "WorkspaceMenuStructure"
        });
    }
}