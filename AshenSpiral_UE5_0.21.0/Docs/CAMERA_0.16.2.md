# Camera controls — 0.16.2

Open **Esc → Settings**. Camera sliders are at the top; drag to adjust and close the menu to check your view. Values apply locally and save when you release a slider, navigate away from Settings, or close the menu. Keyboard/controller changes also save on leaving Settings. Mouse-wheel zoom remains available in gameplay; Q mirrors the current shoulder offset. At zero offset the view is centered.

| Setting | Range | Reset default |
|---|---|---|
| Tilt / pitch | −30° to +20° | −8° |
| Camera height | 20–200 cm above the character's pivot | 100 cm |
| Left/right shoulder offset | −220 to +220 cm | +155 cm (right) |
| Distance / zoom | 360–780 cm | 528 cm |
| Field of view | 65–105° | 85° |

Negative pitch looks down; positive pitch looks up. Raise height or increase the shoulder offset to move the character out of the central view. **RESET CAMERA** restores all five defaults and saves them. Spring-arm wall collision and the existing near-wall hero hiding remain in place. A nearby wall can still shorten the requested camera distance.

Each PC uses its own local camera settings. These are separate from combat stats and saved currency. Existing settings without the new fields use the old camera defaults. Values are bounded and non-finite config values fall back to defaults. The camera is reapplied to a newly possessed character; a co-op peer's preferences are not sent over the network.

## Install

Close the game/editor, extract the full 0.16.2 archive to a fresh folder and run `Play_Standalone.bat`. Both players should use the update to see the new controls. Save schema 1 and protocol 1600 are unchanged. This includes the 0.16.1 fern-placement compile fix. Camera preferences live in Unreal's local `GameUserSettings.ini`; they are not part of the portable currency/profile export.

## Check on your PC

1. Open Settings and move each slider; close the menu and aim at the lobby dummy. Confirm the crosshair and shots follow your chosen camera.
2. Repeat near a wall. Check that the spring arm shortens and the local hero hides as before.
3. Set different views on host and P2, then respawn/return to the hub. Neither player's settings should change the other's view.
4. Close normally and relaunch the same installed folder; settings should reload. Verify Q swaps the selected offset and RESET CAMERA restores the defaults.

Unreal Engine is unavailable in the authoring environment. Existing standalone suites and source checks can run here; the new sliders, actual camera rendering and Windows persistence need this in-engine check.

API references consulted for implementation:
- Epic SSlider arguments: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Slate/SSlider/FArguments
- Epic spring-arm component: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USpringArmComponent
