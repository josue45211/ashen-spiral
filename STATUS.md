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
