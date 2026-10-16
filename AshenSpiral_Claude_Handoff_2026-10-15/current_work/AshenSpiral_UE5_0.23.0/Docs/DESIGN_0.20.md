# 0.20 — Bug fixes, armor and scope decisions

This release starts from the supplied 0.19.0 source ZIP, SHA-256 `6d1c3d739fb7ef7aa071d84c34233b7f851964b0736d42b33e052ff3286ddc8b`. The screenshot requests and later **0.20-bugfix-decisions.pdf**, dated October 13, control the work. The PDF supersedes conflicting portions of the earlier expansion worksheet. Base player/weapon/enemy tuning, new unlocks, boss redesigns and the Heat ladder are deferred. Exceptions are the specific behavior changes approved in the PDF below.

## Screenshot and gameplay fixes

The hub currency block has a wider right anchor and separate Tablet and Skill Point rows. Retired materials no longer occupy rows. Reward cards use content-sized title and brief rows with an independent bottom selection label; the 368-unit minimum leaves room for the reported long titles. The panel scales down to the viewport. Existing rarity borders, illustrations and Shift-hover numbers remain.

The final ammunition expenditure starts reload after the attack's projectiles/effects are created. Releasing Attack cannot leave the weapon empty indefinitely. Existing manual reload, capacity traits, reload poses and start/ready audio remain.

The optional vault's `0/2 SIGILS / CLAIMED` status readout is removed. The replicated claimed state turns its material into a solid gray-blue finish with zero emission, leaving its geometry in place. Nearby interaction prompts and sigil targeting retain their purpose.

## Chest guide

Enemy navigation used a 92 cm radius / 210 cm half-height probe. Reusing it for the player's guide unnecessarily excluded passages usable by the player's 38 cm radius / 88 cm half-height capsule. The guide now builds its own clearance grid, checks floor support, and validates each walking edge. It considers multiple reachable entry nodes and goal nodes inside the chest's 280 cm interaction reach. It no longer assumes a fixed approach east of the chest or appends an unchecked final segment.

Weighted Dijkstra chooses the cheapest route across that graph, including validated jump links. Jump arcs and their entry/exit connections are checked; route smoothing cannot skip a jump. Waypoint skipping requires a clear next walking segment. The result is a shortest route in the sampled graph, not a proof of the globally shortest continuous path through arbitrary geometry.

Seed 6767 at displayed chamber 23 (zero-based room 22) generates seed 2310805, loop shape 3, eight rooms and exit room 5. Its native regression checks supported room/corridor routes from every room into chest reach. This reproduces the deterministic layout, **not the reported live collision failure**. Imported collision, props, route visibility and actual movement still need the editor test.

## Latest PDF decisions

| Decision | Implemented behavior |
|---|---|
| 1. Treasure Sense | Talent 9 is hidden/unpurchasable; owning its bit refunds one Skill Point and clears it. No replacement talent. Scavenger branch requirements become 1 and 3 spent points so its remaining nodes stay reachable. |
| 2. Vault Sense | Trait 55 remains reserved and retired. |
| 3. Relic Burn | Ashen Kindling retains its existing pulse damage and applies Burn at 60% of the owner's skill damage scale. Cinder Wake applies real Burn to enemies struck by its trail, respecting line of sight and height. |
| 4. Shattered Ring | Text now says six poison arrows; projectile visuals are unchanged. |
| 5. Second Echo | Store base radius in the delayed field, apply area scaling once for damage, and draw the ring at that same radius. |
| 6. Rune Nova | Remains available with all six active skills; behavior is listed below. |
| 7. Shield caps | Ash Mantle and Linked Landing use `max(current, min(cap, current + gain))`. Grants cannot lower an existing higher shield. War Cry/barrier grants use the same guard. |
| 8. Guardian Link | Hit taker and talent holder must be within 720 cm horizontally. Existing cooldown and shield amounts remain. |
| 9. Twin | Description says the main well is full size and the second is 72% size. |
| 10. Razor Dash | While a trait-enabled dash is moving, the movement sweep ignores enemy actors, including reciprocal enemy movement. Allies, walls, floors, props and the practice dummy still collide. Ignores are removed on dash end/downing. |
| 11. Dead materials | Crystal Shard and Vine Fiber balances convert 1:1 into Scrap. Their old slots remain reserved in saves. Ward drops and victory rewards use the three active materials; UI/catalog hide retired currencies. No new currency sinks. |
| 12. Side traps | A dedicated spawn counter alternates flame and rotating-blade traps. Vault alternation is unchanged. |
| 13. Milestones | Small Ember/Tablet payouts and equipable achievement titles, with no direct combat-stat, XP or Skill Point rewards. |

### Rune Nova interpretation

| Active skill | With Rune Nova |
|---|---|
| Rune Lance | Existing 300 cm base-radius impact blast. |
| Winter Fan | Retains its cone and gains a 300 cm impact bloom at the aimed impact, capped to 650 cm forward reach. A target in both zones is hit once. |
| Storm Thread | Each arc impact gains a 300 cm blast at that hop's existing damage. A shared visited set prevents a target from taking multiple arc/splash hits during one cast. |
| Venom Bloom | Base field radius increases 320 → 620 cm. |
| War Cry | Base impact radius increases 330 → 630 cm; shield/buff values remain unchanged. |
| Cinder Mine | Base explosion radius increases 330 → 630 cm. |

Each area applies the owner's area scale once. Existing damage, Focus costs, cooldowns and status rules remain; no recursive proc chain is added. Sight checks protect targets behind cover.

### Career and migration

Every fifth level grants one Relic Ember; every tenth also grants one Tablet. Each new achievement tier grants one Ember, and tier III also grants one Tablet. Any earned achievement name/rank can be equipped as a cosmetic title from Character & Achievements and appears in the party listing. Existing earned milestones get the same one-time catch-up.

Profile schema stays 1. Additive `MigrationVersion`, `RewardedLevel`, `RewardedTiers` and `TitleChoice` fields preserve old saves. Migration and milestone payments are sanitized before a verified save transaction; their paid cursors and currency balances commit together. Repeated sanitation/receipts do not add another refund or payout. Direct stat progression, old purchases and unrelated IDs remain intact. Existing A/B profile saving and import review remain. Both players need protocol 2000 because title metadata was added to profile submission/replication.

## Character and enemies

The supplied reference informed a closed helmet with a narrow blue visor, dark layered gunmetal plates, a triangular blue chest core and a long black cape. Original segmented meshes provide separate upper/lower limbs, hands, boots and cape sections. Smooth metal/cloth parameters avoid applying the world-stone texture to armor. Humanoid enemies use horned helmets, red accents and torn capes; the leaper and broodcaller remain nonhuman relatives of the same style.

Movement uses a velocity-driven planted/swing foot cycle, two-bone knee/elbow solving, local ground samples, upper/lower body counter-motion, breathing, landing compression, weapon-support positioning, reload gestures and cape lag. These are procedural modular poses. They are not a full skinned production rig, mocap library, or cloth simulation. `ARMOR_0.20_PREVIEW.png` and `GAIT_GEOMETRY_PREVIEW.gif` show actual source geometry; final material response, pose transitions and performance require Unreal.

The bootstrap revision forces the bundled meshes to reimport and recreates the owned material. There are 71 meshes, three textures and eight audio files. Normal setup uses bundled assets; optional art/preview regeneration is separate.

## Focused local acceptance after building

1. Confirm menu version 0.20.0, both peers on protocol 2000, and a successful full first build/import. Test solo and separate host/client processes before two-PC play.
2. Inspect the hub at 1280×720, 1920×1080 and ultrawide, all four HUD sizes, 100%/120% text, and large currency counts. Skill Points must stay within the right margin. Inspect Expanding Force, Ability Cooldown, Emberforge Evolution and Shift details with four/five reward choices.
3. For each of nine weapons, spend the final round and release Attack immediately. Reload must start, finish once, refill the magazine and play its ready cue. Repeat with partial manual reload, Quick Hands, Deep Magazine, pause and P2.
4. Activate two sigils and claim a vault. No status counter should remain; claimed geometry must lose glow for both peers while sigil prompts and stacked notices remain readable.
5. Run seed 6767 through chamber 23; approach from all eight rooms and both sides of the loop. Follow the guide into actual chest reach. Also test gap/jump shortcuts, an airborne player, destroyed props, a sealed route and returning after wandering off-path. The arrow must never claim a route through a failed final connector.
6. Verify Ashen Kindling/Cinder Wake create Burn and respect cover. Compare Second Echo's ring/damage with zero and three area ranks. Test all six active skills with Rune Nova; check cone/bloom overlaps and overlapping Storm blasts hit each enemy once per cast.
7. Start above 60 shield and trigger Ash Mantle/Linked Landing; shield must not drop. Guardian Link must respond within 720 cm and stop outside it. Razor Dash must pass enemy crowds while walls and allies still block; check the end pose does not leave the character stuck inside an enemy.
8. On a backed-up legacy test profile, own Treasure Sense and hold both removed currencies. Check the one-point refund, 1:1 Scrap conversion, remaining Scavenger purchases and unchanged unrelated progression. Restart/import/retry a receipt: no duplicate rewards. Test migration save failure/recovery with disposable profiles.
9. Cross a level multiple of five/ten and all achievement tiers; check modest payouts, old-profile catch-up, title equip, title rank refresh and party replication. No direct stats should change from milestone awards.
10. Inspect armor from front/back and near/far camera positions with every weapon: idle, forward/backward/strafe, slopes, jump, air dash, landing, reload, charged attacks, downing and remote interpolation. Check hands, feet, cape intersections and draw-call/frame-time cost with a full enemy wave.

These are **pending engine acceptance cases**, not completed playtests. Exact executed source/native checks are in `VALIDATION.md` and `Tests/validation_results.json`. Technical material API reference: https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialExpressionConstant . Earlier game/interface research remains in `DESIGN_0.18.md` and `DESIGN_0.19.md`.
