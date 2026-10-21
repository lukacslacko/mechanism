using UnrealBuildTool;
public class MaterialHallXR : ModuleRules
{
    public MaterialHallXR(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");
        PrivateDependencyModuleNames.AddRange(new[] { "CoreUObject", "Engine", "OpenXRHMD", "HeadMountedDisplay", "AugmentedReality" });
    }
}
