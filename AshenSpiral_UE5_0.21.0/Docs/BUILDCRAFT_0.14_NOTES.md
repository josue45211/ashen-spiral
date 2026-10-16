# Ashen Spiral 0.15.0 — Buildcraft update

Source update for Unreal Engine 5.8.3 on Windows. Not a prebuilt executable and not yet compiled or playtested in Unreal. Run `Play_Standalone.bat` from a fresh extraction; both co-op players need this version (protocol 1500).

## New run-building system

There are **63 new run traits**, alongside the existing six stackable stat upgrades, 24 relics and ten Spiral styles. `UPGRADE_CATALOG.md` lists every new trait's effect, rarity and normal pick cap. Open **Esc → Your Build** to inspect the catalog and your owned ranks.

- The first three chest choices mix damage, survival and a modifier when all categories still have eligible choices. A permanent reward-rank upgrade adds another choice. An available relic or style remains an additional choice.
- Later drafts give shared build tags up to twice their original weight. Off-build options remain possible. Early cooldown and attack-speed options receive extra weight.
- Full-health healing, unavailable cleanse/evolution/duplicate picks, capped traits and weapon-incompatible projectile modifiers are excluded.
- Start with one personal reroll per run. Loaded Dice grants another; rerolling changes only your unclaimed choices. The reward shop's first option is free; other options still cost 25 shared Cinders.
- Traits and Cinders are run-only. Permanent currencies, unlocks and Hearth upgrades continue using the existing version-independent save location and schema.

The list covers flat and percentage damage, attack speed, critical chance/power, weapon families, five status types, pierce/ricochet/split/extra shots, splash and chains, elite/boss/opening-hit bonuses, lifesteal and kill healing. Defensive choices include health, immediate healing, armor, post-hit protection, shield regeneration, evasion, retaliation, regeneration and a once-per-run lethal save. Movement choices include speed, longer/invulnerable dashes, charges, damaging dashes, slows, knockback, stun/root and pull/push effects. Economy and rare choices include extra kill currency/drop luck, rerolls, pickup healing, room-start buffs, evolution, relic discovery, duplication, cleanse, and a health-for-damage pact.

This game uses **Focus** for the requested ammo/mana-reserve and regeneration options; it does not add a separate ammunition or XP system. Cinders are the existing shared in-run gold equivalent. Vault Sense fulfills the trap/chest-sensing modifier. Afterimage lengthens damage invulnerability rather than introducing a new hitstun system.

## Three original adaptations

| Reference | Newly implemented Ashen Spiral feature |
|---|---|
| Crab Champions' optional stacking Gauntlet challenges | **Next-room wager:** each player opts in at the reward menu. Only unanimous consent enables the next tier, up to three. Each tier adds 20% enemy HP and 12% enemy damage, with +15 shared Cinders and +3 Ash per player on room completion. A declined wager returns the next room to normal; already earned rewards stay banked. |
| RuneScape: Dragonwilds' explorable vaults | **Two-sigil Rune Vault:** an optional side encounter every fourth room, beginning in room 2. Activate both sigils during an active wave while navigating flame and spinning-blade traps. This disables the vault traps and unlocks a once-per-party cache: 20 shared Cinders, plus 2 Ash and one ward material for each player. An unlocked cache can be collected after combat. |
| Ember Knights' weapon-modification and skill/relic build synergies | **Emberforge Evolution + Battle Conduit:** evolve an owned repeatable trait by one effective rank, and turn primary hits into Focus and Spiral recovery. The offer names the trait it will evolve. Two distinct traits may be evolved per run; one-time traits and dash-charge capacity cannot be evolved. |

These are adaptations with original code, names, meshes and balancing, not copies of those games' assets or exact systems. Reference material: [Crab Champions developer update](https://steamcommunity.com/app/774801/discussions/0/596285421871325618/), [Dragonwilds official vault update](https://dragonwilds.runescape.com/news/eye-on-ashenfall-0.12.0), [Ember Knights official store description](https://store.steampowered.com/app/1135230/Ember_Knights/?l=english).

## Rune Lance and Focus

Press **F** (controller **RT**) to cast Rune Lance: 48 base damage, 25 Focus cost, 1.2-second cast cooldown and 1,500 cm range. It starts with a 60-Focus pool and regenerates 5 Focus/second during combat or lobby practice. Reserve/recovery traits improve those values; Rune Nova converts the impact into a 300 cm area blast. Battle Conduit counts up to one primary hit every 0.15 seconds; every sixth hit restores 12 Focus and removes 0.3 seconds from Spiral's remaining cooldown.

Focus is shown above the dash/Spiral controls. Inter-wave Rune casts are cosmetic, do not spend Focus, and do not grant combat effects. Normal regeneration and on-hit/on-cast benefits are disabled between waves. The lobby dummy remains a practice target with no currency or kill rewards.

## Readability and controls

- Larger bright projectiles and short luminous trails make ranged shots easier to see. Ricochets redirect to an unhit visible enemy; pierces handle multiple targets on one long frame.
- Damage numbers are larger, outlined, colored by damage type, and stay for 1.25 seconds. Settings offers Large / Extra Large sizing. Critical hits add a larger pop, gold marks and a CRIT label. Burn is orange, poison green, frost cyan, bleed pink, shock yellow, physical white, Spiral violet, received damage red and shield loss blue.
- Health pickups use a **+ cross** mesh; shield pickups use a **shield** mesh; Cinder shards remain gold crystals.
- A downed partner turns red. The HUD adds a red cross, distance, screen-edge indicator and a joint-based red silhouette through walls. This is a HUD silhouette, not a post-process pixel outline.
- Dash charges display individually. Charges replenish sequentially; the existing limit of one air dash per landing remains. Longstride increases dash duration/distance and its protective window.
- **Settings → Borderless Fullscreen** fills the desktop in Standalone Game or a packaged build; Windowed returns to 1280×720. The command refuses to resize the editor's PIE viewport. No deliberate aspect-ratio stretching is used.
- Space jumps, Shift dashes, V revives, E interacts/advances, F casts Rune Lance. The controls page supports rebinding F as well.

### Escape in the editor

In Unreal, open **Edit → Editor Preferences → Keyboard Shortcuts**, search **Stop**, and change the **Play World (PIE/SIE) → Stop** binding from Escape to **Shift+Escape** (or clear it). This is an editor/user preference, not a game input binding. Escape will then be available to the game's menu without intentionally stopping PIE. Standalone Game also separates the game window from the editor play viewport. The project does not silently overwrite your editor shortcuts.

## Enemies, routes and hazards

| Enemy | Silhouette and behavior |
|---|---|
| Rift Leaper | Low four-legged crawler; crouched anticipation, pouncing contact attack, collision-checked optional gap jumps when they shorten the ground route. |
| Bulwark | Broad plated figure with shield and mace; slower movement, frontal 50% damage reduction, telegraphed charge/slam. Flank it. |
| Ember Shaman | Robed figure with tall flame staff and horns; warns before blasting each player's position. |
| Rift Sniper | Thin long-legged rifle silhouette; 1.5-second windup with a blue aiming line before a fast shot. |
| Broodcaller | Bulky green carrier with back pods; can summon two minions on each of its first two casts, then attacks normally; heals nearby non-boss allies for 8 HP. Summons respect the enemy cap. |

Enemy routing now looks ahead through capsule-clear, fully supported floor, and refreshes immediately on reaching a nearby waypoint. Gap approach stubs are represented as floor while the gap stays unsupported. Spawn points remain connected to the main traversable region. After sustained movement failure, recovery uses a later safe route node instead of repeatedly choosing the same stuck point. That fallback is intentionally a teleport recovery, not a claim that all collision issues are solved.

Leapers validate a sampled ballistic capsule arc before a gap jump. Other enemy types retain the ground route. Charges/pounces require a supported direct route so they do not intentionally attack across empty gaps. Imported geometry, actual CharacterMovement and network behavior still require live acceptance testing.

Flame traps use a six-second cycle: one-second warning, active from seconds 2–4. Blade traps warn before becoming active from seconds 1.5–4.5; damage follows the rotating blade strips rather than the whole disc. Low blades/spikes can be jumped over. Trap damage is disabled outside combat. Optional vault traps disarm after both sigils. Vault Sense reveals trap positions/timing and the cache on the HUD/minimap.

## Balance and safeguards

- Normal trait caps are 1–5 picks; evolved repeatable traits may reach one additional effective rank. Crit damage caps at 260%, combined damage reduction at 65%, and proc chances at 75%. Dash charges cap at three.
- Extra/split projectiles cannot create additional splits or trigger the new primary-hit proc chain. DOT, splash, chains, retaliation and orbit damage do not recursively trigger primary-hit effects. Direct extra projectiles can still apply their weapon's native element.
- Burn/poison/bleed durations extend up to 1.8×; hard crowd-control duration stays capped. Bosses resist stun/root/knockback and receive chill rather than freeze. Bleed caps at two stacks and poison at three.
- Lifesteal caps at 3 HP per 0.25 seconds. Chains, shock, splash, pulls and retaliation have internal cooldowns. Wisps cap at three; evolution increases total wisp damage without adding a fourth mesh.
- Projectiles, fields, pickups and enemy summons have population limits. Old pickup actors expire after 90 seconds.
- Blood Pact reduces current max HP once by 15% and grants +45% damage with one +10%-incoming-damage curse. Cleanse removes the curse, not the traded HP. Later health upgrades add their normal 15 HP to actual max health.
- Reward receipts queue if an earlier personal save is still waiting for acknowledgement, so a vault reward cannot overwrite a room-completion receipt. Reconnect snapshots include the new run traits, Focus, charges, wager vote and pending receipt queue.

Values are an initial bounded tuning pass, **not a claim of playtested balance or measured 60 FPS**.

## Focused local acceptance checklist

1. Fresh extraction: build/import with `Play_Standalone.bat`; footer must read 0.15.0. Check all 44 meshes import with expected dimensions. Keep the earlier folder until the new build runs.
2. Lobby: try all six weapons, Space/Shift, F/RT, and the dummy. Confirm the bow faces forward, melee weapons do not swing through the torso, projectiles are readable, Focus spends/regenerates, and dummy hits grant no rewards.
3. Rewards: inspect mixed categories, repeatable ranks, personal rerolls and late-build weighting. Verify weapon-only eligibility, full-health healing exclusion, evolution target, duplicate cap, cleanse and Blood Pact's health preview.
4. Combat: exercise every trait family, especially multiple projectiles + pierce + ricochet + split, elemental combinations, Conduit + Rune Nova, three dash charges, orbit damage, Phoenix once/run, and shield/health recovery. Test low frame rates and confirm no runaway proc loop.
5. Co-op: test each player choosing/starting the next room, one versus two wager votes, death/revive and silhouette behind a wall, downing during a dash, and disconnect/rejoin with an evolved trait and partially recharged charges. Verify client HUD and authoritative action agree.
6. Navigation: visit all map shapes/wards, dense waves, cluttered corners and gap links. Watch normal routes, leaper jumps, recovery fallback and each new attack windup. Check that enemies do not damage through walls.
7. Vault: in room 2 activate both sigils during combat, inspect flame/blade warnings, unlock the cache, and try collecting twice/from both players. Exactly one party cache reward should be credited. Test receipt retry/reconnect and trap disarming.
8. Inter-wave: continue firing, dashing and casting but verify no lifesteal, on-hit stacks, Focus regeneration, shield proc or kill buffs are banked. Repeat after entering the next wave to confirm normal effects resume.
9. Display: use 720p/1080p, borderless and windowed, large numbers and extra-large crits; inspect clipped menu text and enemy bars' 3–5 second fade. Ensure PIE resolution is unchanged by the borderless button.
10. Saves/performance: export a backup, run a second extraction with the same profile, verify permanent currency and unlocks, then benchmark dense waves on both host and client. The native tests do not establish real save I/O, runtime balance, latency or FPS.
