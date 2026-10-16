ASHEN SPIRAL — CLAUDE HANDOFF
Export requested October 15, 2026 (America/Los_Angeles), at approximately 50% usage remaining.

THIS IS A RECOVERY / CONTINUATION PACKAGE, NOT A FINISHED 0.23 RELEASE.

Start with CLAUDE_START_HERE.txt, then handoff/IMPLEMENTATION_STATUS.csv and
handoff/CURRENT_WORK_NOTES.txt. The current source is deliberately frozen exactly
where work stopped. Do not assume it builds or that all requested systems exist.

CONTENTS
- completed_versions/: the five latest completed source packages, 0.18.0,
  0.19.0, 0.20.0, 0.21.0, and 0.22.0, with their original ZIP bytes preserved.
  "Completed" means previously delivered source packages, not Unreal-verified
  executables. See their original validation notes.
- current_work/AshenSpiral_UE5_0.23.0/: full work-in-progress source tree with
  tools, source art, configuration, tests, documentation, and extracted references.
  It combines partial 0.23 systems work with the 0.23.1 visual brief. There is no
  separately completed 0.23.1 build.
- prompts_and_inputs/: original uploaded prompts, media, logs, screenshot inputs,
  PDF decisions, and the historical user-supplied source ZIP. These are evidence;
  older instructions are superseded only where later prompts explicitly do so.
- catalogs/: the available editable catalogs for 0.20, 0.21, and 0.22.
- conversation/: available verbatim messages, a reconstructed continuation
  summary, recovered historical context, and an explicit coverage/gaps statement.
- handoff/: status, next steps, source delta from 0.22, validation evidence,
  release/input inventories, and operating constraints.
- MANIFEST_SHA256.json and SHA256SUMS.txt: checksums for every payload file.

CRITICAL WARNINGS FOR THE RECEIVING AGENT
1. The WIP has known missing animation/RPC definitions and fails source preflight.
   Do not present it as playable. Receipt/profile changes also need integration.
2. README.md, START_HERE.md, RELEASE_MANIFEST.json, 0.22 validation results and
   many documentation files inside current_work are inherited from 0.22.
   They do NOT certify this WIP. This outer handoff describes its actual status.
3. No Unreal compilation, asset import, runtime, save roundtrip, or two-player
   playtest was available in this environment. Native/source checks are narrower.
4. The conversation export is NOT the full verbatim chat. Earlier turns are only
   available in summaries and project documents. The exact coverage is explained
   in conversation/COVERAGE_AND_GAPS.txt; missing words have not been invented.
5. 0.23.1 is an additional visual scope, not a cancellation of 0.23 gameplay work.
6. User explicitly changed the export trigger from a future 20% warning to NOW
   at 50%. Implementation was stopped to produce this archive.

UNPACKING
Unzip this archive once. The current work is already an ordinary project folder.
Unzip a completed_versions ZIP separately only when you need that baseline.
For comparison, use completed_versions/AshenSpiral_UE5_0.22.0.zip and the supplied
handoff/CHANGES_FROM_0.22.patch; do not apply that patch on top of the WIP again.

Original input archives retain their names, including spaces/parentheses where
present. Build outputs/caches and private transfer tooling are not included.
