# Ashen Spiral 0.15.0 — Frontier update

## Install and keep progress

Extract the whole ZIP into a fresh folder, close the old editor/game, then run `Play_Standalone.bat`. Both players need 0.15.0 / protocol 1500. This is a UE 5.8.3 source project, not a compiled Windows executable. The setup imports 49 original meshes. Do not copy old Binaries or Intermediate folders. `Reimport_Source_Art.bat` is available if updating generated art in an existing project.

The per-user `AshenSpiral/Profiles` path, currency fields and schema 1 are preserved. Older saves default to Rune Lance when the new optional Ability field is absent. Keep the same profile slot and play mode: editor PIE and the two-player test helper can use different profiles. For an older slot/computer, use the existing export/review/import flow. Keep a backup until a local engine test confirms your balances. Save compatibility has been reviewed in source; Unreal SaveGame deserialization has not run here.

## Controls, camera and legibility

- Space jumps; Shift dashes, including the existing one air dash per jump.
- Mouse wheel changes camera distance between 360 and 780 cm. The default is 528 cm, 20% farther than before. Q swaps shoulder. Settings can reset the camera.
- The center crosshair is projected into the world; projectiles travel from the weapon muzzle toward that target. The character sits to the side. Nearby cover displays a red impact X and COVER BLOCKS SHOT. Camera collision still pulls the boom inward near walls.
- Arrows no longer split or curve merely from crossing a Spiral well. Explicit Split, Volley and extra-projectile upgrades retain their documented effects. Glaive orbit remains.
- Gameplay uses permanent mouse capture and viewport locking, with focus restoration after returning to the game. Menus lock the visible pointer to the viewport. Alt-Tab and the editor's intentional Shift+F1 release still work. Prefer standalone/borderless for two-PC play.
- HUD glyphs render at output pixel size instead of enlarging the engine bitmap font. Settings toggles HUD text between 100% and 120%; borderless uses the desktop output resolution. This does not change your TV's own scaling or sharpening settings.
- Tab opens/closes your current build: weapon, style, ability, acquired traits/evolutions, stat ranks, relics, talents and Hearth upgrades. The full available catalog remains in `UPGRADE_CATALOG.md`.
- Lobby HUD shows Ash, Relic Embers, Tablets, Skill Points, five materials and run Cinders. Reward voting is labeled WAGER VOTE.

## Rune vault repair

Aim near the sigil to highlight it with corner brackets. Gold means in range, gray means move closer, and teal marks an activated sigil. E selects the focused object or falls back to a nearby station. The server validates a 3.2 m horizontal reach, height and an unobstructed path.

Either partner can activate either sigil, independently. Activation is allowed during countdown, combat and reward phases. Both sigils disable the vault's own traps; the cache grants its shared/personal rewards only once. Ordinary random traps and blocking props are excluded from the sigil clearing. HUD displays the two-sigil count. A completed shrine or cache cannot be farmed through repeat input/reconnect.

## Six distinct active abilities

Equip any one for free at the Sanctum, then test on the dummy. Right-mouse Spiral is separate and retains its ten styles. F / controller RT casts the selected ability. Start with 60 Focus, regenerating during combat and lobby practice. Cooldown upgrades apply to all six, up to 50% reduction; Focus reserve/recovery traits remain useful.

| Ability | Focus | Base cooldown | Behavior and combination |
|---|---:|---:|---|
| Rune Lance | 25 | 1.2 s | 48 damage aimed shot; +25% on staff-marked enemies. Rune Nova turns its impact into an area blast. |
| Winter Fan | 30 | 6 s | 24 damage in a 650 cm, 120-degree cone. Applies frost buildup twice; combine with a frost weapon to reach freeze. |
| Storm Thread | 32 | 5 s | Up to four targets; 38 initial damage, each hop retains 76%. +30% against frost-affected or poisoned enemies. |
| Venom Bloom | 35 | 7 s | Four-second ground field, 320 cm base radius, 7 damage pulses and poison. Pulses deal +30% to bleeding enemies. |
| War Cry | 28 | 9 s | 330 cm pulse for 18 damage and control; adds 22 shield and grants +20% melee weapon damage for four seconds. |
| Cinder Mine | 30 | 6 s | One-second delay, 330 cm blast, 68 damage and burn; +25% against enemies pulled by a well. |

Values precede damage/area upgrades. Existing status stack caps, frost immunity and boss control restrictions remain. Ground abilities have a 1,000 cm placement limit. Inter-wave casts remain cosmetic: they grant no shield, healing, on-cast buffs or damage carried into the next wave. War Cry practice damage works on the lobby dummy; its timer is cleared when entering an expedition room. Balance is an initial bounded tuning pass, not a completed playtest.

## Biomes, collision and traps

Four progression wards and 32 chambers remain, with five visual themes: Archives (1–8), Catacombs (9–16), Luminous Caverns (17–20), Obsidian Gardens (21–24) and Verdant Reliquary (25–32). The cave has irregular original rock formations, faceted crystals, overhead rock ribs, a distinct palette and crystal-adorned enemies. Existing materials/boss progression remain tied to the original wards. Theme-specific enemy weighting begins after the first six chambers so early enemy introductions remain intact.

Architectural meshes use mesh-matching static collision, including arch openings and pillars. Decorative backdrop columns now sit outside the full generated map bounds instead of sometimes appearing inside large rooms. Navigation is rebuilt after breakable obstacles spawn. Actor capsules remain the intentional movement/combat collision; imported hitboxes and actual passage clearance need an engine test.

Wall jets show a thin gold warning line before the damaging beam. Moving saws shuttle along visible rails and can be jumped. These occupy optional side clearings, with moving saws introduced after the first ward. New trap damage scales 16/20/24/28 and cycle/motion speed scales 1.00/1.15/1.30/1.45 across wards. Moving contacts use swept travel each frame. Existing floor flame/blade cycles also accelerate. Vault Sense identifies traps; vault activation only disarms the dedicated vault pair.

## Two new Crab Champions-inspired feature families

**Breakable encounter props:** loot crystals yield four shared Cinders and a two-Cinder pickup. Ember casks explode in a 320 cm radius, damage/burn enemies and can trigger neighboring casks; nearby players can take 16 damage. Shoot, swing or use an area attack. Combat phase gating prevents inter-wave farming. Props are capped at 24 and placement is collision checked.

**Ember Bargain:** every third chamber offers a personal, optional shrine. E opens an explicit offering button. Pay current health equal to rounded-up 12% of max health for a weighted compatible stat/trait upgrade. The payment cannot kill you or lower max health. Instant rewards and Blood Pact are excluded; normal eligibility/caps apply. Each partner may use it once per chamber, including after clearing, with use preserved across reconnect. An exhausted eligible pool costs nothing.

These adapt destructible encounter rewards and optional risk/reward shrine ideas with original implementation and art. References: [official Crab Champions developer announcements](https://store.steampowered.com/news/?appgroupname=Crab+Champions&appids=774801&enddate=1735718400&feed=steam_community_announcements), [official Variety Update hotfix](https://steamcommunity.com/games/774801/announcements/detail/6651325981005953140), and [Room 8's Dragonwilds environment-art production account](https://room8group.com/cases/runescape-dragonwilds-bringing-ashenfall-to-life/). The cave borrows broad stylized fantasy direction, not commercial assets or a promise of matching production quality.

## Local acceptance checklist — still pending in UE

1. Build/import from the new folder. Verify footer 0.15.0 and all 49 mesh imports; record actual compile/import errors if any.
2. At 1080p and native 4K output, inspect HUD and menus at both text sizes. Check long weapon/ability names and lobby currency labels.
3. Test bow shots through wells with no Split trait, then with Split on enemy impact. At both shoulders and zoom limits, test nearby walls, enemies at elevation and camera collision. The red cover indicator should match obstruction.
4. On P2's separate PC, spin the mouse and repeatedly fire near screen edges. Open/close menus and Tab, Alt-Tab away/back, then fire again. Confirm input does not remain held after focus loss.
5. Visit a vault in room 2. Let each player activate a different sigil, then repeat with one player doing both after combat. Verify highlights, disabled vault traps and exactly one cache award. Check walls still prevent remote interaction.
6. Test every active ability on the dummy, swap loadouts, restart and reconnect. Verify costs, upgraded cooldowns, status combinations and no inter-wave benefits.
7. Walk and fire against pillars, arches, crystals and cave rocks. Enemies should navigate around breakables and reach every player-accessible clearing. Inspect all five themes and cave overhead camera clearance.
8. Compare wall jets/saw speed and damage in early/late wards. Jump moving saws; verify warning lines and actual contact areas at low and high frame rates.
9. Break crystals/casks with ranged, melee and area attacks. Check shared currency, chain explosions, player damage and no rewards between waves.
10. Use the shrine with both players, at low health, at trait caps and after reconnect. Confirm one personal use and the displayed cost. Verify Tab shows each player's acquired upgrade.
11. Compare saved balances/loadout before and after starting this build in the same profile mode. Confirm no currency reset and default Rune Lance on an older profile.
12. Retain previous revive, P2 chest advancement, air dash, party pause and receipt-retry tests. Test real two-PC latency before calling multiplayer verified.

`python Tests/run_tests.py` passes four native suites plus source/art checks. These do not execute Unreal actors, rendering, physics, networking or SaveGame I/O. Detailed evidence is in `VALIDATION.md` and `Tests/validation_results.json`.
