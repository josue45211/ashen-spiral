# Ashen Spiral — 0.22.0

**Grav Hammer rework. Unreal Engine 5.8.3 source prototype.**

Extract the complete ZIP into a new folder and run **Play_Standalone.bat**. Both co-op players need **0.24.0 / protocol 2400**. Profile schema 1, migration 20 and the shared per-user progress folder remain supported.

## Hammer changes

- Hold LMB for left swing, right swing, then a **planted frontal smash**. No jump or radial primary hit on the combo. Damage and full-cycle timing remain 62 / 62 / 161.2 and 0.72 / 0.72 / 1.188 seconds.
- The head follows the swing tangent; two hands stay on the shaft. Shoulder/hip coil, a braced finisher, crouch-launch, cape response and combo glow support the new poses.
- Hit three commits through recovery. Earlier swings can be canceled by dash or leap, resetting to hit one. Whiffs keep the combo; the 1.2-second grace window scales with attack speed.
- **RMB / controller X:** mobility leap, 7.5-second cooldown, no ash cost, 100 base radial damage on actual landing. Startup is vulnerable; flight and landing recovery are protected.
- **G / controller LT:** former Spiral hold/release ability, with its existing behavior. These control changes apply to the hammer only.
- Contact pose holds, light/heavy knockback, debris and four original sound cues. **Settings → Motion sickness reduction** disables hammer shake and saves per player.

Blade, ranged weapons, boss attacks, prices and Archives art retain 0.21 behavior. Nine weapons, six active skills, 65 selectable run traits, six core upgrades, progression, co-op and prior bug fixes remain included.

## Review and testing

`reference/` contains the supplied prompt and all three reference sheets. `Docs/DESIGN_0.22.md` maps requirements to implementation and lists the playtest. The complete editable catalog is `Docs/ASHEN_SPIRAL_0.22_EDITABLE_CATALOG.txt`.

`Docs/HAMMER_0.22_COMBO_PREVIEW.png`, `HAMMER_0.22_LEAP_PREVIEW.png` and `HAMMER_0.22_COMBO.gif` show actual source meshes and runtime pose curves; they are **not Unreal captures**. Animation remains procedural modular armor, not skeletal/mocap animation.

**78 original meshes, 12 material maps, 12 sound cues, ten native C++ suites**, source generators and Windows setup helpers are included. Native rules/geometry checks pass. **Unreal module compilation, asset import, collision, audio, saves and two-player playtesting have not run here.** Binaries and imported assets are generated on your PC. See `Docs/VALIDATION.md`.
