# 0.19 — Reward clarity and weapon rhythm

## Interface

Reward choices are 200×304 design-unit cards in one compact row (up to five). A thin cyan, violet or gold border plus a written rarity label distinguishes common, rare and legendary traits. Original vector illustrations show the effect; titles and short action descriptions stay visible. Hover brightens the card. Holding Shift while hovering fills a reserved detail strip with the full effect, rank/cap and applicable current-to-next values. Moving the pointer or releasing Shift returns the compact hint. Skill cards use a thicker border, tinted face and ACTIVE SKILL label; Spiral-style cards use the same distinct treatment. The Sanctum skill collection wraps within its existing scrollable page.

The reward page has a soft world dim and no large enclosing panel. The Reroll button, stakes checkbox and currency share one bottom row. Exact stakes numbers appear while Shift is held. Ordinary gameplay notices and corner readouts have no opaque panel behind their words: text shadows and small etched accents provide contrast. Health/resource bars and map geometry remain because they convey values and positions. New notices still push older messages upward, hold 4.5 seconds and fade for 4.5 seconds.

Vault Sense's old ID 55 remains reserved, but it cannot be offered, duplicated or evolved. Its old overlay code is removed. The catalog now contains 66 stable IDs, 65 selectable traits and six core stat upgrades. New traits append at IDs 63–65; no existing ID shifts. Original evolution flags are still a 64-bit field, so these three utility traits deliberately cannot evolve.

## Reload and initial balance

Every weapon spends one ammunition/charge unit per primary trigger, including melee swings, charged hammer releases and six-pellet scattergun attacks. Bonus projectiles and impact splits do not spend additional rounds. R or left-stick click starts a partial reload. Attempting to attack an empty magazine starts a reload automatically. Reload blocks primary fire; movement, dash and active skills remain available. There is no reserve-ammunition economy.

The server owns ammunition and reload deadlines; clients render the replicated state. Reload uses the shared simulation clock and freezes with the run. Being downed or starting a revive cancels it. Menu input reset stops held fire without granting a refill. Chamber transitions and hub weapon changes load a fresh magazine; reconnect restores the saved rounds and timer. Hands/weapon lower and cycle back into place during reloading. A progress line and timer accompany the ammo readout. Two original synthesized sounds distinguish starting from finishing; completion is a reliable multicast event with a clear local ready tone.

| Weapon | Capacity | Reload, sec | Base damage / trigger | Trigger interval, sec | Baseline sustained DPS |
|---|---:|---:|---:|---:|---:|
| Cinder Blade | 9 | 1.10 | 40 | 0.32 | 90.5 |
| Grav Hammer | 4 | 1.45 | 62 | 0.65 | 61.2 |
| Ash Bow | 10 | 1.25 | 39 | 0.3 | 91.8 |
| Spiral Glaive | 7 | 1.30 | 33 | 0.48 | 49.6 |
| Ember Staff | 14 | 1.45 | 31 | 0.34 | 69.9 |
| Rift Scythe | 7 | 1.25 | 41 | 0.4 | 70.9 |
| Cinder Scattergun | 6 | 1.65 | 96 | 0.72 | 96.5 |
| Tempest Rotary | 40 | 1.85 | 14 | 0.105 | 92.6 |
| Rift Mortar | 3 | 2.10 | 132 | 1.05 | 75.4 |

Damage is before critical hits, elemental damage, wells, splash, orbiting hits and other unique weapon effects. Scattergun damage assumes all six pellets land at full falloff. Sustained DPS is the conservative continuous-fire baseline `capacity × damage / (capacity × interval + reload time)`, including the final shot's interval before automatic reload begins. Hammer requires deliberate releases; glaive orbit and hammer wells add damage beyond this table. Damage increases compensate for the new downtime, while small magazines retain stronger commitment windows. These are first-pass tuning values, not measured in-engine balance results. Firearms normalize flat Honed Edge bonuses by fire interval for equal added burst DPS; reloads and traits change sustained gains.

| Trait | Each pick | Limit and scope |
|---|---|---|
| Deep Magazine | Add 20% of **base** capacity, rounding that per-pick increment up to whole rounds | Five picks. Every pick helps: mortar 3→4→5→6→7→8; rotary 40→48→56→64→72→80. All weapons. |
| Quick Hands | +15% reload speed | Five picks. Time = base / (1 + 0.15 × rank); rank five takes 57.1% of base time, not a 75% time reduction. All weapons. |
| Tight Grouping | 15% less angular spread | Five picks, 75% maximum reduction. Applies to scatter pellets, rotary cone, Echo Shot, Fan of Sparks and Forked Flight offsets. Projectile weapons only. |

The existing split-shot angle becomes 24°→6° per side at maximum Tight Grouping. A naturally accurate main shot stays accurate; the trait also improves its bonus-shot pattern. Enemy baseline walking speeds are multiplied by 1.25 for normal enemies and 1.35 for elites/bosses, before existing chill/root modifiers. Special attack lunges are unchanged.

## Raise the Stakes

The checkbox saves to the same user-settings file as HUD preferences and is submitted with the profile on connection. It remains selected through chambers, hub returns and relaunches. In co-op both connected players must opt in. Unchecking makes the next chamber normal if unanimity is lost. The first chamber starts normally; later opted-in clears raise the tier to a maximum of three.

Each tier retains +20% enemy HP, +12% enemy damage, +3 Ash and +15 Cinders. It now also increases **rare trait selection weight by 12%** and **legendary trait weight by 20%**, rounded upward in the integer-weight sampler. These are weight bonuses, not percentage-point drop chances. Existing category mixing, build affinity, eligibility and caps still apply. The cleared chamber's tier drives its reward draft and rerolls; changing a checkbox at the chest only affects the next chamber. Relic/style-only chests have no trait-rarity tier to modify.

## Shortest routes across gaps

The guide uses a weighted graph of supported walking nodes plus valid bidirectional links across authored gap edges. Walking costs one per 120 cm node; jump links cost horizontal distance / 120 cm. Dijkstra chooses minimum distance on this graph, and clear walking sections are smoothed without skipping the takeoff/landing markers. A separate guide grid preserves islands that the enemy walk-only connectivity filter would remove. Enemy pathfinding retains its existing behavior.

Jump candidates need supported endpoints, a maximum 800 cm separation, and twenty static-collision capsule sweeps along an approximate arc based on the hero's 660 cm/s jump velocity and gravity scale 1.6. Longer links require the existing air dash. The gold route rises over a jump segment and shows JUMP + AIR DASH nearby. While airborne it retains and follows that jump segment rather than replanning from empty space. The guide does not automatically launch the character. Imported collision, actual takeoff speed, timing, camera visibility and co-op behavior require local playtesting; the graph tests cannot validate the physical jump.

## References and resulting choices

Research consulted October 13, 2026. These are design inputs; all glyphs, audio and implementation in this update are original. Numeric tuning above is specific to Ashen Spiral, not copied values from another game.

- [Crab Champions — Noisestorm's Anvil Update Beta](https://steamcommunity.com/app/774801/discussions/0/596261560284901413/): the developer describes reload UI, low-ammo feedback and less visually busy effects. Applied as a compact reload strip, distinct ready sound and sparse notification accents.
- [Crab Champions — developer UI update](https://steamcommunity.com/games/774801/announcements/detail/3041595663506724929): compact icon-based build information informed the short illustrated cards.
- [RuneScape: Dragonwilds — official Using Magic guide](https://runescapedragonwilds.help.jagex.com/hc/en-gb/articles/33785973124497-Using-Magic): its spell imagery and selection interfaces informed the separation between skill illustrations and passive traits.
- [Dragonwilds — Eye on Ashenfall 0.9.3](https://dragonwilds.runescape.com/news/eye-on-ashenfall-0-9-3): interface simplification and removal of redundant widgets informed the removal of broad background panels.
- [Roboquest — official developer news archive](https://store.steampowered.com/news/posts/?appids=692890&enddate=1684927449&feed=steam_community_announcements): the energy-weapon reload changes and shared reload perks informed consistent reload/recharge rules across all weapon types.
- [Gunfire Reborn — official developer news archive](https://store.steampowered.com/news/posts/?appgroupname=Gunfire+Reborn&appids=1217060&enddate=1770378512&feed=steam_community_announcements): early reload pacing and spread adjustments informed short base reloads, differentiated magazine sizes and a separate precision trait.
- The supplied card-reference images informed the illustrated rectangular composition, restrained text and visible rarity frame. They are references only and are not bundled as game assets.

## Focused local acceptance

1. Build/import with Play_Standalone.bat; confirm the footer says 0.19.0 and bootstrap imports eight sounds. Test both local windows with the same protocol 1900.
2. Try every hub weapon. Empty its capacity, confirm one unit per primary trigger, blocked shots while reloading, hand/weapon motion, progress, and a ready sound. Reload a partial magazine with R. Check pause, downing, revive cancellation and reconnect during reload. Check P2 receives its own ammo display and sound.
3. Compare Deep Magazine ranks on the mortar and rotary; every pick must add rounds. Compare Quick Hands duration and Tight Grouping with pellet, echo, fan and split patterns at ranks zero and five.
4. Clear a chest, hover each card and hold/release Shift. Confirm the full numbers and rank preview fit, clicking selects the hovered reward, disabled choices cannot be selected, and three/four/five cards fit at 720p and 1080p. Inspect the Sanctum skill page and controller focus. Check all four HUD scales with consecutive sigil/chest notifications.
5. Enable stakes, clear multiple chambers, reopen the menu and relaunch. The checkbox must persist. Test one versus two votes in co-op, then manually uncheck. Current chest odds must not change merely from toggling.
6. Follow a gold shortcut where walking around is longer; jump and air-dash as needed. Confirm the route keeps pointing over the gap during flight, and routes around blocked geometry. Recheck maps with broken props, moving back from corners and chest arrival.

Source-layout preview images are documentation illustrations using actual glyph geometry. They are not captures of Slate or gameplay. See VALIDATION.md for tests actually run.
