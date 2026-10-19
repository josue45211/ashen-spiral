# Ashen Spiral 0.24 - Telegraph language

How every threat on screen is drawn, so a player can read a fight at a glance. This is the rulebook new enemy abilities follow; the code that draws them is cited where it matters.
Standing doctrine (Crab Champions research, Part C of the 0.24 order): every attack has a **body-language wind-up**, a **sound cue** and a **warning that is exactly the area it
damages**. A circle, strip or ring on the ground always means "damage lands here when it fills", and nothing else is drawn like one.

## 1. Colour = meaning

| Colour (linear RGB) | Means | Used by |
|---|---|---|
| Red-orange `(1.0, 0.16, 0.03)` | Damage lands in this exact shape when it fills | melee slash sector, ground-pound disc, charge strip, targeted ground blasts and meteors, boss attacks |
| Amber `(1.0, 0.62, 0.12)` | An aimed shot: a beam or tracer showing where it will go | the sniper's tracking beam (turns white-amber when it locks), archer tracers |
| Green `(0.20, 1.0, 0.35)` | Healing and brood: a tether to an ally being healed, egg sacs | broodcaller tethers and eggs |
| Violet-white `(0.86, 0.74, 1.0)` | Lightning and storm | sky lightning strikes, the shocking elite's bolts, the lightning synergy |
| Cyan `(0.12, 1.0, 1.0)` | Frost, and **only** frost (status tint, frozen elite ground, the Hoarfrost Maul) | frost status on any enemy, frostbound elite |
| Gold `(1.0, 0.85, 0.30)` | A window for you: your reach cue, a parry stance flash, a guard that just broke | crosshair reach cue, duelist parry glow, bulwark guard-break sparks |
| Dim grey-white shards | Cosmetic only, never a threat | smoke puffs, egg shells, death bursts |

Blue was freed for frost in 0.24: lightning moved to violet-white, the sniper's aim moved to amber, and the status ring was replaced by a body-tint pulse.

## 2. Shape = what it covers

| Shape | Mesh | Means |
|---|---|---|
| Half disc (sector) in front of the enemy | `SM_StrikeSector` | a swing; you are hit only inside exactly this shape (`ASEnemyRules::StrikeHits`) |
| Full disc centred on the enemy | flat disc, or `SM_Ring` outline plus a growing fill | a shockwave or slam around it; jump over the ring when it travels, or be outside the radius |
| Strip from the enemy toward the target | flat cube | a lunge or charge path; step out of the strip |
| Ring on the ground at a target spot | `SM_Ring` (`AASField`) | a delayed blast at that place; leave before it fires |
| Beam | thin cube | an aimed shot; leave the line before it locks |
| Tether | thin cube | a link between two enemies (healing); never damage |

The warning appears the moment the wind-up starts. A **dim full-size copy** shows the final area; a **bright copy grows over it** as the wind-up completes, and damage lands when it
is full. Opaque materials are used on purpose (no transparency in this project), so a warning is hidden when its attack ends, never faded.

## 3. Body language

Every ability owns an anticipation pose, a fast strike and a recovery, authored as data (`ASEnemyPose.h`) and viewable offline (`Tools/preview_enemy_poses.py`, contact sheets
per ability). The poses follow one grammar:

* **Anticipation is slow and large** (a raised weapon, arms thrown wide, a crouch with the feet planted); the visor and chest core **brighten** as it completes.
* **The strike is fast** (about a sixth of a second).
* **The recovery is a short exposed pause** (0.36 to 0.9 s depending on the attack): the enemy stands still. That is your opening, and it is the same for every enemy.
* Heavier enemies wind up longer and recover longer. A heavy blow is never fast.

## 4. Sound cues

Each ability plays a wind-up cue where the telegraph starts and a release cue where it lands, on every machine, from the replicated state (no extra network traffic). The cue
table lives in `ASEnemyRules.h` and the files wanted are listed in `Docs/AUDIO_MANIFEST_0.24.md`. Until new files exist, each cue plays an existing sound at its own pitch and volume.

## 5. Elites and mutations

An elite is a behaviour remix, not a bigger health bar. Its mutation is shown by a **steady body tint** (never a ring on the floor) and by its colour language:
frostbound is cyan, burning is orange-red (a damage hazard), shocking is violet-white, venomous is green.

## 6. Checklist for a new ability

1. Wind-up of at least 0.5 s (a counter-strike may be shorter if its whole stance is the telegraph), with its own pose track and a cue.
2. A ground warning if and only if it damages an area, and the warning is the damage shape (one rule decides both; test them together).
3. A recovery of at least 0.2 s.
4. A colour from the table above, and never blue for anything but frost.
5. Listed in the audio manifest with a fallback sound.

## 7. The 0.24 abilities and how to read them

Numbers live in `ASEnemyRules.h` (the ability table) and `ASEnemyAbilities.h` (their geometry), and are exercised by `Tests/enemy_ability_024_tests.cpp`. All of it is UNVERIFIED in
the engine: nothing was played. Poses: `Docs/ENEMY_POSES_0.24_PREVIEW.png`.

| Ability | Who | What you see and hear | How you beat it |
|---|---|---|---|
| Shove | Grunt | 0.6 s: both hands drawn back, a red **half disc 2.6 m** on the floor filling up | step out of the half disc; if it lands it hurts a little and throws you about 5 m |
| Back-hop | Archer | 0.5 s crouch, then a 5 m hop straight away from you (dust at both ends), staff still pointing at you | it lands exposed for 0.4 s; close in or shoot it; it only hops when you are within 5 m |
| Parry | Duelist | 0.95 s guard: blade up across the body, visor and chest core glowing | **do not hit the front** while it is up (blows take 15% and spark gold); hit it from the side or behind, or wait it out: it is exposed for 0.4 s afterwards |
| Riposte | Duelist | straight out of a block: a red half disc 2.6 m for 0.28 s | step out, or dash through it |
| Ground pound | Brute | 1.0 s: maul overhead, a **red disc 7.2 m across** filling up; then a ring sweeps out at 9 m/s | jump the ring (feet 45 cm off the floor) or dash through it; standing still on the ground is what hurts |
| Enrage | Brute | at half health, a 1.2 s roar with its core flaring; afterwards a fast hot-red pulse on the body | it is 25% faster, winds up 20% quicker and recharges 40% quicker: stagger it before it does more |
| Rift blink | Leaper | 0.55 s squash with violet light, then it vanishes along a violet trail and reappears about 2.6 m **behind you** | the strike (red half disc 2.6 m, 0.4 s) starts the moment it appears: turn and dash |
| Meteor rain | Shaman | 1.0 s: staff raised to the sky; red discs (2 m) appear on **each hero** and around them, with fireballs falling onto them | move out of every disc before its fireball lands (0.85 s for the first, 0.3 s later for each next) |
| Guard break | Bulwark | its shield guard halves blows from its front; a **heavy blow (12% of its health or more) from the front** rips the guard open: it staggers for 0.8 s, drops its shield and takes full damage for 3.5 s, with gold sparks | hit it hard from the front (the Hoarfrost Maul does), or go around behind it |
| Tracking laser | Sniper | 1.5 s: an amber beam follows you for the first 70%, then **turns white-amber and thick: locked**; the shot goes along the locked line | break the line (move sideways) after the lock; afterwards it runs to a new vantage point 13-16 m away for up to 3 s, at 1.5 times its speed, and cannot shoot until it gets there |
| Egg sacs | Broodcaller | its summon lays 2 green eggs (3 in co-op) in a fan in front of it; they pulse faster as they near hatching (3.2 s) | pop an egg with any hit (22 health) before it hatches into a minion |
| Pack dive | Gloomwing | the first of a pack to dive screeches (its wind-up is the call); the idle Gloomwings near the same target then dive one after another, 0.38 s apart (at most four) | read the chain: each dive has its own 0.7 s wing-back wind-up; the gaps between volleys are yours |

Chained abilities (riposte after a block, the blink's strike) are never chosen by the chooser; the enemy asks for them itself, and their poses start where the one before them ended.

