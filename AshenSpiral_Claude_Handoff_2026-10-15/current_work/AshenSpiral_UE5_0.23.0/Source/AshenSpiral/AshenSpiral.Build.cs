using UnrealBuildTool;
public class AshenSpiral : ModuleRules
{
    public AshenSpiral(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "UMG", "Json", "JsonUtilities","OnlineSubsystemUtils","RenderCore"
        });
        PrivateDependencyModuleNames.Add("AudioMixer"); // 0.24.2: the dev harness records the game's audio output (as.dev.record, non-shipping) to verify the roars by ear-free measurement
    }
}
