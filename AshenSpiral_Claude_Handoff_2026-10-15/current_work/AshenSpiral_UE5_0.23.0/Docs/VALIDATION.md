# Validation — 0.22.0

Run on the supplied source in the coding workspace, 2026-10-15. These are native shared-rule tests, source checks and offline mesh previews; they are not an Unreal build or playtest.

## Automated results

- PASS: 1451458 assertions; 10000 deterministic layouts; 2263 optional gap edges.
- PASS: 2169971 gameplay logic/geometry checks; 1598508 sampled weapon vertices.
- PASS: 456281 buildcraft checks; 18000 weighted reward drafts; 66 stable trait IDs (65 selectable).
- PASS: 45507 adventure checks; camera, popup clearance, interaction reach, six abilities, shrine safety, biome mapping and four-ward trap motion.
- PASS: 77444 career checks; linear XP thresholds, 36 achievement milestones, bounded Cinder conversion and status combinations.
- PASS: 652 interface/arsenal checks; HUD scale, fade duration, legacy unlock bits, firearm balance and upgrade eligibility.
- PASS: 108681 reload/navigation checks; 1200 route-oracle comparisons; 40000 reward drafts.
- PASS: 10961 0.20 regression checks; migrations, one-time milestone rewards, shield grants, traps, joint lengths, 400 multi-endpoint route oracles and seed 6767 chamber 23 floor model.
- PASS: 11548175 melee/0.21 checks; 1687 full-cycle pose samples; 450 Archives rooms; 3000 melee reward drafts.
- PASS: 9537191 hammer/0.22 checks; 2886 actual-mesh pose samples; tangent orientation, two-hand reach, contact timing, haste, frontal coverage and leap immunity boundaries.

PASS: both runtime fern expressions compile with float/double overloads; seven positions are finite and on the unit circle. Unreal module compilation was not run.

Structural/source-art checks: 445,864; 51 source files; 32 RPC implementation-presence checks. 78 OBJ meshes, 80,514 vertices, 26,838 triangles, no degenerate triangles. 12 512×512 maps and 12 valid mono 44.1 kHz PCM WAVs. These structural checks are not UHT/C++ module compilation.

## Scope protection

The baseline ZIP SHA-256 is `3e412acec71d9b5fcb2a82382cca35588c17a02704919bc7139762efa244501e`. A direct comparison verifies 404 Blade/Scythe pose samples are unchanged. Non-hammer weapon meshes, arsenal values, trait rules, enemy/boss source, original spell/well source and Archives rules are byte-identical. All non-hammer content tables and every price remain unchanged. See `Tests/protected_0.21_comparison.txt`.

## Source visual review

`HAMMER_0.22_COMBO_PREVIEW.png` maps ten reference frames to actual meshes and pose samples. `HAMMER_0.22_LEAP_PREVIEW.png` shows six leap poses. `HAMMER_0.22_COMBO.gif` previews the procedural cycle. These show source geometry, not Unreal lighting/materials, movement, audio, latency or animation playback. The numerical tests enforce hand reach, support spacing, tangent head orientation, basic floor/deep-body clearance and timing. They cannot establish an in-game animation-quality acceptance result.

## Still pending

- UE 5.8.3 UHT/module compile, content import, material compile and Windows launcher/package execution.
- Real grounded/airborne sweeps, low-ceiling and gap collisions, enemy-only ignores, landing ordering, stun/damage interaction and root-motion correction.
- Host/P2 simultaneous use, asymmetric latency, packet loss, reconnect and independent motion preferences.
- In-game animation/art review against all three provided sheets; sound levels, shake comfort and measured performance.
- The acceptance checklist in `DESIGN_0.22.md`.

Protocol 2200 and bootstrap revision 10 require both players to use this complete release. Profile schema 1 and migration 20 are unchanged. Run `python Tests/run_tests.py` to repeat these native checks; use the included Windows launchers for the actual engine build and playtest.
