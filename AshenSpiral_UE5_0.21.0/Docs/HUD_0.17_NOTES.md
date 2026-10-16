# HUD and control update 0.17.1

This is a complete UE 5.8.3 source package. Extract it into a new folder, close the editor, and run Play_Standalone.bat or Setup_and_Open.bat. Both co-op players need 0.17.1 / protocol 1700. The viewport subclass requires an editor restart; Live Coding alone does not install it into an existing viewport.

## Settings

Open Esc → Settings. All five existing camera controls remain available.

| Setting | Choices | Saved where |
|---|---|---|
| Crosshair shape | Cross, Dot, Ring, Cross + Dot | Local game-user settings |
| Crosshair color | White, Cyan, Lime, Yellow, Pink, Red, Blue, Orange | Local game-user settings |
| Crosshair size | 50–200%, steps of 10% | Local game-user settings |
| Damage-number size | 80%, 100%, 120%, 140%, 160% | Local game-user settings |
| Character color | Ember, Teal, Azure, Emerald, Violet, Rose, Silver, Crimson | Personal progress profile; replicated to the partner |

Reset Crosshair restores a white cross at 100%. Reticle position remains screen center, matching the existing aim ray. Size/color/shape change the display without changing projectile aim. Damage-number scaling defaults to 120%; an old 125% setting is snapped to 120%. The larger critical-hit multiplier remains, on top of the selected base size.

Outgoing damage uses three rising rows above the reticle; incoming damage uses a higher offset. Text clearance includes crosshair size and the critical-hit flash. Hit-type colors, dark outlines and CRIT labels remain. At most 64 recent popups are kept. The shared geometry is tested, but real font bounding boxes, dense combat readability and TV/Windows DPI presentation still need an engine check.

Character selection changes armor/cloth/glow accents and existing player-colored effects; neutral metal remains neutral. It does not change your player slot, pickups or spawn placement. Existing profiles keep their prior player-default appearance until a color is selected. Currency fields, save schema 1 and the version-independent progress folder are unchanged.

## Navigation and minimap

After the last wave in a room (normally wave 3, one wave for a boss), a rotating arrow and distance appear below the objective banner. It points toward the reward from your camera's yaw. It says CLAIM REWARD before your selection, NEXT ROOM after your claim, and HUB PORTAL on final victory. It indicates straight-line direction; follow connected floors around obstacles using the minimap.

| Map marker | Color |
|---|---|
| Enemies, elites, bosses and spawn warnings | Red |
| Traps revealed by Vault Sense | Purple; dim purple when disabled |
| Rune Vault | Green; muted after claiming |
| Reward/exit and dash-gap links | Gold |
| P1 | Cyan, with a dark outline |
| P2 | White, with a dark outline |

Gold elite outlines are removed. Training targets and dead enemies are excluded from enemy markers. Player identities are identical on both clients and independent of chosen armor color. A downed player's marker keeps its identity color and gains a cross. The old orange trap markers are replaced with purple; the supplied screenshot's repeated orange squares corresponded to sensed traps in the source.

## After room 32

Victory allows movement, jumping, dashing, shooting and casting until entering the return portal. Normal cooldowns still apply. A downed teammate is restored to at least 1 health. Post-clear casts remain cosmetic and cannot grant combat rewards or on-hit/on-cast benefits. Approach the teal portal and press E; either player can request the return once pending rewards are saved.

## Local acceptance checklist — not yet run

1. Build the full project with UE 5.8.3, restart, and verify the 0.17.1 footer.
2. Test each crosshair shape and color at 50%, 100% and 200%. Check a bright surface, a cave and a close wall; confirm aiming still matches the center.
3. Try all five number sizes on the practice target and in combat. Check normal/critical/DOT and incoming shield/health hits at 720p, 1080p and the TV's native output resolution, with both HUD text sizes.
4. Restart the game and check that HUD settings and character color persist. Confirm an older profile retains its balances and appearance until a color is selected.
5. Host and join using separate profiles. Change each player's character color and confirm the partner sees it. Check reconnect and return to the hub; verify player map colors remain fixed on both screens.
6. Clear a normal room's third wave. Turn the camera through 360 degrees, approach the reward and claim it on each client. Check that each client's label reflects its own claim, and P2 can advance when both are ready.
7. With Vault Sense, compare trap positions against purple markers, including disabled vault traps. Check red normal/elite/boss markers and the absence of old gold elite outlines.
8. Finish room 32, including a case with one partner downed. Both players should be able to run, jump, dash and fire before using the return portal. Verify saved permanent rewards and no extra temporary reward menu.
9. Run every mouse test in INPUT_CAPTURE_0.17.md. Retain the prior camera, achievement, save and co-op acceptance cases.

## Verification

The available native suites and source/art checks pass. They do not build the Unreal module or run Slate, Windows input, physics, save I/O or networking. This session has no connected Windows desktop and no Unreal installation. See VALIDATION.md for exact results and limits.
