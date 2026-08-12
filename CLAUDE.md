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

Each `config.cpp` with no ancestor `config.cpp` becomes one PBO — currently 8 mod PBOs (`Data`, `GUI`, `LanguageCore`, `Models_Shapes`, `Sounds`, `Scripts_Client`, `Scripts_Server`, `Party`) plus 15 `extra_*`. Renaming a top-level folder renames its PBO. PBO names are lowercased by the build, so `Extra/KillFeed` packs as `extra_killfeed.pbo`.

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

### Late joiners

`MissionServer.InvokeOnConnect` → `BattleRoyaleServer.OnPlayerConnected` is the **only** way the mod learns a player exists; nothing overrides `OnClientNewEvent` / `OnClientReadyEvent`. A player who arrives after the lobby ended is spawned at the lobby centre, gets the lobby loadout (`MissionServer.EquipCharacter` is unconditional), and is then **never added to any state** — so they hold no zone, take no damage, miss the `SetInput` broadcast and do not count towards the player count. There is no spectator mode to put them in, so they are disconnected after `late_join_kick_seconds` (`general_settings.json`, default 15).

**The grace period does not start at `InvokeOnConnect`, and must not.** Vanilla calls it from *two* places (`missionserver.c:326` and `:346`): the `ClientReady` path runs after `SelectPlayer`, but the **`ClientNew` path fires the moment the character is created, while the client is still loading the world** — and for a brand-new character `ClientReadyEventTypeID` never fires at all. Measured 2026-08-10 over a full run: `ClientPrepare` → `ClientNew` alone took 20 s, and no ready event was logged once. A countdown started at connect is therefore mostly loading screen: the player never sees the notification, and the disconnect lands on a half-established connection — the failure the original code's comment was worried about. So `ScheduleLateJoinKick` only *records* the joiner; `ArmLateJoiner` starts the clock, driven by the client→server `PlayerLoadedIn` RPC that `BattleRoyaleClient.SendLoadedInOnce` fires once `IsPlayerSelected()` is true. `BR_LATE_JOIN_READY_TIMEOUT_SECONDS` is only a backstop for a client that never reports.

Five things about that path are non-obvious enough to have each caused a bug:

- **The guard casts to `BattleRoyaleDebugState`, not `BattleRoyaleDebug`.** `BattleRoyaleCountReached` derives from it too, so joining during the pre-match countdown is accepted and added normally. Rejection begins at the state after that. This is deliberate.
- **`Error()` unwinds the stack, so nothing after it runs.** The global `Error(err)` is `Error2("", err)`, same as `BattleRoyaleUtils.Error`. The original kick was `m_Timer.Run(...)` on the line *after* an `Error(...)` and therefore never executed once. Never put recoverable work below an `Error` call.
- **`GetGame().SendLogoutTime(player, 0)` does not kick anyone.** Client-side it reaches `MissionGameplay.StartLogoutMenu`, whose whole body is guarded on an `m_Logout` that only exists once the player has opened Esc → Exit themselves. It is a *display* call for a logout already in progress, not a way to start one. Use `GetGame().DisconnectPlayer(identity)` — but not before the client has finished connecting, which is why the kick is deferred at all (`BR_LATE_JOIN_KICK_MIN_SECONDS`).
- **The admin exemption has to be checked in two places.** `admins_steamid64` members are allowed to stay, but an exempt admin is by construction in no state, so `OnPlayerTick`'s not-in-state branch would re-evict them. Both entry points funnel through `ScheduleLateJoinKick`, which consults `a_LateJoinExempt`; `IsLateJoinExempt` matches the cached `PlayerBase.player_steamid` first, since `PlayerIdentity` may already be gone.

**Nothing leashes a late joiner** — the lobby clamp is in `BattleRoyaleDebugState.OnPlayerTick`, which only runs for state members, so they were always free to walk out. What they could not do was get anywhere, since `OnPlayerConnected` dropped everyone at the lobby centre. An **admin** joining mid-match is therefore placed at the circle currently in play instead (`GetAdminJoinPosition`), falling back to the lobby when no circle is live yet. That helper needs two casts, because `GetActiveZone()` is not on the base state: `BattleRoyaleRound` has it and `BattleRoyaleLastRound` is a *sibling* of Round exposing `GetPreviousZone()`. Both answer the skip-aware "circle actually in play", which is the one to stand in.

Pending kicks live in `a_LateJoiners` and are swept from the 10 Hz block in `BattleRoyaleServer.Update()` — **not** an engine `Timer`. One shared `ref Timer` used to serve every joiner, so a second one inside the window silently replaced the first.

### Match state machine (server)

States are an **ordered `array<ref BattleRoyaleState>`** built in `BattleRoyaleServer.Init()` (`BattleRoyaleServer.c:88-133`). There is no state enum — the numeric filename prefix in `Scripts/Server/5_Mission/BattleRoyale/Server/States/` encodes the order:

`1_BattleRoyaleDebug` (lobby / ready-up) → `2_BattleRoyaleCountReached` (countdown, lock server) → `3_BattleRoyaleSpawnSelection` (only if `enable_spawn_selection_menu`) → `4_BattleRoyalePrepare` (loadout + teleport) → `5_BattleRoyaleStartMatch` (warmup) → `6_BattleRoyaleRound` × `num_zones` → `7_BattleRoyaleLastRound` → `8_BattleRoyaleWin` → `9_BattleRoyaleRestart` (`RequestExit`). A match is one-shot; the server process restarts between matches.

**The completion protocol is inverted.** `BattleRoyaleState.IsComplete()` returns `!IsActive()` (`0_BattleRoyaleState.c:85-92`), so a state signals "done" by calling **`Deactivate()`** on itself — typically from a timer callback, or from an overridden `IsComplete()` that tests its condition, calls `Deactivate()`, then `return super.IsComplete()`.

`BattleRoyaleServer.Update()` is the only driver: it `Update()`s every state each frame and checks `IsComplete()` at 10 Hz, then deactivates → migrates the player list → activates the next non-skipped state (`SkipState()` lets a state opt out entirely; rounds use it for the dynamic starting zone).

`BattleRoyaleState` extends `Timeable` (`Scripts/Server/3_Game/Logic/Timeable.c`) — use `AddTimer(duration, this, "MethodName", params, looping)`; looping timers are stopped automatically by `Deactivate()`.

Long-running work uses script coroutines — `GetGame().GameScript.Call(this, "MethodName", NULL)` plus `Sleep()` (see `4_BattleRoyalePrepare.ProcessPlayers`), not the call queue.

### Teleports and stuck players

**A player wedged in geometry has their inventory locked, and that is why they used to spawn naked.** Vanilla `PlayerBase.OnCommandFallStart` / `ClimbStart` / `LadderStart` / `SwimStart` each take a `LOCK_FROM_SCRIPT` lock on the character's inventory and only release it in the matching `...Finish` (`playerbase.c:3964-4085`). A player stuck mid-jump never finishes the command, so the lock is still held when `4_BattleRoyalePrepare` dresses everyone — and **a locked inventory refuses `CreateAttachment` silently, returning NULL**. `LocalDestroyEntity` is not an inventory move and ignores the lock, so the lobby clothes still went away: the symptom is "stripped and never re-dressed", not "still in lobby clothes". `ClearStuckMovementState` ends the command and drains any leaked lock count before dressing, every creation result is now checked, and `ProcessPlayers` re-dresses anyone left with zero attachments after the teleport. **A ladder is the deterministic repro** — same lock, reproducible where wedging yourself in a prop is not.

**Never start a movement command outside `CommandHandler`.** Vanilla only ever does it there and always `return`s immediately after (`dayzplayerimplement.c:2366-2400`, four times in a row). Starting one from a juncture handler produces a player who *looks* right but whose movement input drives nothing until some real command transition resyncs them — one jump was the cure. `BR_NotifyTeleported` therefore only sets a flag; the overridden `CommandHandler` consumes it and issues the transition — a `Fall` if the controller reports airborne, otherwise an unconditional `Move`, which is that jump without the jump. `BattleRoyalePrepare` keeps `BR_ForceMoveCommandImmediate` because it needs the inventory unlock synchronously and cannot yield, which is safe only because input is disabled for that whole state.

Both teleports (match start and F2 unstuck) go through **one** sync juncture, `BR_SYNC_JUNCTURE_TELEPORT` (88). Its server half repositions; a **client** half is also required, in `Scripts/Client/4_World/.../PlayerBase.c`, because the server half is `#ifdef SERVER` and the client would otherwise keep predicting the old command. The client does receive the juncture — verified by instrumenting both sides.

**Known bug, deliberately left in (2026-08-09): an F2 unstuck taken *while on a ladder in the lobby* still arrives playing the ladder animation, pinned until the player jumps once.** Everything else is clean — the match-start teleport handles a laddered player correctly, position and animation both, and neither path hovers any more. The difference between the two is the lead: `4_BattleRoyalePrepare` already ended the command server-side in `ClearStuckMovementState` and has input disabled for the whole state, while F2 has only the juncture. **One hypothesis has since been tested and refuted.** The theory was that the client consumes the forced `Move` before the corrected position arrives, restarts Move while still at the ladder, and vanilla re-enters the ladder command. Re-issuing the `Move` every frame of the settle window for as long as `GetCommand_Ladder()` or `GetCommand_Climb()` was running changed **nothing at all** — not the symptom, not even its texture — and everything else stayed clean. Had the ladder command genuinely still been running, forcing Move at frame rate would have looked like *something*. So the command state after the teleport is most likely already correct and **what is stuck is the animation graph, not the command** — which is also why a jump cures it, a jump being a real animation transition. That experiment was reverted rather than kept.

The untried lead is `HumanCommandLadder.Exit()` / `CanExit()` (`P:\scripts\3_game\human.c:644-664`) — vanilla's own way off a ladder, which plays the exit transition instead of replacing the command underneath it. Note `Exit()` plausibly only works while still *at* the ladder, so it would have to run before the teleport rather than in the juncture, and `Unstuck()` already defers 1-3 s, which is where the sequencing would go. **Instrument before writing any of it**: log instance type, `GetCurrentCommandID()` and whether `GetCommand_Ladder()` is null on both sides at juncture receipt and for a few ticks after. That single measurement distinguishes "command stuck" from "graph stuck" outright, and one instrumented run has already settled this subsystem once where reasoning had failed twice. Three explanations have now been wrong here; do not make a fourth without data.

**The controller does not re-check its ground contact after a scripted `SetPosition`, so a teleport must never rely on the engine noticing a drop.** `BR_TELEPORT_DROP_HEIGHT` was briefly 1 m on the theory that a player dropped in would be airborne, fall, and land — and that the landing would reset the animation graph. `PhysicsIsFalling` read "not airborne" on both sides with the metre applied, which was known at the time and read too narrowly ("the Fall branch is not what fixes the unstuck" rather than "nothing converts this drop into a fall"). What shipped was a character hovering a metre up after *every* teleport, on both paths, until the player's first input dropped them. It is now `0.05` — a seating epsilon so the capsule does not start inside the surface, nothing more. Two things replace it: `CommandHandler` ends every teleport with a real command transition (above), and `BattleRoyaleDebugState.FindUnstuckPosition` runs its lobby-centre fallback through `IsSafeForTeleport` instead of taking a raw `SurfaceY`, which is the wedging the metre of clearance was really hiding. `BR_TELEPORT_SETTLE_SECONDS` (0.75 s) is why the airborne check is a window rather than one tick: on the client the juncture can arrive before the corrected position does.

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

| File in `$profile:Vigrid-BattleRoyale\` | Class | Registry key | Scope |
|---|---|---|---|
| `general_settings.json` | `BattleRoyaleGameData` | `GameData` | general match-flow settings not owned by another file below |
| `lobby_settings.json` | `BattleRoyaleLobbyData` | `LobbyData` | pre-match lobby flow: ready-up, autostart, spawn selection |
| `zone_settings.json` | `BattleRoyaleZoneData` | `ZoneData` | zone geometry, shrink timing, zone damage, shrink-notification timing |
| `voice_settings.json` | `BattleRoyaleVoiceData` | `VoiceData` | party-only voice while frozen, speaking-list panel |
| `spawns_settings.json` | `BattleRoyaleSpawnsData` | `SpawnsData` | lobby spawn point and match spawn placement |
| `pois_settings.json` | `BattleRoyalePOIsData` | `POIsData` | POI position overrides |
| `server_settings.json` | `BattleRoyaleServerData` | `ServerData` | Vigrid API/webhook + autolock infra — **no mission override** |
| `leaderboard_settings.json` | `BattleRoyaleLeaderboardData` | `LeaderboardData` | scoring curve + persistence knobs — **no mission override**, integrity-sensitive |

The mission override (`$mission:Vigrid-BattleRoyale\`) is **not a merge** — `JsonFileLoader<T>.LoadFile()` is called twice into the same instance (`Load()` then `LoadMission()`), so only keys present in the mission JSON get overwritten. `Upgrade()` runs inside `Load()`, before the mission pass, and `Save()` only ever writes the profile path.

A field can also be locked out of mission override *within* an otherwise-overridable file: `LoadMission()` snapshots it before the deserialize call and restores it right after. `BattleRoyaleGameData.admins_steamid64` is the example — general_settings.json supports mission overrides, but the admin list is a server-operator concern, not mission content, so it's exempted. Reach for the same idiom for any future field that needs this.

Each class carries `int version` plus an `Upgrade()` migration. `Load()` re-saves after reading, so new fields appear in existing profile JSONs on next boot. Moving a field to a different settings file is *not* treated as a migration — the field starts from its new class's default and the old key is left inert in the old file.

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
- **`PartyMissionGameplay.OnUpdate` is the same three-part shape as the map's**: `HandlePartyClose()` (returns `bool`, short-circuits) → a single `if (m_UIManager.GetMenu()) return;` gate → `HandlePingInput()` + `HandlePartyOpen()`. Esc is **polled here as `UAUIBack`**, not left to the engine — while any scripted menu is open `MissionGameplay.OnUpdate` never reaches its `Pause()` branch, so Esc over the menu is a dead key. The gate is "any menu at all", never a list of ids: the list it replaced named vanilla's `MENU_MAP` rather than the Vigrid one, so P/T/Y still fired under the Vigrid map, the inventory and the in-game Esc menu — and `EnterScriptedMenu` passed `GetMenu()` as the **parent**, which made whatever was open the party menu's parent and handed focus back to it on close. The parent is `NULL`. It stays id-agnostic on purpose: Party must not name `MENU_VIGRID_MAP`. Unlike the map, `VigridPartyMenu` keeps `UseKeyboard() == true` and so keeps vanilla's three-device `LockControls()` — the player is meant to be frozen while it is open, so none of the map's `Supress()` work applies here.

**`VigridPartyAPI` is two-sided.** The file carries no top-level guard; each half is `#ifdef`-ed inside the class body. The **server** half is the grouping API described above. The **client** half answers "what can this player currently see of their party" — roster (`GetMemberCount`, `GetMemberUid`, `GetMemberName`, `IsMemberVisible`, `GetMemberPosition`, `GetMemberColour`, `GetRosterSeq`, `IsStateStale`), pings (`GetPingCount`, `GetPingPos`, `GetPingOwnerUid`, `GetPingColour`) and the two position helpers (`FindLocalPlayer`, `ResolveBodyPos`) that `VigridPartyNametags` also uses. Three things to know:
- **The client readiness check is `IsClientReady()`, not a second `IsReady()`.** One name would compile — the guards are mutually exclusive — but the two mean different things, and an *unguarded* 4_World/5_Mission caller would compile against both and silently mean something different per side.
- `GetMemberPosition` returns `vector.Zero` for "no data", and **must not be asked for your own slot**: `ClientData.m_PlayerBaseList` excludes the local player, so self falls through to the interpolated push and lags. Draw yourself from `GetGame().GetPlayer()`.
- **Ping indices are compacted** — expired entries are filtered at read time, honoured locally to the frame, so the map and the world markers agree on when a ping disappears.

Slot colours live in `Party/Scripts/3_Game/VigridPartyPalette.c`, unguarded, so both a 4_World API and a 5_Mission renderer can reach them. `VigridPartyPings.ColourForSlot` is a one-line delegate to it. Alpha is baked into the ARGB value for two independent reasons: an `ImageWidget` ignores its parent's alpha, and a `CanvasWidget` has no alpha to inherit at all.

**Pings.** World markers, ported in spirit from Carim: T places one where you are looking (8 km camera raycast), Y clears all of yours. Server-authoritative, unlike Carim's client-owned list — `VP_PingAdd` carries a position and nothing else, and owner, timestamps and expiry are minted from `sender`. `VP_PingSet` pushes the party's whole set back down (owner uids, positions, **ms remaining** — never an absolute expiry, since the two clocks are unrelated), which makes it idempotent and removes the need for Carim's 60 s heartbeat. Four settings in `party_settings.json` (`version` 2): `ping_enabled`, `ping_max_per_player` (3, FIFO-evicts the owner's oldest), `ping_ttl_seconds` (30; **0 = permanent**, Carim's behaviour) and `ping_cooldown_ms`. Two deliberate choices worth not undoing: the handlers **skip `RejectIfUnavailable`**, because it also refuses while the formation is locked and the lock is on for the whole match — the only time markers are useful; and pings are **session-scoped**, never written to `parties.json`. Rendering is `VigridPartyPings`, sharing `VigridPartyScreen`'s projection and edge-clamp geometry with the name tags. Pings also appear on the in-game map (see *Map*), drawn read-only through the client API — Party itself still knows nothing about the map.

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

### Map (`Extra/Map/`)

A standalone in-game map replacing DayZ Expansion Navigation. Builds into `extra_map.pbo` and defines `VIGRID_MAP`. Hooks nothing of the host mod's, so it works on any DayZ server — Battle Royale is not required.

**Same discipline rule as `Party/`, `Extra/KillFeed/` and `Extra/SafeZone/`: nothing under `Extra/Map/` may reference a `BattleRoyale*` symbol.** It carries its own logger (`VigridMapLog`, CLI flags `-map-*`, `serverDZ.cfg` key `MapLogLevel`), settings (`$profile:Vigrid-Map\map_settings.json`), client prefs, `stringtable.csv` (`STR_MAP_*`), layouts, keybinds and RPC namespaces (`RPC-VigridMap` / `RPC-VigridMap-Server`, messages `VM_*`). `VIGRID_MAP_PREFIX` is the single place the asset path appears.

- **M** opens a fullscreen `UIScriptedMenu` (`MENU_VIGRID_MAP = 178`), **Esc** or a second **M** closes it. Pan and zoom are the engine's own — **never override `OnMouseWheel`** or native zoom dies. `ClampZoom()` holds the range each frame because there is no zoom event to hook.
- **Left-click** places your marker, **right-click** clears it. One per player, permanent, party-shared, and the placer is the only one who can remove it. Both click guards from `BRMapHandler` are load-bearing: without the moved-mouse test every pan drops a marker.
- **The map does not stop the player, and Esc has to be polled by hand.** `VigridMapMenu` declares `UseKeyboard() == false`, so `LockControls` takes only the mouse focus and movement keys still reach the game; vanilla `MissionGameplay` carries a branch written for exactly `!UseKeyboard() && UseMouse()` that disables the mouse as a *game* input, so panning and clicking cannot fire the weapon underneath. **Two vanilla-looking hooks are both wrong here, and each cost a build to find out.** `AddActiveInputRestriction(EInputRestrictors.MAP)` is vanilla's own "this player has a map open" restriction, but its entire body is `UAWalkRunForced.ForceEnable(true)` — it pins the player to walking speed. And the exclude group `{"map"}`, which is what Expansion's map menu uses, is nothing but `<include name="menu" />`, and `"menu"` includes `"movement"` — it takes WASD straight back off. (Expansion gets away with it because its map keyboard-locks anyway. The vanilla exclude groups live in **`P:\bin\specific.xml`**, not in any `inputs.xml`; that file is the reference when composing one.) **And an exclude group of your own is wrong too, which is the third and least obvious one.** Gameplay actions do have to be stopped — vanilla's mouse-disable for `!UseKeyboard` menus calls `Input.DisableKey`, which is only the low-level device, while fire, melee and user actions are read engine-side from the `UApi` binding, so a left click to place a marker also swung the weapon. But `AddActiveInputExcludes` and `RemoveActiveInputExcludes` **both** end in `GetUApi().UpdateControls()` ("call this on each change of exclusion"), which rebuilds the control state and drops the **held** state of every input including `UATurbo` — closing the map mid-sprint dumps the player out of sprint until Shift is re-pressed. That is inherent to adding or removing a group at all, not to its membership, and it is the same mechanism that walks a vanilla player when they open the inventory. `VigridMapMenu.SuppressGameplayInputs` calls `Supress()` on the individual inputs once per frame from `Update()` instead: nothing global, nothing to tear down, and safe to call from a menu `Update` whose ordering against `MissionGameplay.OnUpdate` is undefined, because `Supress` is forward-looking ("supress press event for next frame"). Never point it at a movement key — the rest of that doc line is "while not pressed ATM, **otherwise until release**". Esc is polled as `UAUIBack` in `MapMissionGameplay.HandleMapClose` because while any scripted menu is open `MissionGameplay.OnUpdate` never reaches its `Pause()` branch, so Esc is a dead key rather than a competing one; every vanilla menu answers this the same way. Opening is guarded on `m_UIManager.GetMenu()` being null rather than on a list of menu ids — the list drifted, and passing `GetMenu()` as the parent made whatever was open the map's parent menu.
- **N** toggles the HUD minimap, which is **opt-in — off by default**. Effective visibility is `VIGRID_MAP_MINIMAP && minimap_allowed && minimap_enabled`, and each switch can only opt further out than the one before it. The first is the **build's** — a define in `Extra/Map/config.cpp`; comment it out and `VigridMapMinimap`, its widgets and the N handler are compiled out entirely, leaving the fullscreen map untouched. The second is the admin's (pushed over `VM_Settings`, ships **on**), the third the player's, in `$profile:Vigrid-Map\map_client.json` (ships **off**). So out of the box the key works but nothing is shown until it is pressed; the layout is still built on first `Update` and just kept hidden, so toggling on later costs nothing. Changing the `minimap_enabled` initialiser only affects players with no prefs file yet — anyone who already toggled keeps their saved choice. Two things deliberately survive a minimap-less build: `minimap_allowed` on the wire and in `map_settings.json`, because a *client* build flag must not change the wire format, and the N entry in `Data/Inputs.xml`, because XML cannot be conditional — it still lists under Options → Controls doing nothing.

**Nothing can be drawn over a `MapWidget` with script-created widgets.** `CreateWidgets(path, parent)` returns a valid widget, `SetPos` puts it in the right place, it tracks pan and zoom — and it is never rendered, with no warning and no failed image load. The only overlay that works is a **`CanvasWidget` declared in the layout as a child of the MapWidget**, which is what `SpawnSelectionMenu`'s heat map already does. Canvas has only `DrawLine` and `Clear`, so there is no text on the map and every glyph is a fan of strokes. Four canvases in `map_menu.layout`, z-ordered by `priority`: `LineCanvas` 999, `ZoneCanvas` 1000, `MarkerCanvas` 1001, `TeamCanvas` 1002.

**Glyph vocabulary**, all in `VigridMapRender` and all sized in screen pixels rather than metres: zone rings and centre dots, a **ring with a cross** for a placed marker, a **hollow triangle** for a teammate, a lighter **diamond** for a party ping, an **axis-aligned plus** for you on the fullscreen map, and a **notched dart** for you on the minimap. The circle is spent twice already, so both live-position glyphs are straight-edged, and triangle-vs-diamond needs separating because they are the pair that could still collide at small size. That was originally done on three axes — vertex count, stroke weight, opacity — but the ping's 1 px stroke proved not reliably visible over satellite imagery at 12 px, so it was raised to match the triangle's 2 px. **Vertex count and opacity (0.75 vs 1.0) are now the only things telling them apart**, so `VIGRID_MAP_PING_ALPHA` must not be raised to 1.0 without giving the ping a different silhouette. If they ever do read alike, the tested fallback is a six-pointed asterisk for pings — three lines through a common centre, no enclosed area, unmistakable for any polygon.

Ping alpha has one more trap: on the **world** marker the crosshair-fade floor `VIGRID_PARTY_PING_CENTER_MIN_ALPHA` *multiplies* with `VIGRID_PARTY_PING_BASE_ALPHA`, so a floor that looks reasonable on its own can be far darker in practice — 0.25 × 0.75 left a ping at 0.19 alpha exactly when the player was looking straight at the thing they had just placed. It is 0.55 now.

The two "you" glyphs differ on purpose. The fullscreen map's plus does **not** rotate — a rotating "you" is harder to *find* on a big map — while the minimap's dart carries heading, since that is the whole reason to glance at it. Two things about the dart are load-bearing: its angle is the **camera** bearing, never `player.GetYawPitchRoll()` (body yaw snaps in steps and does not return to its start after a 360, so the arrow drifts), and it is **drawn rather than a rotated `ImageWidget`** — `icon_arrow` points *down* at rest, reads ambiguously at small size because both ends look like a point, and vanilla's dedicated `Marker_Arrow.edds` does not resolve from a mod PBO at all (silently — no `.rpt` error). The concave notch is what makes the direction unmistakable.

**The repaint gate is split, and that is not an optimisation detail.** Zones and markers are edge-triggered on `m_RenderDirty || transform_moved || watchdog_due`. Teammates have **no edge** — Party's roster sequence moves when the party changes shape, never when somebody walks — so `TeamCanvas` repaints on a 10 Hz clock instead. **The probe results must be assigned unconditionally**, outside the repaint branch: left inside it, a team-only frame leaves the probe stale, `transform_moved` latches true, and the static layers silently repaint at frame rate. Every canvas must `Clear()` before any early return, or the last frame burns in permanently.

The Battle Royale mod talks to it **only** through `VigridMapAPI` (`Scripts/4_World/VigridMapAPI.c`), each call site wrapped in `#ifdef VIGRID_MAP`:

```c
#ifdef VIGRID_MAP
    VigridMapAPI.SetZones( cur_center, cur_radius, next_center, next_radius );
#endif
```

- Client: `SetZones` / `ClearZones` — push, not pull, because the addon may not reach into `BattleRoyaleClient`. Called every frame from `BattleRoyaleClient.Update` (it diffs internally) and `ClearZones()` from the destructor.
- Server: `ClearAllMarkers()` from `1_BattleRoyaleDebug.Activate()`, beside the SafeZone call; `SetMarkersActive(bool)`.

Party is reached **only** through `VigridMapTeam` (`Scripts/4_World/VigridMapTeam.c`) — the addon's only `#ifdef VIGRID_PARTY` code, verified by grep. It is in 4_World rather than 5_Mission precisely so its server half can serve `VigridMapMarkerStore` too; every body has an `#else` returning an empty answer, so `Party/config.cpp` renamed to `.disabled` leaves a working map with teammates and pings simply absent.

Markers are server-authoritative: owner is always `sender.GetPlainId()`, the whole visible set is pushed as a **snapshot** (not deltas — tiny, and idempotent under packet loss), with a 5 s resync so joining a party mid-match works. A marker records its placer's **party slot at placement time**, and colour is resolved from that through `GetColorForSlot` — never through the live roster, which would turn the marker off-white the moment its owner disconnected.

**Satellite vs topographic map.** The satellite view was never the terrain's: it came from Expansion Navigation setting `maxSatelliteAlpha = 1.0`, where vanilla ships `0`. `Extra/MapSatellite/` restores it and **ships enabled**. It cannot be a runtime setting: `MapWidget` exposes no satellite API and the key appears nowhere in the vanilla scripts, so the `config.cpp` ↔ `config.cpp.disabled` rename is the whole control surface. It patches two **global, top-level** classes, both *outside* `CfgWorlds` — an override written against `CfgWorlds` is inert: **`MapDefaults`** (`P:\DZ\data\config.cpp:1717`) for the raster and vector layers, and **`CfgLocationTypes`** (`P:\DZ\gear\navigation\config.cpp:14`) for the place-name labels. Between them they cover every terrain and every `MapWidget`: the fullscreen map, both minimaps and the spawn-selection screen.

**Every value in that addon is derived from vanilla, not from Expansion** (re-derived 2026-08-09). Expansion's source is **CC BY-NC-ND 4.0**, whose *NoDerivatives* term is at odds with copying their config and altering values in it. Nothing was lost: what was wanted from them was a *fact* (where the shadow key lives) and a *technique* — "zero the alpha on the fill layers that fight a photo, keep the linear features that still read" — neither of which is anyone's expression. The rule every colour obeys, and the one to follow when adding one: **keep Bohemia's RGB, change only the alpha, and only where a layer occludes the imagery.** Anything vanilla already gets right is simply not restated.

**⚠️ If you redeclare an existing class, carry its parent — never declare a parentless `class RscMapControl`.** Doing that hard-froze every client on 2026-08-07 (proven by bisection 2026-08-08). Vanilla ships `class RscMapControl: MapDefaults {}`, and redeclaring it **without restating the parent replaces it rather than merge-patching it**, so the real map control kept satellite and lost everything else — including the `ptsPerSquare*` tessellation densities, which is how it wedged. The addon's header carries the full evidence, including the two hypotheses that were refuted by a build each (`maxUserMapAlpha = 1`, and the absent `scaleMin`/`scaleMax`/`scaleDefault` — both harmless). **This inheritance rule applies to every config override in the repo, not just this one.**

That rule is also why the `CfgLocationTypes` patch touches **only `Name` and `NameIcon`**: both are *parentless* in vanilla, so there is no parent to drop, and every other label class derives from one of them and inherits the change for free. Vanilla's per-tier sizes and its `importance` values (which gate what appears at which zoom) are left alone.

Two things only visible once satellite renders, and they need different fixes. Vanilla's overlay layers are tuned for a pale contour drawing and fight imagery (`colorForest` is bright green at α 0.5, `colorSea` is fully opaque), so fill layers that duplicate what the photo already shows go to α 0 or near it while linear features — roads, rails, power lines, the grid — stay at vanilla. And **place names needed a shadow, not a colour**: contrast comes from `shadow = 1` on `CfgLocationTypes::Name` plus the larger `MetronBook-Bold58` face (Metron is a bitmap font, so raising `textSize` on the 28px face just goes soft).

**Correction, 2026-08-09:** this file and the addon both used to claim `MapDefaults` exposes *no outline or shadow key* for place names, so contrast had to come from hue. **That was wrong.** `colorNames` / `sizeExNames` style grid and mountpoint labels; settlement names come from `CfgLocationTypes`, which has always had a `shadow` key. The amber was the right instinct aimed at the wrong key — which is why the names stayed unreadable no matter what was set there.

Note `alphaFadeStartScale`/`alphaFadeEndScale` are `2` (satellite at every zoom) rather than vanilla's `1`. Dropping to `1` would fade satellite out when zoomed out and fall back to the topo layer — but that layer's alphas have deliberately been dialled down, so it would fade to a near-blank map. The fade values and the colour block are a package. `scaleMin`/`scaleMax`/`scaleDefault` are **not** set at all — vanilla does not declare them, `VigridMapMenu.ClampZoom()` re-clamps every frame anyway, and their absence was proven harmless by one of the bisection builds.

The choice is purely a look, and the two layers are interchangeable as far as the addon is concerned: every overlay is a `CanvasWidget` child drawing in screen space over whatever the `MapWidget` renders, so none of the map work depends on which layer is underneath.

### Vigrid API / webhooks

`Scripts/Server/3_Game/BattleRoyale/Webhook/` — REST via `GetRestApi().GetRestContext(BATTLEROYALE_API_ENDPOINT)`. Every call site is gated on `BattleRoyaleConfig.GetConfig().GetServerData().enable_vigrid_api`; `use_autolock` is a separate, independent gate that is *not* covered by it. Each webhook pairs with a `RestCallback` subclass that retries from `i_TryLeft`.

Client-side matchmaking is `MatchMakingWebhook` (`Scripts/Client/3_Game/BattleRoyale/Webhook/`), used by the modded `MainMenu`.

## Asset paths

Script and JSON references use the PBO-relative form with forward slashes: `Vigrid-BattleRoyale/GUI/layouts/...`. `config.cpp` `samples[]` uses backslashes with a leading slash. Profile/mission paths use `$profile:` / `$mission:` with **escaped** backslashes (`"$profile:Vigrid-BattleRoyale\\"`). The prefix comes from `PrefixLinkRoot` in `Workbench/project.cfg`.

Note the imageset's internal name is `battleroyale_gui`, not the filename `dayzbr_gui` — hence `"set:battleroyale_gui image:..."`.

## Adding things

- **A gameplay phase** — subclass `BattleRoyaleState` in `States/` with the next numeric prefix, override `GetName`/`Activate`/`Deactivate`/`IsComplete`/`OnPlayerTick`, and insert it at the right index in `BattleRoyaleServer.Init()`.
- **A field in an existing settings file** — add the member to the data class, bump `version`, add an `Upgrade()` branch if migration is needed. Nothing else. **If the new field is a `ref array`, the `Upgrade()` branch is not optional:** a field initialiser survives deserialization for scalars but *not* for arrays, so on every server that already has the JSON the array loads back empty and the feature silently does nothing. Refill it in `Upgrade()` when it comes back empty — and only when empty, so an admin who deliberately cleared it keeps their choice. `BattleRoyaleGameData` and `BattleRoyaleServerData.placeholder_player_names` both do this.
- **A whole new settings file** — new `BattleRoyaleDataBase` subclass + one `m_Configs.Insert()` in `BattleRoyaleConfig.Init()` (marked `//--- adding a new config? copy below`) + a typed accessor.
- **A server → client value** — field + handler + `AddRPC` in the `BattleRoyaleRPC` ctor + a line in its `Reset()`, then read it from `BattleRoyaleClient.Update()`.
- **A client → server command** — `AddRPC` in the `BattleRoyaleServer` ctor, or in a state's `Activate()` paired with `RemoveRPC` in `Deactivate()`.
- **A standalone tweak** — new folder under `Extra/` with its own `config.cpp` and `Scripts/<stage>/`; it becomes its own PBO. Rename `config.cpp` → `config.cpp.disabled` to exclude it from the build (see `Extra/DisableFogChernarusPlus/`).

## Notes

- `Extra/` holds 16 independent single-purpose sub-addons, each its own PBO. 15 are built; `Extra/DisableFogChernarusPlus/` is parked as `config.cpp.disabled` and produces no PBO. Most are small script tweaks; the exceptions are `Extra/KillFeed/`, `Extra/SafeZone/` and `Extra/Map/`, self-contained addons documented under *Architecture → Kill feed*, *→ Safe zone / lobby truce* and *→ Map*. Each folder carries its own `README.md`, indexed by `Extra/README.md`.
- **An incremental `Deploy.bat` does not always delete the PBO of an addon you just disabled.** It cleans orphans only sometimes, so a `config.cpp` → `.disabled` rename can leave the previous PBO in `%ModBuildDirectory%` and the addon still loads — which silently invalidates a discipline negative-build. Check the output folder and delete the `.pbo` plus its `.bisign` by hand.
- `Extra/RandomMenuGear/` re-dresses the main-menu intro character in a random outfit plus a slung rifle and a melee weapon, re-rolled on every menu show. It hooks vanilla `IntroSceneCharacter.CreateNewCharacterById` (creation, prev/next arrows) and `MainMenu.OnShow` (returning from a submenu — that path calls `OnChangeCharacter(false)` and never recreates the character). It is **not** a fix for the broken character save that makes the menu character render naked; it only decorates the spawned object. Gear is applied with `GameInventory.CreateAttachmentEx` and deliberately never written into `MenuDefaultCharacterData` — that map is serialized to the server on connect and saved locally, so writing to it would leak menu gear into the real spawn loadout. Same discipline rule as `Party/` and `Extra/KillFeed/`: no `BattleRoyale*` symbol may be referenced.
- Spectating is not implemented — `GUI/layouts/hud/spectator/player.layout` exists but nothing references it. Death shows a screen, then forces disconnect.
- `Workbench/version` (`0.8.100368`) is a DayZ build number read by nothing. The mod version is `BATTLEROYALE_VERSION`.
