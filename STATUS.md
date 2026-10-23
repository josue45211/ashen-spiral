# Ashen Spiral Agent Status

updated: 2026-10-23 07:50 PDT
phase: 0.26 RUN - Phase 0 (G1-G8) DONE and committed (10 commits); settling the held 0.24.2 aesthetic work, then Phase 1 (Twin Claws + Pounce)
current_item: settle the held aesthetic pieces (elite brutes as small Titans, floor relief, pickups, vault chest), then Phase 1 design: Twin Claws weapon id 9, RMB Pounce (root-motion dynamic leap + glue), G for Spiral

<!-- 0.26-RUN:BEGIN -->
## 0.26 RUN - started 2026-10-23 (Edison's "0.26 Huge Update Plan", inbox 2026-10-22 22:02 parts 1+2; Josue + P2 locked it 2026-10-22)
Model/effort: Sonnet 5.5 on max effort, as ordered (nothing to flip). Reviews off, build directly; I escalate only money, deletions, expensive-to-undo architecture and save-format changes.
Where I work: C:\Games\AshenSpiral (the main clone). The Playtest folder is only the matched-build playtest copy.
Definition of done per item: UBT editor build unity AND non-unity clean, Tests/run_tests.py exit 0 (Saved/Scratch/run_tests_clean.ps1), crash-safety self-review, in-engine playtest of the changed behaviour with screenshots and a log scan. Presentation ships in the same item (POP + PIZZAZZ mandate).

### Step 0 - folder check: DONE
- C:\Games\AshenSpiral_UE5_0.24.2_Playtest is NOT a git repo: a playtest-zip extraction (the zip was cut from commit 6b57967), no history. The bridge (python pid 13268, Tailscale IP, port 8765) runs from it, so the bridge serves ITS INBOX.md and STATUS.md.
- C:\Games\AshenSpiral EXISTS and is healthy: branch astra/0.23-handoff = origin/astra/0.23-handoff at 49c2e1e, git fsck clean, full project tree, and the build / test / run helpers in the project's Saved/Scratch. All work happens there. No folder was created, moved or re-initialised.
- The zip is OLDER than that HEAD (the Titan boss and aesthetics passes 1-9 came after 6b57967). The new playtest zip is exported from the main clone at the end of the run.
- So Edison's poll stays accurate, this STATUS.md is mirrored to <Playtest>\STATUS.md (the file the bridge serves) after every update. New inbox entries arrive in <Playtest>\INBOX.md.

### Held 0.24.2 work found in the tree (uncommitted: the previous run's rule was "nothing is committed unverified")
A snapshot is kept at git ref refs/wip/pre-0.26-2026-10-23 (fff08f6; no branch, Config files left out). I verify each piece in-engine and commit it on its own before building on it:
- co-op net base: ASNetBase.h + PreReplication in ASCharacter and ASEnemy, cvar as.net.dropbase (12.7k "NOT Supported" warnings in the live playtest; may be tied to G1)
- elite brutes as smaller Titans (off by default) and the elite mutation tint (ASEnemy, ASEnemySkeletal)
- softer pickups (glow 4 to 1.6), vault chest glow, Endless floor debris relief (cvar as.floor.relief .25)
- dev harness: as.dev.perf (GPU frame time; Build.cs adds RHI), new DevScripts, RunCoopCheck.ps1, import_vault_assets.py
- Config/DefaultInput.ini and DefaultEngine.ini are editor re-saves: never committed; the clean test wrapper swaps in the committed ones.

### Plan progress
- [~] Settle the held 0.24.2 work: the net-base fix is verified and committed (f878474); the aesthetic pieces (floor relief, pickups, vault chest, elite brutes as small Titans) are next
- [x] Phase 0 DONE (2026-10-23, 10 commits f878474..9778d50, each checked in-engine with numbers + screenshots; unity + non-unity builds clean, 0 warnings from our sources, run_tests exit 0 with all 23 suites):
    G1 co-op elevation desync (3f89f4e): the engine rebuilt every proxy's mesh from a cached (0,0,0) offset, 88 cm too high and turned 90 degrees; fixed, before/after numbers in MERGE_LOG
    G2 posture (21d3823): blade and maul stances were bent over; now upright (TwinBlast guard, Steel's two-handed carry)
    G3 hammer 3rd swing (6907bf6): finisher speed 55% -> 6%, feet and travel match
    G4 hammer (2476d2e): left hand held on the shaft by a two-bone IK through every swing; hammer drawn 1.35x
    G5 boss roar (0e3c86f): rebuilt from scratch (4 ms onset, 67 Hz pulse, deep); old roars kept, as.roar.old 1 plays them
    G6 footsteps (908ce7d): fire on real foot strikes (21 steps, strictly alternating), 3 dB quieter
    G7 downed marker (eb27f83): ice-white under the blood moon, red elsewhere
    G8 launches (9778d50): about 22% farther (slam 7.2 -> 8.8 m)
    UNVERIFIED by humans (feel / art / ears): the new roar (lion vs synthesiser), the blade's new guard stance, the hammer's size and elbows, the finisher feel at 6%, footstep level, plus the standing list: wall slams, pile-ups, elemental fairness
- [ ] Phase 1: Twin Claws + Pounce (with presentation)
- [ ] Phase 2: team abilities - Mark & Pop / Link / Twin Cast
- [ ] Phase 3: skill tree, three lanes
- [ ] Version 0.26.0 / protocol 2600, export the playtest zip, announce it here
- [ ] Phase 4: polish loop (every pass marked POLISH) until Josue says wrap up

### For the next playtest (Phase 0 A/B switches, all console variables)
- as.roar.old 1 plays the 0.24.2 roars; as.hero.twohand 0 drops the hammer IK; as.hero.stance <clip> tries any stance clip; as.foley.gain <x> scales the footsteps; as.net.dropbase 0 turns the net-base fix off.

### Needs Josue (0.26 run) - nothing blocking
- Pushing: the 0.26 plan says nothing about pushing, so I commit locally only. Say "push" (or restore the 10/19 "push when green" order) and I push astra/0.23-handoff.
- Bridge location (FYI): it serves from the Playtest folder and I mirror STATUS.md there. If you would rather serve from the main clone, copy mission-control-server-v4.py to C:\Games\AshenSpiral and start it there.
<!-- 0.26-RUN:END -->

## 0.24.2 PLAYTEST ZIP (Edison 09:01 order) - for Josue and P2; both PCs must run this exact file
- Path: C:\Games\AshenSpiral_UE5_0.24.2_Playtest.zip
- Size: 733,522,091 bytes (699.5 MB), 824 entries. SHA-256 9CD2CDF9B19ACEA9D658151FB9CE63E4C2903DA8173F184D6BF7EFAC1B6CB631
- Built from HEAD 6b57967 (the boss-roar commit) by Tools/export_playtest_zip.py: git archive of the committed project plus all 376 local, gitignored assets the build uses - the Mannequin hero and its 45 clips, the Dragon Cave ruins with their own textures, the courtyard floor, the weathered-concrete blocks, the Molten hammer, the footsteps, the three roars. Version 0.24.2, protocol 2402.
- Checked from a fresh folder: extracted, its own Setup (Tools\Setup.ps1, or Setup_and_Open.bat) built and imported with 0 errors in 3m38s, and the game ran - every roar played its delivered file, the arena looked as on this machine, no missing-material fallbacks.
- On each PC: extract to a new folder, run Setup_and_Open.bat (or Tools\Setup.ps1), then play as usual. Test_Two_Players.bat for a local two-player check.

## State when the 0.26 run began (2026-10-20, 0.24.2) - older history is below, newest sections at the bottom of the file
Version 0.24.2 / protocol 2402. Branch astra/0.23-handoff, HEAD 3408fa0 (+ this STATUS update), pushed. Every item: UBT editor build unity + non-unity clean, Tests/run_tests.py exit 0, and checked IN-ENGINE (scripted standalone runs; screenshots in Saved/Screenshots/Dev).
- Hub (3408fa0, after the zip): the portals are turning spiral vortices in their colours (sealed ones a dark slow smoke) and the station flames burn instead of glowing white eggs. Checked in-engine.
- Reward chest (60a34d7, after the zip): it is a wooden chest with a domed lid and gold bands; it had drawn as a stepped octagonal block since the first meshes (its lid was modelled standing upright). Checked in-engine.
- Effects (2035790, after the zip): swing arcs, slashes, shockwaves and ring flashes are streaked bands of light with soft edges and a white-hot line instead of flat cream shapes; burning/frozen/poisoned/enraged enemies glow over their armour instead of turning orange. Warnings stay solid. Checked in-engine.
- Enemy armour (c03bb40, after the zip): the armoured enemies' plates, weapons and shields are worn painted metal - mottled paint, bare steel along scratches, fine hammered relief (Paragon's surface layers from the Game Animation Sample) - instead of flat paint. Checked in-engine (close-ups).
- Read from the LIVE co-op playtest's host log (69 min, P2 joined from another PC, Endless shuffle 19): 0 errors, 0 ensures, no crash; the roars play; the knock/launch fixes are being exercised (~220 wall slams, ~300 pile-ups). One real finding: 12.7k `FNetGUIDCache::SupportsObject ... NOT Supported` warnings - characters standing on the arena's run-time-built floor replicate it as their movement base, a client cannot resolve it, and UE then skips movement simulation for those proxies (enemies and the other hero shown without smoothing on P2's machine). Fix written and held (ASNetBase.h, ~12 lines of effect: the replicated copy drops a base no client can name); it is measured before and after with a two-instance co-op probe once the machine is free of the playtest. Not in the zip either way.
- Ward bosses are the Warrior Titan (86d078d, Edison 08:30 items 2-3; after the zip, so NOT in it): the four ground bosses are now a textured armoured warrior from Fab (bronze plate, gold trim, cream cape) instead of the 21-part armour rig, about 2.4x a hero's height. It walks with Unreal's locomotion and plays a fitting animation for every boss attack (overhead smash, ground punch, summoning cast, the volley throw, a pulling sweep, the half-health fury flex, the charge's blow), glows with the same rage and status pulses, and falls with the Mannequin's death before the usual flare. Each ward's boss has its own colour cast. Checked in-engine (look_boss.txt: arrival, walk, five attacks, fury, phase two, death, a second ward). UNVERIFIED for human eyes: whether it fits the art direction (P2's call; the asset plan held it for that) - `as.boss.skeletal 0` brings the armour rig back. The flying boss keeps its rig.
- Boss roar (6b57967, Edison 08:30 item 1): the roars were a pitched-down hammer thud (no roar file had ever been delivered); now real roars cut from the Horror Template's monster sound - boss wind-up, the half-health roar, the elite's rage. Verified in a game run by recording the game's own audio: each roar is among the loudest sounds in the mix (10-13 dB above the game's usual level, 3-6 dB above hits).
Done today and pushed:
- Fix batch F1-F8 (d257312, cf0e428): launches fly and slam, wall slams/pile-ups hurt, element procs scale with hits, no collapsing enemies, ring-attack warning, weapon glint, real lightning bolt.
- Endless map (f48e00d): walls pushed out beyond the ruin ring, which is now solid ground to explore. Footsteps quieter and natural.
- Phase F (7ad453e): heroes are the Unreal Mannequin (P1 Manny, P2 Quinn) with Unreal's locomotion, weapon stances and attack animations; the old rig is the fallback.
- Carry-over verification (9737c88): elite mutations, ward bosses, flying boss, proc chains, metamorphoses, reward ceremony, death screen, co-op on one PC, Endless, hub, collapses - all PASS in-engine.
- AAA aesthetics passes 1-4 (fcbab93, 7ec661d, 59a2d97, 724e77e): the ruin ring had been drawing as Unreal's grey grid since 0.25 phase C, and every weapon and several enemies had parts drawn as the grid since the first meshes - both fixed; the ruins wear the Dragon Cave pack's own textures; ruined-courtyard floor; weathered-concrete cover blocks; volumetric clouds per sky stage, sun haze, light shafts; effects are real light that fades; lit vaulted chambers; weapon finishes; your chosen colour on the Mannequin.
Playtest zip: C:\Games\AshenSpiral_UE5_0.24.2_Playtest.zip (carries all the local content; checked from a fresh folder: Setup 0 errors, game runs). Commits after 6b57967 (the Titan) are not in it; it is re-exported only on Edison's order.
Needs Josue: nothing.
Not blocking (Gate 0 notes): the Fab/vault licences of the Molten hammer, Concrete column, Dragon Cave, Game Animation Sample and Warrior Titan are still unconfirmed in Docs/CREDITS.md.

## Night report (2026-10-16, historical)
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
- Bridge v3 (POST /exec) is already LIVE on port 8765 (installed by someone else, not by me; exec.log shows read-only git calls from Edison since 15:35) and its allowlist can be bypassed for arbitrary code execution. Replace it with a hardened version, revert to v2, or keep as is? See Notes.

## Token usage and cost
Not available to me: this session exposes no usage counters, so I cannot report input/output tokens or USD without inventing numbers. Please read it from the Claude Code /cost or /usage view.

## Notes
- v3 server (checked 2026-10-16 15:36): I did not install it, but it is already running (mission-control-server.py is the v3 text and netstat shows it listening on 0.0.0.0:8765). exec_allowed() accepts any command whose first token is `python` and that merely contains the token `Tests/run_tests.py`, e.g. `python -c "<code>" Tests/run_tests.py` runs arbitrary code. The git allowlist also permits file-writing options such as `git diff --output=<path>` and `git branch -D`. The server binds 0.0.0.0 with no authentication, so anyone on the LAN or tailnet could use it, which contradicts the comment that the worst case is reading git history. I have not stopped or edited it. Proposed hardening if you want it: exact-match command list (no free arguments), git restricted to fixed read-only invocations without --output, bind to the Tailscale interface only, shared-secret header.
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

## Weekend round (2026-10-16 evening)
- Port source: C:\Games\Astra_0.23\AshenSpiral_UE5_0.23.1_Hotfix1 is a complete tree (.uproject, Source, Content, Config); Complete_Export used only as cross-reference.
- #1 Bounds-click bug: menu/pause input now confines the cursor to the viewport in fullscreen (LockInFullscreen + LockMouseToWidget, refreshed each tick); windowed stays unlocked for two-window local co-op. UBT editor build OK, run_tests.py exit 0. UNVERIFIED in-engine: needs P2 to confirm in fullscreen (menus, party pause, alt-tab, multi-monitor).
- #2 Lucky shop: stall (mesh, collision, label) is now hidden/non-solid until a shop is open at the endless checkpoint (bLuckyShop && Reward phase), then appears beside (not on) the chest; station 222 is only interactable while open. Visibility is driven from replicated state in AASWorld::Tick on clients and server. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: appear/disappear timing, placement relative to chest, collision. Stock logic port (Astra, good-but-pricey) still pending.
- #3 Synergy feedback: on a synergy trigger each player gets a new client RPC SynergyCue(Trigger): a large word (LIGHTNING BLAST / STORMBREAK / WILDFIRE) coloured by synergy type floats above the crosshair damage numbers for 1.6s, plus a 2D sound. Audio is a PLACEHOLDER (existing S_Hit/S_Collapse/S_Spiral at different pitches). Build OK, run_tests.py exit 0. UNVERIFIED in-engine: word position/readability, colours, audio feel; note the added RPC changes the replicated function list (both PCs must run the same build). Honours the damage-numbers setting for the word only.
- NOTE: git history was rewritten mid-session by something else (LFS migration; my earlier commit hashes 7adacac/fdc4eb7 are now 846775c/cddf07a). Commit hashes in older STATUS sections may be stale.
- #4 Hammer 3rd attack: finisher no longer roots the player; movement speed is scaled to 20% (80% slow, ASHammer::FinisherSlow, TUNABLE) while committed. Leap still roots as before. Attack/dash/reload remain blocked during the finisher (unchanged). New test in hammer_022_tests. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: feel, and the planted-pose animation still plays during the finisher (feet may look planted while sliding).
- #5 Hammer gravity wells: removed the well plant/collapse that every hammer swing triggered (ASMeleeCombat.cpp). The Spiral ability's own wells (ASSpell/AASWell) and the Gravity Hook trait are untouched, since the order named hammer swings only. Build OK, run_tests.py exit 0 (the one build warning is an engine-header deprecation, not ours). UNVERIFIED in-engine: that no well appears on swings.
## Astra port (source: Hotfix1 tree)
- P1 Lucky shop stock logic: ported Astra's per-player rolled trait offer as a new item "Trader's Pick" (rolled from the player's build via ASBuild::Offers when the shop opens, replicated PlayerState::ShopOffer, 150 Scrap). Our shop UI kept (it won). Prices raised for "good items, HIGH price" (Dressing 60, Ward 55, Reroll 90 Scrap; Ember Core 30, Heart 22 Ash), all TUNABLE in ASExpeditionRules.h. Tests updated for the new prices. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: card text/layout, offer rolling, purchase. Known gap: ShopOffer is not in the save snapshot, so a rejoin mid-shop hides the Pick card until the next shop.
- P2 Portal queue flow (Astra's flow): E at an Ember/Ashen/Blood archway now opens a portal page (modifiers or LOCKED condition, seed, host-only ENTER). ENTER -> new server RPC EnterPortal -> GameMode::StartPortal (host, all-ready, at the archway, unlock checks), replicated bEnteringPortal, 1.2s walk-in of both heroes to the archway with "ENTERING THE SPIRAL" HUD, weapons blocked, then BeginRun/BeginEndless. The Endless portal's page ENTER uses the same path (portal 3). The old hub-menu difficulty cards and Begin button were removed (hint text instead) since the menu cycling was the confusing part. New replicated flag + 2 RPCs: both PCs need the same build. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: whole flow, walk-in look (heroes glide, no walk animation yet), co-op ready gating, host-only messaging.
- Standing direction (Josue): when a design call is ambiguous, prefer whatever is closer to Crab Champions' flow, incl. how new items are thrown into the run. Saved to memory.
- Damage-number personalities (replaces the synergy word + "CRIT" label): numbers now carry their meaning in motion, not text. Shared pure rules in ASPresentationRules.h (DamageLookFor): Crit = oversized punch, freeze beat, decaying shake, gold/orange halo and one shock ring; Burn = accelerating embers that flicker and cool to dark red; Poison = slow wobble that drips down as it dies; Frost = snaps in, hangs still with a shiver, shatters outward; Bleed = heavy, falls like a drop; Shock = erratic stepped jitter and flicker; Spiral = orbits as it climbs; Synergy (Lightning Blast / Stormbreak / Wildfire) = biggest numbers, slam-in, long hold, tremble, three shockwave rings, coloured by synergy type. Synergy hits previously showed no number at all; each target hit now sends one to both players via new RPC SynergyNumber. The SynergyCue sound stays (placeholder). Tests: persona bounds/distinctness added to interface_arsenal_tests. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: how each persona actually looks and reads (all of it is feel), readability at 64 simultaneous numbers, and sync with the placeholder audio.
## Pizzazz queue (order from Josue: 6,1,3,9,8,7,5,2,10,4)
- #6 Wave start/clear: HUD banner band sweeps in from the left, holds, exits right (rules + test in ASPresentationRules/interface_arsenal_tests); fires from replicated state: "WAVE n [OF N]" at each wave start (BOSS in red), "CHAMBER CLEARED" / "WAVE n CLEARED [CHECKPOINT CHEST]" on clear. Plus a server-side slow-motion beat (30% speed for 0.45 real seconds, GameMode::ClearBeat) on the final clear of a normal chamber or non-boss Endless wave; it resets on hub/pause. No protocol change except the existing world time-dilation replication. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: banner look/placement/readability, slow-mo feel and that dilation replicates cleanly to the client and restores (watch for stuck slow-mo), banner not double-firing on reconnect.
- #1 Kill moments: enemies used to vanish with no death effect. Every kill now throws a burst of glowing shards (new cosmetic actor AASShardBurst, effect kind 11 through the existing GameState effect multicast) tinted by the enemy's active status (bleed/burn/poison/frost, else warm gold). Tiers from ASPresentation::KillBurstFor: normal 8 shards, elite 16 (bigger, brief 0.07s hit-stop at 15% speed), boss 30 (largest, 0.16s hit-stop at 12%, low rumble). Hit-stop reuses the real-time dilation guard from the wave beat (never overrides a running beat). At most 14 bursts live at once. Tests added (tier ordering, hit-stop bounds, shard arc). Build OK, run_tests.py exit 0. UNVERIFIED in-engine: how the shards look and read against the arena, hit-stop feel at elite/boss tiers, performance in a swarm wipe, whether the status tint is the right proxy for the killing element.
- #3 Pickups (Crab Champions flow) + port item "pickup graphics": Astra's pickup meshes (SM_HealthPickup/ShieldPickup/Crystal) and pickup code are byte-identical to ours, so there was nothing to port; the difference had to be flow, so I built the flow. Drops are tossed up, bounce and settle into a pulsing hover with a spin that eases down (all local/cosmetic on a child mesh); after 0.55s the server pulls a pickup toward the nearest player who needs it (health only if hurt, shield only if not full, cinders always) within 520cm, speeding up as it closes in; grab radius unchanged (170). Collecting plays a coloured ring flash plus a distinct chime per type (warm low health, bright shield, quick cinder tick; placeholder assets, different pitch). Rules and tests added (toss, bounce, hover band, magnet curve). Pickup root is now a scene component with the mesh as a child. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: toss/bounce look (no ground collision, so drops on uneven floor may float or clip by a few cm), magnet feel and range, chimes, replication smoothness of the pulled movement for the client.
- #9 Boss intro: during a boss's existing 2s spawn protection (AASGameState::BossIntroAge, no new replication) the HUD draws cinematic letterbox bars and slams the boss's name (from ASData::Bosses by Ward) in oversized red then settles it; the camera FOV pushes in by up to 10 degrees and eases back (skipped under Reduce Motion; bars/name still show). Rules + bounds tests in ASPresentationRules/interface_arsenal_tests. Build OK, run_tests.py exit 0. UNVERIFIED in-engine: look and timing of bars/name, FOV push feel, name text width/centering, whether Endless boss waves pick the right Ward name (uses Room's Ward).

## FINAL STATE at AWAKE (2026-10-17)
Stood down on the AWAKE entry after finishing the boss intro. No game launches were done.

DONE this weekend (each: unity UBT editor build OK + run_tests.py exit 0, all UNVERIFIED in-engine): bounds-click fix; lucky shop hidden until open; synergy numbers; hammer finisher 80% slow; hammer well removal; lucky shop stock port; portal queue flow port; damage-number personalities; wave banner + clear slow-mo; kill moments; pickup flow; boss intro.

NOT DONE / QUEUED:
- Astra port list not started: chests after wave 25 (RewardWave rules already encode every-other-wave / every-3-after-50, but not checked against Astra), endless arena port, camera look/zoom, minimap look + spawn markers, hammer RMB with arc-stop fix, lighting after wave 20, camera shake.
- Pizzazz not started: low-health state (8), enemy spawn telegraphs (7), chest/reward reveal (5), dash trail (2, also the P2 dash challenge: improve my own dash, do not use his), wave 50 transition (10), hammer slam (4).
- Verification pass: -DisableUnity build, crash-safety review of new RPCs (SynergyNumber, SynergyCue, EnterPortal, CloseMenuForPortal), the single launch gate.
- Push: see Notes below for the result of the push to origin.
- Bridge v3 security question is still open under Needs Josue.
- Push result (2026-10-17): astra/0.23-handoff pushed to origin (c946608..dfd1608), no auth problems, no force needed. frankenstein/0.24-merge not created because backlog (a)-(c) are not green.

## Part 2 pizzazz (Edison's list, per CONTINUE ORDERS 2026-10-17 12:11)
- P2#3 wave-start banner (re-verified against spec): title now scales in fast with an overshoot; elite-wave variant added (ELITE WAVE, orange) next to the existing boss variant (BOSS, red). Test added. UNVERIFIED in-engine.
- P2#5 kill pop (re-verified): hit-stop retuned to the spec window, elite 35ms / boss 50ms (was 70/160ms); every damage number now pops in (30% overshoot, 60ms). Tests updated. UNVERIFIED in-engine.
- P2#7 pickup magnet (re-verified): pickups accelerate toward a player who needs them within 520cm; collect plays an expanding flash ring plus chime. Spec met as built. UNVERIFIED in-engine.
- P2#1 portal entry juice: FOV kick (+14 degrees in 0.25s, easing to a held +6 push during the walk-in; off under Reduce Motion) and a short fade to black over the last 0.5s of the 1.2s walk-in, driven by a client-side portal clock (PlayerController::PortalAge). Tests added. UNVERIFIED in-engine.
- P2#4 low-HP vignette: red frame of stacked soft bands, fully off at full health, intensity grows with missing health (max 0.62 alpha), pulses from 1Hz to 4Hz as health drops; non-finite health yields nothing. Hidden in the hub. Tests added. UNVERIFIED in-engine.
- P2#2 chest-open fanfare: claiming a chest reward makes the chest swell and settle (cosmetic copy of SM_Chest, effect kind 15, 0.45s), plus a gold ring flash and a low sound; rewards float up as "+X" text via new client RPC RewardText (e.g. +40 CINDERS, +35% HEALTH, +Run upgrade name, +N ASH at Endless checkpoints). Tests added (float text curve, chest pop). UNVERIFIED in-engine: chest pop overlaying the static chest cleanly, text position/readability, stacking when several fire at once.
- P2#8 trait-pickup fanfare: acquiring a trait (chest choice, Trader's Pick, Ember Core) shows a larger violet/orange banner "+ TRAIT NAME" high on screen and plays a placeholder sound hook (existing S_Reward at 1.15 pitch). Uses the same RewardText RPC with Big=true. Shrine trait trades are not covered yet. UNVERIFIED in-engine.
- P2#6 dash trail: while a hero moves faster than 1500cm/s (dash and hammer leap) every client drops a fading afterimage every 30ms (new cosmetic actor AASDashGhost: a stretched glowing body-coloured sphere that shrinks 45% and fades over 0.28s; existing Sphere mesh and style material, no new assets). Max 20 alive. Tests added. This is my own dash visual, not P2's code. UNVERIFIED in-engine: whether the silhouette reads as the character's afterimage, density at 20 cap, and whether the hammer leap trail is wanted.
- P2#9 menu button juice: every menu button (the shared SASMenu::Button builder, so cards, close, tabs) swells to 1.045x on hover, squashes to 0.94x on press, eased over 90ms via a render-transform attribute (pure curve in ASPresentationRules, tests added), plus a quiet high-pitched hover tick (existing S_Hit, placeholder). The reward-choice tiles use their own button and are unchanged. UNVERIFIED in-engine: transform pivot and scaling inside scroll boxes (clipping at list edges), hover sound volume, that disabled buttons feel inert.

## EXPORT for playtest (2026-10-17)
- Package: C:\Games\AshenSpiral_UE5_0.23.2_Playtest.zip (36 MB, 340 files, SHA-256 4c2c05ffcf4978329c186dd44150fd01ec3c1f418a2362120fe38833c73da66b), built with `git archive` from committed HEAD 3e05548 so it contains no Binaries/Intermediate/Saved and none of the editor's uncommitted changes (the editor had auto-added an Android file-server block with a security token to DefaultEngine.ini; that is NOT in the zip).
- Same layout as earlier packages: extract to a new folder on BOTH PCs, run Play_Standalone.bat (or Setup_and_Open.bat); first launch builds and imports source art into Content, so it takes a while. RELEASE_MANIFEST.json inside is the stale 0.22 one.
- Checks before export: unity AND -DisableUnity UBT editor builds Succeeded (zero errors, no warnings in project files), run_tests.py exit 0. No game launch was done by me.
- Playtest checklist: Docs/PLAYTEST_0.23.2.md (also in the zip), 21 numbered items plus known gaps.
- Not in this build: death slow-mo + stats reveal (Part 2 #10), the rest of the Astra port list, spawn-telegraph / hammer-slam / wave-50 juice, the dash challenge.

## 0.23.2 playtest-fix batch (2026-10-17 evening) - 19 of 21 items done, 2 need Josue
Bar per item: UBT editor build OK, Tests/run_tests.py exit 0 (run inside the MSVC env: `vcvars64.bat` first, otherwise the native tests are skipped and the script exits 2), crash-safety self-review. Full `-DisableUnity` editor build run once at the end on HEAD b424959: Succeeded, no warnings from project files. NOTHING verified in-engine (no runs, per orders). Details and confidence notes for every choice are in MERGE_LOG.md.

Done (commit, item):
- 3080d2a P0 cursor: confined only during active gameplay (re-applied every tick); free in menus/pause; windowed unchanged.
- 8130fa4 Portals: one proximity "press E" prompt for Ember/Ashen/Blood/Endless; all players must be at the archway; host-only; lock reasons, seed, modifiers in the prompt. Old portal pages are dead code.
- 68fe942 HUD: duplicate wave toasts removed; arena-shift warning is a banner; low-HP vignette is a smooth gradient.
- cc94f9e Trait banner shown to the partner above the acquirer (chest, Trader's Pick, legendary, Ember Bargain).
- 420c737 Boss IMMUNE tag; menu buttons keep words intact (buttons widened 166->196).
- 0c3665f Final-kill slow-mo 30% for 2.0s with both cameras turning to the kill (engine ClientSetRotation, no new RPC); both downed = run ends at once (also Endless/solo).
- d66f5c8 Obstacles invisible to P2: client safety rebuild (LOW confidence, see below).
- 2dd99ef Run reward log + reworked tab menu (weapon, base stats, run-only build, long block last, no achievements/permanent entries). Heart of the Spiral and every shop/chest/checkpoint reward now listed.
- 5d305d5 New traits Attunement (+12%/rank status damage) and Teamwork (+20%/rank synergy damage, co-op only).
- 14ed528 Dash trail: character-shaped afterimages, driven by real movement (both players see both), plus hammer leap.
- b424959 Friend/foe rings (player-colour under players, red under enemies); character flair = same ring.
- fef9f6e Version label 0.23.2.

UNVERIFIED playtest checklist (all of the above is compile + native tests only):
1. Cursor: fullscreen, dual monitor. Free while paused/menu; cannot escape while playing. P2 at 4096x2160: if one edge still leaks, tell me which edge (plan: inset clip rect 1-2px).
2. Portals: walk up, prompt text correct per portal; "waiting for other players" when one is away; only host can press E; locked portals show the reason; seed field is now on hub menu page 1.
3. Wave banner is the only wave-state text; arena-shift banner appears 3s before a shift; vignette smooth, pulses faster as HP drops, off at full HP and in the hub.
4. Trait banner: P2 sees P1's banner above P1 (chest pick, Trader's Pick, Ember Bargain).
5. Boss spawn: grey IMMUNE tag during the intro. Menu: "ACHIEVEMENTS" etc. on one line, no card text colliding with buttons (pages 2/4/9).
6. Slow-mo: last kill of a chamber, and Endless reward waves only; both cameras turn to the kill; 2s feels like a beat, not lag; camera snap-back acceptable? Both players downed ends the run immediately (solo Endless too).
7. Arena blocks: P2 sees every block the host sees. If not, send P2's client log (look for "AshenVisual: client geometry safety rebuild").
8. Tab menu: lists Heart of the Spiral (x N, +25 max HP each), shop items, chest rewards, Endless exclusives, traits; no permanent/skill-tree lines; stats show permanent values.
9. Traits: Attunement raises burn/poison/bleed numbers; Teamwork only offered in co-op and raises synergy damage for both players.
10. Dash trail reads as the character, fades cleanly, visible for both players, also on hammer leap; performance OK. Friend/foe rings readable, not too loud.

## Needs Josue (0.23.2 batch)
- Item 21, circles by the chest after waves 20/50: I can't tell which visual this is (candidates: next-wave spawn-warning markers, chest arrival glyph, chest effect rings; nothing in code is keyed to 20/50). What do they look like (colour, flat on floor or floating)? Not changed.
- Item 11, arena shift cancels combos + freezes: not easily fixable. The shift rebuilds ALL arena geometry (floors too) in one frame. Real fix = partial rebuild of only the cover blocks. Recommendation: do it after the playtest together with the obstacle-visibility bug, since both are the same code and neither is verified. Not changed.
- Item 13 confidence is LOW: I added a client-side safety rebuild but could not find the root cause by reading. Needs P2's client log if it still happens.
- Count note: the 0.23.2 batch lists 21 items, not 22.
- Pushed: astra/0.23-handoff at 15018f2 (plain push, no force).

## 0.23.3 batch (2026-10-17/18 night) - 19 items + chest circles done, none verified in-engine
Bar: UBT editor build per item (unity), one full -DisableUnity build on HEAD f457c8a: Succeeded, no project warnings; Tests/run_tests.py exit 0 (inside vcvars64) per item; crash-safety review per item. f457c8a is the last code commit. Details, root causes and confidence are in MERGE_LOG.md (entries dated 2026-10-17/18). NOTE: commit 56aedd3 is a partial, unbuilt intermediate (colour change); bf69498 fixes it. Use HEAD.

Done: 1 cursor (exclusive fullscreen + 2px OS clip) | 2-5 portal prompt/ready/centre text/lock reason | 6 arena shift sink/rise/siren/slide | 6a combo-cancel fix (epoch) + cover-only rebuild | 7 slow-mo 1.0s, look lock, enemy death | 8 tab text | 9 uniform tab cards | 10 dash trail (replicated, no walk trails) | 11 friend/foe colour | 12+14 hammer leap arc + speed | 13 finisher slow 45% | 15 info by default | 16 cover-blocks-shots removed (narrow) | 17 UI revamp (theme, menu chrome, HUD bars) | 18 hold-TAB setting | 19 Wide Arc trait | chest circles (3 Endless rings) removed.

UNVERIFIED 0.23.3 checklist (playtest):
1. P2 at 4096x2160: cursor cannot leave on any edge in borderless AND exclusive fullscreen; free in pause/menu.
2. Portals: prompt only at the archway; guest E = ready, host E = enter; centre "waiting" text; locked portals say what to clear; Endless portal works the same; hub menu has an "Endless records and upgrades" card.
3. Arena shift: 3s banner + quiet siren, blocks sink in the last second, new blocks rise for a second after a concrete-slide sound; NO combo cancel/freeze when a shift lands mid-combo (try a 3-hit and a hammer finisher); P2 sees every block. Report siren quality.
4. Final-kill slow-mo is about 1s, both cameras turn to the kill, mouse-look is locked during it, the enemy topples instead of vanishing.
5. Tab: no explainer text; every acquisition card is "NAME xN" + effect + tag; Hold-TAB setting works (default off).
6. Dash trails: both players see both players' trails; NO trails while walking/running; trails on the hammer leap.
7. Colours: other player reads green, enemies read red, your own character keeps its colour.
8. Hammer RMB: the arc continues from ledges (no vertical drop) and is faster than a full run; the 3rd attack slow feels right (45%).
9. Shift-hold info is now always shown; no "COVER BLOCKS SHOT" anywhere.
10. Wide Arc trait appears for melee weapons only and widens swings.
11. UI: menus (rounded panel, cards, hover) and HUD bars/backing read well; keyboard/controller navigation still works on every page.
Needs Josue: nothing blocking. Decisions I made on my own are listed as IFFY in MERGE_LOG.md. Not pushed: no push order was given for this batch.

## 0.23.4 batch (2026-10-18) - all 15 items done, none verified in-engine
Bar: UBT editor build per item (unity), one full -DisableUnity build on HEAD: Succeeded, no project warnings; Tests/run_tests.py exit 0 (inside vcvars64) per item; crash-safety review per item. Last code commit: 671fade. Details, root causes and confidence in MERGE_LOG.md (entries dated 2026-10-18). Correction: the 0.23.3 section above says "Not pushed"; it WAS pushed (ab85081).

Done: 1 slow-mo removed (hit-stop and topple kept) | 2 host-driven portals | 3 siren removed | 4 player AND enemy rings removed | 5 hold-TAB single key listener | 6 P2 wall collision split from animation | 7 uniform wall height (380) | 8 shift v2 timeline + tomb sounds | 9 ease-in blocks with slam thuds | 10 Wide Arc +20%/rank | 11 triangle ally marker | 12 hammer leap aims at the crosshair incl. height | 13 lightning strikes (Endless 50+) | 14 melee hit-confirm + edge-of-reach cue | 15 Gloomwing flying enemy.

UNVERIFIED 0.23.4 checklist (playtest):
1. No slow-mo on the last kill; enemies still topple and fade.
2. Portals: only the host can depart, when everyone is in range; no ready prompts, no centred message; locked portals still show their reason.
3. No siren. Shift: banner 3s; 1-2s blocks sink with a low tomb sound; at 2s a thud; 2-3s new walls rise (second tomb sound); thud and dust when they finish; blocks visibly accelerate. P2 walks into sinking/rising walls: collision matches what P2 sees (host too).
4. No ring under any player or enemy; hue tints remain; a green triangle with "P2"/"P1" follows the other player (edge pointer off screen, red when downed).
5. Hold-TAB (setting on) on P2: the panel stays open while held and closes on release, no flashing. Toggle mode still works.
6. All pillars and walls are the same tall height; low blocks are still jumpable.
7. Wide Arc is clearly noticeable (rank 5 doubles reach); every melee hit ticks faintly and edge-of-reach hits get a brighter tick and a gold ring.
8. Hammer RMB: aim up at a ledge/down off a ledge and the leap lands on the crosshair point; continuous arc from elevation; still fast.
9. Endless wave 50+: lightning strikes show a floor outline with a filling ring, hit after 0.5s, favour crowds, hurt players and enemies; check damage feel (20% / 30%).
10. Gloomwings: appear in story from room 4 and Endless from wave 8; hover low, circle, telegraph, dive; melee and ranged can hit them; both players see them move, flap and die (topple).
Needs Josue: nothing blocking. Judgment calls are IFFY in MERGE_LOG.md. Not pushed: no push order was given for this batch.

## 0.23.5 batch (2026-10-18) - all 15 items done, none verified in-engine
Bar: UBT editor build unity AND -DisableUnity on the final tree: Succeeded, no project warnings; Tests/run_tests.py exit 0 (inside vcvars64); crash-safety review per item. Code commit: e5ddcd6 (one commit for the batch). Root causes, tunables and confidence are in MERGE_LOG.md (not committed).

Done: 1 co-op ready-up is host-only (Ready card and READY/NOT READY text deleted) | 2 P2 walls: collision on only when ~70% risen, held off while a hero stands in the footprint, smoothed client clock | 3 enemy rings gone, status effects pulse the body tint | 4 monster pathing root cause fixed (nav grid was built while new walls had collision off) | 5 hammer RMB no longer cancels when enemies start spawning | 6 hue tints removed | 7 low blocks full height | 8 shift slowed (sink/rise 1.6s, quadratic ease) | 9 max-range tick replaced by an in-reach gold crosshair | 10 lightning dodge 0.75s | 11 lightning scales (volleys up to 6, faster cadence) | 12 lightning 20% max HP for players and enemies | 13 pre-attack circles only for the melee swing | 14 Vanguard flat +25% for 10s | 15 flying boss on Endless boss waves 30/60/90.

UNVERIFIED 0.23.5 checklist (playtest):
1. Co-op: only the host presses E at an archway once everyone is in range; the guest has no step; hub menu has no ready card or READY / NOT READY text.
2. Endless after the first shift, as P2: walls look and block the same (walk toward a sinking wall and a rising wall; no invisible collision, no rubber-banding). Collision turns on only once the wall is mostly up.
3. Monsters path around walls after every shift (no standing against walls; watch for the stuck-and-teleport behaviour).
4. No circle under any enemy or player. Frozen / burning / poisoned / bleeding enemies pulse their body colour instead.
5. Hammer RMB cast during the countdown, just as enemies spawn, completes the leap.
6. No green or red hue on players or enemies; the P1/P2 triangle remains.
7. All obstacles, including the big squares, are full height; nothing is jumpable cover.
8. Shift pacing: 1s banner, blocks sink for 1.6s (slow start, fast finish, thud), swap, new walls rise for 1.6s; the acceleration is now noticeable.
9. Melee: the crosshair turns gold and opens up when an enemy is inside swing reach, normal again outside it; faint tick on each connecting swing, no gold ring.
10. Lightning (wave 50+): 0.75s to dodge; enemies and players lose 20% max HP; more frequent, and several strikes at once as the wave climbs (check wave 65 / 80 / 100 feel).
11. Telegraph circles: only plain melee enemies show one while winding up; shooters, leapers, shamans and bosses do not; the lightning outline is easy to spot.
12. Vanguard: +25% damage for the first 10 seconds of each room, one pick only.
13. Endless wave 30 (and 60, 90): the boss flies, circles, dives, and alternates dives with its normal attack; both players see it fly; it can be hit by melee when it dips. Waves 10, 20, 40, 50 bosses stay on the ground.
Needs Josue: nothing blocking. Judgment calls are IFFY in MERGE_LOG.md.

## 0.24 (2026-10-19) - ALL 7 MILESTONES DONE, nothing verified in-engine
Bar per milestone: UBT editor build unity AND -DisableUnity (0 errors; every warning is from an engine header, none from our sources), Tests/run_tests.py exit 0, native tests where logic exists, crash-safety review. Details, tunables and confidence: MERGE_LOG.md (not committed).
Milestones (7 in the approved plan; M6 was split into four commits):
1. M1 fixes: Ready orb removed, Hoarfrost Maul rename, golden-path text gone, violet-white lightning, per-swing slash arc, sniper/Broodcaller/Kind 2 bugs - DONE (ab27859)
2. M2 rings: every enemy ring hunted down, melee warning = exact damage shape, as.ringaudit - DONE (2bc2491)
3. M3 P2 walls: server decides Endless cover collision, host+client shift logging - DONE (8a56919); root cause still unproven, the next playtest logs (AshenShift / AshenCorrect lines on both machines) decide
4. M4 flying boss, per-weapon on-hit scaling, protocol 2400 / version 0.24.0 - DONE (b407a3c, 4369082, 4f80445)
5. M5 AI state machine, fluid movement, Tier 0 pose tracks, sound cues, skeletal spike (Mannequin deaths retargeted; no runtime skeleton) - DONE (02d12f3, b7f6c69, d69c39c)
6. M6 abilities, elite mutations, boss redesign, readability - DONE: a1 regular kinds (86bc5ce), a2 bulwark/sniper/broodcaller/Gloomwing (9d58ee1), a3 elite mutations (9761588), a4 full ward boss redesign (d1c5d49)
7. M7 Part C - DONE (cdd275f): C-3 perfect dash, C-1 proc echoes, C-2 impact knockback, C-5 reward ceremony without slow-mo, C-6 four metamorphoses, C-7 death-screen lines
Not done by design: Tier 1/2 skeletal rigs (Docs/SKELETAL_SPIKE_0.24.md: creature kinds have no animation source; humanoids use baked Mannequin death curves). Parked Part C items: dash-triggered traits, the rest of the metamorphoses.
Needs Josue: nothing. Answered 2026-10-19 16:42-16:46: Mannequin licence is a non-issue (stays in Unreal); Tier 1 creature rigs stay off (no Blender rigs) until P2 gives art direction after the 0.24 run.

### Part C, as built (all values TUNABLE in Source/AshenSpiral/Public/ASPartCRules.h)
- C-3 PERFECT DASH: a blow that lands inside the dash protection or within 0.15 s after it ends is dodged outright: dash refunded (cooldown 0, +1 charge), 0.15 s extra protection, gold ring, "PERFECT DASH" notice. 0.6 s cooldown. Dash-triggered traits are parked.
- C-1 PROC ECHOES: with Chain Lightning, Storm Touch or Impact Bloom, an element proc (burn, poison, bleed, frost) echoes to the nearest enemy within 4.5 m, then once more (depth 2), at 60% then 36% strength; one chain per 0.45 s.
- C-2 IMPACT KNOCKBACK: a launch of 250+ strength (Heavy Impact, Repulse) makes the enemy a projectile for 0.7 s: a wall slam hurts it (17-42 damage, credited to the hero), hitting another enemy hurts both and throws the struck one on at 65% strength (up to 2 hops, each weaker). Gravity pulls do not count.
- C-5 REWARD CEREMONY: opening the chest = 70 ms hit-stop (host only) and a smooth camera push-in (FOV -9) that holds while the choices are up, then the choices open after 0.4 s. No slow-mo. Off with Motion sickness reduction.
- C-6 METAMORPHOSES at the catalogue maximum rank: WILDFIRE (Kindling Touch x3: a burning enemy explodes on death, 50% of its health to neighbours within 3.8 m and re-ignites them), SHATTER (Winter Touch x3: a frozen enemy shatters, 60% of its health within 3.4 m and chills them), PINBALL (Heavy Impact x3: impacts do 2.5x), STORM WEB (Chain Lightning x3: two extra links, 40% more reach). A banner names it when you reach it.
- C-7 WHAT YOU LEARNED: the death screen shows up to three lines picked from what the run did (metamorphosis, echoes, impacts, perfect dashes, build size, co-op, Endless depth).

### UNVERIFIED 0.24 playtest checklist (everything below needs a human)
1. Hub: no Ready orb; Esc menu has the seed field; hammer is the Hoarfrost Maul; no "follow the golden path" text.
2. P2 walls (Endless, after several shifts): walls look and block the same for P2. If not, send both logs: compare AshenShift hashes/transitions and AshenCorrect lines.
3. Blade/Scythe show a crescent slash arc per swing. Lightning is violet-white (blue is frost only). No circle under any enemy; as.ringaudit lists nothing unexpected.
4. Flying boss (Endless wave 30/60/90): wings flap, stays in the arena, melee can reach it, hatchlings burst out at half health.
5. On-hit rewards scale with the weapon base swing rate: Hammer procs hit hard but rarely, Rotary procs often but light. Watch Rotary and Hammer.
6. Enemy fluidity: ranged kinds keep their distance and side-step, melee circle while recharging, frail ranged kinds retreat once at low health, stragglers still come to you. Every attack has a body wind-up and a sound (fallback sounds until Edison delivers E_*.wav).
7. Abilities: grunt shove, archer back-hop, duelist parry and riposte, brute ground pound and enrage, leaper rift blink, shaman meteor rain, bulwark guard-break, sniper tracking laser and relocation, broodcaller egg sacs, Gloomwing pack dive. Read each against Docs/TELEGRAPH_LANGUAGE_0.24.md section 7.
8. Elite brutes spawn with one mutation (frost, burning, shocking, venomous): tint pulse, extra ability, lingering zones.
9. Ward bosses: three attacks each, a 1.5 s roar at half health with the ward set piece, then a harder second phase (Docs/TELEGRAPH_LANGUAGE_0.24.md section 8). Endless wave 50+ bosses keep the old attack.
10. Armoured walkers die in one of six collapses by the direction of the killing blow.
11. Part C (above): perfect dash feel and window; echoes visible and not overwhelming; wall slams and pile-ups hurt and chain; chest opening pushes the camera in with no slow-mo; metamorphosis banner at max rank and its effect; death screen "WHAT YOU LEARNED".
12. Balance to watch: Hammer knockback/pull now ~2.2x per hit; Rotary ~0.3x; elite fire trail littering small rooms; echo chains with many procs.
Judgment calls and confidence per item: MERGE_LOG.md.

## 0.25 asset research (2026-10-19) - research only, nothing imported
Doc: AshenSpiral_Claude_Handoff_2026-10-15/current_work/AshenSpiral_UE5_0.23.0/Docs/0.25_ASSET_RESEARCH.md
11 assets in the vault (8 FBX packs, 3 Unreal projects). Usable with selection: 3. Need work: 6. Skip for now: 2.
Top 5: Dragon Cave stone kit (endless arena skin and decay), Paragon animations retargeted to Manny, Molten Face Hammer (elite hammer enemy and slam socket), Concrete column texture (arena blocks), Game Animation Sample (foley, player model option).
Needs Josue: (1) licences are NOT in the vault files; confirm each listing's licence on its Fab library page. (2) Four listings are AI-generated (Inferno Dragon, Warrior Titan, Harpy, Stonereaver): decide if acceptable. (3) Replacing the player model is HIGH-COST: only on your say-so.
Bugs from the 0.24 playtest are separate and should land before art touches those systems: P2 invisible walls, endless cover desync, flying enemies leaving the map, unlit sigils/vault (looks like a regression, cause not investigated).

### 0.25 asset plan finalized (2026-10-19) - plan only, no code, nothing imported
Doc: AshenSpiral_Claude_Handoff_2026-10-15/current_work/AshenSpiral_UE5_0.23.0/Docs/0.25_ASSET_PLAN.md
Phases: A hammer + import pipeline (S), B concrete block material (S), C endless arena skin and decay stages (M), D Paragon enemy animations (L), E audio (S). Held: player model, AI creatures.
Needs Josue: (1) licence check on the five packs, (2) bug-fix batch before B and C, (3) Paragon repackaging only if its licence is clear, (4) leave the player model closed, (5) AI creatures stay held for P2.

### 0.25 phase A done (2026-10-19) - UNVERIFIED in-engine
Unity and non-unity editor builds clean (0 errors, none of the warnings from our sources), run_tests exit 0, new native suite maul_025 (177 checks).
- Elite maul slam and its warning disc now land at the head of the maul, in front of the brute, instead of under its feet (Pound and the frostbound slam).
- Molten Face Hammer imported through the editor commandlet (Tools/import_vault_assets.py) and used by elite maul wielders. Local only: not committed; the game falls back to the built-in maul without it.
- Needs Josue: the Molten Face Hammer licence is still unconfirmed (Gate 0); Docs/CREDITS.md says so. P2's machine shows the built-in maul unless the imported assets are copied to it.
- Playtest: the slam ring and disc sit where the maul head lands; the molten hammer's size, grip and facing; whether it reads as molten (emission 5x, tunable in M_MoltenHammer).

### 0.25 phase B done (2026-10-19) - UNVERIFIED in-engine
Unity and non-unity editor builds clean (0 errors, none of the warnings from our sources), run_tests exit 0. Visual change only: the Endless cover blocks show a world-aligned concrete (M_ArenaConcrete, one 2048 tile cut from the Concrete column pack and made seamless). Their mesh, collision and shift code are untouched, so the P2 wall bugs (Gate 1) are not made harder to read.
- New: Tools/make_concrete_tile.ps1 (makes the tile with Windows' own imaging), and Tools/import_vault_assets.py now also imports it and builds the material. Local only, like phase A: without the import the blocks keep the flat tint.
- Needs Josue: the Concrete column licence is unconfirmed (Gate 0); Docs/CREDITS.md says so. P2's machine shows the flat blocks unless the imported Content/Art/Imported is copied to it.
- Playtest: blocks read as concrete (not a grey checker or a flat colour); tiling is not distracting; tall walls are slightly lighter than low blocks; sinking and rising blocks still look right. If the checker shows, open M_ArenaConcrete (the graph was built blind, no shader was compiled in the NullRHI import).

### 0.25 phase C done (2026-10-19) - UNVERIFIED in-engine
Unity and non-unity editor builds clean (0 errors, none of the warnings from our sources), run_tests exit 0, new native suite decor_025 (1.7M checks over 400 seeds).
- The Endless arena has a scenery ring beyond its walls in three stages that follow the sky (waves 1-20, 21-49, 50+), a ground plate under it, and treasure props round the chest. Visual only: no collision, nothing inside the playable square.
- 63 Dragon Cave meshes imported locally (SM_DC_*); a machine without them keeps the bare arena. Local only, like phases A and B.
- Needs Josue: the Dragon Cave licence is unconfirmed (Gate 0); Docs/CREDITS.md says so. P2's machine shows the bare arena unless the imported Content is copied to it.
- Playtest: the ring is visible over the walls and reads as ruins, not clutter; the stage changes at waves 21 and 50 are noticeable; nothing floats; frame rate in Endless is unchanged; the loot reads as treasure beside the chest and does not block it.

### Movement audit (2026-10-19) - research only, no code changed
Doc: AshenSpiral_Claude_Handoff_2026-10-15/current_work/AshenSpiral_UE5_0.23.0/Docs/MOVEMENT_AUDIT.md
The hero is an ACharacter with a custom UCharacterMovementComponent subclass (dash as a custom mode with client prediction; GetMaxSpeed is overridden, so MaxWalkSpeed is unused). Differs from the template: speed 640 (real speed from ASRules::MoveSpeed), acceleration 2600, braking 2100, JumpZ 660, air control .45, gravity 1.6, orient-to-movement off (the character faces the aim). Acceleration, braking and air control are safe feel changes. Gravity 1.0, speed 600 and orient-to-movement each break something (hammer leap tuning and tests, enemy balance, aimed attacks and animation). Recommendation in the doc: decide orient-to-movement and turning together with the player-model swap.

### 0.25 phase E done (2026-10-19) - UNVERIFIED in-engine
Builds clean (unity and non-unity: 0 errors, none of the warnings from our sources), run_tests exit 0, new native suite foley_025 (46k checks). The heroes now make footsteps: one per half stride while running on the ground, low volume, 12 concrete steps from the Game Animation Sample (local import, silent without it), the other hero fainter with distance. The boss-roar sound from the Horror Template was NOT used (6.4 s, unheard). Needs Josue: the Game Animation Sample licence is unconfirmed (Gate 0). Playtest: steps are audible but not annoying, they match the feet, they stop when airborne, dashing or standing, and the other hero's steps fade with distance.
### 0.25 phase D (state): Paragon clip pipeline built and run (import, sample, bake, preview) for slash, shove and pound; the baked tracks ship OFF because they previewed worse than the hand-made poses. Another session owns the rest of D.

## 0.24.2 fix batch (2026-10-19/20 overnight, Opus session) - F1-F8 DONE, pushed (d257312, cf0e428); version 0.24.2 / protocol 2402
Bar per item: UBT editor build unity AND non-unity Succeeded (only engine-header warnings), Tests/run_tests.py exit 0 (new suite element_0242), and NEW: checked IN-ENGINE where it is visual or physical (standalone game, test profile slot 95, scripted by the new as.dev console harness; screenshots in Saved/Screenshots/Dev). Feel still needs the playtest.
1. F1-F3 knockback: launches now FLY (a low hop then a skid, AI suspended until the body rests): maul slam ~7 m, leap landing ~4 m, Repulse ~3 m, elites a bit over half. Wall slams and pile-ups are found by a sweep every tick (NotifyHit kept as a second opinion), hurt 10+5.5% of the launch strength times the hero's damage, a slam rebounds and dazes 0.5 s and smashes crates/crystals. In-engine: slams and pile-ups fire (log lines "AshenKnock:"), a crowd launched together no longer "piles up" on itself. Root cause of the playtest failure: ground friction, braking and the AI's own stops killed every launch in ~0.1 s / 20 cm.
2. F4 elemental: element procs carry damage proportional to the hit (burn 1.8x, poison 2.1x, bleed 1.6x, frostbite 1.4x of the hit per proc, before the per-weapon on-hit scale), the weapon's own element adds 18% x scale, and frost makes enemies brittle (+15% damage chilled, +40% frozen). Tested parity: an element rank adds about what a Chain Lightning link adds, on all nine weapons. The Hoarfrost Maul's own frost now adds 40%+ to its hits.
3. F5 collapsing enemies: the Paragon clips are OFF again (they were the difference between the 0.24 playtest, where this passed, and 0.24.1, where it failed).
4. F6 ring attacks: their own warning - a thick outline where the wave ends with two bright ripples racing out to it (a filled disc still means "leave this area"); the wave itself is a ~40 cm glowing wall: jump it. Checked in-engine.
5. F7 melee: a white-gold glint flashes at the weapon tip 0.28 s before any telegraphed blow lands, with a faint "shing". Checked in-engine.
6. F8 lightning: NO lightning pack exists on this PC (vault, Fab library database, launcher cache, Downloads, project - all searched); the only download after the research is "Black Hole" (a space scene). The strike is now a real forked bolt (three flickering shapes), a flash of violet-white light, sparks and a scorch mark. If you meant a specific Fab listing, add + download it and it can be swapped in. Checked in-engine.
Also fixed (found in-engine): ring flashes stood as vertical arches over every elite slam/chain/wildfire/lightning, others floated mid-air -> all lie flat on the floor; the Bellwether's stampede strip never showed -> shows.
UNVERIFIED (playtest): launch distances and feel, slam damage balance, element balance with real builds, the ring warning and glint as read by humans, lightning look in co-op.
Needs Josue: nothing.

## 0.24.2 new-permission items (2026-10-20, Opus session) - Endless map, footsteps, hero look DONE and pushed; each checked in-engine
1. Endless map (f48e00d): the walls stand far out (5600, was 3100) beyond the ring of ruins, which is now solid ground to explore (each piece has a collision footprint, the same on every machine; later-stage pieces switch on with the sky and push anyone out of the way). The cover still fills the old fight square and the host still publishes it (no regression of the 0.24.1 sync). In-engine: walked into ruins, stopped at the surface every time.
2. Footsteps (f48e00d): quieter and natural - separate walk, run, strafe and backpedal sounds from Epic's Game Animation Sample, natural pitch, plus landing and jump sounds.
3. Hero look (Phase F): the heroes are the Unreal Mannequin - P1 Manny, P2 Quinn - with Unreal's own walk/run/strafe/jump/land and foot placement. Each weapon is held in a fitting combat stance (sword guard, heavy maul drag, bow, staff, gun at the hip, mortar on the shoulder) and swung with Paragon attack animations on the upper body, so you can run while attacking. Dashes, the maul's leap, flinches, heavy knockbacks and the downed fall are animated; dash afterimages are glowing copies of the pose. No cape (the base UE look). The old armour rig is the fallback (console as.hero.skeletal 0). In-engine: every weapon photographed idle and attacking (Saved/Screenshots/Dev/hero_*.png).
UNVERIFIED (playtest): P2 as Quinn over the network; how each attack animation's timing feels against its hit; reload and cast animations; the downed pose held in co-op (solo ends the run at once).
P2's machine needs the local content: run Tools/import_mannequin.ps1 there, or use the next playtest zip (it will carry Content/Characters). Without it P2 sees the old armour rig, which still works.
Needs Josue: nothing.

## 0.24.2 carry-over verification (2026-10-20, Opus session) - every item checked IN-ENGINE
Scripted runs of the real game (standalone, test profiles 90/91/95; scripts in Tools/DevScripts/verify_*.txt; screenshots Saved/Screenshots/Dev/ver_*, coop_*). What a script cannot judge is listed under UNVERIFIED.
- Elite mutations: PASS. Four elites, one per mutation, each in its own colour; each used its signature move (log "AshenAbility:"): frostbound glacial slam (cyan disc + ring), burning cinder burst (fire pools in a line), shocking storm call (sky column), venomous seed bombs (pools).
- Ward boss redesign: PASS. All four wards ran their own kits (Indexer: marks, page-storm ring; Bellwether: marks, toll; Horticulturist: blooms, ring, seeds; Sovereign: roots, crown slam), roared at half health and fought on in phase two with the hot-red pulse (phase-two slam, charge, seeds+bloom, roots). The Sovereign's GRASP did not come up in the 5 s window (phase two only, random pick).
- Flying boss: PASS. Wings, flight, and the dive onto the hero.
- Proc chains: PASS. With Chain Lightning, element procs echoed to neighbours (run facts: 2 echoes in one 3 s fight).
- Metamorphoses: PASS. Taking Kindling Touch, Chain Lightning, Winter Touch and Heavy Impact to their maximum showed the WILDFIRE, STORM WEB, SHATTER notices; frozen enemies shattered with frost rings when killed; a launched grunt slammed a wall.
- Reward ceremony: PASS. Opening the chest pushes the camera in through the hit-stop.
- Death-screen lines: PASS. "The ember dims, but what you recovered remains." plus a WHAT YOU LEARNED line drawn from the run (the perfect dash).
- Co-op (P2): PASS on one PC (listen host + guest). The guest is Quinn and the host Manny on both screens, with the P1/P2 markers; the guest's attacks and dash ghost (cyan) show; enemies, hazards and the arena match on both screens.
- Endless, no regression: PASS. Waves, the checkpoint chest ("WAVE 1 CLEARED"), cover, the ruin ring and the walls; the same cover on host and guest.
- Hub: PASS. Loads, the Expedition panel opens.
- Armoured walkers' collapse: PASS. Five grunts struck down together fold in different falls, then shrink away.
UNVERIFIED (playtest): balance and feel of all the above; two real PCs over the internet; the Sovereign's grasp; the Wildfire explosion itself (the burning enemies died between frames).
Needs Josue: nothing.

## AAA aesthetics pass 1 (2026-10-20, Opus session) - checked in-engine (screenshots Saved/Screenshots/Dev/look_*.png)
- Found and fixed: the Endless ruin ring has drawn as Unreal's grey checker grid in game since 0.25 phase C (a material flag missing for its Nanite meshes). It now shows real stone.
- The ruins wear the Dragon Cave pack's own textures: mossy boulders, weathered concrete columns, bark, bone and gold (35 pieces). Pillars, walls and bridge arches stay plain concrete (their textures need a renderer setting that lives in a file we never commit).
- The arena floor is a ruined courtyard: rubble and earth with old paving slabs surviving in patches, ash-toned.
- Real volumetric clouds that follow the stages (fair weather, storm deck, burning storm cells), warmer haze toward the sun, soft light shafts, ambient occlusion and gentler contrast.
- Weapons have proper metal/wood finishes; the hero wears your chosen character colour again.
UNVERIFIED (playtest): taste. Performance on P2's machine (real-time sky capture, clouds, 2048 textures); please report frame rate.
P2's machine needs the new local imports (Content/Art/Imported/Ground and /Decor; the dressed SM_DC meshes): the next playtest zip will carry them.
Needs Josue: nothing.
- (pass 3) Found and fixed an old bug: every weapon and several enemies had parts drawn as Unreal's grey checker grid (only the first part of each mesh ever got a material). Weapons now show clean steel, wood and bronze. Effects (swing arcs, flashes, afterimages) are now real light that fades out cleanly. The dark vaulted chambers have twice the lanterns and a brighter fill. Setup is sturdier (re-runs no longer fail).
