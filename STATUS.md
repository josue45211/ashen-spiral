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
