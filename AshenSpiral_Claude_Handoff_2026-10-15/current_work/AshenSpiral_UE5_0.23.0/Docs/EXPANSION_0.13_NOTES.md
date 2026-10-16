# 0.13.0 expansion update

This is the complete Unreal source project. Both co-op players must update to **0.13.0 / protocol 1300**. Extract into a fresh folder and use `Play_Standalone.bat`; read `START_HERE.md` for setup. The previous jump, revive, stat-upgrade, lighting and menu-wrapping changes are retained.

## Weapons, shop and practice

All six weapons now use local +X toward the target and +Z upright. The bow is held in the left hand with the right hand drawing the string behind the grip; its arrow points toward the target. Bow shots use an arrow mesh. The other weapons have slimmer grips, shaped blades/heads, guards, collars and proportions suited to the character. Arms connect the shoulders, elbows and grip positions. These are articulated static meshes; a full skeletal animation pass is still outside this update.

The hub has six physical displays with pedestals, alternating mounting boards and colored element labels. Approach a weapon and press **E → TRY WEAPON** to borrow it for practice, even if it is locked. Buying/equipping for an expedition remains a separate choice. Starting an expedition restores your purchased loadout. Testing does not unlock the weapon or grant currency.

The practice target near the Sanctum accepts weapon and Spiral damage, crits and elemental effects. It has 10,000 HP, cannot die or award rewards, and resets after five seconds without damage. Burn/poison ticks count as damage and delay the reset. Equip unlocked Spiral styles at the Sanctum and try them on the target. Combat-only proc rewards, such as healing or shields from casting, are disabled in practice.

## Movement, enemies and arenas

**Space jumps; Shift dashes.** One dash is permitted while airborne before landing. The normal cooldown still applies; an airborne dash preserves vertical motion and gravity, so it is not a hover or extra jump. The saved-move path records air-dash state for client prediction; that engine/network behavior requires a live test.

Seeded arenas select among five footprints: compact clusters, long ribbons, cross-shaped corridors, perimeter loops and sparse branches. They contain **10–32 clearings**, with widths/depths of 960, 1,200 or 1,440 cm and independently varied spacing. The exit follows a solid route; optional gold-marked gaps are shortcuts. The minimap fits the generated bounds.

Enemy placement uses a 120 cm navigation grid with floor support, capsule clearance for the largest enemy, swept connections and a flood fill from the player start. A* follows four-way routes around props instead of turning repeatedly into them. Enemies pursue through occlusion, do not physically block each other, and recover to a reachable node after sustained lack of movement. These rules are implemented; imported collision behavior and runtime cost have not been measured in Unreal.

## Co-op and inter-wave attacks

Each player still makes their own reward choice. Once **both choices and save acknowledgments** are complete, **either player, including P2, presses E at the chest** to continue. Distance, phase and downed-state checks remain authoritative. Waves inside a room continue automatically. Initial departure and returning the party to the hub remain host actions.

Attacking and casting remain available in countdowns and room rewards. Effects created there are permanently marked cosmetic: they cannot damage next-wave enemies or grant healing, shields, prepared crits, haste, projectile/well interactions, cooldown refunds or other on-hit/on-cast benefits when combat resumes. Legitimate combat-created effects retain their existing lifetime; lingering-well carry is restricted to actual combat wells. Normal attack/ability cooldowns still run between waves.

## Elements and feedback

These are initial tuning values with bounded stacking and boss restrictions, not a claim of completed live balance testing. Direct weapon hits apply their element. Damage-over-time scales with damage upgrades, cannot crit and does not recursively trigger on-hit effects.

| Weapon | Element | Initial behavior |
|---|---|---|
| Cinder Blade, Ember Staff | Burn | 6 damage/sec for 3 sec at base power. Refreshes duration, does not stack; retains the stronger active burn. |
| Ash Bow, Rift Scythe | Poison | 3 damage/sec per stack for 4 sec; maximum 3 stacks. Further hits refresh duration. |
| Grav Hammer, Spiral Glaive | Frost | +34 buildup per hit; decays 12/sec. Chills normal enemies by 35% and bosses by 25%. |

At 100 frost buildup, normal enemies freeze for 0.8 sec, with a 3.8 sec immunity window measured from the trigger. Buildup resets to 70. Bosses can be chilled but never hard-frozen. Existing stronger slows take precedence.

| Feedback | Appearance |
|---|---|
| Physical / Spiral | White / violet |
| Burn / poison / frost | Orange / green / cyan |
| Health damage received / shield absorbed | Red / blue, near the player's health card |
| Critical hit | Larger typed number, gold CRIT label, diamond marks, crosshair flash and world burst |

The health card includes a large green health bar, a trailing damage segment, numeric HP and a separate blue shield bar. Normal enemy bars are 58 logical HUD pixels wide; elites use 76. They remain fully visible through three seconds after damage, fade over the next two seconds, and are hidden at five seconds. Another hit restarts the timer. Bars are limited by distance and line of sight, with small status pips. Bosses retain their separate top bar. The practice target bar stays visible.

## Keep currency between versions

Permanent currency and purchases now use the platform user-settings folder under `AshenSpiral/Profiles`, independent of the extracted version folder. **Esc → Progress → OPEN FOLDER** opens the exact location. This is local persistence on the same computer; it is not cloud synchronization.

For a first move from 0.12 or an earlier Unreal build:

1. Close other game/editor instances. Keep the old project and its saves.
2. Extract the complete new ZIP. Run **Import_Progress.bat** and select an old `Saved/SaveGames/AshenUE_Profile..._A.sav` or `_B.sav`. The script copies the chosen file and its companion when present, then launches this build. It does not alter the old saves.
3. In the offline hub, open **Esc → Progress → REVIEW IMPORT**. You may also paste an existing save/export path directly on that page. When both compatible A/B files are readable, the review chooses the newer revision.
4. Verify the displayed old/new balances and selected source. Choose **RESTORE**. The game exports your current profile first, replaces it with the reviewed profile, writes a verified save and reloads the offline hub.
5. Check Ash, Relic Embers, Tablets, materials, Skill Points and unlocks. Restart once to confirm they reload before discarding any old folder.

Future 0.13+ copies use the same location and profile slot automatically. **EXPORT** creates a portable `.ashenprofile` in the Exports subfolder; copy that file to move between PCs or keep a backup. Restore replaces the profile; currencies are not added together. Original Godot saves are unsupported.

The saved profile includes currencies, owned weapons, permanent Hearth/talent upgrades, unlocked pools, selected loadout and recorded progress. Cinders, active expeditions, chest stat ranks, relics found during a run and transient effects remain run-only. Standalone, PIE and explicit `-ProfileSlot` sessions may use different profiles; the local two-player launcher uses 90 and 91. Import/export can transfer a chosen profile between modes when desired.

Alternating A/B copies retain a prior revision. Writes use a temporary file with readback validation before replacing the selected slot. The game also attempts project-local recovery copies. If no stored copy can be read, automatic saves stop so the files are preserved. Explicit restore backs up those files. If one selected import is damaged, try its companion. A stale-revision check detects another instance that already saved; close other copies using the same slot to avoid simultaneous writes.

## Checks to run in Unreal

The native/source checks in `VALIDATION.md` passed. They do not execute UnrealHeaderTool, UE C++ modules, Slate, the art importer, movement physics, SaveGame I/O, Windows helpers or live replication. Complete these acceptance checks locally:

1. Build/import, verify the 0.13.0 footer and inspect all six held/displayed weapons. Fire the bow, switch camera shoulders, move and attack while looking for hand/body intersections. Confirm menus remain legible at your normal viewport size.
2. Jump then dash ascending and descending as host and client. Check one airborne dash, landing reset, normal cooldown, walls, gaps and pause/resume. Repeat with representative network latency.
3. Try several seeds and all four wards. Follow enemies spawning near shelves, pillars, crystals and trees; include bosses on bridges. Check navigation logs, stalled waves and frame time at the busiest wave.
4. Try each shop weapon, including a locked one. Damage/reset the practice target with attacks, Spiral, DOT and frost. Confirm no purchases, drops, currency or expedition equipment changes occur from a free trial.
5. Verify burn refresh, poison cap, frost expiry/immunity and boss chill. Inspect outgoing/incoming numbers, shield absorption, crit effects and bar visibility at 0/3/4/5 seconds after the final damage tick.
6. Let P2 claim first, attempt to advance before P1, then finish both choices and advance with P2. Repeat with delayed save acknowledgment and after a revive in Reward.
7. Spam attacks/Spiral/dash effects between waves with proc relics equipped. Start combat while those visuals remain: verify no stored damage, healing, shields, prepared crits, haste, projectile splitting or refunds from cosmetic effects.
8. Import a backed-up earlier profile, restart, then launch a separately extracted copy using the same mode/slot. Check all balances and purchases. Export/restore, check A/B fallback with backed-up test files, and verify failure messages for an unwritable destination. Test slots 90/91 separately from your main profile.

Use `Collect_Diagnostics.bat` if a local test fails and preserve both players' logs for co-op issues. Record the seed, room/wave, phase, weapon and engine version with the report.
