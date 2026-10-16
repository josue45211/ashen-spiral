# Ashen Spiral Agent Status

updated: 2026-10-15 21:36 PDT
phase: Phase 2 - stopped at design-decision boundary (B6/B7/B8 not started)
current_item: waiting on Josue answers for B6, B7, B8 (see Needs Josue)

## Done
- Phase 1: C4458 rename, DamageReaction/AbilityGesture/cape definitions, ProfileId stamped in SendReceipt. Final check at HEAD 717b261: run_tests.py exit 0; UE 5.8.3 editor build Succeeded unity and -DisableUnity
- A3 cape: six-segment flowing cape (heavy-cape polish is the 0.23.1 track, untouched)
- A5 hit reactions, A1 Blade poses, A2 locomotion, A6 ability gesture: implemented (Scythe untouched)
- B2 second character: hub switch UI, server gating, client revert on refusal, fresh-slot commit failure handled
- B3-B5: audited; synergy liveness guards added; Vengeance mask restored to all weapons (answered by Josue)
- B1 difficulty: host selects Normal/Hard/Very Hard in hub; enemy HP/damage and offer quality wired; 1142 native rule checks (Tests/expedition_023_tests.cpp)

## In Progress
- (none)

## UNVERIFIED (compile + native tests only; no in-engine run was done)
- A3 cape: no visual check of drape, clipping, whip or flare
- A5 hit reactions: direction/sign of recoil rotation never seen
- A1 Blade poses: needs P2 art review (no reference frames supplied)
- A2 locomotion: cadence/lean feel, foot skating
- A6 ability gesture: never seen
- B1 difficulty selection: menu cards and replicated state never exercised, felt difficulty untested
- B2 slot switch: never run (two-player, reconnect, save-failure, A/B recovery all untested)
- B3-B5: synergy trigger/cooldown/kill-credit and lethal-save precedence never run in co-op
- Receipt stamping: reasoning + lexical test only, no live reward round trip

## Needs Josue
- Stale tests: I updated 3 tests to match intentional 0.23 changes (FullHeal is now an alias of Vengeance, Emberforge needs a capped trait, gait bounds 37/28 instead of 34/24). Are those behaviors correct?
- Scythe (A6): leave as-is until P2 looks, or redo its poses blind?
- B1 portals: built as hub menu cards, not physical portal geometry. Add physical portals now, or leave to the 0.23.1 gothic-portal track?
- B1 unlocks: Hard/Very Hard are open from the start (briefs give no gate). Confirm?
- B6 gambling: what is "a well"? The only well in the code is the hammer gravity well. Add a new wishing-well station (where: hub, every N rooms, shrine rooms?) or reuse the shrine?
- B6 currency: stake permanent profile Ash (needs a save-safe receipt debit and a live Ash sync) or run Cinders?
- B7 arenas: P2 art direction needed. Should I build placeholder arenas around each boss's attacks (untestable without running the game), or wait for P2 sketches?
- B8 Endless: ready to build as a vertical slice behind the Endless flag, but need: (a) entry as hub menu card or physical portal, (b) straggler rule between waves (keep enemy kind/HP, cap active enemies at 36?), (c) arena as a new layout in ASWorld or a reskinned existing one. Which?
- In-engine smoke tests: want me to attempt a PIE/standalone launch via Setup_and_Open (imports assets, long), or will Josue/P2 playtest from the UNVERIFIED list?
- INBOX.md: contains an instruction relayed via Edison that I did not execute as a command (it arrived through a file, not through you). Confirm you want the per-item definition of done applied?

## Notes
- Build logs: C:\Games\final_unity.log, C:\Games\final_nonunity.log; scratch build copy C:\Games\AshenSpiral_build_0.23
- Branch astra/0.23-handoff, nothing merged. Per-item commits cfe023c..717b261
