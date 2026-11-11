# Main-menu matchmaking

> **Status (2026-11-11):** the API contract below is current. The async client, its states and the testing section describe branch `worktree-matchmaking-ux`, not yet merged: on `main` the menu still uses the blocking `GET_now`/`POST_now`, and `Tools/mm_stub.py` and `-br-mm-endpoint` do not exist yet.

The client half lives in two files: `Scripts/Client/3_Game/BattleRoyale/Webhook/MatchMakingWebhook.c`
(requests, reply parsing) and `Scripts/Client/5_Mission/Mission/MainMenu.c` (the Play button). The
backend is the Vigrid API at `BATTLEROYALE_API_ENDPOINT` (`https://api.vigrid.ovh/`). Everything here
is optional: with the API down, the Play button behaves like plain PLAY.

## What the player sees

| State | Button | Status line | A click |
|---|---|---|---|
| `None` | PLAY | - | joins `-connect`; **with no `-connect`, opens the server browser** |
| `Checking` | PLAY | - | same as `None`, and drops the pending check |
| `Available` | MATCHMAKING | - | starts a search |
| `Searching` | SEARCHING... 0:12 | Click to cancel | cancels, back to MATCHMAKING |
| `Fallback` | CONNECTING... | the reason | joins `-connect` now, skipping the 2 s pause |
| `Connecting` | CONNECTING... (greyed) | - | ignored |

- The availability check runs once per menu build, and only when there is a `-connect` target.
- Opening a submenu (`OnHide`) cancels a search, so a match found meanwhile cannot pull the player out of Options.
- Coming back after a connect that did not stick (`OnShow`) hands the button back.
- `-connect=ip:port` is split. `-port` defaults to 2302.

**Nothing blocks.** Both requests are async (`RestContext.GET`/`POST` with a `MatchMakingRequest`
callback), and `MainMenu.Update()` polls the latched result. The old code used `GET_now`/`POST_now`.
That froze the whole client for up to the engine's 10 s + 10 s timeouts on every menu build, which is
what players saw whenever the API was down. The timeout is the menu's own watchdog
(`BR_MATCHMAKING_TIMEOUT_S`), not `RestApi.SetOption`, because that one is process-wide and would
change every other mod's REST calls too. A reply that arrives after a cancel or a timeout lands in a
request nobody holds any more, and is ignored.

## The API contract (v1)

This is what the client expects. A replacement backend has to answer exactly this, or bump `version`
and teach the client.

### `GET matches/matchmaking/<ip>:<port>?version=1&tick=<float>`

"Is this `-connect` server part of the Vigrid network?"

| Reply | Client |
|---|---|
| body `true` (trimmed) | shows MATCHMAKING |
| anything else, any HTTP error, or nothing within 6 s | stays on PLAY |

⚠️ **This route exists in neither `LeMyst/Vigrid-API` (full history) nor `LeMyst/Vigrid-Web` (current
tree)**, checked 2026-11-11. The client has called it since #64 (2025-02-07), after the API's last push, so the
MATCHMAKING button could never appear. The new backend has to implement it.

### `POST matches/matchmaking?version=1&tick=<float>`

The body is JSON, the client's whole `GetModInfos()` list:

```json
{ "mod_list": { "DZ": "1.30", "Vigrid-BattleRoyale": "0.2.1-Vigrid", "CF": "..." } }
```

| Reply | Client |
|---|---|
| `ip;port;password`, where the password may be empty or `null` | connects (`ConnectFromServerBrowser`) |
| a positive integer | retry in that many ms, clamped to 3–60 s |
| anything else (the old API sends `none`) | retry in 10 s |
| any HTTP error, or nothing within 6 s | **falls back to the `-connect` server**, showing *Matchmaking unavailable* |
| still no match once a retry would pass 180 s | falls back, showing *No match found* |

`tick` is a cache-buster, not data. The constants are in `BattleRoyaleConstants.c` (`BR_MATCHMAKING_*`).

### What the old backend gets wrong

`Matches::matchmaking` in `LeMyst/Vigrid-API` (`app/Controllers/Matches.php`):
- **It does not `return` after `echo "none"`.** It goes on into `array_rand([])`, which throws, so "no
  match" reaches the client as an HTTP 500, and the client falls back straight away instead of retrying.
- **A match is eligible while `started_at` is null, however old it is.** A server that crashed in its
  lobby attracts players forever. It needs a heartbeat or a `created_at` window.
- **Eligibility is an exact JSON match of the mod list.** Only `DZ` and `DayZ Frostline` are stripped,
  so any other DLC entry, or one version mismatch between the client and the server's registration
  (`CreateMatchWebhook`), means no match.
- **The reply carries the server password in plain text**, to any caller. That is inherent to this
  design, so it is worth knowing about before reusing it.

## Testing without the API

`Tools/mm_stub.py` is a stdlib stand-in for both routes. A **diag** client can be pointed at it with
`-br-mm-endpoint=http://127.0.0.1:8765/`, which is read in `MatchMakingWebhook.GetContext()` under
`#ifdef DIAG` and is absent from retail builds.

```bash
python Tools/mm_stub.py                      # available, "none" twice, then 127.0.0.1;2302;
python Tools/mm_stub.py --search error       # HTTP 500 -> fallback
python Tools/mm_stub.py --search hang        # watchdog -> fallback
python Tools/mm_stub.py --search none        # budget -> fallback after ~3 min
python Tools/mm_stub.py --available false    # plain PLAY
```

The flag has to reach `ClientLaunchParams`. From a worktree, the cleanest place is a worktree-local
`Workbench/user.cfg`: a copy of the primary's with one more `ClientLaunchParams=` line at the end,
since the file is last-wins. `-br-autoconnect` presses Play once the availability check has settled,
so a diag run goes through the search with no input. The stub's console then shows the requests, and
the client `script_*.log` shows the `[MatchMaking]` lines.

Cancel and the button's look can only be checked by eye.
