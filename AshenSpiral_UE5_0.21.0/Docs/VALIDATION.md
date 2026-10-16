# Verification — 0.21.0

Validation date: October 15, 2026 UTC. All nine standalone C++ suites pass. **Unreal Engine is unavailable here; no successful Unreal build, art import or live gameplay/co-op session is claimed.**

## Executed checks

| Suite | Result |
|---|---|
| Shared rules | PASS: 1451458 assertions; 10000 deterministic layouts; 2263 optional gap edges. |
| Gameplay / geometry | PASS: 2169971 gameplay logic/geometry checks; 1598508 sampled weapon vertices. |
| Buildcraft | PASS: 456281 buildcraft checks; 18000 weighted reward drafts; 66 stable trait IDs (65 selectable). |
| Adventure | PASS: 45507 adventure checks; camera, popup clearance, interaction reach, six abilities, shrine safety, biome mapping and four-ward trap motion. |
| Career | PASS: 77444 career checks; linear XP thresholds, 36 achievement milestones, bounded Cinder conversion and status combinations. |
| Interface / arsenal | PASS: 652 interface/arsenal checks; HUD scale, fade duration, legacy unlock bits, firearm balance and upgrade eligibility. |
| Reload / navigation | PASS: 108681 reload/navigation checks; 1200 route-oracle comparisons; 40000 reward drafts. |
| 0.20 regressions | PASS: 10961 0.20 regression checks; migrations, one-time milestone rewards, shield grants, traps, joint lengths, 400 multi-endpoint route oracles and seed 6767 chamber 23 floor model. |
| 0.21 melee / Archives | PASS: 11548175 melee/0.21 checks; 1687 full-cycle pose samples; 450 Archives rooms; 3000 melee reward drafts. |

Structural/source-art validation: 445,156 checks across 48 source files; 29 RPC implementation-presence checks. These are lexical checks, not an Unreal compiler.

Art: 78 meshes; 80,514 vertices and 26,838 triangles; zero degenerate triangles; valid UV/normal indices and recorded centimeter bounds. Twelve 512×512 texture maps and eight mono PCM sound cues validated.

The 0.21 suite checks 1,687 full-cycle melee pose samples for arm reach, shaft support, deep torso/helmet penetration and floor clearance using actual weapon OBJ vertices. It tests charge budgets of 1–3, saved-count behavior, complete-cycle attack-speed scaling, one impact per swing across frame rates, 3,000 melee reward drafts, bounded enemy approach and 450 deterministic Archives seed/room/size combinations with actual prop geometry. It does not execute actor Tick, network packets, collision queries or root-motion movement.

Melee grip, finisher and updated Archives source previews were rendered and visually inspected. They are geometry/rule previews with neutral software lighting, not Unreal captures. The current editable catalog is generated from runtime content tables, with explicit melee combo tuning and ranged-only reload rewards.

## Pending in Unreal / Windows

- C++ module and UnrealHeaderTool compilation; OBJ import, material compilation, texture assignment and cooked asset inclusion.
- Actual swing/impact synchronization, hand and armor clipping from all camera angles, root-motion lunge/hop landing, ledge/wall/ally collisions and high attack-speed feel.
- Host/client movement prediction, correction under latency/loss, shared pause, cancellation, reconnect and correct air budgets after rejoin.
- Runtime Archives lighting/readability, collision/navigation, interactions, visual variety, draw calls and performance.
- Save serialization/migration, sound playback, Slate layout, Windows launcher and cooked-game packaging.
- P2 reference matching: the eight-image Archives pack and new Blade pictures were unavailable. This pass follows the written brief.

Run the focused checklist in DESIGN_0.21.md on both PCs. Retain the 0.20 seed-6767/chamber-23 navigation, HUD/rewards, progression and co-op checks. Source-only success does not establish game correctness.

## Reproduce

Run `python Tests/run_tests.py` with a C++17 compiler on PATH. Optional visual generation uses NumPy/Pillow: compile Tools/export_melee.cpp, write its output to Docs/MELEE_ARCHIVES_SAMPLES.json, then run Tools/preview_melee_archives.py. The provided source-art files are ready for Unreal import; normal setup does not require these Python dependencies.
