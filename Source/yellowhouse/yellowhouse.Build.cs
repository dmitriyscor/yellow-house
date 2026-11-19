using UnrealBuildTool;

public class yellowhouse : ModuleRules
{
    public yellowhouse(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput"
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.AddRange(new[] { "ApplicationCore", "WindowsDualsense_ds5w" });
        }
    }
}
