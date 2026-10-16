# 0.17.1 HUD compile hotfix

The supplied 0.17.0 build log reports one blocking source error:

```text
ASHUD.cpp(259,55): error C4456: declaration of 'R' hides previous local declaration
ASHUD.cpp(163,64): note: see declaration of 'R'
```

The crosshair uses a radius named `R` in the outer scope of `DrawHUD`. The interaction highlight declared another `R` in a nested scope. Unreal treats this local-variable shadowing diagnostic as an error. The interaction radius is now named `MarkerRadius`, with every use in its highlight block updated. Values and drawing behavior are unchanged.

The Visual Studio preferred-version message and engine deprecation messages in this log are warnings, not the source error that blocked this build.

## Launch the corrected project

1. Close Unreal Editor.
2. Extract the complete ZIP into a fresh folder.
3. Run `Play_Standalone.bat` to rebuild, import and launch.
4. Check that the menu footer says **0.17.1**.

This package includes the full 0.17 HUD and mouse-capture update. Protocol 1700 and the per-user progress folder are unchanged. Both players should use the corrected package.

## Verification

The uploaded diagnostic was matched to the bundled source, and the corrected HUD block was compared with the original to verify an identifier-only change. Package contents, release hashes and ZIP integrity were checked. The previous standalone gameplay-test results are inherited; they were not rerun for this compile-only fix.

**A full Unreal build and live Windows/co-op testing remain unverified here.** If compilation still fails, upload the new `Saved\Logs\Build-Editor.log` so the next actual diagnostic can be addressed.
