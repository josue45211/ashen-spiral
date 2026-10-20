using UnrealBuildTool;
// 0.24.2: editor-only tooling (commandlets that build assets the game uses, such as the hero's animation blueprint). Never part of a packaged game.
public class AshenSpiralTools : ModuleRules
{
    public AshenSpiralTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph", "Kismet", "AssetTools", "AssetRegistry"
        });
    }
}
