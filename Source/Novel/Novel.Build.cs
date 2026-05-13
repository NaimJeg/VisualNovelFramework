// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Novel : ModuleRules
{
    public Novel(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "DeveloperSettings",
            "SlateCore",
            "NovelRuntime",
            "NovelUMG"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "InputCore",
            "Slate"
        });
    }
}
