# Ashen Spiral Agent Status

updated: 2026-10-15 21:14 PDT
phase: Phase 2 - incomplete batch items
current_item: A2 locomotion (built OK, running tests, then commit); next A6 Scythe/G

## Done
- Phase 1 complete: C4458 rename, DamageReaction/AbilityGesture/cape definitions, ProfileId stamped in SendReceipt (commits cfe023c, 03ff6c2)
- Phase 1 verification: Tests/run_tests.py exit 0 (native + gameplay PASS under MSVC); UE 5.8.3 editor build Succeeded, unity and -DisableUnity
- A3 cape baseline: six-segment flowing cape implemented in Phase 1 (03ff6c2); heavy-cape polish belongs to the 0.23.1 track, left alone
- A5 hit reactions: player + enemy reactions and attack release consumed in animation (3b5a344)
- A1 Blade poses: cosmetic pose pass (95ad817); still needs P2 art review

## In Progress
- A2 locomotion: stride-matched cadence + acceleration lean written, editor build Succeeded; tests/commit pending

## Needs Josue
- Vengeance mask: currently Blade/Hammer only (mask 3, never approved). Keep narrowed, or restore to all weapons? Not baking it in; left as inherited until you answer.
- Stale tests: I updated 3 tests to match intentional 0.23 changes (FullHeal is now an alias of Vengeance, Emberforge needs a capped trait, gait stride/lift bounds 37/28 instead of 34/24). Are those three new behaviors correct?

## Notes
- Build logs: C:\Games\build_unity.log, C:\Games\build_nonunity.log; scratch build copy C:\Games\AshenSpiral_build_0.23
- Remaining order: A6, B2 (save/profile integration), B3-B5 verification, B1, B6, B7, B8
- No Unreal live-play or two-player testing has been run; only compile + native tests
