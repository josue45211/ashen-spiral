# 0.22.0 — Grav Hammer rework

The supplied `reference/0.22-hammer-prompt.md` and three PNG sheets define this release. Scope is the hammer. “Melee only” is interpreted as the hammer for the new inputs, because the same brief explicitly excludes Blade and ranged changes. Scythe also retains its controls.

## Pose foundation and reference mapping

The hammer uses `ASHammerRules.h` for shared C++ pose curves. Its +Z shaft fixes both hand targets 24 cm apart. Its squared head's +Y long axis follows the projected head-path tangent; a symmetric half-turn does not spin the wrists, which use a separate continuous grip rotation. The shortened pommel clears the torso. Shoulder positions follow the torso coil; hips counterbalance and the cape responds to the swing.

| Reference frames | Runtime action | Base cycle | Base damage |
|---|---|---:|---:|
| 1–2 | Left-to-right frontal swing | 0.72 s | 62 |
| 3–5 | Right-to-left frontal swing | 0.72 s | 62 |
| 6–7 | Overhead wind-up, visible hip/shoulder coil | Part of 1.188 s | — |
| 8–10 | Planted frontal smash and recovery | 1.188 s total | 161.2 |

All three attacks use the frontal hemisphere with height/line-of-sight checks. Primary reach remains 280 / 280 / 410 cm plus enemy capsule radius. The finisher has no jump or automatic forward travel. If the earlier swings are airborne, hit three waits for the ground. Its primary hit excludes enemies behind the player; the retained well collapse can independently damage around its own well.

Hold LMB to complete/repeat the chain, including whiffs. The hammer has 1.2 s of grace **after recovery**, scaled by the same attack-speed multiplier as the full animation. Hits one/two may be canceled by dash or leap, returning the next hit to one. Hit three blocks dash, jump, leap, Spiral, equipped active skill and revive until its full recovery completes. Releasing attack or opening a menu does not bypass that commitment; death, room teardown and loadout changes may force cleanup. A true party pause freezes the existing simulation clock.

The complete primary combo remains 285.2 base damage / 2.628 s = approximately 108.52 DPS. Criticals, traits, wells, missed hits and frame quantization affect actual combat DPS. Attack-speed bonuses scale all phases proportionally. Visual contact holds of 60 / 60 / 140 ms at base speed are included **inside** each cycle, preserving this timing; the whole game is not time-dilated.

## RMB mobility leap

| Parameter | Value |
|---|---:|
| Cooldown | 7.5 s from accepted request |
| Ash / dash charge cost | 0 / 0 |
| Vulnerable crouch startup | 0.16 s |
| Nominal flight | 1.04 s |
| Protected landing recovery | 0.22 s |
| Apex above launch | 208.35 cm |
| Horizontal reach | 1.15 × ordinary dash reach; 648.6 cm at base dash |
| Radial landing damage | 100 base |
| Radial reach | 370 cm plus enemy capsule radius |

Ordinary ground dash has no upward arc, so “50% higher” uses the existing normal jump's approximately 138.9 cm apex (660 cm/s at 1568 cm/s² gravity). Long Dash increases reach through its existing dash-duration rule. Neither attack haste nor Spiral cooldown changes the leap cooldown.

Leap requires grounded, available hammer control. It cancels an ordinary swing but cannot cancel the committed finisher. It starts vulnerable, launches at frame two, and is protected while airborne and until the landing recovery pose completes. Existing immunity from another source is not deliberately erased during startup. The recovery returns to the ready grip; damage immunity ends with it.

A `FRootMotionSource_JumpForce` is applied on the server and owning client. Simulated proxies use movement replication. Movement sweeps preserve walls, ceilings and props; temporary enemy-only movement ignores allow clearing a swarm. Damage fires from confirmed `Landed` ground contact, with a one-shot state transition, height and visibility checks. A floor-contact fallback handles a missed landing notification; a missed-platform timeout clears immunity without phantom impact. Falling below the existing rescue threshold clears the leap before fall damage. These source paths still require Unreal collision/network testing.

Leap has its own per-character cooldown/state and does not collapse a well. The LMB finisher retains the existing nearest-own-well selection and collapse/plant behavior. Reconnect snapshots retain the leap cooldown but do not resume an interrupted leap/combo.

## Controls, impact and accessibility

- Hammer LMB / controller RB: automatic combo.
- Hammer RMB / controller X: leap. This uses the existing “Spiral” key binding, now labeled for the dual role in Controls.
- Hammer G / controller LT: former Spiral hold/release ability. Rebind under “Hammer former alt ability.”
- Other weapon inputs and equipped F / controller RT skills remain unchanged.
- Combo knockback: 230 / 230 / 950 cm/s; landing knockback 700 cm/s. Existing elite half-strength and boss/training-dummy immunity remain.
- The finisher has stronger shake, a directional debris burst and a heavy sound cue. Leap debris radiates from landing. Four original synthesized WAV cues are included.
- Settings → Motion sickness reduction disables all hammer camera offsets immediately and persists per player. No existing saved reduction setting was present in 0.21, so this release adds one.

## Verification and limits

Ten native C++ suites and structural/source-art checks pass; see `VALIDATION.md`. The new suite samples 2,886 complete combo/leap poses using actual OBJ vertices, checks rotated-shoulder reach, exact two-hand spacing, head tangent alignment, floor/deep-body clearance, haste/contact-hold timing, frontal classification and immunity boundaries. The rendered PNG/GIF previews use source meshes and pose curves, not the Unreal renderer. They do not establish in-game quality or physics correctness.

No live Unreal editor/runtime was available. UE module/UHT compilation, import, audio mix, latency correction, performance and the following acceptance checklist remain **pending**:

1. Host and P2 equip hammers; hold attack on dummy, enemies and empty space. Confirm 1 → 2 → 3 at base and max attack speed, with no dropped finisher.
2. Attempt dash, leap, jump, F, G, revive and menu release during hit-three wind-up. Ordinary inputs must not cancel it; movement returns after recovery. Check downing/room transition still cleans up.
3. Confirm all combo primary hits exclude a rear target; verify each player's finisher collapses only their own nearest well.
4. Leap through a swarm and across a reachable optional gap. Check walls, low ceilings, short platforms and missed landings; confirm one landing hit and no indefinite immunity.
5. Damage the player during crouch, flight, touchdown and after standing. Confirm the vulnerable/protected boundaries and 7.5 s cooldown on each peer. Repeat with 100–150 ms latency and packet loss.
6. Confirm continuous two-hand contact, tangent head orientation, planted feet, cape response, and a satisfying heavy payoff from normal play camera angles. Use the supplied references for art review.
7. Toggle Motion sickness reduction during an impact and after restart. Both light/heavy shake must disappear; P2's preference must remain independent.
8. Switch to Blade/Scythe/ranged, check old attacks/controls and ranged auto-reload, then reconnect and verify cooldown/progression.

## Primary API references consulted

- [Epic: FRootMotionSource_JumpForce](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FRootMotionSource_JumpForce)
- [Epic: ACharacter::Landed](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ACharacter/Landed)

Release protocol is 2200; art bootstrap revision is 10. Profile schema 1 and migration 20 are unchanged. The original 0.21 ZIP remains untouched.
