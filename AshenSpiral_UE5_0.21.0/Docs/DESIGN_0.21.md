# 0.21 — The Melee Patch + Archives

Implements the locked October 14, 2026 session brief from Josue and P2. Melee grips were implemented before the combo layer, on the verified 0.20.0 source ZIP.

## Air dash

`AirDashesUsed` replaces the old airborne boolean. Landing resets it. Eligibility checks available charges and `AirDashesUsed < MaxDashCharges()`. In this source, `DashCharges` is the **remaining** resource, decremented with every dash; comparing the counter to that shrinking value would block the second dash again. Total capacity is the intended fixed limit.

The counter is included in saved-move clear, combination, capture and replay; reconnect capture/restoration; fresh-run/reset paths; and the local HUD. The server derives it from accepted movement requests. Saved moves retain prediction state rather than granting clients authority over extra charges. Airborne recharge does not bypass the per-landing limit.

## Melee model

Cinder Blade, Grav Hammer and Rift Scythe (IDs 0, 1, 5) bypass ammunition consumption, have zero magazine/reload values, reject reload and cannot receive Deep Magazine or Quick Hands. Ranged magazines, reload motions, cues and automatic empty-magazine reload remain.

One complete pose cycle is one attack. The server schedules one hit at its impact fraction. Attack speed scales wind-up, impact and recovery together; the next attack waits for recovery. Releasing Attack lets the current swing finish. A pause longer than 0.9 seconds after recovery restarts at hit one. Dash, downing, revive, input reset/menu, loadout/phase/arena changes cancel a pending hit. Hammer no longer requires a held-charge release.

| Weapon | Base cycles, seconds | Base primary damage | Finisher |
|---|---|---|---|
| Cinder Blade | 0.28 / 0.28 / 0.35 | 40 / 40 / 52 | Third-hit lunge, up to 160 cm |
| Grav Hammer | 0.72 / 0.72 / 1.188 | 62 / 62 / 161.2 | Third-hit 85 cm hop, up to 60 cm forward; 410 cm radial reach |
| Rift Scythe | 0.40 | 41 | Repeating sweep; no new finisher |

Initial unmodified full-cycle primary DPS is approximately **145.05 / 108.52 / 102.5**. Figures exclude critical hits, traits, elements, wells, missed targets and frame quantization. Hammer has broader slam coverage and existing well interactions. These numbers are implementation choices for playtest, not measured balance results.

Attack-speed multiplier: `(1 + 0.08 × permanent rank + 0.12 × run rank) × Attuned`, with Attuned 1.25 when active. Blade impacts at 48% / 48% / 60% of each cycle; Hammer at 56% / 56% / 76%; Scythe at 48%. The menu's swings/second includes third-hit recovery. Blade/Hammer show combo pips instead of ammunition.

### Grip and movement foundation

Both hands stay on the Hammer shaft, separated by 24 cm in model space. Its longer shaft lets a reachable two-hand pose bring the head close to the floor at impact. Scythe has a separate curve and 30 cm hand separation, avoiding its long butt passing through the torso. Blade uses its grip origin, a free balancing arm and opposing cuts before the lunge. Smooth interpolated key poses include upper-body wind-up and follow-through; existing locomotion/cape behavior remains.

Finishers use CharacterMovement root-motion sources. The server validates clear grounded travel in 10 cm increments; the owner receives the same source and simulated proxies receive movement replication. An airborne attack still swings but grants no extra automatic hop/lunge. Grounded finishers do not intentionally route through walls or over invalid floor. Actual root-motion landing, obstruction and latency behavior require Unreal testing.

Damage resolves against current aim/targets at impact with sight/height rules. Hammer retains its well behavior: normal hits plant a normal well; the third collapses the nearest owned well within 750 cm, or plants the existing charged well if none qualifies. Boss/portal anti-cheese is deferred.

### Smart approach

The multiplier is 1 at and below `max(700 cm, attack range + 200 cm)`. It rises smoothly over the next 1,700 cm to **1.35 for ordinary/elites** or **1.15 for bosses**. Existing base-speed multipliers and slows still apply. Wind-ups stop movement; special charge/leap speeds and boss attacks remain. This is a distance-dependent boost, not another flat base-speed increase.

## Forgotten Archives

Only biome 0 expedition rooms use the pass. The hub, other biomes, route graph, room cap, footprints and encounter/boss mechanics retain their existing logic.

- Shelf local +X is the open front. Shelves and books rotate together toward usable room space, replacing fixed-yaw backwards shelves.
- Seeded compositions weight stacked aisles, scriptoria, ruined bays and spare reading bays differently. Narrow rooms use shallow perimeter furniture. Unequal heights, missing shelves, book spacing, floor rotation and restrained tints break repetition.
- Seven new meshes: intact/broken shelves, intact/broken desks, pointed rib, ironwork and scattered paper/wood debris.
- Three original PBR sets, nine 512×512 maps: damp cracked stone, worn timber and corroded iron. Bootstrap sets normal/roughness compression correctly.
- Above-head pointed ribs, tall piers, low cool/warm lighting, broken furniture, folios and sparse static dust flecks add age and verticality. The 48-light cap remains.
- Conservative prop bounds preserve the 720 cm central cross and wall margins. Native checks use actual mesh vertices over 450 room/seed/size combinations. Decorative ribs/debris do not collide; live navigation still needs checking.

**Reference boundary:** `game/AshenSpiral/reference/` (eight images + README), `0.21-fix-candidates.md`, and new Blade pictures were unavailable in the supplied files, available Library results and retrieved context. The dash bug was verified in source. This art interprets the written gothic-cathedral direction; it does not claim to match unseen P2 images. Supply that pack for the next visual review.

## Local acceptance — Josue + P2

1. Build/import a clean 0.21 folder on both PCs. Confirm menu **0.21.0 / protocol 2100** and existing progress. Begin with the hub practice dummy.
2. With two, then three charges: jump, spend every available air dash, confirm another is blocked, land and repeat. Check both peers, ground dash into jump, recharge while airborne, pause and reconnect; repeat with network latency.
3. Hold Attack for 20+ swings on each melee weapon. Press Reload during idle/wind-up/recovery: expect no reload or cue. Verify all six ranged weapons still reload on empty and on R.
4. Check Blade cut/cut/lunge; Hammer left/right/lift-hop-slam, heavy third damage and well collapse; Scythe full sweeps with both hands attached. Compare impact timing at 30/60+ FPS, with zero/max attack-speed ranks and Attuned.
5. Release/tap Attack, resume briefly, then wait over 0.9 seconds after recovery. Dash or open a menu before impact, become downed, revive, switch trial weapons and advance chambers. Confirm no stale hit/travel enters the new state.
6. Try finishers beside walls, shelves, bridges, ledges, slopes and the other player. Check clipping, displacement, landing timing and network corrections. Attack while already airborne. Rejoin during a combo and confirm a clean restart.
7. Kite normal enemies, elites and a boss from far away. Confirm bounded approach, normal speed before attack range, and retained frost/root/stun and special telegraphs.
8. Review several ward-one seeds and footprints, including vault/shrine/chest rooms. Check inward spines, aged material detail, readable lighting, varied dressing, rib underside, furniture collision, dust/frame cost and open paths. Confirm wards 2–4 and the hub retain their prior art. P2 compares the pack when supplied.
9. Retain seed **6767 / chamber 23** chest guidance, both-player advance, reward text/HUD sizing, save receipts, migration and reconnect regressions. These are pending engine tests, not completed results.

## Deferred and baseline mismatch

Difficulty portals/fresh slots; boss synergy and portal/hammer spawn-kill changes; fight-specific boss arenas; endless/mastery/unlocks/looping; other-biome art; and the 0.22 locomotion/cape/enemy/hit-reaction/remaining-weapon overhaul are deferred.

The brief cites Bow third-shot double as design context. It was absent from the supplied 0.20 baseline. Bow firing behavior is retained without adding an unrequested ranged rework.

## API references checked

- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FRootMotionSource
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FRootMotionSource_MoveToForce
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FRootMotionSource_JumpForce
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FRootMotionFinishVelocitySetting-

The API review supports the implementation choice; it does not replace a successful Unreal build or co-op test.
