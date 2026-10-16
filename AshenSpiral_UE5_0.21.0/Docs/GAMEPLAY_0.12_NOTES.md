# 0.12.0 gameplay update

## Movement and revive

- Space / controller A uses Unreal's native Character jump, including movement prediction. Shift / controller B dashes. Both Shift keys work. Jump is blocked while downed, reviving, paused, or on the run-end screen. Dash still starts from the ground.
- Controls includes RESET JUMP/DASH for older saved bindings. It restores those actions' keyboard and controller defaults while preserving other bindings.
- The downed pawn now owns the server-side revive timer. The previous implementation excluded the reward phase, leaving a downed partner unable to finish the room.
- Hold V / left shoulder continuously for 2 seconds within 3 meters, with a clear path and a grounded helper. Release, range, and line-of-sight failures reset progress. The HUD displays progress for the helper and the downed player. Successful revival restores 45% health and grants 2 seconds of protection.
- Reviving is allowed during Combat, Countdown, and Reward. Both players down during combat still ends the run.

## Cooldown and upgrades

The base Spiral cooldown is **3.3 seconds**, up from 2.2: a 50% longer wait before upgrades. Cooldown starts when the ability is released. Dash retains its separate cooldown upgrades.

| Stat | Permanent Hearth rank | Room-reward rank | Limit |
|---|---|---|---|
| Ability cooldown | −10% of base cooldown | −8% of base cooldown | 5 ranks each; 65% combined reduction |
| Attack speed | +8% | +12% | 5 ranks each |
| Damage | +10% | +15% | 5 ranks each |
| Maximum health | +8 HP | +15 HP and heal 15 | 3 permanent / 5 run ranks |
| Movement speed | +5% | +5% | 5 ranks each |
| Critical chance | +4 percentage points | +5 percentage points | 5 ranks each; 50% total chance |

Percent stat bonuses add within each stat. Attack interval divides by the speed multiplier; a speed bonus does not subtract that percentage from the attack interval. The hammer retains its deliberate 0.65-second charged-release threshold. Critical chance starts at 5%; critical weapon attacks deal 1.8× damage. Damage upgrades affect attacks and abilities. Movement upgrades change running speed, including its water slowdown, without extending the fixed dash distance.

Core Hearth cards and room reward cards show current → next values. The overview and expedition pages show the active build. Permanent upgrades apply on every run. Run upgrades reset in the hub/new expedition and are included in reconnect snapshots.

Chests offer four choices (five with Another Possibility). Ability cooldown and attack speed remain available while another rank has an effect; another eligible stat rotates in, followed by relic/style alternatives. Choose **one** reward. Fully capped stats stop appearing. Shop chests retain the existing 25-Cinder price on choices after the first; resource and healing chest bonuses still apply.

The Hearth now has 20 entries. Existing Unreal saves retain the original 16 rank positions, including purchased cooldown ranks; four new stats append at zero. The save schema is unchanged because the existing rank array already supports variable length. Actual SaveGame loading still needs an Unreal test. Both peers must use **0.12.0 / protocol 1200** because replicated gameplay state changed.

## Weapon grip and swing

Weapons now attach to the right hand instead of inheriting the forearm's steep pitch. The connected arm follows the hand. The idle weapon points up, and melee swings travel forward beside the character. Sword blade and hammer-head source geometry now extends above the grip instead of through it. Every weapon's grip is centered at its hand pivot. The importer refreshes the corrected weapon meshes when bootstrap runs after an update.

The shared pose and source OBJ vertices pass a geometry check against the character torso/head bounds across sampled idle, walking, and swing positions. This checks source math and geometry, not the Unreal importer, camera view, rendering, or animation quality.

## Install and focused local checks

Extract into a fresh folder, copy only your old `Saved/SaveGames` if retaining progress, and run `Play_Standalone.bat`. Both PCs must update. Confirm the 0.12.0 menu footer. The earlier black-view/light-mobility and wrapped-menu-text patches remain included.

After the local Unreal build succeeds:

1. Test Space jump and Shift dash on the host and client. Confirm one jump per landing, release behavior, and no jump while downed. Test RESET JUMP/DASH if using prior saved bindings.
2. Down the client and revive with the host, then reverse roles. Repeat during combat and after the last enemy dies. Check both progress bars, 45% restored health, and ability to claim both rewards and advance.
3. Release revive halfway, walk beyond 3 meters, move behind a wall, and try while airborne. Each invalid channel should reset. Confirm attacks/dash/menu cancel it, shared pause freezes gameplay, and a combat team wipe ends the run.
4. Check 3.3 seconds without cooldown ranks, then check one permanent rank (2.97 seconds) and one run rank (3.04 seconds without permanent ranks). Confirm the displayed value matches subsequent casts. Check attack-speed, damage, health, movement, and critical cards; select different rewards on each peer.
5. Restart to check permanent purchases and old-save rank preservation. Reconnect during Combat and Reward to check current run upgrades. Confirm a new expedition clears run-only upgrades.
6. View every weapon idle, moving, and attacking from both sides. Confirm upright grips, forward swings, and a visible world. Check long menu labels after resizing the window.
7. In editor preferences, remap Play World Stop from Esc to Shift+Esc (check both shortcut slots), then verify Esc opens/closes the game menu. See `START_HERE.md`.

## Verification boundary

Standalone C++ logic/geometry checks and structural checks pass. UnrealHeaderTool, the Unreal module build, Windows helpers, source-art import, local/two-PC play, and frame rate have **not** been tested here. The checks above are pending acceptance checks, not completed test results. Full details and recorded outputs are in `VALIDATION.md` and `Tests/`.
