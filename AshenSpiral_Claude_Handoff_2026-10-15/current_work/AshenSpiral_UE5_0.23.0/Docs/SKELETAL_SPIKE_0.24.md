# Ashen Spiral 0.24 - Skeletal spike (order B3)

The order: spike first, rigid-skin the existing armour parts to the Mannequin skeleton, import through the editor commandlet, retarget `ABP_Manny_Combat`, no hand animation, free
commercial-safe packs only as a fallback, never buy anything. This is what I did, what it showed, and what shipped because of it. Pictures: `Docs/SKELETAL_SPIKE_0.24_PREVIEW.png`
(an offline geometry preview, not an engine screenshot).

## Result in one paragraph

Rigid-skinning our armour to the Mannequin works and looks good in motion, but it costs a lot for what it gives (new asset pipeline, shipped content, per-frame cost, no way for me
to test it without running the game) and its two visible problems (stubby arms, root motion) are exactly what retargeting fixes. So 0.24 does **not** ship a skeletal runtime.
It ships the *retargeting* instead: the Mannequin's own animation is sampled offline in the commandlet and baked into the pose channels of our data-driven rig (Tier 0), and the
game plays those numbers. First use: **all six Mannequin death animations**, so every armoured enemy now collapses instead of toppling and shrinking. Attack, hit-reaction and
locomotion bakes are ready to do the same way and are parked (list at the end).

## What I ran (and what I did not)

* **A throwaway content-only project** under `Saved/Scratch/spike_project`, holding a copy of the engine's own Mannequin template content (114 files, 93 MB). The real project's
  `Content/` was never touched and no Epic asset is in the repo.
* **Two editor-commandlet runs, import/analysis only, no gameplay**: a probe (23 s) and an export (11 s) through `UnrealEditor-Cmd.exe ... -run=pythonscript` exactly as
  `Tools/Setup.ps1` does for the art import (`Tools/spike_manny_probe.py`, `Tools/spike_manny_export.py`), started by `Tools/spike_run.ps1` against the project that
  `Tools/spike_make_project.ps1` creates.
* Offline, outside Unreal: `Tools/spike_manny_retarget.py` (measure, rigid-skin renders) and `Tools/spike_manny_bake.py` (retargeting). They need numpy and Pillow like
  `preview_armor.py`; I installed them in a throwaway virtual environment, not globally.
* **Not done:** nothing was played in the engine. A Python-generated *skeletal mesh asset* was not built (see "Why no runtime skeleton"), and `ABP_Manny_Combat` itself was not
  retargeted as an AnimBP (it only drives two montages, `AM_ComboAttack` / `AM_ChargedAttack`, over clips I sampled directly).

## What the engine gave me

| Fact | Value |
|---|---|
| Mannequin content in the engine install | `Templates/TemplateResources/High/Characters/Content/Mannequins` (skeleton `SK_Mannequin`, `SKM_Manny_Simple`, `SKM_Quinn_Simple`, Control Rigs, Physics Asset, 97 animation sequences incl. unarmed attacks, charged attack, 8-direction walk and jog, jump/dash/land, 6 deaths, 8 hit reacts, rifle and pistol sets) |
| Combat variant | `Templates/TP_ThirdPerson/Content/Variant_Combat`: `ABP_Manny_Combat`, two attack montages, a `BP_CombatEnemy` with a StateTree and EQS queries (evade, fallback, flank) |
| Python API in 5.8 | `unreal.AnimPoseExtensions`, `AnimPose`, `AnimPoseEvaluationOptions` are exposed: a pose at any time, any bone, in component space. Worked first time. |
| Skeleton | 161 bones (twist, corrective, finger and IK bones included) |
| Sampling cost | 37 animations at 30 fps for 27 bones: 11 seconds, 2.4 MB of JSON |

## What the numbers say about our rig

| | Mannequin | Ashen Spiral rig |
|---|---|---|
| Arm (upper + fore) | 27.8 + 27.3 = **55.0 cm** | 42 + 42 = **84 cm** (ratio 1.53) |
| Leg (thigh + calf) | 43.3 + 42.2 = **85.6 cm** | 38 + 38 = **76 cm** (ratio 0.89) |
| Shoulder height / half-width | 143.6 / 19.0 | 133 / 34 (drawn at .82 x .70 of that) |

Our knights have long arms and short legs by comparison. That one table decides most of the design below.

## The rigid-skin result (picture rows 1 and 2)

Our 21 parts driven by the Mannequin's bones, limbs fitted joint to joint every frame (so nothing ever disconnects), everything else rigidly attached to its bone.

* **Works.** The armour follows a real lunge (`MM_Attack_01`) and a real collapse (`MM_Death_Front_01`) with no gaps, and it is dramatically more natural than anything I
  can hand-author for a collapse.
* **Stubby arms.** Fitting 42 cm meshes to 27 cm bones squashes them to 0.65 of their length: the silhouette is no longer our knight.
* **Root motion.** The clips travel (the left death slides the body 2 metres), so the root bone must be stripped.
* **The cape** rigidly attached to the spine sticks up when the body folds forward; it needs its own drape (the Tier 0 solver already has one).
* **Estimated runtime cost** (not measured): 36 enemies x a 161-bone pose each frame is roughly 2 ms of animation evaluation, plus per-part transform updates on top of what posing 23
  components already costs.

## Why no runtime skeleton in 0.24

1. **Nothing to test with.** I cannot run the game here. A skeletal runtime (a hidden skeletal mesh component, bone-attached parts or a generated skinned asset, the right
   animation chosen per ability on every machine, clips shipped in `Content/`) is exactly the kind of change that fails only in a playtest. The data-driven solver can be checked
   offline against the real meshes (`Tools/preview_enemy_poses.py`) and by 660,000 native checks.
2. **The proportions mismatch costs fidelity either way.** Fixing it in a skinned asset means hand-tuning per-part scales; in the solver it is two ratios.
3. **Tier 1 creatures have no animation source.** The Mannequin is a biped; the leaper, broodcaller, Gloomwing and flying boss are not. Without hand animation (excluded) there is
   nothing to drive a creature skeleton with, and no free creature pack could be fetched and checked in this environment. They stay on Tier 0 poses (done), which is also what the
   preview shows works.
4. **Licensing and size stay simple** when only numbers ship (see below).

## What shipped instead: retargeting into the rig's own channels

* **Solver upgrade** (`ASEnemyPose.h`): the whole upper body can now hinge at the hips (`Bend` pitch and `BendRoll`), and hands are defined relative to their shoulders so arms
  hang correctly when the body bends. The neutral pose is bit-identical to before (test-enforced).
* **Baker** (`Tools/spike_manny_bake.py`): per frame it removes root motion, expresses the pose relative to the first frame (every curve starts at our neutral pose), rescales hand
  motion by the arm ratio and body motion by the leg ratio, decomposes the spine into a swing rotation (well defined even past horizontal), and fits each channel with at most 11
  keys (Ramer-Douglas-Peucker).
* **Six deaths** baked into `ASEnemyDeathTracks.h` (generated, committed): three front variants (doubled over), back, left, right. In game an armoured walker picks its variant
  from the direction of the killing blow in its own frame (`ASEnemyPose::DeathVariantFor`), plays it over the existing 0.75 s death, flares its visor and core, then shrinks away
  as before. Pictures: rows 3 and 4. Blobs, the Gloomwing, the flying boss and dummies keep the old topple.
* Everything runs on every machine from replicated state, so no extra network traffic.

## Licence: what I can and cannot say

* Epic labels the Manny and Quinn Mannequins on Fab "UE-Only Content - Licensed for Use Only with Unreal Engine-based Products" (search results quoting the Fab listing). That
  wording permits use inside games made with Unreal, commercial ones included, and forbids using them in other engines or redistributing them on their own. The Mannequin
  content inside the engine's `Templates` folder is Epic's own engine content under the Unreal Engine EULA.
* **I could not read Epic's EULA pages (HTTP 403 to my fetch), so this rests on search snippets and older forum answers, not on the text.** Please confirm the current wording
  before a commercial release. If it is ever a problem the exposure is small: only numeric curves derived from the motion are in the repo, never the mesh, skeleton or animation
  files. If it is ever a problem the baked death tracks can be replaced by hand-authored ones (same format), or dropped: the old topple-and-shrink is still in `AnimateDeath`
  for the kinds that do not use them.
* Nothing was purchased and no marketplace download was used.

## How to rerun the pipeline

1. `Tools\spike_make_project.ps1` (copies the template content into a scratch project).
2. `Tools\spike_run.ps1 -Script Tools\spike_manny_export.py` (samples the animations into `Saved/Scratch/manny/manny_samples.json`).
3. `python Tools/spike_manny_bake.py Saved/Scratch/manny/manny_samples.json deaths Source/AshenSpiral/Public/ASEnemyDeathTracks.h` (regenerates the header).
4. Build and run `Tools/export_enemy_poses.cpp`, then `python Tools/preview_enemy_poses.py poses.json <out>` (renders `poses_death_*` sheets from the real meshes). To look at
   the rigid-skin evaluation:
   `python Tools/spike_manny_retarget.py <samples.json> <out> rigid MM_Attack_01 MM_Death_Front_01`.

## Parked for a later update (each is the same recipe)

* **Attack variants:** `MM_Attack_01..03` as a grunt's three slashes, `MM_ChargedAttack` as the brute's ground-pound and the boss slam (needs the strike window found from hand speed).
* **Hit reactions** (8 clips): as an additive flinch on torso, head and hands layered over whatever pose is playing.
* **Locomotion:** the 8-direction walk and jog cycles as foot and body motion for strafing and backpedalling (today the gait follows the direction of travel procedurally).
* **Sniper:** `MM_Rifle_Fire` recoil.
* **A real runtime skeleton** if it is ever wanted: build it from the measurements above (limb fit per bone, the cape through the cloth solver, root motion stripped), keep the
  procedural rig as the fallback, and playtest it before anything else depends on it.
