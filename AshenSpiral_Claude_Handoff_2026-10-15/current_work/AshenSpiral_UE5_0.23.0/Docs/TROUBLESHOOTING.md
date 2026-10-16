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

## UE 5.8.3 source errors fixed in 0.11.2

If a 0.11.0 build reports `OutLifetimeProps` as undeclared, cannot find `Misc/LexFromString.h`, cannot find `Engine/SkyAtmosphere.h`, or reports C4458 for a local variable named `Owner`, use 0.11.2 or later. Those were source compatibility defects in the initial Unreal package, not missing Windows SDK/.NET components.

If a 0.11.1 build reports `UnrealString.h.inl should only be included after defining UE_STRING_CLASS`, use 0.11.2 or later. The 0.11.1 compatibility pass incorrectly included an internal `.inl` header directly; 0.11.2 removes that dependency and parses run seeds locally.

## Missing modules / compile error

For `ASWorld.cpp(336)` error **C2668** mentioning ambiguous `FGenericPlatformMath::Cos` or `Sin`, use **0.16.1**. The 0.16.0 fern code passed an integer to float/double overloads; this hotfix explicitly passes a float. The accompanying newer-than-preferred Visual Studio message and engine-header deprecation messages were warnings in the supplied log. They were not the two errors that stopped that build.

The supplied project has not been compiled against Unreal here. A C++/UnrealHeaderTool error may indicate a source/API issue, not a problem with your installation. Preserve `Saved\Logs\Build-Editor.log` and the exact engine/toolchain versions. Fix the first reported source error, close the editor, then rerun setup. Do not assume a source error will be repaired by repeatedly installing framework SDKs.

The build helper compiles directly; it does not require a pre-generated `.sln`. To work in Visual Studio, use Unreal's normal Generate Visual Studio project files action after installing the supported toolchain.

## Import fails or meshes are missing

Read `Saved\Logs\Bootstrap.log`. Keep the complete `SourceArt` folder next to the `.uproject`. The importer explicitly selects the static-mesh FBX factory for OBJ, disables axis/unit conversion, and verifies each imported mesh against the centimeter dimensions manifest. Importer property availability and headless material compilation have not been exercised in UE 5.8.3 here. A reported API/property error needs adjustment against the installed editor API.

After correcting an import problem, close the editor and run `Reimport_Source_Art.bat`. It deliberately overwrites the imported art/materials. Do not use this after hand-editing those imported assets without a backup. It does not regenerate source meshes. `Saved\AshenBootstrap.json` records successful bootstrap; absence of that file means setup did not report success.

## Empty scene / wrong map

An empty editor viewport is expected before Play. Load `/Game/Maps/AshenSpiral` and press Play. Its GameMode constructs the hub during gameplay. After the initial successful setup, double-clicking the `.uproject` is appropriate. Opening the `.uproject` before bootstrap can produce missing-map warnings.

## Blank HUD, missing damage numbers or checkerboard materials

Use the complete 0.17.1 source package in a fresh folder and run `Play_Standalone.bat` so both the module and art material are rebuilt. HUD text now uses a Slate widget rather than Canvas font drawing. It shares that text layer with floating damage numbers; check Settings if numbers were explicitly disabled. The importer enables the material's instanced-static-mesh usage before saving. Allow shaders to finish compiling. The sun and fill now have distinct forward-shading priorities.

These changes have not been rendered in Unreal here. If a menu/HUD remains empty, capture the entire game window and run `Collect_Diagnostics.bat`; include the runtime and bootstrap logs. Do not erase saves to troubleshoot UI rendering.

## Mouse escapes gameplay or cannot leave for a second monitor

Use the complete 0.17.1 package and restart the editor after rebuilding; it registers a new game viewport client. During gameplay the owning local viewport requests permanent relative-mouse capture and confinement. Open Esc or Tab to release capture and locking; close the menu to relock. After switching applications, click back in the game, or press F10 after the game has keyboard focus. An inactive game window must not reclaim focus. In PIE, remove plain Escape from the editor's Stop Play Session shortcut; otherwise the editor can intercept it before the game.

See `INPUT_CAPTURE_0.17.md` for exact code and the host/client test matrix. Test all four edges, with special attention to the left/bottom edges, on a secondary monitor and after resizing. Run the `ASInputStatus` console command and collect logs if the problem persists. The diagnostic should report `ASGameViewportClient` as the viewport class. A different class usually means old configuration/binaries are still in use. Mouse confinement has not been tested in Windows here.

## Runtime black view / clipped menu words (0.11.3)

Use FIX3 or later for the directional-light mobility warning and menu text clipping reported after FIX2. See `FIX3_NOTES.md`. Extract into a fresh folder, run `Play_Standalone.bat`, and verify the 0.17.1 footer. The source now sets mobility before transforms, explicitly disables physical camera exposure for its stylized lighting, and wraps long menu text before layout.

If the issue remains during gameplay, close the game and run `Collect_Diagnostics.bat`. Upload the ZIP under `Saved\Diagnostics` with a screenshot of the entire game window. This includes the runtime `AshenSpiral.log` and `AshenVisual` startup entries, which are more useful for rendering issues than the build log.

## Dark scene, shader compilation or poor performance

Complete material/shader compilation before assessing the view. The default renderer is DX12/SM6 with dynamic lighting. Quality presets 0-3 change scalability and resolution scale. F9 displays frame time and live actor counts; it is diagnostic, not an FPS guarantee. Begin with Medium or Low on constrained hardware.

For diagnosing a DX12 launch failure, an engine-based fallback command is:

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Games\AshenSpiral_UE5_0.17.1\AshenSpiral.uproject" /Game/Maps/AshenSpiral -game -d3d11 -windowed
```

Adjust only the paths to match your installation. DX11 changes rendering capabilities and visual output; this fallback has not been tested. Do not take it as a minimum-hardware or operating-system compatibility certification.

## Save or reward is pending

Use a writable project/user-data location. The prototype waits for an acknowledged local write before advancing through pending rewards. Do not delete save files as a first troubleshooting step. Back up both A/B slots. Editor Play and standalone sessions can intentionally have different profiles. A profile duplicated between two local clients is rejected; use the provided two-player helper for separate slots.

## Network join fails

Check identical version/protocol, reachable IP, UDP 24567, session code, and host still being in the hub (except a reserved reconnect). Codes are not internet matchmaking. See `COOP.md`. Nothing in setup changes your firewall or workplace network policy.

## Packaging fails

Complete editor build and content bootstrap first. Then inspect `Saved\Logs\Package-Windows.log`. The provided packaging target is a Development Win64 build; a packaged output has not been produced or tested here. Share the entire resulting Windows folder after testing it, not just the launcher executable.

## Jump, dash, or revive after upgrading

The current defaults are Space to jump, Shift to dash, and hold V to revive. If saved custom mappings still use Space for dash, open Controls and select RESET JUMP/DASH. In the editor, remap the Play World Stop shortcut away from plain Esc so you can open the game menu; see START_HERE.md.

Both co-op players should use 0.22.0 / protocol 2200. Close menus and hold V for 2 seconds while grounded within 3 meters of the downed player, with a clear path. The HUD should show channel progress. This works during the wave and the room-reward phase. If it fails, record which player was host, which was downed, the room phase, and both runtime logs using Collect_Diagnostics.bat.

## Progress missing after updating

Open Esc → Progress and check the displayed balances and folder. Version 0.12 and earlier saved beside the project; use Import_Progress.bat to select the old A/B save, then REVIEW IMPORT and RESTORE in an offline hub. Future 0.13+ builds use the same per-user folder. PIE, standalone and explicit ProfileSlot sessions can still select different profiles. Do not delete A/B files to fix a mismatch. See EXPANSION_0.13_NOTES.md for recovery and transfer checks.

## Enemies or feedback need inspection

If an enemy remains stuck, record the expedition seed, room/wave, obstacle location and runtime logs. The `AshenNavigation` log records grid dimensions and reachable nodes. This implementation checks capsule-sized spawn points and route edges, then recovers enemies after sustained failed movement; collision and frame-time behavior still require Unreal testing.

Enemy bars appear after damage, hold for three seconds, then fade over two seconds. DOT keeps refreshing that timer while it deals damage. The practice target resets five seconds after its last damage tick. Check the damage-number setting if numbers are hidden. Cosmetic inter-wave spells intentionally do not give healing, shields, crit preparation or well buffs in the next wave.
