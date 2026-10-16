> Historical expansion proposal — deferred where it exceeds the later `REQUESTED_BUGFIX_DECISIONS.pdf`. This worksheet is retained as input, not as a description of shipped 0.20 behavior. The PDF explicitly rejects the Treasure Sense rework and Crystal/Vine sinks, and defers broad balance, new unlocks, boss redesigns and the Heat ladder. Use `DESIGN_0.20.md` and `ASHEN_SPIRAL_0.20_EDITABLE_CATALOG.txt` for implemented behavior.

**Ashen Spiral – Editable Game Catalog Delta** 

**Target version = 0.20.0 / protocol 2000** 

**Prepared = 2026-10-13** 

**Baseline = 0.19.0 (SHA-256 6d1c3d739fb7ef7aa071d84c34233b7f851964b0736d42b33e052ff3286ddc8b)**

This is a complete delta worksheet. Every changed or new entry is listed. Unmentioned entries keep 0.19.0 values and behavior. Apply in order: fixes first, then numbers, then new content.

---

### 01. PLAYER BASELINE, CONTROLS AND RELOAD RULES

```

[PLAYER_BASE] Starting character

max_health = 130

starting_shield = 0

base_critical_chance_percent = 6

base_critical_damage_multiplier = 1.8

critical_chance_cap_percent = 50

focus_max = 60

focus_regeneration_per_s = 5.5

focus_regeneration_phases = Combat and lobby; no regeneration in countdown/reward/victory

running_speed_cm_s = 675

water_speed_multiplier = 0.45

jump_vertical_velocity_cm_s = 660

gravity_scale = 1.6

air_control = 0.45

jumps_per_landing = 1

fall_recovery_damage = 12

chamber_entry_minimum_health_percent = 35

chamber_entry_protection_s = 3

fresh_build_protection_s = 2

after_damage_protection_s = 0.5

coop_incoming_damage_multiplier = 1.0

body_collisions = Players and enemies block each other; normal body contact has no damage

requested_changes = Increased base HP and run speed, reduced fall damage, removed co-op damage penalty for primary co-op focus. Early-game feel smoother; late-game power still gated by investment.

```

```

[DASH] Dash

(base values unchanged)

requested_changes =

```

```

[REVIVE] Co-op revival

hold_s = 2

horizontal_reach_cm = 300

vertical_reach_cm = 150

restored_health_percent = 50

protection_s = 2.5

xp_to_helper = 20

focus_restore_to_helper = 8

requirements = Helper grounded, alive, in clear line of sight; combat/countdown/reward. Progress resets if help stops.

requested_changes = Slightly stronger revive for co-op primary mode. Helper gains small Focus on successful revive.

```

```

[RELOAD_GLOBAL] Reloading / recharging

(unchanged)

requested_changes =

```

```

[CONTROLS] Defaults

(unchanged)

requested_changes =

```

---

### 02. WEAPONS

```

[WEAPON_03] Spiral Glaive

unlock_cost_ash = 35

native_element = Frost

base_damage_per_trigger = 44

base_trigger_interval_s = 0.41

base_capacity = 7

base_reload_s = 1.25

magazine_gain_per_rank = 2

capacity_at_magazine_rank_5 = 17

behavior = Projectile speed 1600 cm/s; lifetime 4 s. Primary glaive entering a gameplay well is consumed and enables +5 well damage per pulse for 2.5 s.

requested_changes = Buffed damage and fire rate so it competes with mid-tier weapons while retaining well-interaction identity.

```

```

[WEAPON_09] Rift Chain (NEW)

unlock_cost_ash = 50

native_element = Poison

base_damage_per_trigger = 28

base_trigger_interval_s = 0.38

base_capacity = 9

base_reload_s = 1.35

magazine_gain_per_rank = 2

capacity_at_magazine_rank_5 = 19

behavior = Chain projectile, max 3 jumps, 0.78 damage retention per jump. Search radius 580 cm. Gains +12% damage while any owned well is active. Does not pierce or ricochet.

requested_changes = New mid-tier weapon. Rewards well placement and multi-target play. Co-op friendly.

```

```

[WEAPON_10] Ashen Pike (NEW)

unlock_cost_ash = 60

native_element = Burn

base_damage_per_trigger = 55

base_trigger_interval_s = 0.55

base_capacity = 5

base_reload_s = 1.40

magazine_gain_per_rank = 1

capacity_at_magazine_rank_5 = 10

behavior = Melee reach 420 + target capsule. Charge threshold 0.55 s. Charged release plants a temporary well (70% normal radius, 3.0 s duration) at 280 cm ahead. Own wells last 85% normal duration while Pike is equipped.

requested_changes = New melee weapon with well-planting identity. Dark-fantasy spear aesthetic, sci-fi energy edge.

```

```

[WEAPON_11] Void Harpoon (NEW)

unlock_cost_ash = 75

native_element = Frost

base_damage_per_trigger = 48

base_trigger_interval_s = 0.70

base_capacity = 4

base_reload_s = 1.55

magazine_gain_per_rank = 1

capacity_at_magazine_rank_5 = 9

behavior = Projectile speed 2100 cm/s; lifetime 3.2 s. On hit: pulls target 180 cm toward player (elites/bosses 90 cm) and applies 48 frost buildup. Cannot pierce. Direct hit only.

requested_changes = New ranged crowd-control weapon. Pulls enemies into wells or toward the party. Strong co-op utility.

```

All other weapons keep 0.19.0 values. Honed Edge and firearm normalization formulas unchanged.

---

### 03. SPIRAL AND SELECTABLE ACTIVE ABILITIES

```

[SPIRAL] Core hold/release ability

focus_cost = 0

base_cooldown_s = 3.3

cooldown_reduction_cap_percent = 65

minimum_cooldown_s = 1.155

cooldown_formula = 3.3 * max(0.35, 1 - 0.10*permanent_cooldown_rank - 0.08*run_cooldown_rank)

perfect_hold_window_s = 0.60 to 1.00 inclusive

normal_radius_cm = 450

perfect_radius_cm = 340

base_duration_s = 4.4

normal_pull_cm_s = 300

perfect_pull_cm_s = 460

pulse_interval_s = 0.25

normal_pulse_damage = 6

perfect_pulse_damage = 11

normal_collapse_damage = 40

perfect_collapse_damage = 82

placement_distance_cm = 560

simultaneous_well_cap_shared = 8

behavior = Line-of-sight checked. Pull affects normal enemies, not elites/bosses. Perfect is smaller, stronger. Oldest well collapses at the shared cap. In co-op, wells are shared ownership for pulse/collapse credit.

requested_changes = Widened Perfect window, minor duration/pull/damage buffs, shared ownership for co-op. Core ability feels more reliable and rewarding.

```

```

[ABILITY_00] Rune Lance

focus_cost = 22

base_cooldown_s = 1.15

base_damage = 56

aim_range_cm = 1700

marked_target_multiplier = 1.3

behavior = Single aimed sweep. Rune Nova replaces single-target impact with a 300 cm area blast; the marked-target bonus is only in the non-Nova branch.

requested_changes = Slight damage and range buff, reduced focus cost. Rune Nova eligibility restricted to Rune Lance only (Section 17 fix).

```

```

[ABILITY_06] Rift Sever (NEW)

unlock_cost = Free; all eight available

focus_cost = 28

base_cooldown_s = 5.5

base_damage = 42

line_length_cm = 1100

line_width_cm = 140

field_duration_s = 2.5

field_damage_amp_percent = 12

behavior = Line attack. Surviving enemies leave a weakening field that increases damage taken by 12% for 2.5 s. Clear sight required.

requested_changes = New active. Utility + damage, strong with wells and co-op focus fire.

```

```

[ABILITY_07] Cinder Veil (NEW)

unlock_cost = Free; all eight available

focus_cost = 26

base_cooldown_s = 8

damage_reduction_percent = 18

burn_aura_dps = 9

aura_radius_cm = 280

duration_s = 4

behavior = Grants damage reduction and a burn aura. Aura scales +1 DPS per 10 current Focus (max +6). Applies to self only; in co-op the aura also affects the partner if within radius.

requested_changes = New defensive/offensive hybrid active. Rewards Focus management and co-op proximity.

```

Active ability cooldown formula and Focus rules unchanged. All abilities practiceable in hub.

---

### 04. SPIRAL STYLES

Existing styles 00–09 unchanged except description fixes.

```

[STYLE_04] Sanctuary

effect = collapse_heal_hp = 4; owner only; combat benefit.

requested_changes = Minor heal buff.

```

```

[STYLE_06] Twin

effect = second_well_radius_multiplier = 0.72; lateral_offset_cm = 240. Main well stays full-sized; second is non-Perfect. Does not add a third well with Twin Singularity.

requested_changes = Description corrected to match implementation (Section 17).

```

```

[STYLE_10] Pulse (NEW)

availability = Unlocked by style-discovery purchase 5

effect = pulse_interval_multiplier = 0.82; well_radius_multiplier = 0.90.

persistence = Hub selection permanent; chest style choice replaces for the run.

requested_changes = Faster pulses at slight radius cost. Spiral-centric.

```

```

[STYLE_11] Anchor Point (NEW)

availability = Unlocked by style-discovery purchase 6

effect = collapse_damage_multiplier = 1.28; pull_strength_multiplier = 0.80.

persistence = Hub selection permanent; chest style choice replaces for the run.

requested_changes = Stronger collapse, weaker pull. High-risk high-reward.

```

```

[STYLE_12] Mirroring (NEW)

availability = Unlocked by style-discovery purchase 7

effect = On Perfect Spiral, create a mirror well at 55% strength, 180 cm opposite the player, non-Perfect, same duration.

persistence = Hub selection permanent; chest style choice replaces for the run.

requested_changes = Spatial play and co-op positioning reward.

```

```

[STYLE_13] Drain (NEW)

availability = Unlocked by style-discovery purchase 8

effect = Wells slowly restore 1.5 Focus per second to the owner while any enemy is inside. Cap 8 Focus per well lifetime.

persistence = Hub selection permanent; chest style choice replaces for the run.

requested_changes = Focus sustain style. Pairs with high-Focus actives.

```

---

### 05. CORE RUN UPGRADES

Unchanged. Max picks and effects remain the same.

---

### 06. RUN TRAITS

All existing traits keep 0.19.0 values except noted fixes.

```

[TRAIT_66] Cinder Resonance (NEW)

source_key = CinderResonance; reward_offer_id 266

status = Active

rarity = Common

category = Modifier

max_picks = 3

can_evolve = Yes, once for +1 effective rank

eligible_weapons = All

effect = +10% well pulse damage and +5% well duration per rank.

requested_changes = New Common trait. Directly rewards Spiral investment.

```

```

[TRAIT_67] Spiral Harmony (NEW)

source_key = SpiralHarmony; reward_offer_id 267

status = Active

rarity = Rare

category = Modifier

max_picks = 2

can_evolve = Yes

eligible_weapons = All

effect = Active ability cooldown reductions also apply 50% of their value to Spiral cooldown (still respect 65% cap).

requested_changes = New Rare. Links active and Spiral cooldowns.

```

```

[TRAIT_68] Well Harvest (NEW)

source_key = WellHarvest; reward_offer_id 268

status = Active

rarity = Common

category = Modifier

max_picks = 3

can_evolve = Yes

eligible_weapons = All

effect = Enemies that die inside an owned well drop +1 Cinder (capped at 2 extra per chamber).

requested_changes = Economy trait that rewards well placement.

```

```

[TRAIT_69] Adaptive Edge (NEW)

source_key = AdaptiveEdge; reward_offer_id 269

status = Active

rarity = Rare

category = Damage

max_picks = 2

can_evolve = Yes

eligible_weapons = All

effect = After using a selectable active, next 3 primary attacks gain +15% damage.

requested_changes = Burst window after ability use.

```

```

[TRAIT_70] Fracture Line (NEW)

source_key = FractureLine; reward_offer_id 270

status = Active

rarity = Common

category = Modifier

max_picks = 3

can_evolve = Yes

eligible_weapons = All

effect = Primary hits on chilled or poisoned enemies deal +10% damage and have 12% chance per rank to apply the opposite status.

requested_changes = Status synergy trait.

```

```

[TRAIT_71] Echo Chamber (NEW)

source_key = EchoChamber; reward_offer_id 271

status = Active

rarity = Rare

category = Modifier

max_picks = 2

can_evolve = Yes

eligible_weapons = All

effect = Wells last +12% duration and pulse +2 damage per rank.

requested_changes = Pure well strength.

```

```

[TRAIT_72] Second Skin (NEW)

source_key = SecondSkin; reward_offer_id 272

status = Active

rarity = Common

category = Survival

max_picks = 3

can_evolve = Yes

eligible_weapons = All

effect = After taking damage, gain 6 shield that decays over 3.5 s (does not reduce existing higher shield).

requested_changes = Small reactive shield. Co-op friendly.

```

```

[TRAIT_73] Predator’s Mark (NEW)

source_key = PredatorsMark; reward_offer_id 273

status = Active

rarity = Rare

category = Damage

max_picks = 2

can_evolve = Yes

eligible_weapons = All

effect = First hit on an elite or boss applies a 4 s mark that increases damage taken by 10% from all sources.

requested_changes = Elite/boss focus. Strong in co-op.

```

Section 17 description fixes applied to Kindling Touch, Cinder Wake, Twin, etc. (text only).

---

### 07. RELICS

```

[RELIC_24] Ashen Focus (NEW)

discovery_batch = 5

effect = Perfect Spiral restores 15 Focus.

requested_changes = Directly addresses Focus-limited abilities.

```

```

[RELIC_25] Shared Spark (NEW)

discovery_batch = 5

effect = When you land a Perfect Spiral, the partner restores 8 Focus if within 900 cm.

requested_changes = Co-op primary relic.

```

```

[RELIC_26] Wellbound (NEW)

discovery_batch = 6

effect = Owned wells grant +8% damage to all party members standing inside them.

requested_changes = Positioning and co-op reward.

```

```

[RELIC_27] Aftershock (NEW)

discovery_batch = 6

effect = Collapse deals an additional 25% of its damage as Burn over 2 s.

requested_changes = Collapse identity expansion.

```

```

[RELIC_28] Linked Landing (NEW – revised)

effect = On successful revive, both players gain 18 shield (never reduces existing higher shield) and 1 dash charge.

requested_changes = Section 17 shield hard-cap fix + stronger co-op revive identity.

```

```

[RELIC_29] Rift Anchor (NEW)

discovery_batch = 7

effect = First well placed each chamber has +25% duration and cannot be collapsed by the shared cap until a second well is placed.

requested_changes = Safety net for Spiral-focused builds.

```

All other relics keep 0.19.0 behavior. Vault Sense remains retired reserved ID.

---

### 08. PERMANENT HEARTH UPGRADES

Existing 20 upgrades unchanged in effect. New branch added for long meta:

```

[HEARTH_SPIRAL_00] Spiral Pulse (NEW branch)

base_cost_ash = 18

max_rank = 5

effect = +1 well pulse damage per rank (applies to normal and Perfect).

requested_changes = Long-term Spiral investment. Linear value to ~20 h.

```

```

[HEARTH_SPIRAL_01] Spiral Duration

base_cost_ash = 20

max_rank = 5

effect = +0.15 s well duration per rank.

```

```

[HEARTH_SPIRAL_02] Perfect Window

base_cost_ash = 22

max_rank = 4

effect = Perfect hold window expands by 0.04 s on each side per rank.

```

```

[HEARTH_SPIRAL_03] Shared Ownership

base_cost_ash = 35

max_rank = 1

effect = In co-op, wells placed by either player count as owned by both for pulse, collapse, and trait credit.

```

Additional micro-upgrades (Crystal Shard / Vine Fiber sinks) appear in Section 10.

---

### 09. PERMANENT CONSTELLATION TALENTS

Treasure Sense fixed or replaced:

```

[TALENT_TREASURE_SENSE]

cost_sp = 1

effect = Elites are highlighted with a subtle gold outline for 8 s when they spawn. +10% chance for elite to drop an extra Cinder.

requested_changes = Section 17 fix. Restored useful effect.

```

New talents (8):

```

[TALENT_ECHO_CHAMBER]

cost_sp = 1

effect = Wells last +10% and pulse +1 damage permanently.

```

```

[TALENT_FRACTURE_LINE]

cost_sp = 1

effect = Primary hits on chilled/poisoned enemies deal +8% damage.

```

```

[TALENT_SECOND_SKIN]

cost_sp = 1

effect = After taking damage, gain 5 decaying shield (3 s). Never reduces higher existing shield.

```

```

[TALENT_MOMENTUM_VAULT]

cost_sp = 1

effect = Dash distance +6% and restore 1 Focus on dash.

```

```

[TALENT_WELLSPRING]

cost_sp = 2

effect = When a well collapses, restore 3 Focus to self (and 2 to partner if within 700 cm).

```

```

[TALENT_PREDATORS_MARK]

cost_sp = 2

effect = First hit on elite/boss applies 3 s mark (+8% damage taken).

```

```

[TALENT_CINDER_CACHE]

cost_sp = 1

effect = 6% chance on kill to drop a 8-Focus orb.

```

```

[TALENT_IRON_RHYTHM]

cost_sp = 2

effect = After three consecutive Perfect Spirals, next active ability costs 0 Focus (once per chamber).

```

Total new SP cost ~11. Combined with existing 21 SP, progression stretches comfortably into the 20–40 h range.

---

### 10. CURRENCIES, PURCHASES, CRAFTING AND UNLOCKS

```

[CURRENCY_CRYSTAL_SHARD] New sinks

Temporary chamber buffs (hub or shrine):

- 12 Shard = +10% damage for next chamber

- 15 Shard = +15 max HP for next chamber

- 18 Shard = +1 dash charge for next chamber

Permanent micro (repeatable, diminishing):

- 40 Shard = +1 max Focus (max 5 times)

- 50 Shard = +0.5% base critical chance (max 4 times)

```

```

[CURRENCY_VINE_FIBER] New sinks

- 12 Fiber = 8 shield on chamber entry for next chamber

- 20 Fiber = +5% movement speed for next chamber

Permanent micro:

- 45 Fiber = +2 fall damage reduction (max 3)

- 55 Fiber = +0.1 s after-damage protection (max 3)

```

Ash sinks expanded by the new Spiral Attunement Hearth branch and three new weapons (≈ +220 Ash). Combined with longer rank curves, linear returns extend to ~20 h; soft diminishing thereafter toward 50 h.

Style discovery purchases expanded to 8. Relic discovery batches expanded to 7.

---

### 11–13. CHESTS, REWARDS, LEVELS, ACHIEVEMENTS

```

[ACHIEVEMENT_12] Flawless Expedition (NEW)

metric = FlawlessRuns

milestone_1 = 1

milestone_2 = 5

milestone_3 = 20

requirement = Complete a full 32-chamber expedition without dying (solo or co-op).

reward = Cosmetic only.

```

All other achievements remain cosmetic. XP and levels remain cosmetic. No power granted by achievements or character level.

Reward drafting, rerolls, evolution, and Raise the Stakes unchanged except new trait/relic IDs are added to the pools with appropriate weights.

---

### 14. STATUS EFFECTS AND DAMAGE STACKING

Unchanged except:

```

[STATUS_MISC]

Ash Mantle / Linked Landing / Second Skin / any shield grant

shield_application_rule = max(existing_shield, existing_shield + gain) or additive without artificial 60 cap.

requested_changes = Section 17 hard-cap fix. Shields never decrease from these effects.

```

Second Echo radius scaling fixed: echo uses the same final radius as the original collapse (area scale applied once).

---

### 15. ENEMIES, BOSSES, TRAPS AND WORLD PROGRESSION

```

[ENEMY_GLOBAL] Enemy scaling

normal_base_hp_formula = 68 + 36*ward_index

elite_base_hp_formula = 290 + 52*ward_index

boss_base_hp_formula = (explicit solo values raised proportionally)

coop_health_multiplier = 1.32

stakes_health_multiplier = 1 + 0.15*current_tier

requested_changes = Steeper late-ward HP for challenge, reduced co-op multiplier so two players feel stronger together (primary co-op mode). Stakes less punishing. Option B difficulty target: strong expressive play most of the time on stakes 2–3; final boss + select elites remain threatening.

```

Boss solo HP before stakes (approx):

- ch8  1200

- ch16 1750

- ch24 2300

- ch32 2900

New enemy archetypes (high-level only; exact numbers in implementation):

```

[ENEMY_10] Rift Stalker (NEW)

ward_availability = 3–4

role = Elite flanking assassin. Short invisibility after taking damage, lunges from unexpected angles. High priority target in co-op.

```

```

[ENEMY_11] Ash Warden (NEW)

ward_availability = 2–4

role = Support. Places small protective fields on nearby allies that reduce damage taken. Encourages focus fire.

```

Boss attack variety increased slightly (extra telegraph clarity + one additional pattern variant each) without changing core identity. Trap scaling unchanged.

---

### 16. HUD, CAMERA AND COSMETIC OPTIONS

Unchanged. Character color system remains; new armored mesh (mood reference) will use the existing emissive/tint channels for blue energy lines and cape variants. Cape is optional per style or cosmetic unlock. Helmet always closed for the base armored look.

---

### 17. DESCRIPTION / IMPLEMENTATION DIFFERENCES – ALL RESOLVED

1. Vault Sense – remains retired. 

2. Treasure Sense – effect restored (gold outline + small Cinder chance). 

3. Ashen Kindling / Cinder Wake – now correctly apply Burn status. 

4. Twin style – description corrected. 

5. Shield hard-caps – removed. 

6. Hungry Heart – already correct (all pickups). 

7. Shattered Ring – already correct. 

8. Rune Nova – only appears/modifies Rune Lance. 

9. Second Echo – area scale applied once. 

10–16. All other listed discrepancies corrected in text or code path.

---

### 18. 0.20.0 CHANGE SUMMARY / PRIORITIES

**highest_priority_changes** 

- All Section 17 fixes 

- Player baseline survivability + mobility 

- Glaive 44 dmg / 0.41 s 

- Enemy HP + co-op multiplier retune (Option B) 

- New Spiral Attunement Hearth branch + Crystal/Vine sinks (meta length) 

- Three new weapons, two new actives, eight new traits, six new relics, four new styles, eight new talents 

- Co-op positive interactions (shared wells, revive Focus/shield, Shared Spark, etc.)

**desired_overall_difficulty** = Strong expressive play for invested players on stakes 2–3; final boss and select elites remain the real tests.

**desired_weapon_pacing** = All weapons competitive; Glaive and new weapons fill mid/high tiers without invalidating existing identities.

**desired_reward_generosity** = Keep current drafting feel; new content adds meaningful long-term goals rather than power creep.

**items_to_remove** = None.

**new_weapons** = Rift Chain, Ashen Pike, Void Harpoon 

**new_abilities** = Rift Sever, Cinder Veil 

**new_traits** = 66–73 

**new_relics** = 24–29 

**new_styles** = 10–13 

**new_talents** = 8 listed above 

**new_achievements** = Flawless Expedition 

**other_notes** 

- Art direction: mood reference only. Sleek dark-gunmetal armor with blue energy, optional cape, closed helmet. 

- Animations: generate custom sets autonomously where tools allow (player locomotion, dash, Spiral cast, new weapon swings, boss phase transitions). 

- Co-op is primary design target. 

- Meta returns stay linear and satisfying to ~20 h, then gently diminish toward 50 h. 

- Test thoroughly in co-op; measure real clear times and death rates on stakes 2–3 with full meta.

End of 0.20.0 catalog delta. 

Upload the edited text and instruct the team to apply.
