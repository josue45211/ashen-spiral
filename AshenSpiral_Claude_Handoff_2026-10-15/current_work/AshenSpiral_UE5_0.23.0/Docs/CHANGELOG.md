# 0.22.0 — Grav Hammer (2026-10-15)

- Rebuild hammer grip/head alignment and the reference-mapped 3-hit combo; planted frontal finisher, committed wind-up/recovery, proportional contact pose holds and 1.2s haste-scaled grace.
- Add RMB hammer leap: 7.5s cooldown, vulnerable crouch, protected flight/recovery, 100-base radial damage on confirmed landing, per-player state/cooldown. Former hammer Spiral input moves to G / controller LT.
- Add knockback, cape response, combo glow, directional/radial debris and four synthesized sound cues; saved motion reduction disables all hammer shake.
- Protocol 2200, art revision 10. Profile schema/migration and all prices retained. Blade/ranged/boss attacks/Archives unchanged.
- Ten native suites pass. Source geometry previews included; Unreal compile and playtest pending.

# 0.21.0 — Melee + Archives

Air-dash counter across prediction/reconnect/HUD; no melee reloads; full-cycle attack-speed scaling; corrected melee grips; automatic Blade cut/cut/lunge and Hammer left/right/jump-slam; bounded distant-enemy approach. Archives adds inward shelves, varied seeded dressing, pointed ribs, three aged PBR sets and decay details. Network protocol 2100; save schema/migration remains unchanged. Nine native suites and source checks; Unreal validation pending. See DESIGN_0.21.md and VALIDATION.md. P2's referenced pack was unavailable.

# 0.20.0 — Armor, interface and verified-source bug fixes

- Keep Skill Points inside the resource group; allow reward title/brief rows to grow; start reload immediately after the last shot.
- Remove vault status text and turn claimed vault materials solid/non-emissive.
- Separate player-guide navigation from enemy clearance; choose among validated chest approaches and retain collision-checked jumps. Add the seed-6767/chamber-23 floor-model regression.
- Add original reference-inspired closed armor, blue visor/core, cape, sinister enemy variants and articulated procedural gait/aim/reload motion. Bootstrap recreates a material supporting smooth armor alongside textured world surfaces.
- Follow all 13 decisions in the latest PDF: Treasure Sense refund, retired material conversion, true relic Burn, text corrections, single-scale Second Echo, any-ability Rune Nova, nondecreasing shield grants, Guardian Link range, enemy phasing during Razor Dash and alternating side traps.
- Add modest level/achievement Ember/Tablet rewards, idempotent catch-up and earned cosmetic titles. No direct stat rewards. Preserve unrelated balance and content while the larger worksheet remains deferred.
- Protocol 2000; profile schema 1 with additive migration/reward fields. Eight standalone suites. Complete editable catalog and source previews included. Unreal compilation and gameplay verification remain pending.

# 0.19.0 — Reloads and reward cards

- Replace reward rows with illustrated rarity cards, short summaries and Shift-hover detail; introduce separate skill styling and original vector glyphs.
- Remove Vault Sense from selection and its obsolete HUD overlays. Remove large gameplay text panels; retain sparse accents, bars and shadows.
- Add replicated ammunition/reload timers to all nine weapons, R/left-stick reload, empty-magazine reload, procedural motion and two original audio cues. Retune base damage around reload downtime; preserve reload state on reconnect.
- Append Deep Magazine, Quick Hands and Tight Grouping without renumbering old trait IDs. Utility traits use capped ranks and cannot overflow legacy evolution bits.
- Increase normal base enemy movement 25%, elites/bosses 35%.
- Persist the stakes checkbox until manually disabled; improve rare/legendary weights after clearing higher tiers. Preserve co-op unanimity.
- Add weighted, collision-checked jump links to chest guidance and retain airborne guidance. Preserve jump-only islands separately from the enemy walk graph.
- Bump network protocol to 1900. Add a seventh standalone rule suite and source-layout previews. Unreal 0.19 compilation/playtesting remain pending locally.

# 0.18.0 - world navigation, compact HUD and larger arenas

Replace the screen-space reward arrow with a local 3D route guide near the character, with collision-aware A* and safe segment smoothing. Add saved 25/50/75/100% HUD scale, compact corner panels and smaller menus. Stack four notices with animated upward movement and a 4.5-second hold plus 4.5-second fade; remove duplicate static vault labels.

Cap each map at ten actual rooms. Support 1×1, 2×2, 2×3, 3×2 and 3×3 footprints across five topology families, with a guaranteed solid route, weighted-distance reward placement and biome-specific large-room landmarks. Add original Cinder Scattergun, Tempest Rotary and Rift Mortar meshes/behaviors, practice displays and upgrade eligibility. Retain existing weapon IDs, profile schema 1 and prior compile fixes. Network protocol is 1800; source-art revision is 7.

Six standalone shared-rule/geometry suites and source/art checks pass. Unreal compilation, rendering, collision, Windows launch, saves and live co-op remain unverified. Research, source previews and local acceptance cases are in DESIGN_0.18.md.

# 0.17.1 - HUD compile hotfix

- Fixed C4456 in ASHUD.cpp: renamed the interaction-marker radius so it does not hide the crosshair radius.
- Updated release labels; protocol 1700 and save paths remain unchanged.
- Source diff and archive checks completed. Full Unreal build and live testing remain pending.

# 0.17.0 - personal HUD, reward guidance and owning-viewport input

Add four crosshair shapes, eight colors and a 50–200% size slider; save local HUD preferences and offer exactly 80/100/120/140/160% damage-number sizes. Move outgoing/incoming numbers into rising rows above the reticle, with critical-hit clearance and a separate incoming-damage offset. Add a camera-relative reward/portal direction arrow.

Use red enemy/spawn minimap markers, purple sensed traps, green vaults, gold rewards and fixed cyan P1 / white P2 identities. Save and replicate eight character-color choices independently of player identity. Keep weapon use, jump, dash and movement available in Victory; restore downed partners to at least 1 health so they can reach the hub portal.

Replace global viewport access in HUD/menu/input handling with the owning LocalPlayer viewport. Add a custom UGameViewportClient for focus/click recovery and gameplay-only locking, request high-precision relative input, maintain confinement, and release for menus or a shared pause. Add F10 manual relock and ASInputStatus diagnostics. Both players need protocol 1700; existing profile schema 1 and currency paths are retained.

All five native suites, source/art checks and the fern overload regression pass. New shared-math checks cover popup/reticle separation and setting bounds. UE compilation, Windows cursor confinement, Slate rendering, live co-op and save I/O remain unverified. See HUD_0.17_NOTES.md, INPUT_CAPTURE_0.17.md and VALIDATION.md.

# 0.16.2 - personal camera controls

Add five sliders at the top of Settings for pitch, height, shoulder offset, distance and field of view. Apply changes only to the local player, persist them to existing game-user settings, and provide a reset to 0.16.1 framing. Keep mouse-wheel zoom and shoulder swap working with the selected camera. No gameplay, world, art, profile or network-protocol changes.

# 0.16.1 - fern-placement compile hotfix

Fix the two C2668 errors at ASWorld.cpp line 336 in the supplied 0.16.0 build log. Explicit float arguments resolve Unreal's float/double Sin and Cos overloads without changing fern positions. Other trigonometry calls were reviewed for the same issue.

Add a focused check that compiles the actual source expressions against overload signatures matching the diagnostic. The original expressions reproduce the failure; the corrected expressions compile and produce seven finite positions. All five existing native suites and source/art checks pass. Full Unreal compilation/linking and runtime testing remain pending. Protocol 1600 and profile schema 1 remain unchanged.

# 0.16.0 - HUD restoration, physical crowds and cosmetic careers

Move HUD labels and damage numbers into a dedicated Slate rendering layer; raise/widen the shoulder camera, add local owner hiding near walls, and separate captured gameplay input from unconfined menu input. Set explicit instanced-static-mesh material usage and distinct forward-light priorities.

Block player/enemy and enemy/enemy capsules without new contact damage. Improve direct/lookahead pursuit, occupancy-aware spawn/recovery and lateral crowd separation. Add eight original environment meshes, enclosed cave/architectural passages, natural biome props and lighting; keep caves through chambers 17–24 and remove crystals from enemy heads.

Add cosmetic character XP with linear level costs, replicated lobby levels, 36 tiered achievement milestones, verified receipt/reconnect integration and modest leftover-Cinder conversion. Replace the final temporary reward choice with an E-interact return portal usable by either player. Keep schema 1 and existing currency fields; default new career fields to zero. Peer protocol is 1600.

Five native suites and source/art checks pass; UE compilation, rendering, physics, Windows helpers, SaveGame I/O and live co-op remain unverified. See RESTORATION_0.16_NOTES.md and VALIDATION.md.

# 0.15.0 - shoulder aiming, frontier encounters and active abilities

Add mouse-wheel zoom with a 20% farther default shoulder camera, screen-center aiming and a muzzle-cover indicator. Remove automatic bow splitting/curving through wells; deliberate on-hit Split upgrades remain. Capture/lock gameplay mouse input on each local controller and restore capture after refocusing. Draw HUD fonts at output resolution with an optional 120% text size.

Repair vault interaction range, targeting and phase gates; show focused interactable brackets and sigil progress. Use mesh-matching collision on architectural props/arches and move decorative background pillars outside generated map bounds. Add Tab build inventory, the WAGER VOTE label and visible lobby balances. Preserve 0.14 Build Fix 1's literal FString formats and initialized jump-plan vectors.

Add five active abilities beside Rune Lance, shared Focus/cooldown upgrades, free lobby selection and practice, biome-scaled wall jets and moving saws, original cave/crystal art, five visual themes across the existing four progression wards, breakable loot crystals/explosive casks and a personal health-for-upgrade shrine. Preserve schema 1 and version-independent profile paths; add an optional default-zero ability selection field. Peer protocol is 1500.

Four native shared-rule/geometry suites and source/art validation pass. Unreal compilation, importing, collision, rendering, live co-op and save I/O remain unverified. See FRONTIER_0.15_NOTES.md and VALIDATION.md.

# 0.14.0 - buildcraft, wagers, vaults and readable encounters

Add 63 run traits beside the existing six stats, mixed-category synergy-weighted rewards, personal rerolls, bounded trait evolution, duplicate/cleanse/relic/health-trade picks, Focus/Rune Lance and Battle Conduit. Implement capped projectile, elemental, defense, movement, economy and kill-trigger effects without recursive primary-hit proc loops. Add optional unanimous next-room challenge wagers and two-sigil trap vaults. Keep persistent save schema 1 unchanged and queue pending reward receipts.

Add five original enemy meshes/behaviors, route lookahead, supported gap approaches, leaper gap jumps, better stuck recovery, flame/blade traps and Vault Sense. Enlarge projectiles and outlined damage numbers; add health-cross/shield meshes, downed-partner x-ray silhouette/cross, Focus/charge HUD and standalone borderless setting. Preserve 0.13 controls, shop, practice, P2 advancement and inter-wave restrictions. Peer protocol is 1400.

Three native rule/geometry suites and source/art checks pass. Unreal module compilation, collision, rendering, live co-op, save I/O, Windows helpers and performance are NOT verified. See BUILDCRAFT_0.14_NOTES.md, UPGRADE_CATALOG.md and VALIDATION.md.

# 0.13.0 - armory, practice, elemental feedback and varied maps

Rebuild the six weapon meshes with hand-sized grips and a forward-facing drawn bow/arrow. Add one airborne dash per jump while retaining gravity and normal cooldown. Generate five seeded arena footprint families with variable clearings, spacing and room count; route and spawn enemies through capsule-checked nodes/edges connected to the player start. Add sustained-stuck recovery. Allow P2 to advance after both reward claims and verified saves. Permit inter-wave attacks/casts while preventing their damage, proc buffs and later-wave well interactions.

Add six physical weapon displays with free lobby trials and a resetting practice target. Add bounded burn/poison/frost behavior, typed outgoing/incoming damage numbers, distinct crit feedback, a health/shield card and small enemy bars that fade from three to five seconds after damage. Add a version-independent profile folder, verified exports, reviewed old-save restore with backup, and Import_Progress.bat. Retain prior gameplay/lighting/menu fixes. Both peers now use protocol 1300.

Shared-rule/native geometry checks pass. Unreal compilation, Windows helpers, engine rendering/collision, live saving and two-PC tests remain pending. See EXPANSION_0.13_NOTES.md and VALIDATION.md.

# 0.12.0 - jump, revive, stat upgrades and weapon grip

Restore native Space / A jumping; move dash to Shift / B with a movement-defaults reset button. Move the revive timer onto the downed pawn, allow revival during Reward, cancel invalid channels and display progress on both players. Raise the base Spiral cooldown from 2.2 to 3.3 seconds. Expand the Hearth to 20 entries and add six room stat upgrades with current/next previews, stacking limits and reconnect preservation. Attach weapons to the right hand, connect the arm pose, correct sword/hammer geometry and swing forward beside the body. Refresh imported weapon meshes on bootstrap. Update peer protocol and local two-player launcher to 1200.

Retain the previous lighting, startup-settings and menu-wrapping fixes. Native logic/geometry and structural checks pass; Unreal build and live host/client tests remain pending. See `GAMEPLAY_0.12_NOTES.md` and `VALIDATION.md`.

# 0.11.3 - lighting and menu clipping fix

Set light mobility before runtime transforms. Explicitly disable physical-camera attenuation in the manual-exposure volume, initialize sky capture after the atmosphere, and add a small shadowless fill light. Apply graphics quality without changing the window mode/resolution at startup. Widen action columns, wrap labels and other menu text at explicit widths before Slate layout, and preserve automatic row heights. Add startup diagnostics and a project-log collector. See `FIX3_NOTES.md` for installation and the verification boundary. Unreal compilation/rendering have not been run in the patch environment.

# 0.11.2 - UE 5.8 string-header compile fix

Fixed the second live UE 5.8.3 build failure. `ASPlayerController.cpp` no longer includes the internal `Containers/UnrealString.h.inl` file directly and no longer depends on `LexTryParseString` for seed parsing. A local strict int32 parser now validates optional sign, digits, and full int32 range without private/internal Unreal string headers. Added regression checks that reject direct `.inl` inclusion and `LexTryParseString` use in project source.

The previous 0.11.1 fixes remain: replication macro parameter naming, SkyAtmosphere include, `Owner` shadowing, and replication-frequency setter APIs.

# 0.11.1 - UE 5.8 first-build compatibility fix

Fixed the first Windows/UE 5.8.3 compile reported from a live user build. Replication overrides now use Unreal's required `OutLifetimeProps` parameter name for `DOREPLIFETIME`, the 5.8 `LexTryParseString` and `ASkyAtmosphere` include paths are used, and a local `Owner` variable no longer shadows `AActor::Owner` under Unreal's strict warning settings. Updated project-owned replication-frequency assignments to their setter APIs.

Added regression checks for these compatibility issues to `Tests/run_tests.py`. Standalone source/rule tests pass, but this package still has not been compiled or play-tested inside Unreal in the creation environment.

# 0.11.0 - initial Unreal third-person source remake

Rebuilt the primary expedition/combat/progression loop as a native C++ Unreal project targeting the 5.8.3 hotfix. Added a shoulder camera, original 3D modular art, dynamic-lighting configuration, articulated hero pieces, procedural connected rooms, bounded combat actors, six weapon families, four ward themes and a new hub/HUD/menu interface.

Added two-player listen-server source, separate profile/reward handling, readiness, shared pause, reconnect reservation and same-session return to hub. Added alternating save slots, receipt deduplication, preserved upgrade scroll, one-time refinement and repeat skill-point crafting. Added input rebinding, graphics presets and frame-time diagnostics.

Included source-art bootstrap, Windows editor/standalone/two-player/package helpers, repeatable native rule tests, structural checks and explicit pending engine acceptance tests. No Unreal binary build or engine play-test was performed.
