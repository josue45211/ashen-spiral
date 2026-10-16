# 0.11.3 FIX3: lighting, startup display settings, and menu text

This patch is based on the complete 0.11.2 FIX2 source. It retains both previous compiler fixes and the existing save format. It targets the user's installed UE 5.8.3 family; it does not require switching engine branches.

## What the evidence shows

The latest supplied screenshot reports that DirectionalLight_0 must be Movable before it can move. In FIX2, `ASWorld::BeginPlay` rotates the sun and only then changes its mobility. Unreal can reject that rotation. FIX3 changes mobility first. Point lights and fog now also change mobility before their location is set.

The screenshot does not include the black game viewport or clipped text itself. Their exact rendered appearance has not been reproduced here. Inspection found two relevant source defects: manual exposure did not explicitly disable photographic camera attenuation, and centered action labels had no wrapping within their 165-unit boxes.

## Changes

- Lighting: move the sun only after it is Movable; explicitly disable physical camera exposure in the manual-exposure volume; add a low-intensity, shadowless fill light so readability does not rely entirely on Lumen or a sky capture. The sky capture is requested after the atmosphere and fog exist. Since the sky does not animate, it no longer requires real-time capture.
- Startup display settings: apply graphics quality and render scale without reapplying fullscreen or window dimensions when the controller starts or a quality preset changes.
- Menus: action columns are wider; centered labels wrap and grow vertically; titles, descriptions, notices, resources, and readiness text wrap at explicit widths before layout. This avoids automatic-wrapping's delayed desired-size calculation inside the scaled panel. Added breathing room for the scrollbar and header.
- Diagnostics: startup logs include the lighting actors, sun rotation, generated geometry count, pawn, view target, camera location, and viewport dimensions. Search the runtime log for `AshenVisual`. `Collect_Diagnostics.bat` gathers the relevant project logs into one ZIP.
- Version labels now consistently identify this patch as 0.11.3.

## Install and check

1. Close all Ashen Spiral editor/game windows. Extract the whole ZIP into a fresh folder.
2. If keeping existing Unreal progress, copy only `Saved\SaveGames` from the previous project into the new project's `Saved` folder. Keep a backup. Existing Godot saves are unrelated.
3. Run `Play_Standalone.bat`. It builds/imports before opening a 1600x900 game window. Alternatively run `Setup_and_Open.bat` and press Play in the editor.
4. Confirm the menu footer says 0.11.3. Close the menu with Esc; check the hub floor, character, lit surfaces, and shaded surfaces. Reopen it and check every label on Expedition, Overview, and Settings. Scroll the controls/upgrades pages. Resize the window and check again.
5. Start a solo expedition and check the first chamber. Try Performance/Balanced/High/Epic; lighting should remain visible and the window should keep its current dimensions. Then use the existing two-player helper if needed.

An empty editor viewport **before Play** is expected because the world is generated at runtime. A black view **during Play or in the standalone game** is not the intended result. Wait for initial shader compilation to finish before evaluating materials.

If the game view is still black or text still clips, close the game, run `Collect_Diagnostics.bat`, and provide the resulting ZIP plus a screenshot of the complete game window. `Saved\Logs\AshenSpiral.log` is the important runtime log; `Build-Editor.log` is for compilation failures.

## Verification boundary

The available standalone rules/source-art suite is rerun for this patch; results are in `Tests/validation_results.json` and `Tests/native_results.txt`. These tests do not execute Slate, Unreal's renderer, the Windows helpers, or UnrealHeaderTool. Unreal Engine and Windows PowerShell are unavailable in the patch environment, so module compilation, visual output, and FPS still require a local run. This patch fixes the identified source defects but is not a claim of a verified playable Windows build.

## API references checked for this patch

- [Epic: Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine): physical camera exposure affects manual exposure and can darken a scene substantially.
- [Epic: STextBlock AutoWrapText](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Slate/STextBlock/FArguments/AutoWrapText?lang=en-US): automatic wrapping can calculate desired size a frame late; explicit WrapTextAt is recommended when appropriate.
- [Epic: USkyLightComponent](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USkyLightComponent?lang=en-US): capture and real-time capture APIs.
- [Epic: UGameUserSettings](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameUserSettings?lang=en-US): non-resolution settings and saving.
