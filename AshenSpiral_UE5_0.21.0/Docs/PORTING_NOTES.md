# Porting basis and scope

The input was `Ashen_Spiral_v0.10.1_HOTFIX(1).zip`. The active Godot gameplay/content, reward, progression and persistence scripts were inspected. The new project independently expresses their core rules in Unreal C++. The source-content tables in this directory preserve the inspected vocabulary and data for reference; they are not all loaded as runtime JSON.

| Area | Included source implementation | Boundary / adaptation |
|---|---|---|
| View and movement | Third-person shoulder camera, collision probe, walking, jumping, dash, water slowdown, fall recovery | Replaces the 2.5D/top-down view; native jump restored in 0.12.0 and airborne dash added in 0.13.0. Character scale/ranges retuned to Unreal centimeters. |
| Weapons | Cinder Blade, Grav Hammer, Ash Bow, Spiral Glaive, Ember Staff, Rift Scythe | Reconstructed damage and well interactions; animation and hit feel need engine tuning. |
| Spiral | Normal/perfect release, pull, expiry/collapse, style behaviors, relic hooks | 3D aiming, line-of-sight tests and bounded actor populations. Not a frame-identical Godot simulation. |
| Content pools | 24 relic definitions, 10 styles, 20 ranked Hearth entries, six run stat upgrades, 12 talents | Some effects normalized to a 3D prototype; UI descriptions and implementation should be reviewed during balancing. |
| World/run | Four named wards, eight chambers each, elites, four boss themes, timed waves and personal chest offers | Procedural chamber variants, not the original authored geometry or a unique handbuilt map for every room. |
| Progression | Ash, Relic Embers, Tablets, materials, skills and loadouts | Version-independent local profiles, exports and reviewed imports from earlier Unreal saves. No Godot save migration. |
| Crafting | One-time 15 Scrap + 8 Bone Dust -> 40 Ash; repeatable 3 Spiral Cores + 12 Bone Dust -> 1 Skill Point | Failed/repeated invalid purchases preserve inputs; standalone rules tested. Engine save/write path not yet tested. |
| Co-op | Two-player listen server, readiness, shared run currency, independent rewards, pause, reconnect reservation | No relay/matchmaking, crossplay, dedicated-server product, anti-cheat, host migration or authenticated accounts. |
| UI | New Slate hub menus, preserved scroll, input rebinding, quality presets, minimap, HUD | Controller focus, unusual aspect ratios, accessibility and localization need acceptance testing. |
| Graphics/audio | Original 3D prop kit, articulated armor, textures, synthesized effects, dynamic-lighting settings | No finished skeletal animation, facial animation, cinematic sequences, production soundtrack or photoreal art pass. |

## Deliberate balancing/implementation choices

Every clearing has a route over solid bridges, even when optional gold-marked gaps are present. Invalid directions have physical wall barriers. Authority builds a capsule-clearance navigation grid from floors and solid bridges, checks the connecting sweeps, and filters spawns to the player-connected component. Four-way A* routes avoid prop corners. This has no runtime navigation-mesh dependency, but the engine collision results still need playtesting.

Waves have hard active-actor limits to keep bursts bounded. This is intended to control load, not evidence of a particular frame rate. Host and client use the same dash movement class. A minimum 35% health recovery is applied when entering the next chamber, including revived teammates, to make the longer third-person rooms less punitive.

Lucky Find grants a deterministic increase to common room materials rather than a random pickup drop modifier. Some brief status effects are not preserved through reconnect. Audio and visual effects are lightweight procedural replacements, not a lossless media transfer.

## Not a completed one-for-one remake

This package does not claim every original dialogue, lore entry, event branch, endgame variation, interaction, visual asset or edge-case behavior has been ported. Those require a content-completeness pass and live comparison with the original. The package focuses on a usable source foundation for the main expedition, combat, progression and two-player systems.
