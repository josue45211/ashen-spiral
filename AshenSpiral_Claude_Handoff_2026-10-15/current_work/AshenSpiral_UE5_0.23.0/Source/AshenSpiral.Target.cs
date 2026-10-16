using UnrealBuildTool;
public class AshenSpiralTarget : TargetRules
{
    public AshenSpiralTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AshenSpiral");
    }
}
