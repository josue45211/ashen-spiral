# Ashen Spiral — 0.21.0

**The Melee Patch + Forgotten Archives visual pass. Unreal Engine 5.8.3 source prototype.**

Extract the complete ZIP into a new folder and run **Play_Standalone.bat**. Both co-op players need **0.21.0 / protocol 2100**. Existing profile schema 1 and the shared per-user progress folder remain supported. Read **START_HERE.md** for setup and controls.

## What changed

- **Air dash:** every available charge can be used in the air, up to total dash capacity per landing. The counter is retained in saved movement and reconnect state.
- **All melee:** Blade, Hammer and Scythe have no magazines/reloads. Attack speed scales the whole swing, impact and recovery. Magazine/reload rewards are ranged-only.
- **Cinder Blade:** hold Attack for a fast three-hit cycle; hit three lunges along clear ground.
- **Grav Hammer:** hold Attack for left swing, right swing and a longer charged jump-slam with 2.6× damage. A longer shaft supports a two-hand grip throughout.
- **Enemy approach:** a bounded distance boost closes gaps, returning to normal speed before attack range.
- **Forgotten Archives:** inward-facing shelves, varied room dressing, pointed ribs, aged stone/wood/iron, broken tables, scattered folios and dust. Other biomes retain their existing art.

The written October 14 brief controls this version. Its referenced fix-candidates file and P2's eight-image reference pack were unavailable. The dash bug was confirmed in 0.20 source; Archives follows the written direction and still needs P2's reference review.

See **Docs/DESIGN_0.21.md** for tuning and the focused playtest; **Docs/ASHEN_SPIRAL_0.21_EDITABLE_CATALOG.txt** for all current content; and **Docs/MELEE_0.21_PREVIEW.png**, **FINISHER_0.21_PREVIEW.png**, and **ARCHIVES_0.21_PREVIEW.png** for source-geometry previews.

## Included systems

Nine weapons, six active abilities, Spiral wells, elemental effects, 65 selectable run traits and six core upgrades; four wards of eight chambers; 5–10 individual rooms per map with footprints up to 3×3; elites, bosses, hazards, vaults and persistent wager selections. The hub, practice dummy, permanent progression, achievements, save migration and two-player listen-server flow remain included.

Difficulty portals, fresh slots, boss synergy/arena redesign, endless, mastery, other-biome art and the full animation overhaul remain deferred. Melee posing is procedural modular animation, not a skeletal/mocap pack. The brief's Bow third-shot double was absent from the supplied baseline and is not added in this melee patch.

## Contents and verification

**78 original meshes, 12 material maps, eight sound cues and nine standalone C++ suites**, plus source, generators, setup helpers and documentation. Previews use actual meshes and shared pose curves.

Native suites and structural/source-art validation pass. **Unreal module compilation, material import, physics, audio, saves and two-player playtesting have not run here.** This is a source project; binaries and imported assets are generated locally. See **Docs/VALIDATION.md** and **Tests/validation_results.json** for results and limits.
