# First-build troubleshooting

## The script cannot find Unreal

Update the 5.8 installation to **5.8.3 or a later 5.8 hotfix**. The helper checks the engine's `Build.version`, rather than trusting the folder name. For a custom location, open Command Prompt in the project folder and run:

```bat
set "UE_ROOT=D:\Epic Games\UE_5.8"
Setup_and_Open.bat
```

This only sets the variable in that command window. Do not point it to a different major/minor branch and assume compatibility has been validated.

## Missing compiler, SDK, or .NET error

Use Visual Studio Installer to confirm a supported Visual Studio version, C++ workloads, MSVC, and Windows SDK. Follow Epic's compatibility guide in `START_HERE.md`. An installed .NET Framework SDK alone is not a C++ development environment. Read the **first actionable error** in `Saved\Logs\Build-Editor.log`; later errors may be consequences of that one.

## UE 5.8.3 source errors fixed in 0.11.1

If a 0.11.0 build reports `OutLifetimeProps` as undeclared, cannot find `Misc/LexFromString.h`, cannot find `Engine/SkyAtmosphere.h`, or reports C4458 for a local variable named `Owner`, use 0.11.1 or later. Those were source compatibility defects in the initial Unreal package, not missing Windows SDK/.NET components.

## Missing modules / compile error

The supplied project has not been compiled against Unreal here. A C++/UnrealHeaderTool error may indicate a source/API issue, not a problem with your installation. Preserve `Saved\Logs\Build-Editor.log` and the exact engine/toolchain versions. Fix the first reported source error, close the editor, then rerun setup. Do not assume a source error will be repaired by repeatedly installing framework SDKs.

The build helper compiles directly; it does not require a pre-generated `.sln`. To work in Visual Studio, use Unreal's normal Generate Visual Studio project files action after installing the supported toolchain.

## Import fails or meshes are missing

Read `Saved\Logs\Bootstrap.log`. Keep the complete `SourceArt` folder next to the `.uproject`. The importer explicitly selects the static-mesh FBX factory for OBJ, disables axis/unit conversion, and verifies each imported mesh against the centimeter dimensions manifest. Importer property availability and headless material compilation have not been exercised in UE 5.8.3 here. A reported API/property error needs adjustment against the installed editor API.

After correcting an import problem, close the editor and run `Reimport_Source_Art.bat`. It deliberately overwrites the imported art/materials. Do not use this after hand-editing those imported assets without a backup. It does not regenerate source meshes. `Saved\AshenBootstrap.json` records successful bootstrap; absence of that file means setup did not report success.

## Empty scene / wrong map

An empty editor viewport is expected before Play. Load `/Game/Maps/AshenSpiral` and press Play. Its GameMode constructs the hub during gameplay. After the initial successful setup, double-clicking the `.uproject` is appropriate. Opening the `.uproject` before bootstrap can produce missing-map warnings.

## Dark scene, shader compilation or poor performance

Complete material/shader compilation before assessing the view. The default renderer is DX12/SM6 with dynamic lighting. Quality presets 0-3 change scalability and resolution scale. F9 displays frame time and live actor counts; it is diagnostic, not an FPS guarantee. Begin with Medium or Low on constrained hardware.

For diagnosing a DX12 launch failure, an engine-based fallback command is:

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Games\AshenSpiral_UE5_0.11.1\AshenSpiral.uproject" /Game/Maps/AshenSpiral -game -d3d11 -windowed
```

Adjust only the paths to match your installation. DX11 changes rendering capabilities and visual output; this fallback has not been tested. Do not take it as a minimum-hardware or operating-system compatibility certification.

## Save or reward is pending

Use a writable project/user-data location. The prototype waits for an acknowledged local write before advancing through pending rewards. Do not delete save files as a first troubleshooting step. Back up both A/B slots. Editor Play and standalone sessions can intentionally have different profiles. A profile duplicated between two local clients is rejected; use the provided two-player helper for separate slots.

## Network join fails

Check identical version/protocol, reachable IP, UDP 24567, session code, and host still being in the hub (except a reserved reconnect). Codes are not internet matchmaking. See `COOP.md`. Nothing in setup changes your firewall or workplace network policy.

## Packaging fails

Complete editor build and content bootstrap first. Then inspect `Saved\Logs\Package-Windows.log`. The provided packaging target is a Development Win64 build; a packaged output has not been produced or tested here. Share the entire resulting Windows folder after testing it, not just the launcher executable.
