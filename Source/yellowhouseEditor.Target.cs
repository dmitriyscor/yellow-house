using UnrealBuildTool;

public class yellowhouseEditorTarget : TargetRules
{
    public yellowhouseEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("yellowhouse");
    }
}
