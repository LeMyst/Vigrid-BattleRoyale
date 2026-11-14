# In-place reset: the next match without a server restart

**Off by default.** With `in_place_reset` on, a match end starts the next match in the same server
process instead of `RequestExit`: the loot is expired and refilled, the match server is rebuilt, and
the lobby reopens within a second. Code: `Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleInPlaceReset.c`.
Without its native hook, or when anything goes wrong, the server restarts exactly as before.

## Requirements

- **The dzhooks `ce_refill` module**: serverkit `7c13ada` or later, built with `scriptdefine` and
  `ce_refill`, and `DZHOOK_CE_REFILL=1` in the server's environment. It is what refills the loot
  (serverkit [#29](https://github.com/LeMyst/dayz-serverkit/issues/29)). The mod compiles its calls
  only when the hook injects `DZHOOK_CE_REFILL`, so a server without it builds and restarts as before.
- **`CleanupLifetimeLimit` 5000** in the mission's `db/globals.xml`. At the stock 50 the old loot takes
  over an hour to drain, and the next match starts on a mostly un-drained map.
- **`use_autolock` is fine.** The old `AutoLockWebhook` had no unlock and blocked the reset; the stats API's `servers/rcon` unlocks again when the new `BattleRoyaleServer` starts.

## Settings (`server_settings.json`, version 6)

| Key | Default | What it does |
|---|---|---|
| `in_place_reset` | `false` | Turns the feature on |
| `in_place_reset_kick_all` | `false` | `false`: players left at match end stay connected and get a fresh lobby body. `true`: everyone is disconnected and the server reopens; on a full server, players waiting for a slot can get one |
| `in_place_reset_keep_dead_players` | `false` | `false`: as before, the death screen quits to the main menu after 15 s, which keeps the player count (and the server's load) down during the match. `true`: the death screen and the spectator camera wait for the next match instead. Ignored with `kick_all` |
| `in_place_reset_max_matches` | `10` | Restart the process anyway after this many in-place matches. A restart reloads the settings files and clears anything a reset cannot. `0` = never |
| `in_place_reset_drain_seconds` | `200` | Seconds between expiring the old loot and refilling. The refill also runs at the latest when the start countdown begins |

The settings files are read once per process: an edit takes effect at the next full restart.

## Sequence

```
8_Win      15 s win screen. Winners are recorded (RemovePlayer) and, unless kick_all, NOT kicked
9_Restart  leaderboard + last_match.json flushed, then BattleRoyaleInPlaceReset.Begin()
             - CannotRunReason() not empty -> today's 10 s RequestExit
frame +1   GetCEApi().TimeShift(1e9): every CE item expires; the drain starts
           kick_all: notice, 5 s, DisconnectPlayer(everyone), 2 s
frame +2   delete win and lobby chickens; reset the match statics; replace BattleRoyaleServer
           (its Init: new match id, new circles, new weather, unlock, lobby); then, in the same
           frame, for every connected identity: a fresh lobby body (CreatePlayer + SelectPlayer +
           EquipCharacter + InvokeOnConnect), the old body deleted, a MatchReset RPC with the new
           body's network id; finally every other player corpse is deleted with its gear
lobby      drain_seconds after the expiry: refill in four steps, one second apart
             CE_RefillLootTo(25), (50), (75), CE_RefillLoot()
countdown  the four steps again, without waiting for the drain: on a full map ~10 ms a step,
           and it fills whatever the drain removed after the lobby's pass
prepare    any step still pending runs before the teleport
```

Dead identities are disconnected at the reset when `keep_dead_players` is off. With two players the
second one's death ends the match, so they are on the death screen at the reset either way.

## Measurements (DayZ 1.30, ChernarusPlus, November 2026)

- **Drain**, `CleanupLifetimeLimit` 5000: about half of what is left goes per ~50 s CE cycle, 93% after
  ~190 s. At 50: 11% after 220 s.
- **Refill**: ~19,300 items in 8.4 s on DayZDiag and ~14,600 in 4.3 s on retail (the server freezes
  during each step, 1-3 s), back to 95-98% of the starting loot, evenly over the map. A repeat call
  on a full map creates nothing in ~10 ms.
- **Vehicles, infected, animals**: untouched. 43 vehicles and 170 animals on the map through expiry,
  drain and refill, 0 deleted.
- **CE idle mode**: with nobody connected, CE goes idle after `IdleModeCountdown` (60 s) even with
  `IdleModeStartup=0`, and the drain stops. The refill still works. In `kick_all` mode the drain
  therefore resumes only once someone reconnects.
- **Avoidance radii**: nothing within `CleanupAvoidance` (100 m) of a player is expired, and nothing
  is placed within `LootSpawnAvoidance` (100 m) of one. Players are in the lobby during a reset, so
  both only touch the lobby.

## Things that were one-match-per-process and are reset now

| What | Where |
|---|---|
| Announced circles, zone memo, starting player count | `BattleRoyaleState.ResetForNewMatch` |
| Circle chain and its parallel arrays, `s_Prepared` | `BattleRoyaleZone.ResetForNewMatch` |
| Deaths, spectators, audience, team colours, `m_Ended` | `BattleRoyaleSpectators.ResetForNewMatch` |
| Previous match (re-read from the fresh `last_match.json`) | `BattleRoyaleMatchStats.ResetForNewMatch` |
| Auto-grouped party members, invites, formation lock | `VigridPartyAPI.ResetForNewMatch` |
| Auto-run refused since the freeze (a match that ended before the unlock) | `VigridAutoRunAPI.SetAllowedForAll` |
| Admin overlay party colours | `BRMasterControlsModule.ResetPartyIndex` |
| Lobby and win chickens | `BattleRoyaleDebug.DeleteLobbyChickens`, `BattleRoyaleWin.DeleteWinChickens` |
| Player corpses and their gear | `BattleRoyaleCorpses` (fed from `PlayerBase.EEKilled`) |
| Lobby minimum wait (was process uptime) | `BattleRoyaleDebug.GetLobbyElapsed` |
| PAS panel batteries (refitted or recharged at each lobby opening) | `Land_Radio_PanelPAS.BR_RefillAll` |
| Queued state calls | `BattleRoyaleServer.CancelQueuedCalls` |

Client side (`BattleRoyaleClient.ResetForNewMatch`, `BattleRoyaleRPC.ResetForNewMatch`): the win
screen, the match-started latch and the voice cap, the edge trackers, the circles, the boards and the
last-match card, the loaded-in report (re-sent for the new body), the death-screen focus lock and
sound buses, the spectator camera (`BattleRoyaleSpectatorCamera.Retire`) and the party viewpoint.

## Tested (2026-11-07/08, diag rig, `LaunchLocalMP.bat 2` and `3`)

The rig cannot press F1, so the lobby ran with `use_ready_up 0` and `min_waiting_time 60`, and the
drain with `drain_seconds 30`. A throwaway diag-only driver (not committed) killed players during each
match. With two clients it killed the second player 40 s in, which ends the match with the dead player
still on the death screen. With three clients, auto-grouped by `min_party_size 2` into a pair and a
solo, it killed a player who still had a teammate at 15 s (they auto-enter spectate 20 s later), then
everyone outside the first player's group at 50 s. So the reset met a winner, a spectator and a
death-screen player at once. Every configuration was run:

| Configuration | Result |
|---|---|
| stay connected, keep dead players | 6 resets over 4 runs. Winners, the spectator and the death-screen player all get a lobby body in the reset's frame. The death screen closes and that player's input focus matches a living player's. The spectator runs `LeaveSpectate`, and its camera stops pushing its position. Match 3 played normally |
| parties | The reset removed the one auto-placed member (dissolving that pair) and unlocked formation; the next lobby auto-grouped again |
| `kick_all` | Winner kicked by the win state, the dead player by the reset; empty lobby reopened; refill ran on an empty server (10,040 items) |
| stay connected, dead players not kept | Winner kept, dead player disconnected |
| `max_matches 1` | The second match end logged `restarting the process: in_place_reset_max_matches (1) reached` and restarted |
| refill failure (forced -1, test-only switch) | `FAILED ... Restarting the process`, the notice reached the clients, and the server exited 10 s later |
| no `DZHOOK_CE_REFILL` | The guarded code compiled out with no script error; boot logged `requested but OFF: the dzhooks ce_refill module is not loaded` |

Two bugs found and fixed on the way:
- **RPC and body arrive in either order.** The client used to wait for "a body other than the one I
  had", but the MatchReset RPC landed 1.3 s after the new body had already been selected, so it
  waited for ever. The RPC now carries the new body's network id.
- **Too-early refill.** A lobby refill that ran before the drain finished left gaps once the
  remaining expired items were deleted. The countdown's second pass refilled them (10,157 items in
  one test).

### Retail (2026-11-08)

Run on retail `DayZServer_x64` 1.30.164.289, with the full serverkit catalog built from `7c13ada`
(12 modules active, `ce_refill` included). Myst played on a retail BattlEye client. One player cannot
start a match normally (the lobby needs two groups), so each match was started with COT's
match-control **Next**: alone, the player wins the moment the match starts. Two matches, two resets:

- **Resets:** each took 0.12-0.16 s from the match end to the lobby reopening with the player kept.
  The server unlocked at each reset and locked again at each countdown (the `serverlock` natives).
- **Server-browser name:** `[LOBBY 1]` → `[STARTING 0:10]` → `[STARTING]` → `[ENDING]` →
  `[LOBBY 1]`, through both matches.
- **Refill:** ~10,100 items per lobby pass, in steps of 0.6-1.2 s on retail (DayZDiag: 1.5-2.9 s).
- **On the client** Myst saw nothing wrong: the body, the HUD, the win screen going away, the notices
  and the refill freezes all looked fine.
- **No script errors** on the server.

To launch a test build, start `DayZ_BE.exe` with `@Vigrid-BattleRoyale` in `-mod`. The DayZ Launcher
picks the Workshop release `@DayZ-BattleRoyale`, which is an older build: it fails to compile without
Expansion (`expansionclientsettings.c: Unknown type 'ExpansionClientSettings'`).

Not covered: more than three players, and the death-screen and spectator paths on retail (one player
cannot die in a match that ends as it starts); both were run on the diag rig above.

## Known limits

- **`zone_generation_seed`** replays the same layout every match, as it replays it every boot.
- **`zone_selftest_runs` and `zone_ladder_selftest_players`** run once per process, at boot, and not again after a reset.
- **Uptime grows** across in-place matches; `in_place_reset_max_matches` keeps it bounded.
