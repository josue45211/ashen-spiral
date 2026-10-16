# Ashen Spiral Agent Status

updated: 2026-10-15 23:00 PDT
phase: Overnight run - everything implemented; independent review fixes applied; waiting for answers and playtest
current_item: idle (nothing left that is safe without answers); expand tests if time allows

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

## In Progress
- (none)

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
- B6 odds (review also flagged it): defaults give Cinders <=50 and >=1000 an expected return above break-even (1.08x / 1.09x), middle 0.74x. The 50/1000 thresholds were tuned for Ash, not Cinders. Keep or retune?
- B1 co-op: difficulty unlocks check the host's profile only. Intended?
- B8 elimination: downed or eliminated players return at the next checkpoint chest with 35% HP (TUNABLE). Keep, or stay out until the run ends?
- B8 economy: Ash/Scrap earn rates (Scrap 1/5/25 per normal/elite/boss kill, Ash 2 per checkpoint and 10 per boss, banked at run end) and shop prices are my placeholders. OK for playtest?
- B1 walk-in: the portal transition is a colour flood on run start, not an actual walk-into-the-archway animation. Acceptable for the placeholder pass?
- Smoke tests: you said skip launching the game; screenshot checks need a launch. Want me to try a headless NullRHI boot (log scan only) tonight, or leave all in-engine checks for the playtest?

## Notes
- Real clock stamps only; earlier stamps of 22:58 and 23:45 were wrong (system time was about 22:20-22:31).
- Build logs: C:\Games\b_*.log (latest: b_portal2.log non-unity); scratch build copy C:\Games\AshenSpiral_build_0.23
- Branch astra/0.23-handoff, nothing merged. Commits cfe023c to HEAD.
- Release metadata (README, version, catalog, manifest) still the 0.22 documents; Tests/validation_results.json says Unreal compile NOT RUN, which is out of date.
