using UnrealBuildTool;
public class MaterialHall : ModuleRules
{
    public MaterialHall(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "Json", "ApplicationCore", "RHI", "RenderCore", "HeadMountedDisplay", "XRBase", "UMG" });
        if (Target.Platform == UnrealTargetPlatform.Win64)
            PrivateDependencyModuleNames.AddRange(new[] { "OpenXRHMD", "MaterialHallXR" });
    }
}
