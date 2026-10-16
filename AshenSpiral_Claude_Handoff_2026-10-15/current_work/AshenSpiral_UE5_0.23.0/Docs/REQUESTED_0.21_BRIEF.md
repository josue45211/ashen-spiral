# Accepted 0.21 scope

Source: Josue's October 14 live-session prompt brief with P2, supplied in this conversation.

## Track 1 — Josue / melee

1. Replace the boolean air-dash gate with a counter; reset on landing and preserve the count in saved movement prediction.
2. Remove reloads from every melee weapon. Full animation cycles determine firing rate; attack speed accelerates the entire animation.
3. Cinder Blade: automatic fast three-hit combo, third-hit lunge and smooth mobility.
4. Grav Hammer: automatic left swing, right swing, charged jump-slam with much higher third-hit damage.
5. Bounded faster enemy approach at distance, normal speed in combat range; not a flat speed increase.
6. Fix melee holds first: two-handed heavy hammer grip/wind-up and correct sword grip, then build the combos on that foundation.

## Track 2 — P2 art direction / Forgotten Archives

Priority: inward-facing shelves; varied layouts without copy-paste repetition; aged textures; dust, broken furniture, debris and corrosion. Written style target: gothic cathedral, dark stone, verticality, dramatic uplighting, an archive unopened for centuries. Keep this pass bounded to ward one.

Referenced inputs: 0.21-fix-candidates.md item 1; game/AshenSpiral/reference/ (eight images + README); forthcoming Blade look references. These inputs were not found, so they were not reviewed. The supplied written scope and verified baseline source control this implementation.

## Explicitly deferred

Difficulty portals / fresh character slot; boss synergy and portal/hammer spawn-kill fixes; fight-specific boss arenas; endless/mastery/passive unlocks/looping; other biome visual passes; full 0.22 locomotion/cape/enemy/hit-reaction/remaining-weapon animation overhaul.

The Bow third-shot double appears in the prompt as design context, but not in the supplied 0.20 source. It is not added by this melee-only update.

See DESIGN_0.21.md for the concrete implementation, initial tuning, baseline interpretation and pending local tests.
