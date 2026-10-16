# 0.18.0 — Route guide, compact UI and larger arenas

## Chest navigation

The previous screen-space arrow is replaced by an original extruded 3D chevron near the character, approximately 1.55 meters along the route and 1.08 meters above the floor. Smaller ground chevrons reveal upcoming turns. Normal depth testing lets walls occlude the guide.

Each local player computes a route to an approach point beside the reward chest or return portal. A direct supported capsule-clear route is preferred; otherwise four-neighbor A* finds a shortest route on the collision-derived grid. Clear-node lookahead straightens the result. This is shortest-path grid navigation with collision-aware smoothing, not an exact continuous-space geodesic. Optional jump gaps are excluded.

The route refreshes every 0.30 seconds or after 2 meters of movement; the leading arrow follows each frame. A new map or changed breakable count refreshes the navigation grid. The guide hides in menus, while paused/downed, outside reward/victory phases, inside chest interaction range, or when no supported route exists.

## HUD and notifications

The default 100% layout uses dark-teal panels with gold accents: expedition status upper left, a small minimap upper right, health lower left and a short ability dock lower right. A narrow objective/boss strip appears when relevant. Hub resources sit beneath the minimap. Menus use a smaller 880×680 logical panel and less screen dimming.

**Esc → Settings** selects exact HUD sizes of 25%, 50%, 75% and 100%. Panel text, bars, prompts, revive/downed messages and the notification stack scale together. The setting saves immediately. Crosshair size, damage-number size and the optional 120% text preference remain separate; menus retain their normal scale.

One right-side notification lane holds up to four cards. Each new notice animates existing cards upward without resetting their ages. A message holds for 4.5 seconds, then fades for another 4.5 seconds. Fixed card heights and bounded wrapping keep messages apart. The newest four remain during a burst. Redundant static sigil/cache/chest labels are removed; the focused interaction prompt remains.

## Rooms and scenery

The campaign remains four wards of eight chambers. Each chamber has **5–10 actual rooms**, with a maximum footprint of **3×3**. A base cell is 12×12 meters; 2×2 is 24×24 meters and four times its floor area. 2×3 and 3×2 create longer halls; 3×3 creates a large landmark arena.

Five seeded families organize courts, paired galleries, cross sanctuaries, cloister rings and winding galleries. Physical spacing accounts for the largest adjacent footprints, keeping rooms separate and corridors clear. A solid spanning route connects every room; an optional gap is only a shortcut. The chest occupies the room farthest from the start by weighted shortest walking distance.

Landmarks occupy quadrants around open central lanes: library arcades, processional tombs, basalt fans, garden abbey fragments, and limestone formations with shallow turquoise pools. Floor rings and warm/cool lighting establish focal points. LAYOUT_OVERVIEW.png is generated from actual C++ layouts and drawn to scale.

## Three new weapons

| Weapon | Identity | Base tuning | Hub cost |
|---|---|---|---|
| Cinder Scattergun | Six burning pellets; tight cone and distance falloff | 72 total damage; 0.72 s between shots; full damage through 6 m | 65 Ash |
| Tempest Rotary | Rapid frost rounds; sustained fire widens spread; releasing recovers accuracy | 10 damage; 0.105 s between rounds | 80 Ash |
| Rift Mortar | Straight-flying ember rocket; impact/timeout explosion respects static cover | 86 damage; 1.05 s between shots; 2.4 m blast radius | 95 Ash |

All three have original procedural meshes, two-hand poses, numbered physical displays, free practice trials and armory entries. Weapon IDs 0–5 stay unchanged; IDs 6–8 extend the unlock mask without changing profile schema 1. WEAPON_MODELS.png displays actual OBJ geometry under neutral preview lighting.

Flat damage bonuses normalize by attack interval and pellet count. Only one scatter pellet per trigger can initiate primary-hit proc chains. Rocket splash has one primary proc carrier per explosion, falls to 35% at the edge, and does not directly damage players. Environmental explosive casks retain their normal behavior. Pierce, Ricochet and Split do not apply to rockets; eligible extra-shot and area traits do. Echo/Volley scale from total trigger damage for the scattergun.

These are initial source tuning values, not playtested balance. Upgrades can change observed results.

## Research and art direction

Modern interfaces, weapon archetypes and architectural/natural spaces informed original code and geometry. No reference screenshots, third-party models or game assets are shipped as game content.

- [Housemarque on Returnal's UX](https://blog.playstation.com/2021/05/11/unpacking-returnals-ux-design-gameplay-first-ui-retro-futuristic-tech-and-accessibility/): gameplay-first hierarchy informed the clear center, peripheral panels and nearby world guide.
- [Returnal interface reference](https://interfaceingame.com/games/returnal/): screenshots informed spacing and grouping. Hades II interface screenshots were also reviewed for health/resource placement and hierarchy.
- [EA's Dead Space remake announcement](https://news.ea.com/press-releases/press-releases-details/2023/Dead-Space-Remake-of-theSci-Fi-Survival-Horror-Classic-Now-Available-on-PlayStation-5-Xbox-Series-XS-and-PC/default.aspx): the locator concept informed readable guidance through connected spaces.
- [Official Crab Champions developer announcements](https://store.steampowered.com/news/posts/?appgroupname=Crab+Champions&appids=774801&enddate=1709574454&feed=steam_community_announcements): shotgun, minigun and rocket-launcher archetypes informed three distinct firing rhythms. Ashen Spiral uses original names, meshes, tuning and existing elemental/proc rules.
- [Kena: Bridge of Spirits virtual photography](https://fransbouma.com/kena-bridge-of-spirits): overgrown ruins and architectural silhouettes informed garden landmarks.
- [Pays de la Loire / abbey cycling route](https://www.enpaysdelaloire.com/activites/autour-du-velo/boucle-cyclable-le-ble-dur): cloister courtyard imagery informed loops and repeated arcades.
- [Proumeyssac cave reference](https://www.grandsudinsolite.fr/1962--.html) and [Epic's Nordic cave tutorial](https://dev.epicgames.com/community/learning/tutorials/6k3/creating-a-nordic-cave-environment-in-ue5): cavern scale and clustered formations informed limestone and basalt accents.
- [Palouse photography](https://udaypratti.com/palouse): geological forms informed broad spaces and narrow passages.
- [Abbey environment reference](https://www.fab.com/listings/d7810951-6891-411f-8fd0-c6d66fb728c8): architectural rhythm informed broken garden arcades. The asset pack was not imported.

## Local acceptance after compilation

1. Extract the full package into a fresh folder. Run Play_Standalone.bat; check the menu says 0.18.0 and all 61 meshes import. Both co-op peers must use protocol 1800.
2. Try every HUD size at 1920×1080 and a smaller window, with normal and 120% text. Relaunch to verify persistence. Check health, minimap, abilities, interaction, downed/revive and reward menus.
3. Activate both sigils and claim a cache/chest quickly. Verify each new card pushes older cards upward, the stack stays separate from interaction prompts, and messages fade over nine seconds.
4. Clear multiple seeds and each biome. Follow the guide through bends, walk backward at corners, destroy a blocking prop, open/close a menu and approach the chest. Repeat as host and client.
5. Inspect large rooms and corridor joins. Confirm at most ten rooms, all reachable without jumping, open landmark lanes, and no collision/rendering gaps. Check ceilings, lighting and frame time.
6. Try all three weapons on the hub target and in combat. Check scatter falloff, rotary accuracy recovery and rockets against walls and groups. Test Echo/Volley, area upgrades and cover; verify no direct ally damage or recursive procs.
7. Load an older profile and confirm original unlocks/currencies. Purchase new weapons, save/relaunch, then test host/client replication and reconnect. Retain existing input-capture and progression cases.
