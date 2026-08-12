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

**Most of the hard-won DayZ knowledge here is written down three times** — in this file, in the
project memories, and in the global skills under `C:\Users\Myst\.claude\skills\` (`dayz-modding`,
`dayz-source`, `dayz-ui`, `dayz-multiplayer`, `slow-loop-debugging`). That is deliberate — the skill
carries the rule, the memory keeps the dated measurement and the retractions, this file keeps the
mod-specific detail — but it means **a correction has to be applied to every copy**. Several
findings here were overturned after they were first written up, and the wrong version outlived the
correction in a file nobody re-read. So when a memory-consolidation pass runs, or whenever a rule is
re-diagnosed, **grep `C:\Users\Myst\.claude\skills\` and this file too**, not just the memory
directory.

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

**Generation runs smallest first, and this is the single most surprising fact in the subsystem.** `m_PlayAreas[i]` is built from `static_sizes[i]`, so index 0 is the tight final circle and each later index is a bigger circle *containing* the one before it — the `i == 0` branch is the **last-played** zone, which is why `end_in_villages` and `restrict_final_zone` both live there. Consequently `static_sizes`, `static_timers` and `min_players` are all ordered smallest-zone-first, and `num_zones` selects that many tiers **from the small end**: lowering it shortens a match by dropping the *largest* circles while always keeping the tight endgame one. Entries past `num_zones` are unused by design (at the defaults, `static_sizes[6] = 4500` never plays); an array *shorter* than `num_zones` is a misconfiguration and is clamped by `Validate()` (below). `Init()` logs the window in use.

Round duration comes from `static_timers` plus a per-circle offset in `s_PlayAreaDurationOffsets`, filled by `CommitChain` when a circle lands far from its parent so players have time to cross. It is indexed exactly like the settings arrays and is a static parallel to `m_PlayAreas` — it must not become an instance field again, since the circles are shared by every zone object. It is computed from the *finished* chain rather than during placement, because a single scratch slot cannot survive backtracking: a re-rolled level's offset has to be discarded. **The feature was dead until 2026-08-11** — its threshold was 1500 m while the longest possible step at the shipped sizes is under 1000 m, so the array had always been all zeros. `BR_ZONE_OFFSET_MIN_DISTANCE` is 600 m now, capped by `BR_ZONE_OFFSET_MAX_SECONDS`.

**Generation cannot dead-end, and that is a property of the geometry rather than of a retry budget.** The world-fit boxes `[r_i, W - r_i]²` are nested and all share the map centre, so for nested convex sets containing a common point, stepping toward that point weakly decreases the distance to every one of them. "Step the maximum allowed straight at the map centre" is therefore the *provably optimal* continuation of a chain, not a heuristic — which makes `CanChainComplete` an exact oracle, in pure arithmetic with no native calls. Two things follow:

- Used as an **acceptance test on every candidate**, every circle it accepts is provably extendable.
- The first greedy step from any accepted position is itself always acceptable — the **witness step**. So every level has a move that cannot be rejected on geometric grounds.

`TryPlaceLevel` escalates through three tiers (arc, distance window and land requirement all loosening), then one deterministic 96-probe sweep, then the witness step. `BuildChain` backtracks to re-roll a parent when a level fails, and abandons a seed for a different village when one keeps costing placements — that is the "rewind" behaviour. Termination is a proof, not a budget: each level's counter rises monotonically while its parent is unchanged, and at `BR_ZONE_LEVEL_RETRIES` it takes the witness step.

**What this replaced:** a single forward pass that drew 500 random candidates per circle and, on running out, called `GetGame().RequestExit(0)` to take the server down so it would restart and roll again. That was never bad luck — per-step travel is capped at a fraction of `(r_i − r_{i−1})`, which telescopes to a fixed total, so a final circle seeded further than that from where the opening circle can legally sit could **never** be extended. The 500 attempts were provably wasted every time. There is no `RequestExit` in `BattleRoyaleZone.c` any more.

Two consequences worth knowing:

- **`end_in_villages` POIs are pre-filtered by the same oracle**, and the count is logged at boot (`306 in CfgWorlds, 285 after the avoid lists, 255 chain-feasible`). It removes nothing usable: every POI it drops was already a seed that ended in a shutdown. That number is the one to watch when tuning `static_sizes` for a new map.
- **The seed walk must start at a random POI.** Deriving the start from `seed_attempt` made it deterministic and the first acceptable village in `CfgWorlds` order won every match. The self test reported 200/200 with no backtracking, which reads as healthy and was the tell — it now also reports the spread of the final circle and warns when it collapses.

**`zone_selftest_runs` in `zone_settings.json` is the acceptance gate**, and it is a setting rather than a diag entry so it runs on a headless dedicated server. It generates N throwaway chains at boot, reports the failure / backtrack-depth / tier distribution and the spread, then plays normally. 200 runs inside one boot answers "can this configuration dead-end on this map", which relaunching the server twenty times never could — and it costs ~5-9 ms per generation. Measured 2026-08-11 at stock `static_sizes`: Sakhal 200/200, tiers T1 803 / T2 120 / T3 60 / sweep 21, witness step never needed, backtracking exercised twice at depth 2; ChernarusPlus 200/200, T1 953 / T2 47, no backtracking.

`zone_generation_seed` (0 = off) replays one layout. Note it reseeds the **global** RNG, so a non-zero value also fixes loot, weather and spawn placement; at 0 a fresh seed is drawn and logged each boot, so a run is replayable without changing anything observable.

**Misconfiguration clamps rather than halting boot.** `BattleRoyaleZoneData.Validate()` — a new `BattleRoyaleDataBase` hook called after *both* the profile and mission passes — clamps `num_zones` to the shortest settings array, to the longest strictly-increasing prefix of `static_sizes` (a non-increasing pair makes the span ≤ 0 and silently breaks containment), and to what fits the world. It **must never `Save()`**: `Load()` re-saves before the mission pass, so persisting a clamp would overwrite the admin's file permanently. Verified — a boot with `num_zones: 12` clamps to 3, warns, reaches the lobby, and leaves `12` in the JSON.

**Size the opening circle to the map: `r_max ≈ 0.22 × W`.** PUBG's Erangel is 8 km with a ~2 km first circle. Past `0.25 × W` the opening circle's centre is pinned near the map centre every match (`Validate()` warns), because `W/2 − r_max` is all the freedom it has. `zone_settings.json` supports a mission override and the mission is per-map, so per-map sizes need no code; `scale_sizes_to_world` (off by default) does it automatically, holding the final circle fixed and scaling only the span above it — a flat multiply would shrink the endgame, whose size is a function of how many players are left, not of the map.

⚠️ **Sakhal is 15360 m, not 8192** — its difficulty is water, not the world box. Do not re-derive the geometry from a wrong world size.

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

### Spectating

Entered **in place at the moment of death** — the client never disconnects. Gated on
`spectate_enabled` in `general_settings.json`, which defaults **off**; with it off nobody is ever
offered spectating and the death screen keeps the old timing, quitting to the menu on its own after
`BR_DEAD_AUTO_QUIT_MS` (15 s, the same figure the pre-spectate `CallLater(LeaveServer)` used). The
death *screen* itself replaces the engine fade unconditionally — see below.

Two earlier attempts entered spectate on *reconnect* (the system removed in `e6a0e1b`, and the
unmerged `test-vpp-spectate` branch). That can never work here: `2_BattleRoyaleCountReached` locks
the server through the autolock webhook, and vanilla `MissionServer` calls `InvokeOnConnect` only
from `ClientNewEvent`/`ClientReadyEvent`, never from `ClientRespawnEvent`.

The mechanism is DayZ's own **`GetGame().SelectSpectator(identity, "BattleRoyaleSpectatorCamera", pos)`**
(`game.c:378`), called server-side. It resolves a **script class name, not a config class** —
vanilla's own `DayZSpectator` has no `CfgVehicles` entry anywhere in `P:\dz` — so
`BattleRoyaleSpectatorCamera` needs **no `config.cpp` entry**. (Contrast `CreateObject`, which *does*
resolve through config.) There is **no client → server spectate RPC at all**: targeting is automatic,
so there is no client request to authenticate.

**⚠️ KNOWN LIMITATION: a spectator can only see targets within roughly 1 km of their own corpse.**
`UpdateSpectatorPosition()` does **not** move the replication bubble for player entities. The camera
pushes it at vanilla's 0.5 s cadence and `BeginSpectate` calls `SelectPlayer(identity, NULL)` before
`SelectSpectator`, and neither re-centres anything: the bubble stays on the connection's own entity,
which is still the corpse (`GetGame().GetPlayer()` keeps returning it — see below).

**Established 2026-08-10 by deliberate, bidirectional measurement**, using the diag `TP Target` entry
to place the watched player at an exact radius. Exactly two `entity` transitions in the whole run,
each coincident with a teleport across the boundary:

| | |
|---|---|
| TP to **1200 m** | `entity 1 → 0`, sustained **100+ samples over 90 s** |
| target walks back to **1122 m** | still `entity=0` — no recovery |
| TP to **700 m** | `entity 0 → 1`, sustained |

Throughout all of it `UpdateSpectatorPosition` ran continuously (507 pushes) and the camera sat
within **4 m** of the pushed position — the exact condition that means "the push runs and has no
effect". `networkRangePlayers` is unset in `serverDZ.cfg`, so DayZ's default **1000 m** applies.

Client-side the symptoms are: **no character, no camera pan, but the nametag is still there** — the
name is a string from the `SetSpectateTarget` RPC, not from the entity, so it survives the dropout
and is the tell that this is replication rather than targeting.

**The bubble is centred on the corpse, and moving the corpse moves it.** Also measured, with the
diag `TP Corpse to Target` probe, and this one is a controlled experiment rather than a correlation:

```
20:26:38  TP Target -> 1200 m      corpse still at the death spot
20:26:41  entity 1 -> 0            target 1200 m from the death spot
20:27:06  TP Corpse -> target      corpse moves 1160 m, alive=0 (it is the body)
20:27:13  entity 0 -> 1            target STILL 1155 m from the death spot
```

The target never moved closer to where the player died — only the corpse moved — so distance-to-
corpse is the variable and nothing else is. Recovery took ~7 s, which is the juncture plus
re-establishing replication; a real implementation that moved the corpse *continuously* would never
cross the boundary and never pay it.

**A carried corpse leaves the world visually, and that is the cost to design around.** Tested on
ChernarusPlus: with the corpse moved 1200 m to the target, a second player sent back to the death
position found **no body there** — correctly, since it really did move — and had not seen it at the
carried position either. So the server's authoritative position moves (which is why the bubble
follows) while the entity's *replicated* position does not, and no client renders the body anywhere.

That is convenient in one way — nothing skates across the map for bystanders to see, so the 50 m
burial the removed carrier plan used is unnecessary — and fatal in another: **the victim's body and
its gear leave the match.** Any shipped version must therefore drop the inventory at the death
position *before* the body is ever moved. Vanilla `PlayerBase.DropAllItems()` (`playerbase.c:6681`)
is the tool: it walks the inventory and `ServerDropEntity`s each item, so it **creates nothing** —
the items already exist and are only moved out of the corpse. Called before the first carry, they
land at the death position for free, with no position arithmetic. It excludes anything inheriting
`SurvivorBase`, i.e. the body itself, and it does drop clothing too — harmless, since the corpse is
invisible.

The trade that leaves is presentational and is a **gameplay decision, not a technical one**: the
victim's loot becomes a pile on the ground rather than a lootable body. Nothing is destroyed and
nothing is created; it is only how the loot presents.

**This is implemented and validated live (2026-08-10).** `BattleRoyaleSpectators.CarryCorpse`, run
from the 1 Hz block in `Tick()`, tuned by six compile-time constants in `BattleRoyaleConstants.c`
(`BR_SPECTATE_CARRY_*`); set `BR_SPECTATE_CARRY_CORPSE` false to disable it and keep the old limit.
The first carry waits until the target is `TRIGGER_M` (250) away *and* nobody is within
`BYSTANDER_M` (50) of the body, with `FORCED_M` (750) as the backstop so camping a corpse cannot
hold a spectator's view hostage; the gear drops via vanilla `DropAllItems` before the body ever
moves, then it re-carries every `STEP_M` (250) of drift. Measured: carry fired at 252 m with
`0 left` on the body, and the target then tracked to **1237 m with `entity=1`** — past the old wall.

**A teleported target still derenders, and that is now the only way to see the old symptom.** The
same run shows a 19 s `entity=0` window immediately after a diag `TP Target` moved the watched
player 1136 m — and it happened while the target was *99 m* from the death spot, i.e. the opposite
of a range problem. Real players run rather than teleport, so the only production path that
reproduces it is the watched player taking an F2 unstuck. It self-corrects. Do not read such a
window as the carry failing.

There is **hysteresis**, which is what made this so hard to pin down: approaching from inside, a
target stays replicated out to ~1068–1200 m; once dropped, it does not return until well inside
(somewhere between 700 and 1122 m). That is why an early trace read "out at 1068, back at 720", and
why a session that never crossed the boundary — 39 clean samples to 929 m — looks like proof that
nothing is wrong. **A run that stops short of ~1100 m proves nothing.**

Two other things produce `entity=0` and must not be mistaken for this: a *retarget* to somebody far
away lands as one huge single-sample step, and *the match ending* freezes every field to identical
values while `pushes` keeps climbing (`EndSpectate` deliberately leaves the camera running).

**Do not "fix" this by creating a body to carry the bubble.** The diagnosis above is right; the
carrier is still wrong, and every part of *it* failed:

- `4005d62` retracted the original ~1000 m reading, on the grounds that the `entity=0` windows were
  *intermittent* and tracked camera movement of 450 m in 6 s — a target being **teleported**. **That
  retraction was itself an error**, and it is the reason this went round three more times: a
  teleporting target and a range cutoff produce the same `entity=0`, and the observation that one
  exists is not evidence the other does not. The tidy ~1000 m number was never a coincidence.
- **Neither way of creating that body survives a dedicated server.** `CreateObjectEx` gives
  *"Access violation. Illegal read … at 0x0"* in the engine's object setup, identical with and
  without `ECE_INITAI`. `CreatePlayer` then crashed on the first real spectate with
  *"Illegal write … at 0x9"* — fault bytes `F0 44 0F C1 71 08`, a `lock xadd [rcx+8]` refcount on a
  garbage pointer with `rcx = 1`. Neither is catchable in script. The classname is not the problem;
  `SurvivorM_Mirek` is `scope=2` in `dz/characters/data/config.cpp`.
- **`CreatePlayer`'s contract is the explanation**: its doc is *"Assign player entity to client (in
  multiplayer)"*, and vanilla's only `null`-identity call site is `missionbenchmark.c:366`, which has
  no clients. A NULL identity on a live MP server is out of contract. A *non*-NULL one is not an
  option either — the body would then be `ALIVE` and outside the current state, so `OnPlayerTick`'s
  force-logout kicks the very spectator it exists to serve.
- A boot-time probe reported `CreatePlayer` as safe and **that probe proved nothing**: it ran before
  any client existed, which is precisely the condition that makes the call legal. When a failure mode
  is a crash rather than an exception, the probe has to reproduce the real calling context.

`BattleRoyaleSpectators` (`Scripts/Server/5_Mission/BattleRoyale/Server/`) owns it. Three invariants:

- **It holds no object reference at all — only SteamID64 strings**, re-resolved at the moment of use,
  so a freed `PlayerBase` is never a failure mode. This is why the killer is stored as a uid: it must
  survive the killer's own death and disconnect. There are no exceptions to this.
- **A spectator is never revived and never re-added to `m_Players`.** `BattleRoyaleServer.OnPlayerTick`
  force-logs-out anyone outside the current state *and* `EPlayerStates.ALIVE`, and re-adding them
  would stall every `IsComplete()` and corrupt `br_position`.
- **The corpse is never deleted** — it *is* the `PlayerBase`, and it is the victim's loot. Vanilla
  `HandleBody` only deletes a body when the player was alive. The removed `e6a0e1b` implementation
  did `ObjectDelete(player)` with the comment *"this is for network bubble fix"* — destroying the
  victim's gear to solve a problem `UpdateSpectatorPosition` already solves for free.

Target selection is automatic and chained, resolved **at the moment spectating begins** (not at
death, so the chain reflects who is alive when you actually start watching) and re-resolved whenever
the target dies or disconnects:
**T1** a living teammate → **T2** the killer chain seeded at the last teammate to die (a solo player
seeds at themselves, so hop 0 is their own killer), walking `killer_uid` through dead killers →
**T3** the most recent kill whose killer is still alive → **T4** the living player nearest to where
this spectator fell → **T5** orbit the final circle. A `visited` set plus
`BR_SPECTATE_CHAIN_MAX_HOPS` guard the walk. T4 exists because a suicide or a pure zone death with
no kills anywhere otherwise resolved straight to an orbit of an empty circle.

Which tier fired is recorded on the entry as `resolved_tier` and appears as `(T1)`…`(T5)` on every
`Registered` / `Retarget` / `BeginSpectate` line. It is diagnostics only — nothing branches on it —
but without it the chain is five deep and only inferable from who you end up watching.
`m_LastResolveTier` is written at each of `ResolveTarget`'s five exits and read by the caller on the
very next line; it is a return value in all but name, so **never read it anywhere but immediately
after a `ResolveTarget` call**.

**Testing it needs the diag menu's Spectate submenu** (`DIAG_DEVELOPER`, so both `DayZDiag_x64`
sides). **Kill Me** is the one that matters: without it, reaching the death screen at all needs a
second client to land a kill, which made every spectate test a three-client test. It sends
`KILL_SELF`, and the server does `SetHealth("", "Health", 0)` on the sender rather than calling
`RecordDeath` directly — `EEKilled` is the hook under test, so a shortcut would exercise only the
parts that were never in doubt. That also means the killer resolves to the victim themselves, so
**Kill Me reproduces T4 specifically**; a real kill by another player is still the only way to reach
T1 or T2. **Log Spectators** dumps the table with each entry's tier and whether its identity still
resolves ("registered but gone" is the shape of a leak). **Spectate Enabled** flips
`spectate_enabled` in memory only — `Load()` re-saves on next boot, so a diag toggle must never
become a persisted setting.

**The death screen is a `UIScriptedMenu`** (`MENU_BR_DEAD`, `Scripts/Client/5_Mission/GUI/DeathScreenMenu.c`
+ `GUI/layouts/death_screen.layout`), not `UIManager.ScreenFadeIn` — the engine fade is a proto
native taking a string and two colours and has no widget tree, so it cannot carry buttons. It offers
**Spectate** (only once the server has sent `SetSpectateOffer`) and **Quit to menu** (always, so a
server with `spectate_enabled` off still has an exit). Pressing Spectate sends the one client→server
RPC in the feature, `RequestSpectate`, which carries **no payload**: the server resolves the actor
from `sender` and will only act on a uid that is already a registered, still-pending spectator.
Pressing nothing lets `BR_SPECTATE_ENTRY_DELAY_MS` start it anyway.

The screen runs **one of two countdowns**, and which one is the whole of its state machine. While an
offer stands it mirrors the server's `BR_SPECTATE_ENTRY_DELAY_MS`; with **no** offer standing it runs
`BR_DEAD_AUTO_QUIT_MS` and quits to the menu itself. The second case is not just `spectate_enabled`
being off — `EndSpectate` **withdraws** a standing offer, because `EndAll()` also reaches players who
are still sitting on the death screen. Without that withdrawal they were left pressing a Spectate
button the server had already stopped honouring (`RequestSpectate` returns at its `m_Ended` guard
silently), watching a countdown to an entry that would never happen.

**The death screen is driven from `BattleRoyaleClient.Update()`, not from `UIScriptedMenu.Update()`.**
The engine calls `UIScriptedMenu.Update()` exactly **once** for this menu — established by logging
the first five frames and never seeing more than one. Anything per-frame (the countdown, holding the
cursor) therefore has to hang off `BattleRoyaleClient.Update()`, which runs unconditionally from
`MissionGameplay.OnUpdate` outside vanilla's `m_LifeState == ALIVE` gate. Its `Tick()` is the entry
point. Three traps in the layout, all of them about the backdrop:

- **A `PanelWidgetClass` with no `style` paints nothing.** `color` alone is not a fill — the widget
  still lays out and still hosts its children, so the buttons and text draw perfectly over a
  completely transparent background, which is exactly how it presents live. `style DayZDefaultPanel`
  is what gives a panel its fill; `leaderboard.layout` is the working reference in this mod, same
  construct. Removing that style to chase the edge gap below cost the whole backdrop.
- **Widgets inset themselves to the safe zone**, so a `size 1 1` backdrop leaves a gap at the screen
  edge unless it sets `keepsafezone 0` — and even then an edge or two can survive. The backdrop is
  therefore deliberately **overscanned**: `position -0.02 -0.02`, `size 1.04 1.04`. Symmetric, so
  `halign/valign center_ref` on the child dialog still centres it on screen.
- The HUD (party nametags, POI markers) **draws over it** unless it claims a high `priority`.

**Focus accounting is the trap here.** Input focus is an additive counter:
`SimulateDeath` → `LockControls(true)` (+1/device), the menu's `super.OnShow()` → `LockControls()`
(+1/device), `super.OnHide()` (−1/device), `EnterSpectate()`'s single explicit release (−1/device) —
net zero. The death menu therefore adds **no** focus calls of its own. Note `LeaderboardMenu` *does*
add `ChangeGameFocus(1)` / `ResetGameFocus()` on top of `super`; do not copy that pattern, since
`ResetGameFocus` zeroes the counter rather than decrementing it.

Killer attribution goes through `ResolveKillerUid`, because **`EEKilled`'s `Object source` is the
weapon** for every gun and melee kill — the hierarchy-parent step is the same idiom the webhook code
uses at `0_BattleRoyaleState.c:545-550`. `RecordDeath` is **first-write-wins**, which is what makes
the documented double `RemovePlayer` and the unconscious-disconnect path harmless.

One driver: `BattleRoyaleSpectators.Tick()`, from the existing 10 Hz block in
`BattleRoyaleServer.Update()`. Its four passes are liveness sweep (the *primary* disconnect
detector — `PlayerDisconnected` is unreliable for a client controlling no entity), deferred entry,
target liveness (the catch-all), and a 1 Hz keepalive push.

Which states allow it is a `BattleRoyaleState.AllowsSpectate()` virtual, default `false`, overridden
in `6_BattleRoyaleRound`, `7_BattleRoyaleLastRound` and `5_BattleRoyaleStartMatch` (on `b_IsGameplay`).

`BeginSpectate` calls **`SelectPlayer(identity, NULL)` before `SelectSpectator`**, dropping the
corpse from the connection's selection. Only the *selection* goes; the corpse itself is never
deleted, so it stays in the world, replicated and lootable. What actually keeps the watched player
replicated is the camera's `UpdateSpectatorPosition` pushes — see the top of this section, and do not
re-derive the "bubble is pinned to the selected object" theory from the shape of this call.

Client side, **`GetGame().GetPlayer()` does NOT go NULL while spectating** — it keeps returning the
**corpse**, a perfectly valid `PlayerBase` that simply is no longer simulated. (Measured: the tint
trace reads `spectating=1, player=1`.) This was assumed the other way round for most of the feature's
life and it is the trap here, because it makes "is there a player object" a silently wrong test for
"is this an ordinary living client" — no error, no log line, just an effect queued onto a corpse
whose `m_ProcessAddGlassesEffects` nothing ever drains. **Ask `BattleRoyaleClient.IsSpectating()`**;
`GetReferencePosition()`, `GetSubjectPosition()` and `ApplyOutOfZoneTint()` all do.

`BattleRoyaleClient` routes position reads through `GetReferencePosition()`, and
`EnterSpectate()` undoes what `SimulateDeath`
did — the death screen, the five zeroed sound buses, the additive input-focus lock (exactly one
release, behind a latch) and the vanilla HUD. It also calls
`PPEManagerStatic.GetPPEManager().StopAllEffects(PPERequesterCategory.GAMEPLAY_EFFECTS)`: dying
*while unconscious* leaves `PPERequester_UnconEffects` running forever, because `CommandHandler` only
reaches `OnUnconsciousStop()` through an `IsAlive()` branch a corpse never takes — that was a black
halo around the spectator's screen for the whole match. `GAMEPLAY_EFFECTS` rather than `ALL`, so the
ESC-menu and spawn-selection blur survive. The paired half is in `ShowDeadScreen`, which writes full
`m_CurrentShock` on the dying body so `ShockHandler` stops re-asserting tunnel vision every frame,
and clears `"UnconsciousAttenuation"` so the spectator does not hear the match muffled. `BattleRoyaleSpectatorCamera`
(`Scripts/Client/4_World/BattleRoyale/`, **unguarded**, like vanilla's `dayzspectator.c`) anchors on
the target's **head bone** rather than a stance offset, and never reads the target's aim angle —
`GetCommandModifier_Weapons()` can be NULL for a remote entity, which would silently flatten the
camera.

### Localization

`LanguageCore/stringtable.csv`, keys prefixed `STR_BR_`. The `Party/` addon carries its own `stringtable.csv` at its PBO root with `STR_PARTY_*` keys — the engine loads one per addon — so party strings do not go here. Three reference styles:
- Layouts: `text "#STR_BR_..."`
- Client script: `SetText("#STR_BR_...")`
- **Server script: the bare key with no `#`.** `MessagePlayerUntranslated()` / `MessagePlayersUntranslated()` (`0_BattleRoyaleState.c:200-242`) ship the key over the `NotificationMessage` RPC and the client localizes it in `BattleRoyaleRPC.NotificationMessage()`, which also substitutes the `READY_KEY` / `UNSTUCK_KEY` placeholders with live keybinds.

### UI

Layouts live in `GUI/layouts/`. The dominant pattern is imperative — `GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/....layout")` then `FindAnyWidget("Name")`; most layouts have no `scriptclass`. `SpawnSelectionMenu` is a `UIScriptedMenu` (`MENU_SPAWN_SELECTION = 75` in `Scripts/Client/3_Game/Constants.c`, instantiated in `MissionBase.CreateScriptedMenu`). The only declarative `scriptclass` binding is the COT `master_controls.layout` → `BRMasterControlsForm`.

Keybinds are declared in `Data/Inputs.xml` (`UADayZBRReadyUp` = F1, `UADayZBRUnstuck` = F2), registered via `inputs = "Vigrid-BattleRoyale/Data/Inputs.xml"` in `Scripts/Client/config.cpp`.

**The vanilla right-hand HUD is trimmed.** `modded class IngameHud` (`Scripts/Client/5_Mission/GUI/IngameHud.c`) hides the thirst, hunger and temperature notifiers plus the `NotifierDivider` beside Blood, and shifts `BadgesSpacer` / `BadgesPanel` right to close the resulting gap. `Extra/PreventPlayerModifiers/` already makes `ThirstMdfr.OnTick` and `HungerMdfr.OnTick` return immediately, so those three icons never move for a whole match — they are pinned decoration. Gated on `BR_HIDE_SURVIVAL_NOTIFIERS` (`BattleRoyaleConstants.c`), compile-time because the settings files are server-side only and this is a client cosmetic.

Three things about that hook are load-bearing:

- **`Show(false)` targets the parent panel (`Thirsty`, `Hungry`, `Temperature`), never the `Icon<Name>` image inside it.** Vanilla's `InitBadgesAndNotifiers` unconditionally `Show(true)`s every `Icon*` widget and `DisplayTendency` keeps tinting them; a hidden *parent* is not drawn whatever happens to its children, so nothing has to be re-asserted per frame. `NotifierDivider` is the one widget here that no vanilla script references at all — only `BadgeNotifierDivider` is managed, by `IngameHudVisibility`'s `NO_BADGE` flag.
- **The hook is `InitBadgesAndNotifiers()`, not `Init()`.** `Init()` calls it once at startup and `respawndialogue.c` calls it again after a respawn, so one override covers both. That is also why the badge reposition uses **absolute** x (`BR_HUD_BADGES_SPACER_X` / `_PANEL_X`) rather than a delta — a second pass must be a no-op.
- Everything under `HudPanel` is `halign right_ref`, so `position x` is the distance from the parent's **right** edge and hiding a middle widget leaves a hole rather than reflowing. Vanilla's right-to-left order is `Health` 0, `Blood` 43, `NotifierDivider` 86, `Temperature` 96, `Hungry` 139, `Thirsty` 182, `BadgesSpacer` 213, `BadgesPanel` 252. The badge group moves right by 143 so its divider lands on the old 86.

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
- **`Supress()` does not reach the aim axes, so the mouse still turned the camera under the map — and this is the one place an exclude group is right.** It is the fourth wrong-looking-right hook, and it also qualifies the previous bullet's "disables the mouse as a *game* input": vanilla's `Input.DisableKey` block covers mouse buttons 0-4 **and axes 0-5**, and it does run (there is no early `return` before `missiongameplay.c:616`) — but both player cameras read the aim engine-side off the input controller, `GetAimChange()` in third person and `GetAimDelta()` in first (`dayzplayercamera_base.c`, `dayzplayercamera3rdperson.c:441`), so the low-level disable never reaches it. `Supress()` cannot substitute either — it is a *press event* concept and the aim axes are analog. **`HumanInputController.OverrideAimChangeX/Y(ENABLED, 0)` was tried first and measured not to work**: the camera kept turning while an edge log confirmed the calls were reaching the live controller every open and close. The tell was available in advance and missed — `OverrideRaise` and `Override3rdIsRightShoulder` have real vanilla call sites and do work, while `OverrideAimChangeX/Y` have **none anywhere in `P:\scripts`**, only the proto declaration at `human.c:240`. What works is `AddActiveInputExcludes({"aiming"})` / `RemoveActiveInputExcludes` in **`MapMissionGameplay.UpdateAimSuppression`**. `"aiming"` (`bin/specific.xml:149`) is exactly the four aim inputs and does **not** include `"movement"` — which is the whole reason it is usable where vanilla's `{"map"}` is not. **The `UpdateControls()` held-input reset described above is still real and is simply accepted here**: opening or closing the map mid-sprint drops the player out of sprint until Shift is re-pressed. It is edge-triggered, not per-frame (each call rebuilds the control state), and it lives in the mission update rather than the menu so the *remove* edge cannot be missed — a leaked exclude group would leave the player permanently unable to aim. It is called **above** the `m_VigridMap` guard for that reason.
- **N** toggles the HUD minimap, which is **opt-in — off by default**. Effective visibility is `VIGRID_MAP_MINIMAP && minimap_allowed && minimap_enabled`, and each switch can only opt further out than the one before it. The first is the **build's** — a define in `Extra/Map/config.cpp`; comment it out and `VigridMapMinimap`, its widgets and the N handler are compiled out entirely, leaving the fullscreen map untouched. The second is the admin's (pushed over `VM_Settings`, ships **on**), the third the player's, in `$profile:Vigrid-Map\map_client.json` (ships **off**). So out of the box the key works but nothing is shown until it is pressed; the layout is still built on first `Update` and just kept hidden, so toggling on later costs nothing. Changing the `minimap_enabled` initialiser only affects players with no prefs file yet — anyone who already toggled keeps their saved choice. Two things deliberately survive a minimap-less build: `minimap_allowed` on the wire and in `map_settings.json`, because a *client* build flag must not change the wire format, and the N entry in `Data/Inputs.xml`, because XML cannot be conditional — it still lists under Options → Controls doing nothing.
- **K** toggles the HUD compass strip (`VigridMapCompass`), gated exactly like the minimap — `VIGRID_MAP_COMPASS && compass_allowed && compass_enabled` — but the **player default is ON**, since it is a thin band answering a question the HUD could not otherwise answer at all. A 620×38 px strip flush against the top of the screen showing a **90° window**: cardinals every 45°, numeric degrees every 30°, an unlabelled tick every 15°, the bearing read out below, and carets in the bottom lane for the next zone, teammates and party pings. Its three lanes are packed tight — ticks 0-11, labels 11-33, carets 33-38 — because slack at the bottom reads as a misaligned box whenever no caret is up. Labels come in three size tiers, and **each tier is its own widget with its own font face** (`metron-bold28` / `-bold22` / `-bold14`), because glyph size is fixed by the declared face and there is no `SetFont`; `PickLabel` shows one and hides the other two. **`SetTextExactSize` was measured to do nothing** — 28/18/13 on one widget rendered 28/28/28 under `GetTextSize` — and its one call site in all of `P:\scripts` was the tell that was missed. Every length is authored against a 1920-wide screen and multiplied by `parent_w / VIGRID_MAP_COMPASS_REFERENCE_W`, re-applied by `ApplyScale` only when the viewport moves. Heading is the **camera** bearing, the same one line the minimap dart uses; the regression test is that a full 360 returns to the same reading. Two structural choices worth keeping: the entry pool is indexed **by bearing** (entry *i* is permanently the *i*×15° mark) so labels are localised once at creation and never per frame, and the strip is redrawn **every frame** rather than the minimap's 10 Hz, because a band sliding under a fixed cursor is exactly where 10 Hz reads as stutter. Elements fade out over the last 8° instead of being clipped, which is why losing the clipping container below cost nothing.

**A widget's DECLARED position and size are scaled by viewport/1920; `SetPos` and `SetSize` are in real screen pixels.** Measured 2026-08-11 with `GetScreenPos` on a 1280-wide client: a child declared at `position 23` reported `15.33` (×0.667), and the same widget after an explicit `SetPos(23)` reported `23`. **Mixing the two silently misplaces things, and it does not look like a coordinate bug** — it looks like an *angle* bug, because the spacing stays perfectly correct while the whole group shifts. It cost two builds on the compass: first a fixed `620 px` `hexactsize 1` container, whose children were laid out in 620-px units while the container itself rendered ~430 px wide, putting the strip 95 px off centre; then, after reparenting to the full-screen root, a residual 8 px because the pooled entry root *declared* 48 px wide actually rendered 32, so subtracting the declared half-width over-shot. **The rule that falls out: for anything script-positions, position and size it entirely from script and treat the layout's numbers as Workbench-only placeholders.** The one container shape that behaves is the full-screen `size 1 1` / `hexactsize 0` frame — `m_Root.GetScreenSize()` reports real pixels and children `SetPos`ed in those pixels land correctly, which is what `VigridMapMarkers3D` and `VigridPartyNametags` were already relying on.

**Nothing can be drawn over a `MapWidget` with script-created widgets.** `CreateWidgets(path, parent)` returns a valid widget, `SetPos` puts it in the right place, it tracks pan and zoom — and it is never rendered, with no warning and no failed image load. The only overlay that works is a **`CanvasWidget` declared in the layout as a child of the MapWidget**, which is what `SpawnSelectionMenu`'s heat map already does. Canvas has only `DrawLine` and `Clear`, so there is no text on the map and every glyph is a fan of strokes. Four canvases in `map_menu.layout`, z-ordered by `priority`: `LineCanvas` 999, `ZoneCanvas` 1000, `MarkerCanvas` 1001, `TeamCanvas` 1002.

**Glyph vocabulary**, all in `VigridMapRender` and all sized in screen pixels rather than metres: zone rings and centre dots, a **ring with a cross** for a placed marker, a **hollow triangle** for a teammate, a lighter **diamond** for a party ping, and a **notched dart** for you — on **both** maps. The circle is spent twice already, so both live-position glyphs are straight-edged, and triangle-vs-diamond needs separating because they are the pair that could still collide at small size. That was originally done on three axes — vertex count, stroke weight, opacity — but the ping's 1 px stroke proved not reliably visible over satellite imagery at 12 px, so it was raised to match the triangle's 2 px. **Vertex count and opacity (0.75 vs 1.0) are now the only things telling them apart**, so `VIGRID_MAP_PING_ALPHA` must not be raised to 1.0 without giving the ping a different silhouette. If they ever do read alike, the tested fallback is a six-pointed asterisk for pings — three lines through a common centre, no enclosed area, unmistakable for any polygon.

Ping alpha has one more trap: on the **world** marker the crosshair-fade floor `VIGRID_PARTY_PING_CENTER_MIN_ALPHA` *multiplies* with `VIGRID_PARTY_PING_BASE_ALPHA`, so a floor that looks reasonable on its own can be far darker in practice — 0.25 × 0.75 left a ping at 0.19 alpha exactly when the player was looking straight at the thing they had just placed. It is 0.55 now.

**Both maps draw "you" as the same dart, and the fullscreen one used to be an axis-aligned plus (reversed 2026-08-11).** The plus was defended as easier to *find* on a big map, which is true and beside the point: the question a player opens a map to ask is which way they are facing, and the plus could not answer it. Findability is carried by size instead — `VIGRID_MAP_SELF_PX` is 16, the largest glyph on the map, against the teammate triangle's 14, and white, which no `VigridPartyPalette` slot is. Three things about the dart are load-bearing: its angle is the **camera** bearing, never `player.GetYawPitchRoll()` (body yaw snaps in steps and does not return to its start after a 360, so the arrow drifts); it is **drawn rather than a rotated `ImageWidget`** — `icon_arrow` points *down* at rest, reads ambiguously at small size because both ends look like a point, and vanilla's dedicated `Marker_Arrow.edds` does not resolve from a mod PBO at all (silently — no `.rpt` error); and the concave notch is what makes the direction unmistakable. The two call sites differ in **what they anchor to**: the minimap re-centres on the camera each tick and so passes the camera position for both position and angle, while `VigridMapMenu.RenderSelfGlyph` takes the position from `player.GetPosition()` and only the angle from the camera — the map is panned by the player, and in third person the camera is metres behind the body. Note the `"aiming"` exclude group means the fullscreen dart holds the heading you had when you opened the map; live turning can only be observed on the minimap.

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
- Spectating is entered **in place on death** — no disconnect, no reconnect — behind `spectate_enabled` in `general_settings.json`, which defaults **off**. **It has a known ~1 km limitation**: the network bubble stays on the spectator's corpse, so a target further than that is not replicated and the spectator sees a nametag with no character. Measured both directions 2026-08-10. See *Architecture → Spectating*. The orphaned `GUI/layouts/hud/spectator/player.layout` is still unreferenced: there is no spectator HUD, only a notification naming the current target.
- `Workbench/version` (`0.8.100368`) is a DayZ build number read by nothing. The mod version is `BATTLEROYALE_VERSION`.
