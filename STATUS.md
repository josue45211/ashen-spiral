# Ashen Spiral Agent Status

updated: 2026-10-16 15:36 PDT
phase: Night report written; waiting for the 5PM playtest. Nothing left to do without answers.
current_item: idle

## Night report
Final check at HEAD 5c0bcca, run 2026-10-16 15:36 PDT: UBT editor build Succeeded in unity AND -DisableUnity (zero compile errors); Tests/run_tests.py exit 0 (native + gameplay + new suites). No game launch, per Josue.
Everything below is compile + native-test verified only. Nothing was run in-engine.

## Done (every item: UBT editor build OK unity + non-unity, Tests/run_tests.py exit 0; none verified in-engine)
- Phase 1 compile blockers: C4458 rename, DamageReaction/AbilityGesture/cape definitions, ProfileId stamped in SendReceipt
- A1 Blade poses (cosmetic), A2 locomotion, A3 cape baseline, A5 hit reactions, A6 ability gesture wired; Scythe left alone per Josue
- B1 difficulty: host picks Normal/Hard/Very Hard; Hard/Very Hard locked behind clears (rule is TUNABLE in ASExpeditionRules.h); HP/damage/offer quality wired
- B2 second character: hub switch UI, server gating, client revert on refusal, fresh-slot commit failure handled
- B3-B5: synergy liveness guards; Vengeance mask restored to all weapons; Emberforge cap gate reverted per Josue
- B6 wishing well: new station in shrine rooms (TUNABLE), run Cinders, server-side stake and roll, one spin per player per room, reconnect-safe
- B7 boss arenas: four bespoke structural layouts around each boss's attacks; boss spawns at centre
- B8 Endless: hub portal, huge obstacle arena, chained waves (36 cap, stragglers carry), checkpoint chests (1-20 every wave, 21-50 every other, then every third from 50, boss waves always), shuffles (wave-based to 50, 120-180 s combat time after, 3 s telegraph, cover never placed on players), downed bleed bars and elimination, exclusive traits 400-403, sidegrades after wave 15, diminishing returns past cap, kill/checkpoint economy, lucky shop, wave-50+ lethal blasts and homing shots, run-stats receipt, death screen with stats and local top runs
- 0.23.1 A3 heavy cape: damped length-constrained cloth chain with inertia, delayed turn/dash/leap/swing response, idle billow, body/floor clearance
- 0.23.1 B1 portals: Ember/Ashen/Blood arches side by side, chained when locked, Blood drips, tooltips with exact modifiers and unlock condition, colour transition into chamber 1
- 0.23.1 B8: sinister Endless portal (dormant until Normal cleared), sky stages clear / gloomy (21) / red (50, fast unmistakable step + flash)
- Native tests added: expedition rules, boss/endless arena structure, heavy cape chain, Endless offers (about 1.5M checks)
- Independent read-only review of B2/B6/B8 found 9 issues; 8 fixed: stale Endless boss flag (likely out-of-bounds Ward crash), switch ignored mid-run leaving client/server on different characters, revert now uses the server-kept profile id, abandoning an Endless run now banks stats/earnings (and no longer files a fake chamber-32 clear), reconnect snapshot now refreshes well/shop/stat fields, real capsule for post-shuffle unsticking + pending shuffle cleared at checkpoints, empty offer list no longer blocks the chest, failed spawn point no longer skips Endless timers. Also found by clang: ASExpeditionRules.h was missing its ASRules.h include (masked by unity builds).

## Left undone / queued
- Physical walk-through-the-portal animation (colour flood placeholder only; P2 visual track)
- Scythe poses (left alone per Josue), Blade art pass (needs P2 reference frames)
- Full landscape decay for Endless (future reference only, sky stages done)
- Release metadata: README, version, catalog, manifest are still the 0.22 documents; Tests/validation_results.json still says Unreal compile NOT RUN
- Tunables to revisit after the playtest: well odds thresholds (50/1000 were tuned for Ash), Endless earn rates and shop prices, well placement, unlock rules (ASExpeditionRules.h)

## UNVERIFIED (playtest checklist for Josue and P2; compile + native tests only)
- A1 Blade poses: art review needed (no reference frames)
- A2 locomotion feel, foot skating; A5 recoil direction/sign; A6 ability gesture; Scythe untouched, never assessed
- A3 / 0.23.1 heavy cape: drape, clipping against body and weapons, whether it reads as heavy cloth, billow, lag, dash flare, leap lift
- B1 portals: arch placement and size, chain look, drips, tooltip readability, colour transition, lock/unlock flow, difficulty feel
- B2 slot switch: two-player, reconnect, save failure, A/B recovery all untested
- B3-B5: synergies, boss spawn protection, lethal-save order in co-op
- B6 well: placement vs other shrine-room props, UI, spin flow
- B7 arenas: nav grid, collisions, cover readability, boss spawn point, door approach
- B8 Endless: entire loop (entry, wave pacing, straggler rule, chests, shuffle telegraph, nav after shuffle, shop, bleed bars, elimination, death screen, receipt save), arena looks, wave-50 hazards fairness (dash must dodge), sky stages and the wave-50 transition
- Receipts: ProfileId stamping verified by reasoning and a lexical test only

## Needs Josue
- Bridge v3 (POST /exec) NOT installed: the allowlist can be bypassed. See Notes. Install a hardened version, skip it, or install as sent?

## Token usage and cost
Not available to me: this session exposes no usage counters, so I cannot report input/output tokens or USD without inventing numbers. Please read it from the Claude Code /cost or /usage view.

## Notes
- v3 server decision (2026-10-16 15:36): I did not install it. exec_allowed() accepts any command whose first token is `python` and that merely contains the token `Tests/run_tests.py`, e.g. `python -c "<code>" Tests/run_tests.py` runs arbitrary code. The git allowlist also permits file-writing options such as `git diff --output=<path>` and `git branch -D`. The server binds 0.0.0.0 with no authentication, so anyone on the LAN or tailnet could use it, which contradicts the comment that the worst case is reading git history. The running v2 bridge is untouched and still serving. Proposed hardening if you want it: exact-match command list (no free arguments), git restricted to fixed read-only invocations without --output, bind to the Tailscale interface only, shared-secret header.
- Answered questions removed from Needs Josue (odds keep, host-only unlocks, elimination keep, economy placeholders, walk-in acceptable, smoke test skipped).
- INBOX.md is uncommitted by design. The 21:23 test entry and the 21:27 definition-of-done entry were moved under Handled there.
- Review of B2/B6/B8 by an independent read-only subagent found 9 issues, 8 fixed (see git log, commit 5c0bcca).
- Build logs: C:\Games\head_unity.log, C:\Games\head_nonunity.log. Scratch build copy: C:\Games\AshenSpiral_build_0.23.
- Earlier updated: stamps of 22:58 and 23:45 on 2026-10-15 were wrong (written ahead of the system clock).

## Commits (for review)
Range cfe023c..HEAD, oldest first. Paths relative to AshenSpiral_Claude_Handoff_2026-10-15/current_work/AshenSpiral_UE5_0.23.0/ (STATUS.md at repo root).

- cfe023c Fix C4458 shadowed Controller in ASEnemy synergy callout
  - source: Private/ASEnemy.cpp
- 03ff6c2 Phase 1: cape/reaction/gesture definitions, stamp receipt profile id, refresh stale tests
  - source: Private/ASCharacterAnimation.cpp, Private/ASGameMode.cpp
  - tests: bugfix_020_tests.cpp, buildcraft_results.txt, buildcraft_tests.cpp, reload_navigation_results.txt, run_tests.py, trig_compile_results.json, validation_results.json
- 3b5a344 A5: consume player/enemy hit reaction and attack release in animation
  - source: Private/ASCharacterAnimation.cpp, Private/ASEnemyAnimation.cpp
  - tests: bugfix_020_results.txt, bugfix_020_tests.cpp, validation_results.json
- 95ad817 A1: Blade pose pass (fast coil, short follow-through, stronger lunge lean, off-hand counterweight)
  - source: Private/ASCharacterAnimation.cpp, Public/ASMeleeRules.h
  - tests: validation_results.json
  - other: mission-control-server.py
- 40e386a A2: stride-matched gait cadence and visual acceleration lean
  - source: Private/ASCharacterAnimation.cpp
  - tests: validation_results.json
  - other: STATUS.md
- 0e4132d A6: consume AbilityGesture in animation; Scythe left untouched pending P2 review
  - source: Private/ASCharacterAnimation.cpp
- 577119a B2: second character slot - hub switch UI, server-side switch gating with client revert, fresh-slot commit failure handling
  - source: Private/ASGameInstance.cpp, Private/ASGameMode.cpp, Private/ASPlayerController.cpp, Private/SASMenu.cpp, Public/ASPlayerController.h
  - tests: run_tests.py, validation_results.json
- a173b0b B3-B5: synergy liveness guards; restore Vengeance Oath weapon mask to all weapons (per Josue)
  - source: Private/ASCharacter.cpp, Private/ASEnemy.cpp, Public/ASBuildRules.h
  - tests: reload_navigation_results.txt, validation_results.json
  - other: STATUS.md, mission-control-server.py
- 717b261 B1: difficulty selection (host, hub) wired to enemy HP/damage and offer quality; add expedition rule tests
  - source: Private/ASCharacterUpgrades.cpp, Private/ASGameMode.cpp, Private/ASPlayerController.cpp, Private/SASMenu.cpp, Public/ASGameMode.h, Public/ASPlayerController.h
  - tests: expedition_023_results.txt, expedition_023_tests.cpp, run_tests.py, validation_results.json
  - other: INBOX.md, STATUS.md
- da947f4 Update STATUS.md
  - other: STATUS.md
- 24d1a11 Revert Emberforge capped gate (per Josue); lock Hard/Very Hard behind clears (tunable rules in ASExpeditionRules.h); refactor profile resubmit
  - source: Private/ASData.cpp, Private/ASGameInstance.cpp, Private/ASGameMode.cpp, Private/ASGameState.cpp, Private/ASPlayerController.cpp, Private/SASMenu.cpp, Public/ASBuildRules.h, Public/ASData.h, Public/ASExpeditionRules.h, Public/ASGameMode.h, Public/ASGameState.h, Public/ASPlayerController.h
  - tests: buildcraft_results.txt, buildcraft_tests.cpp, expedition_023_results.txt, expedition_023_tests.cpp, validation_results.json
- 4791b79 B6: wishing well station (shrine rooms), server-authoritative Cinder gamble with per-player-per-room key, reconnect-safe, odds UI
  - source: Private/ASGameMode.cpp, Private/ASHUD.cpp, Private/ASPlayerController.cpp, Private/ASWorld.cpp, Private/SASMenu.cpp, Public/ASExpeditionRules.h, Public/ASGameMode.h, Public/ASPlayerController.h, Public/ASWorld.h
  - tests: expedition_023_results.txt, expedition_023_tests.cpp, validation_results.json
- bfed9f8 B7: bespoke structural boss arenas per ward (layout, attack-aligned cover, centre boss spawn, west entry) + structure tests
  - source: Private/ASGameMode.cpp, Private/ASWorld.cpp, Public/ASRules.h, Public/ASWorld.h
  - tests: boss_arena_023_results.txt, boss_arena_023_tests.cpp, run_tests.py, validation_results.json
- 0e0f10b B8.1: Endless mode core - hub portal station, huge obstacle arena, chained waves with straggler carry (cap 36), checkpoint chests, shuffle with telegr
  - source: Private/ASCharacterUpgrades.cpp, Private/ASGameMode.cpp, Private/ASHUD.cpp, Private/ASPlayerController.cpp, Private/ASWorld.cpp, Private/SASMenu.cpp, Public/ASGameMode.h, Public/ASPlayerController.h, Public/ASRules.h, Public/ASWorld.h
  - tests: boss_arena_023_results.txt, boss_arena_023_tests.cpp, validation_results.json
- 31cfaaa B8.2: Endless exclusive traits (IDs 400-403) with runtime effects, sidegrade shift after wave 15, kill/checkpoint economy feeding the end-of-run recei
  - source: Private/ASCharacterUpgrades.cpp, Private/ASGameMode.cpp, Private/SASMenu.cpp, Public/ASBuildRules.h, Public/ASCharacter.h, Public/ASExpeditionRules.h
  - tests: buildcraft_tests.cpp, validation_results.json
- f997b5b B8.3: lucky shop (server-validated Scrap/Ash stock), bleed-out bars, wave-50+ lethal telegraphed blasts and homing shots
  - source: Private/ASEnemy.cpp, Private/ASGameMode.cpp, Private/ASGameState.cpp, Private/ASHUD.cpp, Private/ASPlayerController.cpp, Private/ASSpell.cpp, Private/ASWorld.cpp, Private/SASMenu.cpp, Public/ASExpeditionRules.h, Public/ASGameMode.h, Public/ASGameState.h, Public/ASPlayerController.h, Public/ASSpell.h, Public/ASWorld.h
  - tests: expedition_023_results.txt, expedition_023_tests.cpp, validation_results.json
- 0cecd6d B8.4: carry Endless run state (ranks, earnings, stats, bleed) through reconnect snapshots
  - source: Private/ASGameMode.cpp, Public/ASGameMode.h
  - tests: validation_results.json
- 69dde83 B8.5: Endless power curve - diminishing returns for trait ranks above the normal cap
  - source: Private/ASCharacterUpgrades.cpp
- acdd121 0.23.1 A3: heavy cape - damped length-constrained cloth chain with inertia, delayed turn/dash/leap/swing response, idle billow, body/floor clearance, 
  - source: Private/ASCharacterAnimation.cpp, Public/ASAnimationRules.h, Public/ASCharacter.h
  - tests: cape_0231_results.txt, cape_0231_tests.cpp, run_tests.py, validation_results.json
- 5525cec 0.23.1 B1/B8 looks: gothic Ember/Ashen/Blood portals (chained when locked, dripping blood), sinister Endless portal, tooltips with exact modifiers, ru
  - source: Private/ASGameMode.cpp, Private/ASHUD.cpp, Private/ASPlayerController.cpp, Private/ASPortal.cpp, Private/ASWorld.cpp, Public/ASExpeditionRules.h, Public/ASHUD.h, Public/ASPortal.h, Public/ASWorld.h
  - tests: expedition_023_results.txt, expedition_023_tests.cpp, validation_results.json
- ffaf489 Cape segment width matches SM_Cape half-width (36); refresh STATUS.md
  - source: Private/ASCharacterAnimation.cpp
  - other: STATUS.md
- 5c0bcca Fix issues from independent review of slot switch, well and Endless; add missing ASRules include
  - source: Private/ASGameInstance.cpp, Private/ASGameMode.cpp, Private/ASPlayerController.cpp, Public/ASExpeditionRules.h, Public/ASGameInstance.h, Public/ASPlayerController.h
  - tests: run_tests.py, validation_results.json
  - other: STATUS.md
