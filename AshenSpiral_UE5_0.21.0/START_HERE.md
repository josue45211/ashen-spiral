# Ashen Spiral - Third-Person Unreal Remake
## Version 0.21.0 / Unreal Engine 5.8.3 / Windows source prototype

This update fixes multiple air dashes, removes every melee reload, adds held three-hit Blade/Hammer combos on corrected grips, gives distant enemies a bounded approach boost, and adds the Forgotten Archives visual pass. See `Docs/DESIGN_0.21.md` for tuning and the local acceptance test. The referenced P2 pack was unavailable; Archives follows the written brief. The broader animation overhaul remains in 0.22.

**R** reloads ranged weapons; clicking the controller's **left stick** does the same. The last ranged round starts reload automatically. Blade, Hammer and Scythe never reload; hold Attack to chain full swings. **Hold Shift while hovering a reward card** to see exact effects and current/next values. The stakes checkbox remains selected until you uncheck it; co-op needs both players' votes. Better rarity weights are earned after clearing the harder chamber.

The earlier 5–10 room maps, three firearms, 25/50/75/100% HUD sizing, camera controls, stacked fading text, color/crosshair settings and owning-viewport mouse capture remain included. Esc/Tab releases the cursor into a menu; closing the menu captures it again. F10 requests capture manually. The complete editable catalog is `Docs/ASHEN_SPIRAL_0.21_EDITABLE_CATALOG.txt`.

### Updating from an earlier Unreal build

On first load, retired Treasure Sense refunds its one Skill Point; Crystal Shards and Vine Fiber convert 1:1 into Scrap. Old earned career milestones receive one-time Ember/Tablet catch-up rewards. Existing A/B save verification commits the migration. Keep your old export as usual and do not run old/new builds simultaneously against the same profile. Character & Achievements lets you equip an earned title.

Close Unreal. Extract the complete 0.21.0 ZIP to a new folder such as `C:\Games\AshenSpiral_UE5_0.21.0`, then run **`Play_Standalone.bat`** to build, import, and launch directly into the game. `Setup_and_Open.bat` remains available if you prefer the editor; press Play after it opens. Keep the old folder until you have checked the new build. For a one-time move from 0.12 or earlier, run **`Import_Progress.bat`**, select an old `Saved\SaveGames\AshenUE_Profile..._A.sav` or `_B.sav`, then use **Esc → Progress → Review Import → Restore** in the offline hub. The review selects the newer compatible A/B copy when both exist and shows the balances before replacement. Your current profile is backed up first. Version 0.13 and this update use the same per-user progress folder and unchanged profile schema. Do not copy old Binaries, Intermediate, or the entire Saved folder into the new build. Both co-op players must update to 0.21.0 (protocol 2100).

The menu footer should say **0.21.0**. Check **Space = jump**, **Shift = dash** and **F = equipped active ability**, **Tab = your build**, **mouse wheel = zoom**. If old movement bindings remain, open **Controls → RESET JUMP/DASH**; the active ability and build menu can also be rebound there. Open **Your Build** for your acquired upgrades and equipment and **Settings → Borderless Fullscreen** to fill the desktop in standalone play. Check the restored text/damage numbers, raised shoulder camera, unlocked menu cursor, and the new **Achievements** page. The prior shop and practice features remain included. See `Docs/DESIGN_0.21.md` for this update and the local acceptance checklist, and `Docs/INPUT_CAPTURE_0.17.md` for mouse behavior, and `Docs/UPGRADE_CATALOG.md` for all 71 selectable stat/trait definitions.

**Start here, not by opening the .uproject directly on the first launch.**

This is a new native Unreal C++ implementation based on your supplied Ashen Spiral v0.10.1 HOTFIX. It contains gameplay source, original source art, configuration, import/build helpers, and tests. It is **not a precompiled Windows game**. Unreal Editor was unavailable during creation: Unreal compilation, art import, rendering, physics, networking, packaging, and frame rate have NOT been verified. Source checks and standalone rule tests passed; see `Docs/VALIDATION.md`.

## 1. Install the tools

Install **Unreal Engine 5.8.3** through Epic Games Launcher. The project associates with the **5.8** engine family; that is normal for a 5.8.3 project. The setup script requires patch 3 or later in that family, rather than silently choosing another engine branch.

Install **Visual Studio 2022 version 17.14 or later**, or **Visual Studio 2026 version 18.0 or later**, with **Game development with C++**, **Desktop development with C++**, the MSVC toolchain, and a Windows SDK. Epic's current compatibility table lists SDK **10.0.22621.0 minimum**, with **10.0.26100 or newer recommended**. Epic recommends VS 2026 for general UE 5.8 development; an existing supported VS 2022 installation is also suitable. Follow Epic's linked setup guide for its complete workload/prerequisite list. Installing the .NET Framework 4.8 SDK alone does not supply a C++ compiler or Windows SDK.

Sources checked September 24, 2026:
- Epic's 5.8.3 hotfix announcement, September 22: https://forums.unrealengine.com/t/5-8-3-hotfix-released/2833315
- Epic's Visual Studio compatibility and setup guide: https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine

No separate Python installation, Blender, paid art pack, Marketplace download, or external game-service account is required by the normal setup script. It uses Unreal's included Python interpreter. Unreal and Visual Studio must be installed separately; they are not included in this ZIP.

## 2. Extract and build

Extract the **entire ZIP** into a writable local folder, for example:

`C:\Games\AshenSpiral_UE5_0.21.0`

Avoid running inside the ZIP, Program Files, a network share, or a cloud-synchronized folder. Preserve the `SourceArt`, `Source`, `Config`, and `Tools` subfolders.

Close any already-open Ashen Spiral editor instance. Double-click **`Setup_and_Open.bat`**. It is designed to:

1. Find the installed UE 5.8.3 engine and compile `AshenSpiralEditor`.
2. Import the bundled OBJ, PNG, and WAV files, create the material, and save the map.
3. Check imported mesh dimensions, write a success marker, and open Unreal Editor.

**The map is intentionally empty in the edit viewport. Press Play.** The native GameMode generates the hub and subsequent rooms at runtime. The default map is `/Game/Maps/AshenSpiral`.

The first build/import must succeed before the project can run. These helper scripts have not been executed in Windows here. A failure is reported in the console and `Saved\Logs`; it is not silently marked successful. See `Docs/TROUBLESHOOTING.md`.

## 3. Start an expedition

The expedition menu should open after entering the hub. Choose a seed and start a solo expedition. For co-op, use the Party page first; both players must be ready before the host departs.

Waves begin automatically after a countdown. Clear the enemies, follow the world-space arrow/ground chevrons and gold minimap reward marker, and press **E** at the chest. Each player chooses their own reward. After both claims and save acknowledgments, **either player**, including P2, interacts again to advance. After the final chamber there is no temporary upgrade choice: approach the teal portal and press **E** to return together once saves complete. A run spans four wards, eight chambers each. Each chamber contains 5–10 connected exploration rooms. Larger rooms occupy 2×2, 2×3, 3×2 or 3×3 base-cell footprints; 2×2 has four times the area of one small room.

Press **Esc** for menus. In the hub, visit the Armory, Sanctum, Ashen Hearth, and Ione's Workbench to change your loadout and buy permanent upgrades. **Space jumps; either Shift key spends an available dash charge on the ground or in the air.** Airborne uses are limited to total charge capacity until landing. Normal charge recharge and gravity still apply. Gold-marked gaps are optional shortcuts; solid paths still connect every clearing.

Walk to a displayed weapon in the hub and press **E → TRY WEAPON**. Practice is free, including locked weapons, and the expedition still uses your purchased/equipped loadout. Attack the target near the Sanctum to test weapons, Spiral and elements; it resets after five seconds without damage and grants no rewards.

You can continue attacking and casting during wave countdowns, room rewards and the final victory phase. Jump, dash and movement remain available after room 32; a downed partner is restored to at least 1 health on victory. These casts stay cosmetic and cannot grant on-hit/on-cast buffs or carry damage into the next wave.

Room chests mix damage, survival and modifier options, with later choices weighted toward your build. Cooldown and attack speed receive extra early-run weight. You start with one personal reroll. Choose one reward per room; inspect trait caps and evolution previews before picking. At the same menu, each player can check Raise the Stakes for a harder next chamber. The saved checkbox persists until manually unchecked; both players must opt in. Rare and legendary trait weights increase after that harder chamber is cleared. Permanent hub upgrades persist between runs; chest stats and traits last for the current run and are included in reconnect snapshots.

Choose one of six free active abilities in the Sanctum and test it on the lobby dummy. F uses the equipped ability; all use Focus, which regenerates during combat or lobby practice. Mouse-wheel zoom is available during gameplay; Q switches camera shoulder. Settings offers 25/50/75/100% overall HUD size, 100%/120% HUD text size, 80/100/120/140/160% damage-number size, and crosshair shape/color/size. Character color is saved with your profile and shown to both players; minimap identities remain P1 cyan and P2 white.

Room 2 and every fourth room afterward contain an optional Rune Vault. Either player can approach each sigil within 3.2 meters and press E during countdown, combat or after the clear; standing together is not required. Aim at a sigil for targeting brackets, activate both, then interact with the cache. The two sigils disable that vault's dedicated flame and blade traps. Every third chamber has an optional Ember Bargain shrine: spend current health for one personal upgrade. Loot crystals and explosive casks can be broken during combat.

To revive a downed co-op partner, close your menu and **hold V for 2 seconds within 3 meters**, with a clear path and your feet on the ground. Watch the progress bar. This also works after the room is cleared, so both players can claim their rewards.

## Default controls

| Action | Keyboard / mouse | Controller binding |
|---|---|---|
| Move / look | WASD / mouse | Left / right stick |
| Attack | Left mouse | Right shoulder |
| Reload ranged weapon | R | Left stick click |
| Detailed card information | Hold Shift while hovering | Keyboard modifier |
| Charge and release Spiral | Right mouse | Face-left / X |
| Equipped active ability | F | Right trigger |
| Inspect acquired build | Tab | Select / View |
| Camera zoom | Mouse wheel | Mouse wheel only by default |
| Jump | Space | Face-bottom / A |
| Dash | Left or right Shift | Face-right / B |
| Interact | E | Face-top / Y |
| Revive nearby partner | Hold V | Hold left shoulder |
| Switch camera shoulder | Q | Right stick click |
| Menu | Esc | Start |
| Performance overlay | F9 | Keyboard only by default |
| Re-lock gameplay mouse | F10 | Keyboard only by default |

Hammer: hold Attack for left/right/charged jump-slam; no release charge or reload. Blade: hold Attack for cut/cut/lunge. Attack speed scales each full melee cycle. Air dashes can spend all available charges up to capacity before landing. Spiral: release during the gold timing window for a Perfect Spiral. The base window is 0.70-0.92 seconds and can widen with upgrades. Rebinding is available on the Controls page. Controller menu focus/navigation still needs in-engine acceptance testing.

## Keep Esc from stopping an editor play test

Open **Edit → Editor Preferences → Keyboard Shortcuts**, search **Stop**, and find **Play World (PIE/SIE) → Stop**. Clear Escape with the **X**, or replace it with **Shift+Esc**. Check both shortcut slots so neither still contains plain Escape. Click outside the field to save. You can still end a test with the editor's Stop button; Esc is available to the game menu. `Play_Standalone.bat` also launches outside the PIE viewport.

Epic reference: https://dev.epicgames.com/documentation/unreal-engine/customizing-keyboard-shortcuts-in-unreal-engine?lang=en-US

## Other launch helpers

| File | Purpose |
|---|---|
| `Play_Standalone.bat` | Build/import as needed and launch a standalone game window using the installed editor. Still requires Unreal. |
| `Import_Progress.bat` | Select an earlier Unreal save/export and copy it into this build for in-game review before restoring. |
| `Reimport_Source_Art.bat` | Force reimport the already-bundled source art and recreate the material. Does not regenerate OBJ files. |
| `Test_Two_Players.bat` | Launch separate local host/client processes with disposable profile slots 90 and 91. Not a substitute for two-PC testing. |
| `Package_Windows.bat` | Build and cook a Development game into `Builds\Windows`. Share the entire output directory, not only its executable. |
| `Collect_Diagnostics.bat` | Copy this project's runtime/build/import logs into `Saved\Diagnostics\AshenSpiral-Diagnostics-<timestamp>.zip` for troubleshooting. |

## Saves and original project

Permanent progress is stored outside the extracted project under the platform user-settings directory's `AshenSpiral/Profiles` folder. Open **Esc → Progress → OPEN FOLDER** to see the exact location. Ash, Relic Embers, Tablets, materials, Skill Points, unlocks and permanent upgrades follow the same profile across future versions on this computer. Character color, XP and achievement counters persist too; older profiles start their cosmetic career at level 1. Cinders remain run-only except for a capped departure conversion: up to 10% in ten-Cinder units, at ten Cinders per permanent Ash, maximum three Ash per player per cleared chamber. Active expeditions, chest stat ranks and temporary effects remain run-only. This is local saving; moving to another PC requires an export/import.

The game alternates `AshenUE_Profile..._A.sav` / `_B.sav` copies and verifies writes before accepting purchases. It also attempts a project-local recovery copy. **EXPORT** creates a portable `.ashenprofile`; **REVIEW IMPORT** shows old/new balances, and **RESTORE** backs up and replaces the current profile rather than adding currencies together. Keep the old project until you have confirmed the new balances. Close other copies using the same profile before restoring or playing.

Standalone, editor PIE and explicit `-ProfileSlot` sessions can intentionally use separate profile names. The two-player development helper uses slots 90 and 91. If currency seems missing, check which slot/mode you used; import that profile through the Progress page if you want to transfer it. Compatible old project-local saves can also migrate automatically on first launch when no readable shared save exists.

The original Godot ZIP is unchanged. **Godot saves are not imported.** Unreal SaveGame I/O, Windows file picking and actual cross-version recovery still need a local engine test; see `Docs/EXPANSION_0.13_NOTES.md`.
