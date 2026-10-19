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
