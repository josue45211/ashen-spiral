using UnrealBuildTool;
public class AshenSpiral : ModuleRules
{
    public AshenSpiral(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "UMG", "Json", "JsonUtilities","OnlineSubsystemUtils"
        });
    }
}
