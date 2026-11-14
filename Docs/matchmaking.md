# Main-menu matchmaking

The client half lives in two files: `Scripts/Client/3_Game/BattleRoyale/Webhook/MatchMakingWebhook.c`
(requests, reply parsing) and `Scripts/Client/5_Mission/Mission/MainMenu.c` (the Play button). The
backend is the stats site's `/api/v2/matchmaking` (repo `LeMyst/br-hub`), reached at the compiled
`BATTLEROYALE_API_ENDPOINT`. Everything here is optional: with the API down, the Play button behaves
like plain PLAY.

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

**Nothing blocks.** Both requests are async (`RestContext.POST` with a `MatchMakingRequest`
callback), and `MainMenu.Update()` polls the latched result. The old code used `GET_now`/`POST_now`.
That froze the whole client for up to the engine's 10 s + 10 s timeouts on every menu build, which is
what players saw whenever the API was down. The timeout is the menu's own watchdog
(`BR_MATCHMAKING_TIMEOUT_S`), not `RestApi.SetOption`, because that one is process-wide and would
change every other mod's REST calls too. A reply that arrives after a cancel or a timeout lands in a
request nobody holds any more, and is ignored.

## The API contract (v2)

**br-hub's `CLAUDE.md` (*v2 API contract*, decision D18) is the source of truth; this is what the
client relies on.** A change the client needs goes in as a br-hub issue. Both calls are `POST
<endpoint>api/v2/matchmaking/<route>` with a JSON body, a JSON reply and no token. The site rate-limits
them to 60 a minute per client IP.

### `available`

Body `{"server": "ip:port"}`, the `-connect` target.

| Reply | Client |
|---|---|
| `{"available": true}` | shows MATCHMAKING |
| `false`, an unreadable body, any HTTP error, or nothing within 6 s | stays on PLAY |

`true` means the server is verified on the site and has matchmaking turned on.

### `search`

Body `{"server": "ip:port", "mods": {"<name>": "<version>", ...}}`. `mods` is `GetModInfos()` minus
the entries with `GetDefault()` or `GetIsDLC()` set, the same filter the server's match registration
uses. The site ignores vanilla and DLC names too.

| Reply | Client |
|---|---|
| `{"status": "found", "ip", "port", "password"}` | connects (`ConnectFromServerBrowser`); a `null` password is read as empty |
| `{"status": "wait", "retry_ms"}` | retries after `retry_ms`, clamped to 3-60 s; 10 s if it is missing |
| a 404 (the `-connect` server is not enabled), any other HTTP error, an unreadable body, or nothing within 6 s | **falls back to the `-connect` server**, showing *Matchmaking unavailable* |
| still no match once a retry would pass 180 s | falls back, showing *No match found* |

The site only offers verified, matchmaking-enabled servers in **the same shard** as the `-connect`
server, with a heartbeat under 90 s old, in state `lobby` with an open match, whose mods the client
carries at the same version. It picks the fullest lobby. The constants are in
`BattleRoyaleConstants.c` (`BR_MATCHMAKING_*`).

`BATTLEROYALE_API_ENDPOINT` is `https://vigrid.ovh/`, where br-hub runs. The old PHP API at
`api.vigrid.ovh` serves no v2. The client has no settings file, so the URL can only change in a build.

### Why v1 was replaced

v1 was `GET matches/matchmaking/<ip>:<port>` (reply `true`) and `POST matches/matchmaking` (reply
`ip;port;password`, a number, or `none`), served by `Matches::matchmaking` in the archived
`LeMyst/Vigrid-API`:
- **The availability GET existed in neither `Vigrid-API` nor `Vigrid-Web`**, so the MATCHMAKING button
  could never appear.
- **It did not `return` after `echo "none"`.** It went on into `array_rand([])`, which throws, so "no
  match" reached the client as an HTTP 500 and the client gave up instead of retrying.
- **A match was eligible while `started_at` was null, however old it was.** A server that crashed in
  its lobby attracted players forever. v2 requires a heartbeat under 90 s old.
- **Eligibility was an exact JSON match of the mod list**, with only `DZ` and `DayZ Frostline` stripped.
  v2 asks only that the client carries every server mod at the same version.
- **The reply carries the server password in plain text**, to any caller. v2 still does, by design.

## Testing

A **diag** client can be pointed at another endpoint with `-br-mm-endpoint=http://host:port/`, read in
`MatchMakingWebhook.GetContext()` under `#ifdef DIAG` and absent from retail builds.

- **The happy path: a local br-hub** (`http://127.0.0.1:8765/`). The local diag server has to be
  registered, verified and matchmaking-enabled on it, and sending heartbeats (mod PR #423).
- **The failure modes: `Tools/mm_stub.py`**, a stdlib stand-in on `127.0.0.1:8766`:

```bash
python Tools/mm_stub.py                      # available, "wait" twice, then found 127.0.0.1:2302
python Tools/mm_stub.py --search refused     # HTTP 404 -> fallback
python Tools/mm_stub.py --search error       # HTTP 500 -> fallback
python Tools/mm_stub.py --search garbage     # unreadable body -> fallback
python Tools/mm_stub.py --search hang        # watchdog -> fallback
python Tools/mm_stub.py --search wait        # budget -> fallback after ~3 min
python Tools/mm_stub.py --available false    # plain PLAY
```

The flag has to reach `ClientLaunchParams`. From a worktree, the cleanest place is a worktree-local
`Workbench/user.cfg`: a copy of the primary's with one more `ClientLaunchParams=` line at the end,
since the file is last-wins. `-br-autoconnect` presses Play once the availability check has settled,
so a diag run goes through the search with no input. The stub's console then shows the requests, and
the client `script_*.log` shows the `[MatchMaking]` lines.

Cancel and the button's look can only be checked by eye.
