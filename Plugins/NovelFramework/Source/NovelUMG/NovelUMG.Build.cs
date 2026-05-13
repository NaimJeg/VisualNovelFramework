using UnrealBuildTool;

public class NovelUMG : ModuleRules
{
    public NovelUMG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "SlateCore",
            "DeveloperSettings",
            "NovelRuntime"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "InputCore",
            "Slate"
        });
    }
}
