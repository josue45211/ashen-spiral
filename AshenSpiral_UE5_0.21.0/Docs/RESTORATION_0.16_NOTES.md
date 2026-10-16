# Ashen Spiral 0.16.0 — Restoration and character progression

UE 5.8.3 source update. Extract into a fresh folder and run `Play_Standalone.bat`. Both co-op players need 0.16.0 / protocol 1600. Existing shared profile locations and currency fields remain unchanged. The package includes source and original art; Unreal compilation and live playtesting are pending.

## Interface, camera and input

HUD labels, floating damage numbers, achievement notices and lobby balances now render through a dedicated Slate text widget, replacing Canvas font drawing. Bars, outlines and the crosshair still use Canvas. The text widget attaches again if local HUD ownership arrives after BeginPlay. Menus retain their existing Slate layout and wrapping. HUD text size remains adjustable in Settings; Slate renders at output resolution independently of the 3D quality scale.

The spring-arm target is raised 100 cm, the shoulder offset is widened to 155 cm, and the camera has an 8-degree downward sightline. Default zoom remains 528 cm, with mouse-wheel adjustment from 360 to 780 cm. Camera position lag is disabled so collision recovery does not leave the view trailing behind cover. When a wall pushes the camera within 190 cm of the upper body, the local player's armor and equipment hide for that camera and restore when clear. The partner's view is unaffected. Center-screen aim, muzzle collision feedback and shoulder swapping remain available.

Gameplay explicitly captures and confines the mouse, including the first mouse-down, and recaptures if focus returns without capture. Esc/Tab menus explicitly release capture and viewport locking, allowing movement to a second monitor. An inactive window does not reclaim focus. For editor viewport play, unbind plain Escape from **Editor Preferences → Keyboard Shortcuts → Stop Play Session** so the editor does not intercept it before the game. `Play_Standalone.bat` avoids that editor shortcut entirely.

The supplied screenshots also showed checkerboard world materials and a competing-directional-light warning. The import script now enables instanced-static-mesh material usage and recompiles/saves the material; the two directional lights have different forward-shading priorities. Confirm the result locally after import completes.

## Solid bodies and pursuit

Enemy capsules now block players and other enemies; neither side can step onto the other character's capsule. Dashes use the existing swept CharacterMovement path and stop at an occupied body. No new collision damage is added. Existing telegraphed melee, charge and leap attacks retain their damage.

Navigation returns the player directly when a supported capsule-clear path exists. Obstructed routes use clear lookahead nodes. A failed route holds position instead of retreating to a spawn point. Local separation steers sideways around other enemies without reversing the pursuit direction. Emergency recovery is limited to sustained stalls more than six metres from the player and checks destination occupancy. New spawns and summoned minions select unoccupied, supported navigation positions sized for the largest enemy.

These are capsule-based character hitboxes and mesh-matching static collision. Imported collision, narrow passages, enemy crowds and network correction still require in-engine testing.

## Biomes and environmental reference studies

The cave now lasts for the complete third ward, including its boss. Visual themes are independent of the four existing boss/material progression wards.

| Chambers | Theme | Changes |
|---|---|---|
| 1–8 | Archives | Barrel-vaulted interiors and passages, warm lanterns, shelves and architectural columns |
| 9–12 | Obsidian Gardens | Hexagonal basalt columns, dark boulders and restrained mineral clusters |
| 13–16 | Catacombs | Enclosed masonry vaults and passages, layered skull/bone niches, warm light |
| 17–24 | Luminous Caverns | Continuous rough ceilings over every chamber and connection, rock walls, upward stalagmites, downward stalactites, warm lanterns and cool crystal accents |
| 25–32 | Verdant Reliquary | Natural floor surfaces, fallen nurse logs, fern clusters and tree canopy |

Crystals are environmental formations; enemy head crystals are removed. Cave ceilings no longer depend on a short four-chamber theme range or an instance distance cutoff. Door and passage clearance is kept above the navigation capsule. Eight new original meshes bring the source kit to 57 meshes. This is a stylized natural-form pass, not a photorealistic asset pack.

Research included image searches for cave interiors/flowstone, vaulted libraries, ossuaries, basalt formations and rainforest ruins/nurse logs. Primary references, reviewed October 9, 2026:

- [NPS — Cave formations and photographs](https://www.nps.gov/subjects/caves/speleothems.htm): ceiling dripstone, floor deposits and irregular mineral surfaces informed separate ceiling/floor formations.
- [Trinity College Dublin — Long Room photographs](https://www.tcd.ie/teaching-learning/academic-integrity/): timber vault and tall shelving proportions informed the archive architecture.
- [Paris Catacombs — The ossuary](https://www.catacombes.paris.fr/en/history/ossuary): structured bone walls and architectural features informed the catacomb niches.
- [NPS — Devils Postpile geologic formations](https://www.nps.gov/depo/learn/nature/geologicformations.htm): columnar basalt informed the hexagonal rock kit.
- [NPS — Hoh Rain Forest](https://www.nps.gov/olym/planyourvisit/visiting-the-hoh.htm): ferns, mossy canopy and nurse logs informed the verdant ground layer.

No reference photographs, third-party meshes or source-game art are bundled. All new geometry is generated by `Tools/generate_restoration_art.py`.

## Cosmetic character levels

Level 1 starts at 0 XP. Reaching level 2 takes 100 XP; each subsequent level requires 25 more XP than the last: 125, 150, 175, and so on. Levels change only the displayed title/progress, with no stat bonuses or unlocks. Your level is shown in the lobby, your partner's card and above the other lobby character. The complete progress page is available from **Esc → Character & Achievements** or **Tab → Achievements**.

| Event | XP | Credit |
|---|---:|---|
| Normal / elite / boss kill | 5 / 25 / 100 | Player credited with the final hit or damage-over-time kill |
| Chamber clear | 40 | Each connected player |
| Successful revival | 15 | Reviving player |
| Rune Vault cache | 15 | Each connected player |
| Explosive cask detonation in combat | 2 | Player who triggers it |
| Expedition victory | 150 | Each connected player |

Room, vault and end-of-run receipts persist XP and achievements with the existing verified-save acknowledgment and duplicate-receipt protection. Returning to the hub flushes unbanked career progress and waits for pending saves. Reconnect snapshots retain pending career counters. Hub practice and cosmetic casts between waves do not farm career progress. Abruptly closing the host before a receipt is saved can still lose unbanked progress; this is not a live mid-room autosave or cloud account system.

Older profiles retain currencies/unlocks and start their new career at level 1, with zero new achievement counters. Old lifetime kills are not reconstructed. Keep a profile export before switching back to an older game version, since older code does not know the new fields.

## Achievements

Twelve families have three progressively harder milestones each: **36 milestones** total. Rewards are cosmetic achievement entries and notices; no hidden currency or stat inflation is attached.

| Achievement | Feat | Tier I / II / III |
|---|---|---|
| Ash Hunter | Enemy kills | 100 / 1,000 / 5,000 |
| Giant Slayer | Elite final hits | 10 / 50 / 250 |
| Sovereign Breaker | Boss final hits | 1 / 10 / 50 |
| Wayfarer | Chambers cleared | 10 / 100 / 500 |
| Untouched | Chambers without health or shield damage | 1 / 10 / 50 |
| Perfect Rhythm | Perfect Spiral releases during combat | 25 / 100 / 500 |
| Lifeline | Successful co-op revivals | 1 / 10 / 50 |
| Vaultkeeper | Rune Vault caches recovered with the party | 1 / 10 / 50 |
| Demolitionist | Explosive casks detonated during combat | 10 / 50 / 200 |
| Spiral Conqueror | Complete expedition victories | 1 / 5 / 20 |
| Alchemist | Kills with at least two distinct active statuses | 25 / 100 / 500 |
| Quick Work | Chambers cleared in 90 seconds or less, including countdowns | 1 / 10 / 50 |

Freeze and its frost buildup count as one status for Alchemist. Flawless and timed clears require participation from the chamber start; reconnect preserves eligibility and damage history.

## Leftover Cinders and the final portal

When leaving a cleared chamber, up to 10% of the remaining shared Cinders is banked in ten-Cinder units. Ten Cinders convert to **one permanent Ash per player**, capped at **three Ash per player per chamber**. Shopping and both reward choices happen first. The cap keeps saving Cinders from overwhelming the existing permanent economy.

| Shared balance at departure | Cinders spent | Permanent Ash per player | Cinders carried forward |
|---:|---:|---:|---:|
| 99 | 0 | 0 | 99 |
| 100 | 10 | 1 | 90 |
| 250 | 20 | 2 | 230 |
| 300 | 30 | 3 | 270 |
| 1,000 | 30 | 3 | 970 |

Each chamber converts only once, and conversion receipts must be acknowledged before advancing. Repeated interactions/retries do not award it again. An uncleared chamber or defeat does not perform an additional conversion.

The final chamber gives its permanent clear/victory rewards and the capped conversion without rolling another temporary run upgrade. A teal return portal replaces the chest. After victory, approach it and press **E**; either player can return the party together. The transition waits for pending saves and keeps the co-op session connected. Any remaining run-only Cinders end with the expedition.

## Local acceptance checklist

1. Build and import from the fresh extraction. Verify the 0.16.0 footer, 57 mesh imports, readable menu/HUD text and correct world materials. Save the build/import log if anything fails.
2. At 1080p and the TV's native resolution, inspect every menu, lobby balances, Tab build, character level and achievement page. Test the HUD text scale with long labels.
3. Hit the dummy and enemies with all weapon types. Verify damage numbers, distinct elemental colors, crit feedback, health/shield damage numbers and the enemy-bar hold/fade timing.
4. Aim level, then up/down, close to a wall and pillar on each shoulder and at both zoom limits. The character should hide only locally when the camera compresses into it; verify it returns afterward.
5. Test each PC's mouse separately: fast movement and clicks at every window edge during play; Esc to the second monitor; close the menu; Alt-Tab away and back. The inactive process must not steal the mouse.
6. Walk/dash into one enemy and a dense pack. Bodies block instead of stacking, there is no new bump damage, and a genuine gap remains usable. Test both host and client.
7. Travel several connected clearings away from a wave, including behind walls and around gap shortcuts. Enemies should follow the valid detour. Check spawn markers do not leave enemies inside occupied props/bodies.
8. Inspect each theme and all eight cave chambers, including connecting passages, doors, high camera angles and the ward boss. Ceiling shells should remain present; no enemy should wear a crystal growth. Check roof/formation collision and brightness.
9. Earn XP through kills and clears, cross a level threshold, return to the lobby and restart from a second extraction using the same profile slot. Both peers should see the saved level. Confirm hub practice does not award XP/feats.
10. Test achievement thresholds and simultaneous unlock notices, shield-only damage invalidating Untouched, status combinations and both revive roles. Confirm rejoining preserves pending progress and eligibility.
11. Observe balances around 99/100/250/300 Cinders at departure. Each player should receive the same capped Ash award once. Retry interaction and test a temporarily unwritable save folder; advancement must wait and then resume after a verified save.
12. Complete the last room: no upgrade-choice menu should open. Permanent rewards save, either player can use the portal, the party stays connected, and the next run begins normally. Also test host return during combat while a save is pending.

Five native suites and source/art checks passed. They do not execute Unreal rendering, collision, input, networking or SaveGame I/O. See `VALIDATION.md` and `Tests/validation_results.json` for exact results and limits.
