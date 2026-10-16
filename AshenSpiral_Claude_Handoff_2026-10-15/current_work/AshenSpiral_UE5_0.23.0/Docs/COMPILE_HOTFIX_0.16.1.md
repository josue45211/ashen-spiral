# 0.16.1 compile hotfix

The supplied `Build-Editor(3).log` reports exactly two fatal project compile errors, both **C2668** at `ASWorld.cpp` line 336: ambiguous overloads for `FGenericPlatformMath::Cos` and `Sin`.

The fern-placement loop passed its integer index `J` directly to both functions. Unreal exposes separate float and double overloads, so neither conversion was preferred. Both calls now explicitly use `static_cast<float>(J)`. The seven fern placements retain the same intended angles and positions.

The Visual Studio 14.51 newer-than-preferred message and engine-header deprecation messages were warnings in this log. No compiler reinstall or engine-header edit is part of this fix.

## Install

1. Close the game and editor.
2. Extract the complete `AshenSpiral_UE5_0.16.1.zip` into a new folder.
3. Run `Play_Standalone.bat` from that folder to compile, import and launch. `Setup_and_Open.bat` is also available for editor play.
4. Verify the menu footer reads **0.16.1**. Both co-op players should use the hotfix.

All 0.16 features are included. Existing shared profile locations, profile schema 1 and network protocol 1600 remain unchanged. Keep the prior copy until the new version runs; copying old Binaries or Intermediate folders is unnecessary.

## Validation

- Confirmed the complete supplied log contains only these two project compile errors.
- Reviewed the other runtime trigonometry calls for the same integer-overload problem.
- Compiled the actual corrected expressions against float/double overload stand-ins matching the diagnostic; all seven positions passed finite/unit-circle checks.
- Confirmed the unpatched 0.16.0 expressions fail the same focused compile check with an ambiguous-call diagnostic.
- All five existing native test suites and source/art checks pass.

Unreal Engine is unavailable here. Full UE module compilation/linking, art import and live gameplay still require a local run. See `VALIDATION.md` for exact scope and results.
