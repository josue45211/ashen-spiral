# Two-player co-op

## Included network design

The host runs an Unreal listen server. Combat, wave progression, drops and reward decisions run on the server. Character movement uses Unreal movement replication with a custom saved-move dash. Clients use their own persistent profiles. This is a source implementation; no live networking test was possible in the creation environment.

## Two computers

Both players need version 0.22.0 and protocol 2200. With a source copy, each PC first completes setup. With a locally packaged build, share the entire packaged folder instead of the `.exe` alone.

0.21 adds replicated combo step/start/duration and authoritative finisher root-motion sources. Air-dash prediction/reconnect now store a numeric airborne count. Test multiple airborne charges and both melee finishers under latency, pause, obstruction and reconnect. See DESIGN_0.21.md. Native checks do not validate replication.

0.14 adds personal rerolls, unanimous next-room wager votes, shared vault state and queued personal reward receipts. New trait ranks, evolutions, Focus and dash charges are included in server reconnect snapshots. Test their client display, movement prediction and reconnection locally; no network execution was available during development. The downed-partner HUD now includes a red through-wall silhouette, cross and distance indicator.

In the hub, the host opens **Party**, chooses **Host**, and returns to the newly hosted hub. The Party page displays a session code. The other player enters the host's reachable LAN or trusted-VPN IP address and the session code, then joins. The configured UDP port is **24567**; the join field accepts `address:port`.

A session code identifies a party; it is not a cloud invite service or NAT traversal system. The source does not install VPN software, change firewall rules, forward router ports, or provide a relay. Use an existing approved LAN/trusted VPN. Any firewall change needs the computer owner's approval. Do not expose this development server to arbitrary public players.

Choose loadouts in the hub. Both players mark Ready, then the host starts. Each player must claim their own chest reward; **either player can press E at the chest to advance** once both selections and pending save acknowledgments are complete. P2 cannot skip a partner who has not selected. Starting the initial expedition and requesting an early return remain host actions. After final victory, either player may approach the return portal and press E for the whole party. Returning to the hub is a same-world reset, preserving the established session.

## Reviving a partner

Close the menu, stand within 3 meters of the downed partner with a clear path, and hold **V / left shoulder for 2 seconds**. The helper must be grounded. Releasing the key, moving out of range, or blocking the path resets the progress. Attacking, charging Spiral, dashing, or opening a menu cancels the channel. The progress bar appears for both players. Revival restores 45% maximum health and grants 2 seconds of protection.

Revival is allowed during combat/countdown and after the room is cleared. The timer runs on the downed pawn's server state. A downed player must be revived before claiming their chest reward; both players still choose independently. These changes need live testing with the host and client taking turns as helper. See `GAMEPLAY_0.12_NOTES.md`.

## Reconnect and pause

Esc requests a shared pause during combat/countdown; the pause indicator shows when simulation is frozen. Other hub and reward menus do not automatically pause the world. A departing client reserves its profile for 90 seconds while the host remains connected. Rejoin with the same profile ID, host IP and code. The host can continue alone rather than keep waiting.

The reconnect snapshot restores major run state: build, run stat upgrades, health, shield, relics, position, reward offers and pending receipt. It does not serialize every transient projectile, visual effect, or status timer. Some effects may expire or lose their departed owner. This is not a full save-anywhere system.

There is no host migration. When the host closes the server, the party disconnects. Already acknowledged profile rewards should remain in the client's local save; unacknowledged rewards can be lost if the host exits permanently. Profiles are trusted local data and can be modified outside the game; the project is not cheat-resistant or suitable for competitive progression.

## Current interactions, input and career saving

Either player can activate vault sigils within 3.2 meters during countdown, combat or reward. The cache is shared and awarded once. Shrine offerings are personal, once per chamber, and tracked across reconnect. Both peers use protocol 2200. Gameplay locks/captures the mouse only while the viewport has focus and no menu is open. Esc/Tab menus release both capture and locking so either PC can use another monitor. Closing a menu or returning to the viewport restores gameplay capture. F10 requests relock when gameplay has keyboard focus. See INPUT_CAPTURE_0.17.md for exact code and the test matrix. Test the actual two-PC case, including Alt-Tab, all window edges and closing menus. Character color is replicated separately from fixed cyan P1 / white P2 minimap markers.

Character levels replicate to the partner and are visible in the lobby. Pending XP/career counters join the reconnect snapshot, and queued receipts use the existing verified-save acknowledgment. Cinder conversion consumes the shared balance once and awards the same capped Ash to each player. Room advance and portal return wait for all pending saves; do not close the host while a write is pending.

## Local development test

`Test_Two_Players.bat` builds/imports, launches the host, and prompts before launching the client. The host session code is `LOCAL2`, with profile slots 90 and 91 to avoid duplicate profile IDs. Choose host/client windows carefully. Two game windows on one machine put both workloads on the same hardware and do not establish two-PC performance.

## Acceptance test to run locally

Test host and client jumping and air-dashing (including under latency), free lobby weapon trials, elemental DOT/freeze, P2 advancing only after both claims/save acknowledgments, all nine weapons and simultaneous wells; independent stat/relic/style chest selections; shared Cinders; personal save awards; purchase persistence after restarting each process; shared pause during a dash/projectile; revival in both Combat and Reward with each player as helper; return-to-hub party retention; disconnect and rejoin during Combat and Reward with run upgrades retained; save failure/retry; and host shutdown. Repeat under realistic latency and packet loss before treating co-op as verified. These are pending tests, not completed results.

Each player imports their own earlier Unreal progress in an offline hub before hosting/joining. Future copies on that PC use the same per-user save folder and profile slot. Never run two writable instances against the same slot; the local test helper already separates them.
