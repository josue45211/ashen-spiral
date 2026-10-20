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
frostbound is cyan, burning is orange-red (a damage hazard), shocking is violet-white, venomous is green. Every elite brute spawns with one of the four (a replicated byte); it
adds one signature ability to its kit (section 7), plays a one-off cue when it first comes into view, and the tint is a slow pulse that status effects and rage override.

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
| Glacial slam | Frostbound elite brute (cyan pulse) | 1.0 s: a red disc 3.8 m wide filling up, maul overhead | leave the disc; a cyan frost zone (3 m, 5 s) stays where it landed and hurts anyone standing in it |
| Cinder burst | Burning elite brute (orange-red pulse) | 0.9 s, then three fire pools appear in a line toward you, each with its own red warning disc (0.8 s, 1.05 s, 1.3 s) | step out of the line; each pool then burns for 3.5 s. The burning brute also drops a fire pool every 1.6 s while it walks |
| Storm call | Shocking elite brute (violet-white pulse) | 1.0 s with the weapon raised; violet-white discs with a sky column on each hero and around them (0.85 s, then 0.3 s apart) | leave every disc: a bolt hits (18) and leaves a static field for 3 s |
| Seed bombs | Venomous elite brute (green pulse) | 0.8 s heave; green pods fall onto each hero and around them (1.0 s, then 0.25 s apart) | leave the landing spots: each bursts into a venom pool (6 s) |

Chained abilities (riposte after a block, the blink's strike) are never chosen by the chooser; the enemy asks for them itself, and their poses start where the one before them ended.

## 8. The ward bosses (full redesign)

Each ward's boss owns three attacks and a second phase; numbers are `ASEnemyAbilities.h` (BossKitFor and the Boss* constants), the ability rows are `ASEnemyRules.h` (ids 24-33).
Every boss attack is announced for at least a second and leaves it exposed for at least 0.6 s. At half health the boss stops and **roars for 1.5 s** (it does nothing else), its ward's
set piece happens, and from then on its body pulses hot red and its attacks widen, double up or speed up. The flying boss and the Endless wave 50+ bosses keep the old boss attack.

| Boss | Attacks | Second phase (after the roar) |
|---|---|---|
| **The Indexer** (ward 0 in the code) | **Ledger slam**: a red disc 9 m wide, then a shockwave ring (jump or dash it). **Marks**: a blast mark on every hero (3 m, 0.95 s). **Page storm**: a ring of 12 shots, turned 15 degrees further each time | the roar calls **two clerks**; the slam sends a second ring 0.55 s behind the first; marks are 3.9 m and add three scattered marks; the page storm is 18 shots |
| **The Bellwether** (ward 1) | **Stampede**: a red strip 9.5 m long and 2.2 m wide, then a charge along it (28 damage on contact). **Toll**: a red disc 8 m wide, then two rings 0.55 s apart (jump the first, the second catches the landing). **Marks** | the roar calls **three of the herd**; the charge is 20% faster |
| **The Horticulturist** (ward 2) | **Seed bombs**: four venom pods fall on and around you. **Blooms**: three void blooms open under and around you and stay (6 s). **Thorn ring**: 12 shots | the roar turns **the wells against you** and opens **a ring of six void blooms** around the boss; six pods, five blooms, an 18-shot ring |
| **The Rootbound Sovereign** (ward 3) | **Roots**: roots erupt under every hero after one second (24 damage, then a void zone). **Crown slam**: the shockwave ring. **Grasp** (second phase only): a red disc 9 m wide, every hero in it is flung toward the boss, then a ring follows 0.5 s later: land inside it and jump | the roar raises **four roots near the heroes**; roots add two more, the slam sends a second ring, and the grasp joins |

Void zones are deep violet, venom is green, fire is orange-red, and nothing about a boss is ever blue.


## 9. 0.24.2 additions (playtest of 0.24.1)

* **Ring attacks have their own warning** (`ASEnemyRules::TellRing`: the brute's ground pound, the Indexer's and Sovereign's slam, the Bellwether's toll, the Sovereign's grasp).
  A filled disc means "leave this area"; a ring attack is drawn as a **thick outline where the wave ends, with two bright hot-yellow ripples racing out to it** and quickening as
  the wind-up completes. The wave itself is a **short glowing wall (about 40 cm)**, so it reads as "jump this": jump it or dash through it.
* **Weapon glint**: every telegraphed blow (anything with a ground warning) flashes a white-gold four-pointed star at the weapon's tip **0.28 s before it lands**, with a faint
  high "shing". The leaper (no weapon) glints at its brow. Every machine draws it from the replicated wind-up.
* **Strips show**: the boss stampede's red strip (`TellLine`) was drawn by the code but never made visible; it is now.
* **Lightning** is a jagged, forked bolt that flickers through three shapes in a third of a second, with a flash of violet-white light, white-violet sparks and a scorch mark that
  cools and shrinks over two seconds. The floor warning is unchanged (violet disc, spinning spokes, pale ring filling), plus a thin flickering column reaching down while it charges.
* **Ring flashes lie flat**: every ring-shaped flash (slams, chains, wildfire, lightning, spawns) spreads on the floor; the ones aimed "up" used to stand as vertical arches.
