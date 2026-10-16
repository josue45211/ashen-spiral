# Ashen Spiral Agent Status

updated: 2026-10-15 21:19 PDT
phase: Phase 2 - incomplete batch items
current_item: B3-B5 verification (boss spawn protection, synergies, legendaries)

## Done
- Phase 1 complete: C4458 rename, DamageReaction/AbilityGesture/cape definitions, ProfileId stamped in SendReceipt; run_tests.py exit 0; UE 5.8.3 editor build Succeeded unity + non-unity
- A3 cape: six-segment flowing cape implemented (heavy-cape polish is 0.23.1 track, untouched)
- A5 hit reactions: player + enemy reactions and attack release consumed in animation
- A1 Blade poses (cosmetic, needs P2 art review), A2 locomotion (stride cadence + accel lean), A6 ability gesture wired; Scythe untouched
- B2 second character: hub switch UI (Progress page), server gating (hub only, no pending saves), client revert on refusal, fresh-slot commit failure now blocks instead of being ignored; compiled, tests pass

## In Progress
- B3-B5 audit of boss protection, synergy liveness guards, Vengeance/Phoenix precedence

## Needs Josue
- Vengeance mask: currently Blade/Hammer only (mask 3, never approved). Keep narrowed, or restore to all weapons? Left as inherited until you answer.
- Stale tests: I updated 3 tests to match intentional 0.23 changes (FullHeal is now an alias of Vengeance, Emberforge needs a capped trait, gait bounds 37/28 instead of 34/24). Are those behaviors correct?
- Scythe (A6): no visual assessment possible without running the game; leave as-is until P2 looks, or should I redo its poses blind?

## Notes
- Build logs: C:\Games\build_unity.log, C:\Games\build_nonunity.log; scratch build copy C:\Games\AshenSpiral_build_0.23
- Remaining order: B3-B5, B1, B6, B7, B8
- No Unreal live-play or two-player testing has been run; only compile + native tests
