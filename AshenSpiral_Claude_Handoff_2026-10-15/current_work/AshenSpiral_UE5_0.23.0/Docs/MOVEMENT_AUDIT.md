# Movement audit (2026-10-19)

Audit only: no code was changed. Everything below is read from the source; nothing was run in the engine, so how any of it feels is not judged here.

## 1. What the movement is

The player is `AASCharacter : ACharacter` with a **custom subclass of the engine's movement component**, `UASMovementComponent : UCharacterMovementComponent` (`Source/AshenSpiral/Public/ASMovementComponent.h`, `Private/ASMovementComponent.cpp`). So it is the standard engine movement with these additions, not a custom movement system:

- **Dash** is a custom movement mode (`MOVE_Custom`, custom mode 1) driven in `PhysCustom`: 2350 cm/s along the input direction for `DashDuration()` seconds, swept for collision, with air dashes, dash charges and a recharge timer. It is replicated through custom saved moves (`FSavedMove_AS`, `FPredictionData_AS`, `FLAG_Custom_0`), so client prediction replays it.
- **`GetMaxSpeed()` is overridden** and ignores `MaxWalkSpeed` for the hero: it returns `RunningSpeed()` (see 2), 0 when downed, frozen or in a hammer leap, 20% during the hammer finisher, 45% in water.
- **Hammer leap** (`ASHammerCombat.cpp`) is a root-motion jump force (`FRootMotionSource_JumpForce`, priority 600) with a planned arc, then it hands over to normal falling with the arc's own velocity.
- **Melee lunge** (`ASMeleeCombat.cpp`) is a `MoveToForce` root-motion source, clamped to `RunningSpeed()` when it finishes.
- **Facing:** `bUseControllerRotationYaw=true` and `bOrientRotationToMovement=false`. The character always faces the camera/aim yaw (twin-stick style), and the animation reads movement relative to that facing.
- Enemies use their own `ACharacter` movement with values from the AI kits; they are not part of this audit.

## 2. Current values against the template list

Set in `ASCharacter.cpp` lines 28-34 unless noted.

| Setting | Current | Template | Differs |
|---|---|---|---|
| MaxWalkSpeed | 640 (set, but **unused for the hero**; the real speed is `RunningSpeed()` = `ASRules::MoveSpeed` = 640 x (1 + .05 x permanent rank + .05 x run rank)) | 600 | yes, and changing the property alone does nothing |
| MaxAcceleration | 2600 | 2048 | yes |
| BrakingDecelerationWalking | 2100 | 2000 | yes (small) |
| JumpZVelocity | 660 | 700 | yes (small) |
| AirControl | 0.45 | 0.35 | yes |
| GroundFriction | 8.0 (engine default, never set) | 8.0 | no |
| GravityScale | **1.6** | 1.0 | **yes, large** |
| bOrientRotationToMovement | **false** (face the aim) | true | **yes, a different control scheme** |
| RotationRate | engine default (0, 360, 0), unused because orient-to-movement is off | (0, 500, 0) | yes, but only matters with the row above |
| MaxStepHeight | 42 (not on the list) | engine default 45 | n/a |
| JumpMaxCount / JumpMaxHoldTime | 1 / 0 | not on the list | n/a |

## 3. Would the template values break anything?

Safe to change on their own (nothing else depends on the exact number):

- **MaxAcceleration 2600 -> 2048 and BrakingDecelerationWalking 2100 -> 2000.** Reaching full speed takes about 0.31 s instead of 0.25 s and stopping is slightly longer. The dash sets its own velocity, the leap is root motion, and enemies set their own values. It is purely feel, and it would make movement a little floatier, not more fluid.
- **AirControl 0.45 -> 0.35.** Less steering in the air. The air dash is unaffected (it is its own mode). Feel only.
- **JumpZVelocity 660 -> 700 on its own.** Small; see gravity below for the real effect.

Have real conflicts:

1. **GravityScale 1.6 -> 1.0 (the big one).** Gravity goes from 1568 to 980 cm/s2.
   - A jump apex goes from 139 cm (660 at 1568) to about 250 cm with 700 at 980, and airtime from about 0.84 s to 1.43 s. That is 80% higher and 70% longer, a very different jump.
   - **Hammer leap:** `ASHammer::Height = 208.35` is documented as "1.5 x the 660 cm/s jump apex at gravity 1568"; `PlanLeap` defaults its gravity to 1568 and takes the live gravity from the component, and the arc's finish velocity assumes the scripted arc. The scripted flight (1.04 s) would stay the same, but the plan's height and drop limits (`LeapMaxRise 260`, `LeapMaxDrop 900`) were tuned against the old fall rate, and the fall after the arc would be much slower. `hammer_022_tests.cpp` pins these numbers and would need updating.
   - **Dash:** the air dash adds `GetGravityZ()` to its vertical motion, so it follows gravity automatically, but air dashes would carry further.
   - **Geometry:** whether the player can jump onto or over things depends on jump height. The Endless blocks are all 380 cm now, but other rooms have props, steps and ledges, and enemy pathing/leap logic were checked against the old jump height. I did not audit every room for jump reach.
   - **Enemies:** `LaunchCharacter` knockbacks and pounces on enemies use the enemy's own gravity, which is separate, so enemy movement does not change. But the 0.24 impact-knockback tuning (launch strengths, a 0.7 s impact window) was set against enemy gravity, which this change does not touch.
2. **MaxWalkSpeed 640 -> 600.** Because `GetMaxSpeed()` ignores the property, the real change is `ASRules::MoveSpeed`'s 640 constant. That constant is pinned by `gameplay_tests.cpp`, and the hammer leap speed (`LeapSpeed = max(distance/flight, RunSpeed x 1.35)`), the melee lunge clamp, the locomotion animation weight (speed / `RunningSpeed()`), and the enemy speed tuning (enemy base speeds and `EnemySpeedScale` 1.25 / 1.35 were balanced against a 640 hero) all follow it. 600 is 6% slower, so enemies effectively become 6% faster relative to the hero, which is a balance change, not just a feel change.
3. **bOrientRotationToMovement true with RotationRate 500.** This changes the control scheme: the character would turn toward where it runs instead of always facing the aim. That breaks: aimed melee and ranged attacks (they use the actor's facing), the hammer leap direction and finisher, the strafe-and-backpedal locomotion animation (it reads velocity relative to facing), the revive and interaction facing, and the camera/aim relationship. It would also change what P2 and Josue have been playtesting for weeks. If the goal is the Third Person template's feel, this is the setting that matters most for how it looks and the one that costs the most.

Networking: all of these are constructor defaults applied identically on the server and the clients, so none of them desyncs the prediction. The dash's custom saved-move data is untouched by any of them.

## 4. Recommendation

- If "more fluid" means snappier, our numbers are already higher than the template's acceleration and braking; the template values would be a step toward floatier, not fluider.
- If "fluid" means the template's **animation flow and turning**, that comes from orient-to-movement plus a locomotion animation set, not from the numbers. It is tied to the player-model swap now in scope for 0.25 (the Game Animation Sample character has its own motion-matching locomotion), so it is best decided there, not by changing values now.
- If numbers are wanted anyway, the low-risk set is acceleration 2048, braking 2000, air control 0.35, JumpZ 700 **with gravity left at 1.6** (apex becomes 156 cm, 12% higher, and the hammer leap constants stay valid). Gravity 1.0, speed 600 and orient-to-movement each need their own follow-up work and playtest.

## 5. Custom movement migration

Not applicable: the movement is already `CharacterMovementComponent` based.
