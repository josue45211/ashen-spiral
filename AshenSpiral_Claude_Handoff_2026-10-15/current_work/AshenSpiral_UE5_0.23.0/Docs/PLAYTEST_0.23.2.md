# Ashen Spiral 0.23.2 - Playtest checklist (weekend merge + pizzazz)

INSTALL ON BOTH PCS. Both players must run this same build (new replicated state and RPCs were added; the network
protocol constant did not change, so a mismatched build will not warn you). Extract to a new folder, then run
`Play_Standalone.bat` (or `Setup_and_Open.bat` for the editor). Existing saves are untouched.

Nothing below has been run in the engine by the person who wrote it. Every item is UNVERIFIED. Builds were clean
(unity and non-unity) and `Tests/run_tests.py` passed, but those tests do not run the game. Please mark each line
PASS / FAIL / feels wrong, with a note.

## A. Fixes and weekend work list
1. [ ] **Cursor stays in the window in fullscreen** - open menus, pause (Esc), alt-tab, click at the edges; with a second monitor try to click outside. (windowed mode is intentionally still free)
2. [ ] **Lucky shop stall is invisible until a shop opens**, then appears *next to* (not on) the chest, solid, with its label; gone again after the checkpoint. Only usable while open.
3. [ ] **Lucky shop stock**: new "Trader's Pick" card (a trait rolled for your build, 150 Scrap). Prices are higher now (Dressing 60, Ward 55, Reroll 90 Scrap; Ember Core 30 and Heart 22 Ash). Do prices feel high-but-tempting? (all tunable)
4. [ ] **Hammer 3rd attack**: you can still move slowly (20% speed) during the finisher instead of freezing. The planted pose may look odd while sliding. Hammer leap still roots.
5. [ ] **Hammer swings no longer spawn gravity wells**. (Spiral ability wells and the Gravity Hook trait still exist.)
6. [ ] **Portal flow**: press E at an Ember / Ashen / Blood archway -> portal page (modifiers or lock reason, seed, ENTER). Host presses ENTER with all players ready and standing at the archway -> both walk in, "ENTERING THE SPIRAL" -> run starts. Try: guest pressing ENTER, someone not ready, locked Hard/Very Hard, the Endless portal (same flow), walking away before ENTER. The old hub-menu difficulty cards and Begin button are gone (hint text instead).

## B. Damage numbers (looks only, no labels)
7. [ ] **Crit**: oversized punch, brief freeze, shake, gold/orange halo and one ring. No "CRIT" word.
8. [ ] **Burn** (flickering embers, cools to dark red), **Poison** (slow wobble, drips down), **Frost** (snaps in, hangs still, shatters), **Bleed** (falls like a drop), **Shock** (jittery flicker), **Spiral** (orbits) - does each read as its element?
9. [ ] **Synergy hit** (Lightning Blast / Stormbreak / Wildfire, needs two players' elements): biggest numbers, slam-in, three rings, coloured by type. Synergy hits had NO number before. Plus the placeholder sound (S_Hit/S_Collapse/S_Spiral at different pitches).
10. [ ] Readability when many numbers are on screen at once.

## C. Pizzazz round (code-only, placeholder sounds/shapes)
11. [ ] **Wave banner**: band sweeps across with "WAVE n OF N" (title scales in fast); orange "ELITE WAVE"; red "BOSS"; "CHAMBER CLEARED" / "WAVE n CLEARED (CHECKPOINT CHEST)". Endless too.
12. [ ] **Slow-mo beat** on the final clear of a chamber / non-boss Endless wave (30% speed, ~0.45 s). WATCH FOR: slow-mo getting stuck, or the client not slowing down in co-op.
13. [ ] **Kill moments**: shard burst on every kill, tinted by the enemy's status; bigger on elites and bosses with a very short hit-stop (~35 / 50 ms). Any stutter or framerate dip when wiping a swarm?
14. [ ] **Pickups**: toss, bounce, hover; pulled to you when within ~5 m *if you need it* (health only when hurt, shield only when not full, cinders always); coloured flash + chime on collect. Check drops on uneven floor (cosmetic bounce has no ground collision).
15. [ ] **Boss intro**: letterbox bars, boss name slams in, small camera push-in during the 2 s spawn protection. Check each Ward's name and the Endless boss waves. (push-in is off under Reduce Motion)
16. [ ] **Portal entry juice**: FOV kick then fade to black during the walk-in.
17. [ ] **Low-HP vignette**: red frame, off at full health, pulses faster as health drops. Not shown in the hub.
18. [ ] **Chest fanfare**: chest swells when you claim, ring flash, floating "+X" text (cinders, health, upgrade name, Endless checkpoint Ash).
19. [ ] **Trait-acquire banner**: bigger "+ TRAIT NAME" banner with a placeholder sound when a trait is gained (chest choice, Trader's Pick, Ember Core). Shrine trades don't show it yet.
20. [ ] **Dash trail**: fading afterimages while dashing (also on the hammer leap). Do they look like you? Too many/few?
21. [ ] **Menu buttons**: hover swell, press squash, quiet hover tick. Check clipping at scroll-list edges.

## D. Known gaps (not in this build)
- Not started: death slow-mo + stats reveal; spawn telegraph juice; hammer slam juice; wave-50 transition juice.
- Astra port list not done: chests after wave 25 (cadence rules exist, unchecked against Astra), endless arena port, camera look/zoom, minimap, hammer RMB arc-stop fix, lighting after wave 20, camera shake.
- Trader's Pick offer is not saved: rejoining mid-shop hides that card until the next shop.
- Dash: P2's challenge to build a better dash is not done; this build only adds the trail.
- Bridge server (mission-control v3) allowlist is bypassable; decision pending with Josue.

## E. If something breaks
Run `Collect_Diagnostics.bat` and send the output. Nothing in this list can corrupt saves; the only profile-related change
is the Trader's Pick purchase, which uses the existing shop bitmask.
