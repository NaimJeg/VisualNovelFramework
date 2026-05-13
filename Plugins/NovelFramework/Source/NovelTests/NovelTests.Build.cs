using UnrealBuildTool;

public class NovelTests : ModuleRules
{
    public NovelTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "NovelRuntime"
        });
    }
}
