# Ashen Spiral 0.26.0 - what is new and what to look at

**Both co-op players need 0.26.0 / protocol 2600.** Extract the ZIP into a new folder, run `Setup_and_Open.bat` (or `Play_Standalone.bat`) as usual; `Test_Two_Players.bat` is the two-window check on one PC.
Profiles from 0.24.2 load unchanged (nothing saved changes meaning; the skill tree adds 24 ranks after the old 20, which load as zero). Do not run a 0.24.2 and a 0.26.0 build on the same profile at once.

Everything below was checked by compile (unity and non-unity), the native test suites and, for the phases up to the team skills, scripted in-engine runs. **What nobody has judged yet is the feel: the sounds, the weight of the hits, the look.** Each section ends with what to judge.

## Phase 0 - the gaps from the last playtest
- **Co-op elevation (G1):** your partner no longer appears 88 cm too high and turned 90 degrees; the cause was the engine rebuilding each proxy's mesh from a stale offset.
- **Stances (G2):** the blade's and the maul's stances are upright (they were bent over). **Hammer (G3, G4, G8):** the third swing no longer drags you at 55% speed (now 6%, feet and travel match); your left hand stays on the shaft through every swing; the hammer is drawn 1.35x; launches fly about 22% farther.
- **Boss roar (G5):** rebuilt from scratch (deep, fast onset). `as.roar.old 1` plays the old ones for a side-by-side.
- **Footsteps (G6):** one per real foot strike, 3 dB quieter. **Downed marker (G7):** ice-white under the blood moon, red elsewhere.
- *Judge:* the roar (lion or synthesiser?), the blade's guard stance, the maul's size and elbows, the finisher at 6%, footstep level.

## Phase 1 - Twin Claws (weapon 10 of 10; its stand is at the end of the row of weapon stands in the hub)
- **LMB:** two claws swing in turn, one blow every 0.19 s; each blow shreds ONE target. Blows in a row raise the sparks and the pitch; a pause drops them.
- **RMB: POUNCE** (6 s). You leap onto the enemy under your crosshair and cling to it, facing it, for up to 3.5 s. A dash, its death or a wall lets go. Pouncing at nothing is a short lunge (2.4 s). **G** is the Spiral, as on the maul.
- The show: afterimages on the leap, rake streaks, sparks, 11 sounds, a camera push and hit-stop on the arrival, gold target brackets and a LATCHED row on the HUD, a POUNCE word over you.
- *Judge:* how the pounce feels in the hands, the 11 sounds, the claw stance (no bespoke clip yet), the hub display in shade.

## Phase 2 - three team skills (F; equip at the Ability Sanctum, free, test on the lobby dummy)
- **Ashen Brand:** brand an enemy; your partner's hits fill the brand and it bursts for what it stored, splashing the crowd. Alone it fills from your own hits at a third of the rate.
- **Ember Tether:** a line of embers between you and your partner that lashes whatever it crosses; both of you deal +15% while you stay close. Alone it anchors to an ember stake.
- **Twin Collapse:** every cast is a small collapse; if your partner casts theirs within 1.5 s the two merge: a pull to the midpoint, then a detonation and a stun. Both players need it equipped.
- Every synergy now floats its name as a big word.
- *Judge:* the 13 sounds, how the burst and the collapse feel, the numbers (the brand needs a quarter of an enemy's health, 18 per lash, 95 per collapse).

## Phase 3 - the Skill Tree (new) and Settings > SOUND (new)
**Where:** hub, the Ember Nexus overview card "The Skill Tree" (also on the Ashen Hearth page). Bought with Ash, permanent, kept on the character, hub only.
Three lanes of eight nodes: **MELEE**, **RANGED**, **TEAM**. A node needs some of its own lane's nodes first (its tile says how many): the two openers need none, the three middle nodes need 2, the two late nodes need 5, the capstone needs the other 7. Costs 15 / 30 / 50 / 120 Ash; a lane is 340 Ash, the whole tree 1020. Buying a node is an event: a banner, a burst on the tile and a chime; a capstone gets the big version.

| MELEE | RANGED | TEAM |
|---|---|---|
| Honed Edge: +5% melee damage | Steady Hand: +5% ranged damage | Shared Ward: 12 shield entering a fight, 12 more with your partner up |
| Long Reach: +8% melee reach | Deep Magazine: +15% magazine (at least +1) | Wellspring: +15 maximum Focus |
| Quick Hands: +6% melee attack speed | Quick Reload: reloads 12% faster | Rhythm: team skills recover 12% faster |
| Iron Skin: 6% less damage taken with a melee weapon | Eagle Eye: +4 points critical chance, ranged (50% cap) | Steadfast Hands: revive a partner 30% faster |
| Blood Rush: +3 health per melee kill | Fleet Aim: +5% running speed with a ranged weapon | Brand Hunter: the brand needs 15% less, bursts 15% harder |
| Leap Master: leap and pounce recover 15% faster | Long Shot: +12% projectile speed (and range) | Tether Warden: tether +2 s and +20% instead of +15% |
| Executioner: +25% melee damage below 30% health | Marksman: +15% ranged damage to elites and bosses | Convergence: the Twin Collapse call +0.6 s, the detonation +20% |
| **PREDATOR:** a melee kill gives back your leap or pounce (a dash charge for blade and scythe), at most once per 1.5 s | **PIERCING ROUNDS:** a perfect dash makes your rounds pass through two enemies for 5 s | **SYNERGY ECHO:** every synergy burst repeats 0.8 s later at half its damage |

**Settings > SOUND:** Master volume plus three buses (Combat & effects, Enemies & world, Interface), 0 to 200% in 5% steps, each machine's own, saved; letting go of a slider plays a short sample. "Reset sound" puts everything back to 100%.

*Judge:* whether buying a node feels like an event; whether each capstone changes how you play; the tree page's layout (it scrolls: three lanes, tiles two or three to a row); the six tree sounds; whether any node is too strong or too weak; the loudness steps.
*Not yet seen in a real fight (compiled, unit-tested, reviewed, not run):* Predator's refund, Blood Rush, Executioner, Marksman, Piercing Rounds (the blue rounds and the HUD row), Shared Ward, the Synergy Echo repeat, Tether Warden, Convergence, Brand Hunter, Rhythm, Steadfast Hands - and every sound now goes through the volume module, which has not been run in the game either. Please report anything silent, doubled or wrong.

## Console helpers (backtick/tilde; non-shipping builds)
`as.dev.tree ash 400` (profile Ash), `as.dev.tree buy 7` (the real purchase path), `as.dev.tree set all|none|lane0-2|nodes=0,5,9` (own nodes straight onto your hero), `as.dev.tree status` (every number the nodes change), `as.dev.tree perfect` (a perfect dash), `as.tree.log 1` (the nodes at work), `as.team.log 1`, `as.pounce.log 1`, `as.claw.log 1`, `as.audio.log 1` (every sound that is turned down), `as.dev.weapon 9` (claws anywhere), `as.dev.ability 6|7|8` (Brand, Tether, Twin).
