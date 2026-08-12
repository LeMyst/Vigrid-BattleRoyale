# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A DayZ Standalone mod (Bohemia Interactive Enfusion engine) implementing battle-royale gameplay, written in **EnfusionScript** (`.c` files). Originally by Kegan, maintained by Myst. Version `0.1.0-Vigrid`, defined in `Scripts/Client/2_GameLib/BattleRoyaleConstants.c` and mirrored in `mod.cpp`.

Hard dependencies (mod load order set in `Workbench/project.cfg`): [CF](https://github.com/Arkensor/DayZ-CommunityFramework), [Dabs Framework](https://github.com/InclementDab/DayZ-Dabs-Framework), [Community-Online-Tools](https://github.com/Jacob-Mango/DayZ-CommunityOnlineTools), [DayZ-Expansion](https://github.com/salutesh/DayZ-Expansion-Scripts). `Scripts/Client/config.cpp` requires `DayZExpansion_Scripts`. Each link is the mod's source repository — consult it for a dependency's API rather than guessing.

## Workflow

**Always work in a git worktree.** Before the first edit for any new feature or fix, call the `EnterWorktree` tool — it creates a branch plus a worktree under `.claude/worktrees/` and switches the session into it. Never commit directly to `main`.

- Skip it for read-only work: answering questions, searching, reading logs.
- Skip it if the session is already inside `.claude/worktrees/` — worktrees do not nest.
- **Building from a worktree needs an extra step.** `Deploy.bat` builds whatever `P:\Vigrid-BattleRoyale\` points at, which is the primary checkout (see `SetupMod.bat` below). To build worktree code, re-point that junction at the worktree, or merge back to the primary checkout first — otherwise the build silently packs the old sources. Building the primary checkout never picks up worktree sources: the build skips dot-prefixed folders (see `_EnumPaths.bat`), so `.claude/worktrees/` is invisible to it. Re-pointing the junction still works — `dir` reports paths through the junction, so no dot appears in them.
- Finish the work with a commit on the worktree branch; push and open a draft PR when the change should go to the remote.

## Build & run

Windows-only. Requires DayZ Tools (Steam) and a mounted `P:\` work drive.

**One-time setup:**
1. Copy `Workbench/user_sample.cfg` → `Workbench/user.cfg` and fill in `WorkDrive`, `ModBuildDirectory`, `KeyDirectory`, `KeyName`, `GameDirectory`, `ServerDirectory`, `SPMission`, `MPMission`, profile dirs, SteamIDs. `user.cfg` is gitignored and its values override `project.cfg`.
2. `Workbench/Batchfiles/SetupMod.bat` — creates the `P:\Vigrid-BattleRoyale\` junction into this checkout. The checkout folder name must equal `PrefixLinkRoot` (`Vigrid-BattleRoyale`).

**All batch files must be run with cwd = `Workbench/Batchfiles`** — several `call` siblings by relative path.

| Command | Purpose |
|---|---|
| `Deploy.bat` | Build. Thin wrapper over `CI.bat` → `CI_Build.bat` → `CI1.bat`. Binarize + rapify + pack + sign into `%ModBuildDirectory%@Vigrid-BattleRoyale\`. 2-5 min. |
| `Deploy.bat rebuildAll` | Wipe the output mod folder and rebuild everything. |
| `LaunchOffline.bat` | Single-player with `SPMission`. |
| `LaunchServer.bat` | Dedicated server (clears logs + `storage_*` first). |
| `LaunchLocalMP.bat [1\|2\|3]` | Server + N local clients. `LaunchLocalMP2.bat` / `MP3.bat` are wrappers for `2` / `3`. |
| `LaunchClient.bat [A\|B\|C]` | One client connecting to `127.0.0.1`. `LaunchClientB.bat` / `C.bat` are wrappers for `B` / `C`. |
| `KillGame.bat` | Kill all DayZ/Diag processes. |
| `ClearLogs.bat ["<dir>"]` | Clear **game** logs (`.rpt`, `.ADM`, `.mdmp`) from one profile dir, or all of them when called with no argument — not build logs. |
| `ClearStorage.bat` | Delete `storage_*` under `MPMission`. |

Build result: `Workbench/Logs/Build.log`, plus marker files `Build.success` / `Build.failure` / `Build.deploy`. `CI.bat` hard-fails if `P:\` is not mounted.

**Shared helpers** (`_`-prefixed, not entry points). Everything else calls these instead of re-implementing them:

| Helper | Role |
|---|---|
| `_Config.bat <tag>` | The **only** way config is loaded. Transliterates `project.cfg` then `user.cfg` into `Workbench/combined.cfg.<tag>.bat` (`set "K=V"` per line, `#` and `;` comments skipped) and calls it. `user.cfg` wins by load order. The `<tag>` keeps concurrent callers off each other's artifact. |
| `_Require.bat <Key>` | Echo a config value; non-zero exit if empty. |
| `_EnumPaths.bat <pattern> <outputFile>` | The **only** way the build enumerates sources. Recursive `dir /B /S` from the caller's cwd, minus every path containing a dot-prefixed folder (`.claude`, `.git`, `.idea`, …). Exits 0 when nothing matches. |
| `_SafeDir.bat <path>` | Gate in front of every recursive delete. Normalises the path and exports it as `%brSafeDir%`. Exit `0` safe, `1` unsafe — a drive root, UNC, bare `X:`, leading `\`, or an exact match on a system dir like `%USERPROFILE%` — `2` well-formed but absent (caller skips quietly). Used by `ClearLogs.bat` / `ClearStorage.bat`, whose paths come from the gitignored `user.cfg`. |
| `_LaunchClient.bat <A\|B\|C>` | Resolve the slot's `Player[B\|C]SteamID` / `Player[B\|C]Name` / `Client[B\|C]ProfileDirectory` and start it. |
| `_LaunchServer.bat <SteamID>` | Clear logs + storage, then start the dedicated server. |
| `SetupLaunch.bat <MP\|SP>` | Preamble for every `Launch*.bat`: config, validation, mod list, kill running game. Deliberately no `setlocal` — it exports into the caller. |

Each `config.cpp` with no ancestor `config.cpp` becomes one PBO — currently 8 mod PBOs (`Data`, `GUI`, `LanguageCore`, `Models_Shapes`, `Sounds`, `Scripts_Client`, `Scripts_Server`, `Party`) plus 13 `extra_*`. Renaming a top-level folder renames its PBO. PBO names are lowercased by the build, so `Extra/KillFeed` packs as `extra_killfeed.pbo`.

## Testing

**There are no tests, no linter, and no CI.** `.github/` contains only `copilot-instructions.md`.

Validation loop: `Deploy.bat` → launch (`ClientEXE` defaults to `DayZDiag_x64.exe`) → read the `.rpt` in the profile directory for script errors.

Runtime log verbosity (one at a time): `-br-warn`, `-br-info`, `-br-debug`, `-br-trace`, `-br-none`. On a server, `serverDZ.cfg` key `BRLogLevel` (1-4, negative disables) does the same. Diag builds default to trace via `#ifdef DIAG` → `BR_TRACE_ENABLED` in `BattleRoyaleConstants.c:10-20`.

Log with `BattleRoyaleUtils.Error/Warn/Info/Debug/Trace(string)` (`Scripts/Client/3_Game/BattleRoyaleUtils.c`) — not `Print`. On server+DIAG these mirror into in-game chat via the `ChatLog` RPC.

## EnfusionScript constraints

- **No ternary operator.** Use `if`/`else`.
- **No multi-line `if` conditions** — the whole condition must be on one line.
- **One declaration per variable name per method scope**, even across disjoint branches.
- `modded class X` extends an existing class; `override` is required to replace a method.

## Architecture

### Script modules and the Client/Server split

The engine compiles in fixed stages: `1_Core → 2_GameLib → 3_Game → 4_World → 5_Mission`. `config.cpp` maps a folder to a stage via `class defs { class <stage>ScriptModule { files[] = {...} } }`. Files are registered **by directory**, so adding a new `.c` file needs no config change; adding a new stage folder does.

**`Scripts/Client` vs `Scripts/Server` is not a runtime split.** Both PBOs ship in the same mod and load on both sides. What actually gates execution is the preprocessor guard on line 1 of each file:

- `Scripts/Server/**` — every file opens with `#ifdef SERVER`.
- `Scripts/Client/**` — `#ifndef SERVER` for client-only, **or no guard at all** for shared code that compiles on both sides (e.g. `BattleRoyaleConstants.c`, `BattleRoyaleUtils.c`, `BattleRoyaleBase.c`, `MissionBaseWorld.c`, `BattleRoyalePlayArea.c`).

When adding a file, pick the folder for organisation and **always add the right guard.** An unguarded file under `Scripts/Client/` runs on the server too.

`BattleRoyale_Scripts_Server` lists `BattleRoyale_Scripts_Client` in `requiredAddons`, so server files compile *after* client files in every stage — that ordering is what lets server code `modded class` over client-declared classes.

Compile-time feature flags live in `Scripts/Client/config.cpp:36-43` (`defines[]`). Only `DAYZ_BATTLEROYALE` is active; `BLUE_ZONE`, `BR_MINIMAP`, `MOVING_ZONE`, `BR_TRACE_ENABLED` are commented out but gate real code. Other `#ifdef`s in use: `SERVER`, `DIAG`, `VIGRID_PARTY` (the in-repo party addon, see below), `JM_COT`, `VPPADMINTOOLS`, `EXPANSIONMODMISSIONS`.

### Entry points

`MissionBaseWorld` (`Scripts/Client/4_World/MissionBaseWorld.c`) holds `ref BattleRoyaleBase m_BattleRoyale`, and the free function **`GetBR()`** in the same file is how any code reaches it on either side.

- Server: `BattleRoyaleServer` (`Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleServer.c`), constructed in `MissionServer.OnInit()`, ticked from `MissionServer.OnUpdate`. Also `BattleRoyaleServer.GetInstance()`.
- Client: `BattleRoyaleClient` (`Scripts/Client/5_Mission/BattleRoyale/Client/BattleRoyaleClient.c`), constructed in `MissionGameplay.OnInit()`, ticked from `MissionGameplay.OnUpdate`.

### Match state machine (server)

States are an **ordered `array<ref BattleRoyaleState>`** built in `BattleRoyaleServer.Init()` (`BattleRoyaleServer.c:88-133`). There is no state enum — the numeric filename prefix in `Scripts/Server/5_Mission/BattleRoyale/Server/States/` encodes the order:

`1_BattleRoyaleDebug` (lobby / ready-up) → `2_BattleRoyaleCountReached` (countdown, lock server) → `3_BattleRoyaleSpawnSelection` (only if `enable_spawn_selection_menu`) → `4_BattleRoyalePrepare` (loadout + teleport) → `5_BattleRoyaleStartMatch` (warmup) → `6_BattleRoyaleRound` × `num_zones` → `7_BattleRoyaleLastRound` → `8_BattleRoyaleWin` → `9_BattleRoyaleRestart` (`RequestExit`). A match is one-shot; the server process restarts between matches.

**The completion protocol is inverted.** `BattleRoyaleState.IsComplete()` returns `!IsActive()` (`0_BattleRoyaleState.c:85-92`), so a state signals "done" by calling **`Deactivate()`** on itself — typically from a timer callback, or from an overridden `IsComplete()` that tests its condition, calls `Deactivate()`, then `return super.IsComplete()`.

`BattleRoyaleServer.Update()` is the only driver: it `Update()`s every state each frame and checks `IsComplete()` at 10 Hz, then deactivates → migrates the player list → activates the next non-skipped state (`SkipState()` lets a state opt out entirely; rounds use it for the dynamic starting zone).

`BattleRoyaleState` extends `Timeable` (`Scripts/Server/3_Game/Logic/Timeable.c`) — use `AddTimer(duration, this, "MethodName", params, looping)`; looping timers are stopped automatically by `Deactivate()`.

Long-running work uses script coroutines — `GetGame().GameScript.Call(this, "MethodName", NULL)` plus `Sleep()` (see `4_BattleRoyalePrepare.ProcessPlayers`), not the call queue.

### Zones

`BattleRoyaleZone` (server, `BattleRoyaleZone.c`) is a static registry; all play areas are generated **once per process** into `static ref array<ref BattleRoyalePlayArea> m_PlayAreas`. **Zone 1 is the largest** and the last zone is the smallest — `BattleRoyaleZone.c:71` indexes with `i_NumRounds - GetZoneNumber()`. Radii come from `zone_settings.json` `static_sizes`; the other `shrink_type` values are declared but marked unused.

**Generation runs smallest first, and this is the single most surprising fact in the subsystem.** `m_PlayAreas[i]` is built from `static_sizes[i]`, so index 0 is the tight final circle and each later index is a bigger circle *containing* the one before it — the `i == 0` branch is the **last-played** zone, which is why `end_in_villages` and `restrict_final_zone` both live there. Consequently `static_sizes`, `static_timers` and `min_players` are all ordered smallest-zone-first, and `num_zones` selects that many tiers **from the small end**: lowering it shortens a match by dropping the *largest* circles while always keeping the tight endgame one. Entries past `num_zones` are unused by design (at the defaults, `static_sizes[6] = 4500` never plays); an array *shorter* than `num_zones` is a misconfiguration and is caught per-lookup. `Init()` logs the window in use.

Round duration comes from `static_timers` plus a per-circle offset in `s_PlayAreaDurationOffsets`, filled during generation when a circle lands far from its parent so players have time to cross. It is indexed exactly like the settings arrays and is a static parallel to `m_PlayAreas` — it must not become an instance field again, since the circles are shared by every zone object.

Within a round, the new circle only becomes the damaging boundary at 80% of the round timer (`LockNewZone`); before that `GetActiveZone()` returns the previous zone. Damage is applied per player from `PlayerBase.OnScheduledTick` → `BattleRoyaleServer.OnPlayerTick` → `GetCurrentState().OnPlayerTick`, scaled by zone index.

With dynamic starting zones the rounds ahead of the starting one are **skipped**, but they were still constructed and still hold a fully generated circle. `BattleRoyaleState.b_WasSkipped` (set by `BattleRoyaleServer.GetNextStateIndex()`) is what keeps that never-played circle out of the damage tick and off the wire. Note the two distinct meanings of "previous round" in `6_BattleRoyaleRound.c`: `GetChainedPreviousRound()` is the construction-time chain and is deliberately **not** skip-aware (it is how a round derives its own zone number), while `GetPreviousZone()` is skip-aware and answers "the circle actually in play".

`BattleRoyaleZone.OnActivate(array<PlayerBase>)` is an intentionally empty hook for player-count-driven zone sizing.

### Networking

Primary channel is **CF's `GetRPCManager()`** with string-named RPCs in two namespaces (`BattleRoyaleConstants.c:37-38`):

- `RPC_DAYZBR_NAMESPACE` (`"RPC-DayZBR"`) — server → client
- `RPC_DAYZBRSERVER_NAMESPACE` (`"RPC-DayZBR-Server"`) — client → server

```c
// register (ctor)
GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", this );

// handler signature is always this
void UpdateCurrentPlayArea(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)

// send: (namespace, name, params, guaranteed, [identity], [target])
GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowWinScreen", NULL, true, player.GetIdentity() );
```

**Match state is never sent as a state id.** The server pushes discrete facts (`StartMatch`, `SetPlayerCount`, `SetCountdownSeconds`, `UpdateCurrentPlayArea`, `UpdateFuturePlayArea`, `SetFade`, `SetInput`, `SetTopPosition`, …). Each lands as a plain field on the client singleton **`BattleRoyaleRPC`** (`Scripts/Client/3_Game/BattleRoyale/RPC/BattleRoyaleRPC.c`), and `BattleRoyaleClient.Update()` polls those fields every frame, diffing against `br_previous_*` locals to detect edges.

Two secondary channels: `ScriptRPC` with the `BattleRoyaleCOTStateMachineRPC` enum (`Scripts/Client/2_GameLib/BattleRoyaleEnums.c`) for the COT admin module, and sync juncture id `88` for teleports (`Scripts/Server/4_World/Entities/ManBase/PlayerBase.c`).

### Settings (JSON)

Server-side only (`Scripts/Server/3_Game/Config/`, all `#ifdef SERVER`). `BattleRoyaleConfig` is a singleton holding `map<string, ref BattleRoyaleDataBase>`; reach it with `BattleRoyaleConfig.GetConfig().GetGameData()` etc.

| File in `$profile:Vigrid-BattleRoyale\` | Class | Registry key |
|---|---|---|
| `general_settings.json` | `BattleRoyaleGameData` | `GameData` |
| `lobby_settings.json` | `BattleRoyaleLobbyData` | `DebugData` |
| `spawns_settings.json` | `BattleRoyaleSpawnsData` | `SpawnsData` |
| `zone_settings.json` | `BattleRoyaleZoneData` | `ZoneData` |
| `pois_settings.json` | `BattleRoyalePOIsData` | `POIsData` |
| `server_settings.json` | `BattleRoyaleServerData` | `ServerData` — **no mission override** |

The mission override (`$mission:Vigrid-BattleRoyale\`) is **not a merge** — `JsonFileLoader<T>.LoadFile()` is called twice into the same instance (`Load()` then `LoadMission()`), so only keys present in the mission JSON get overwritten. `Upgrade()` runs inside `Load()`, before the mission pass, and `Save()` only ever writes the profile path.

Each class carries `int version` plus an `Upgrade()` migration. `Load()` re-saves after reading, so new fields appear in existing profile JSONs on next boot.

### Parties (`Party/`)

Teams used to come from the third-party `@Carim` mod through 18 `#ifdef Carim` blocks. They now come from **`Party/`**, a top-level folder in this repo that builds into its own `party.pbo` (prefix `Vigrid-BattleRoyale\Party`) and defines `VIGRID_PARTY`.

**`Party/` must not reference any `BattleRoyale*` symbol.** That is why it carries its own logger (`VigridPartyLog`), its own JSON settings loader, its own `stringtable.csv` (`STR_PARTY_*` keys), its own `Data/Inputs.xml` and its own `$profile:Vigrid-Party\` folder. The rule keeps a later extraction into a standalone `@Vigrid-Party` mod a build-plumbing job rather than a rewrite, and it keeps renaming `Party/config.cpp` → `config.cpp.disabled` working as a one-rename kill switch.

The Battle Royale mod talks to it **only** through `VigridPartyAPI` (`Party/Scripts/4_World/VigridPartyAPI.c`), and every call site is wrapped in `#ifdef VIGRID_PARTY` so the mod still builds with the addon disabled:

```c
#ifdef VIGRID_PARTY
    int groups = VigridPartyAPI.GetGroupCount( GetPlayers() );
#endif
```

- Grouping queries take the population explicitly (`GetGroupCount`, `GetGroups`, `GetTeammates`), so Party never needs to know about match state. `GetGroups()` is a **partition**: solo players come back as groups of one, which is what lets most call sites drop their non-party branch entirely.
- Identity is `PlayerIdentity.GetPlainId()` (SteamID64) throughout — never `GetPlayerId()`, a session index the engine reuses after a disconnect. Note `MissionServer.PlayerDisconnected` is handed `GetId()` (hashed), so the manager keeps a `GetId()` → `GetPlainId()` table.
- `VigridPartyAPI.SetFormationLocked(true)` is called from `1_BattleRoyaleDebug.Deactivate()`. While locked, every composition change is refused — otherwise a player leaving mid-match would raise the group count and stall the round-end condition.
- Files: `$profile:Vigrid-Party\party_settings.json` and `parties.json`. Parties are persisted because the server process restarts between matches.
- RPC namespaces `RPC-VigridParty` / `RPC-VigridParty-Server`, message names `VP_*`. CF's `AddRPC` dispatches by **method name**, so a handler method must be named exactly like its registered string.
- Keybinds live in `Party/Data/Inputs.xml`, declared by a second `inputs=` in `Party/config.cpp`: `UAVigridPartyMenu` (P), `UAVigridPartyPing` (T), `UAVigridPartyPingClear` (Y). Read them with `GetUApi().GetInputByName(...)`, not the generated constants. **Y also toggles Community-Online-Tools' sidebar** (`UACOTToggleButtons`, plain `kY` in COT's own `Inputs.xml`) — an admin pressing it gets both; rebind if that bites.

**Pings.** World markers, ported in spirit from Carim: T places one where you are looking (8 km camera raycast), Y clears all of yours. Server-authoritative, unlike Carim's client-owned list — `VP_PingAdd` carries a position and nothing else, and owner, timestamps and expiry are minted from `sender`. `VP_PingSet` pushes the party's whole set back down (owner uids, positions, **ms remaining** — never an absolute expiry, since the two clocks are unrelated), which makes it idempotent and removes the need for Carim's 60 s heartbeat. Four settings in `party_settings.json` (`version` 2): `ping_enabled`, `ping_max_per_player` (3, FIFO-evicts the owner's oldest), `ping_ttl_seconds` (30; **0 = permanent**, Carim's behaviour) and `ping_cooldown_ms`. Two deliberate choices worth not undoing: the handlers **skip `RejectIfUnavailable`**, because it also refuses while the formation is locked and the lock is on for the whole match — the only time markers are useful; and pings are **session-scoped**, never written to `parties.json`. No map integration — world markers only. Rendering is `VigridPartyPings`, sharing `VigridPartyScreen`'s projection and edge-clamp geometry with the name tags.

### Localization

`LanguageCore/stringtable.csv`, keys prefixed `STR_BR_`. The `Party/` addon carries its own `stringtable.csv` at its PBO root with `STR_PARTY_*` keys — the engine loads one per addon — so party strings do not go here. Three reference styles:
- Layouts: `text "#STR_BR_..."`
- Client script: `SetText("#STR_BR_...")`
- **Server script: the bare key with no `#`.** `MessagePlayerUntranslated()` / `MessagePlayersUntranslated()` (`0_BattleRoyaleState.c:200-242`) ship the key over the `NotificationMessage` RPC and the client localizes it in `BattleRoyaleRPC.NotificationMessage()`, which also substitutes the `READY_KEY` / `UNSTUCK_KEY` placeholders with live keybinds.

### UI

Layouts live in `GUI/layouts/`. The dominant pattern is imperative — `GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/....layout")` then `FindAnyWidget("Name")`; most layouts have no `scriptclass`. `SpawnSelectionMenu` is a `UIScriptedMenu` (`MENU_SPAWN_SELECTION = 75` in `Scripts/Client/3_Game/Constants.c`, instantiated in `MissionBase.CreateScriptedMenu`). The only declarative `scriptclass` binding is the COT `master_controls.layout` → `BRMasterControlsForm`.

Keybinds are declared in `Data/Inputs.xml` (`UADayZBRReadyUp` = F1, `UADayZBRUnstuck` = F2, `UADayZBRDebug` = F3), registered via `inputs = "Vigrid-BattleRoyale/Data/Inputs.xml"` in `Scripts/Client/config.cpp`.

### Kill feed (`Extra/KillFeed/`)

A standalone addon replacing the third-party `nulledkillfeed.pbo`. It builds into `extra_killfeed.pbo` and defines `KILLFEED`. It hooks vanilla `PlayerBase.EEKilled` itself, so it works on any DayZ server — Battle Royale is not required.

**Same discipline rule as `Party/`: nothing under `Extra/KillFeed/` may reference a `BattleRoyale*` symbol.** It carries its own logger (`KillFeedLog`), settings (`$profile:KillFeed\killfeed_settings.json`), `stringtable.csv` (`STR_KF_*`), layouts and RPC namespace (`RPC-KillFeed`). `KILLFEED_PREFIX` in `KillFeedConstants.c` is the single place the asset path appears.

The Battle Royale mod talks to it **only** through `KillFeedAPI` (`Extra/KillFeed/Scripts/4_World/KillFeedAPI.c`), five call sites, each wrapped in `#ifdef KILLFEED`:

```c
#ifdef KILLFEED
    KillFeedAPI.NoteEnvironmentalDamage( player, KillFeedCause.ZONE );
#endif
```

- `SetActive(bool)` — the feed is off in the lobby (`1_BattleRoyaleDebug`), on from `5_BattleRoyaleStartMatch`, off again at `8_BattleRoyaleWin`.
- `NoteEnvironmentalDamage(player, cause)` — called at both `DecreaseHealthCoef` zone-damage sites (`6_BattleRoyaleRound`, `7_BattleRoyaleLastRound`). Scripted damage reaches `EEKilled` with the victim as their own killer, so without this hint a zone death is indistinguishable from starvation. Hints expire after `KILLFEED_HINT_TTL_MS` and are consumed by the death that uses them.

A row renders the killer's weapon **with its attachments** by spawning a client-local `ECE_LOCAL` copy, re-attaching the accessory classnames the server sent, and handing it to an `ItemPreviewWidget` — the same mechanism as the vanilla quickbar. At most `KILLFEED_MAX_ROWS` preview entities exist at once; each is `Delete()`d when its row expires.

`KillFeedSuppress` (`Scripts/4_World/Server/`) turns off other mods' kill feeds so a death is not announced twice, gated on the `suppress_other_killfeeds` setting. Each block is behind that mod's own define — currently only `#ifdef EXPANSIONMODKILLFEED`, which clears `GetExpansionSettings().GetNotification().EnableKillFeed`. That is Expansion's own documented switch, checked at both of its hooks in its modded `PlayerBase`, so clearing it is complete; the change is **in memory only**, so the admin's `NotificationSettings.json` is untouched and removing this addon restores the previous behaviour. Applied from `MissionServer.OnInit` plus one re-apply 10s later, in case another mod loads its settings afterwards.

NulledKillfeed is *not* covered — it is an obfuscated third-party PBO with no API to call. Remove it from `Workbench/ExtraPBOs/` or zero every `"active"` in its own `$profile:KillFeed\Settings.json`.

Note `$profile:KillFeed\` is also used by NulledKillfeed (`Settings.json`). The filenames differ, so they coexist, but deleting the folder to remove one wipes the other.

### Safe zone / lobby truce (`Extra/SafeZone/`)

A standalone addon replacing DayZ Expansion's Safe Zone for the lobby. It builds into `extra_safezone.pbo` and defines `VIGRID_SAFEZONE`. It hooks vanilla `PlayerBase` and `WeaponManager` directly, so it works on any DayZ server — Battle Royale is not required.

**Same discipline rule as `Party/` and `Extra/KillFeed/`: nothing under `Extra/SafeZone/` may reference a `BattleRoyale*` symbol.** It carries its own logger (`VigridSafeZoneLog`, CLI flags `-safezone-*`, `serverDZ.cfg` key `SafeZoneLogLevel`). It ships no assets, no settings file, no stringtable and no RPC namespace.

While active it changes **exactly two things**, and this narrowness is the whole point:

- `WeaponManager.CanFire` returns false, so pulling the trigger does nothing — no shot, no round consumed, no noise.
- `PlayerBase.EEOnDamageCalculated` returns false for damage that another player inflicted, so the hit is discarded before it is applied.

Everything else is deliberately left alone. Weapon raise, ADS, melee swings, reloading and user actions all behave normally — players can still aim and still punch each other in the lobby. Falls, drowning, infected, animals and the mod's own scripted zone damage all still land, because the predicate `VigridSafeZone_IsPlayerInflicted()` resolves the damage source to a player (directly, or via `GetHierarchyRootPlayer()` for a held weapon, or by explosive type) and returns false for anything else — including the victim as their own source, which is how `DecreaseHealthCoef` surfaces.

This is why it is **not** a port of Expansion's safezone. Expansion calls `hic.OverrideRaise(true, false)`, which kills ADS for every item including melee, and hard-returns false from `DayZPlayerMeleeFightLogic_LightHeavy.HandleFightLogic`, which kills melee swings outright; its `EEOnDamageCalculated` cancels *all* damage, not just PvP. If Expansion's safezone is left enabled it stacks on top of this addon and those restrictions come back — set `"Enabled": 0` in the mission's `Expansion/Settings/SafeZoneSettings.json`.

State is global rather than geographic, so there is no zone module, no actor list and no per-tick point-in-shape test. The server owns one static flag and mirrors it onto each player as the netsync bool `m_VigridSafeZoneActive` — netsync rather than an RPC broadcast specifically so a player joining an already-running lobby is disarmed too; `OnConnect` / `OnReconnect` re-assert it.

The Battle Royale mod talks to it **only** through `VigridSafeZoneAPI` (`Extra/SafeZone/Scripts/4_World/VigridSafeZoneAPI.c`), two call sites, each wrapped in `#ifdef VIGRID_SAFEZONE`:

```c
#ifdef VIGRID_SAFEZONE
    VigridSafeZoneAPI.SetActive( true );
#endif
```

- On in `1_BattleRoyaleDebug.Activate()`, off in `5_BattleRoyaleStartMatch.HandleUnlock()` — the latter rather than `Activate()` because input stays locked through the warm-up countdown, so `HandleUnlock` is the first instant a player could actually shoot back. Defaults to off, so the PBO changes nothing on a server that never calls the API.

### Vigrid API / webhooks

`Scripts/Server/3_Game/BattleRoyale/Webhook/` — REST via `GetRestApi().GetRestContext(BATTLEROYALE_API_ENDPOINT)`. Every call site is gated on `BattleRoyaleConfig.GetConfig().GetServerData().enable_vigrid_api`; `use_autolock` is a separate, independent gate that is *not* covered by it. Each webhook pairs with a `RestCallback` subclass that retries from `i_TryLeft`.

Client-side matchmaking is `MatchMakingWebhook` (`Scripts/Client/3_Game/BattleRoyale/Webhook/`), used by the modded `MainMenu`.

## Asset paths

Script and JSON references use the PBO-relative form with forward slashes: `Vigrid-BattleRoyale/GUI/layouts/...`. `config.cpp` `samples[]` uses backslashes with a leading slash. Profile/mission paths use `$profile:` / `$mission:` with **escaped** backslashes (`"$profile:Vigrid-BattleRoyale\\"`). The prefix comes from `PrefixLinkRoot` in `Workbench/project.cfg`.

Note the imageset's internal name is `battleroyale_gui`, not the filename `dayzbr_gui` — hence `"set:battleroyale_gui image:..."`.

## Adding things

- **A gameplay phase** — subclass `BattleRoyaleState` in `States/` with the next numeric prefix, override `GetName`/`Activate`/`Deactivate`/`IsComplete`/`OnPlayerTick`, and insert it at the right index in `BattleRoyaleServer.Init()`.
- **A field in an existing settings file** — add the member to the data class, bump `version`, add an `Upgrade()` branch if migration is needed. Nothing else.
- **A whole new settings file** — new `BattleRoyaleDataBase` subclass + one `m_Configs.Insert()` in `BattleRoyaleConfig.Init()` (marked `//--- adding a new config? copy below`) + a typed accessor.
- **A server → client value** — field + handler + `AddRPC` in the `BattleRoyaleRPC` ctor + a line in its `Reset()`, then read it from `BattleRoyaleClient.Update()`.
- **A client → server command** — `AddRPC` in the `BattleRoyaleServer` ctor, or in a state's `Activate()` paired with `RemoveRPC` in `Deactivate()`.
- **A standalone tweak** — new folder under `Extra/` with its own `config.cpp` and `Scripts/<stage>/`; it becomes its own PBO. Rename `config.cpp` → `config.cpp.disabled` to exclude it from the build (see `Extra/DisableFogChernarusPlus/`).

## Notes

- `Extra/` holds 14 independent single-purpose sub-addons, each its own PBO. 13 are built; `Extra/DisableFogChernarusPlus/` is parked as `config.cpp.disabled` and produces no PBO. All are small script tweaks except `Extra/KillFeed/` and `Extra/SafeZone/`, self-contained addons documented under *Architecture → Kill feed* and *Architecture → Safe zone / lobby truce*. Each folder carries its own `README.md`, indexed by `Extra/README.md`.
- `Extra/RandomMenuGear/` re-dresses the main-menu intro character in a random outfit plus a slung rifle and a melee weapon, re-rolled on every menu show. It hooks vanilla `IntroSceneCharacter.CreateNewCharacterById` (creation, prev/next arrows) and `MainMenu.OnShow` (returning from a submenu — that path calls `OnChangeCharacter(false)` and never recreates the character). It is **not** a fix for the broken character save that makes the menu character render naked; it only decorates the spawned object. Gear is applied with `GameInventory.CreateAttachmentEx` and deliberately never written into `MenuDefaultCharacterData` — that map is serialized to the server on connect and saved locally, so writing to it would leak menu gear into the real spawn loadout. Same discipline rule as `Party/` and `Extra/KillFeed/`: no `BattleRoyale*` symbol may be referenced.
- Spectating is not implemented — `GUI/layouts/hud/spectator/player.layout` exists but nothing references it. Death shows a screen, then forces disconnect.
- `Workbench/version` (`0.8.100368`) is a DayZ build number read by nothing. The mod version is `BATTLEROYALE_VERSION`.
