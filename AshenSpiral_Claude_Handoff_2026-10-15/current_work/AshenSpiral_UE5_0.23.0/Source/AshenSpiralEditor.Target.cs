using UnrealBuildTool;
public class AshenSpiralEditorTarget : TargetRules
{
    public AshenSpiralEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("AshenSpiral");
        ExtraModuleNames.Add("AshenSpiralTools"); // 0.24.2: editor-only asset tooling (commandlets)
    }
}
