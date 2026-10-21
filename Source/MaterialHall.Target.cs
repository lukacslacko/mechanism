using UnrealBuildTool;
public class MaterialHallTarget : TargetRules
{
    public MaterialHallTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("MaterialHall");
    }
}
