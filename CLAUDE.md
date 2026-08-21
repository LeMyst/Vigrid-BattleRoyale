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
- ⚠️ **Then run `Deploy.bat` from the PRIMARY checkout's `Workbench/Batchfiles`, not the worktree's.** `user.cfg` is gitignored, so a fresh worktree has none, and `_Config.bat` refuses to do anything without one — *"ERROR: Workbench\user.cfg not found"*, exit 1, before a single file is packed. **Config and sources come from different places, and only the sources follow the junction:** `_Config.bat:16` sets `cfgRoot=%~dp0..`, so `user.cfg` is resolved next to *the batch file you invoked*, while `CI.bat:49` does `pushd "%workDrive%%prefixLinkRoot%\"` **before** calling `_EnumPaths`, so every source path is enumerated *through the junction*. So the primary supplies the config, the junction supplies the code, and the worktree needs no `user.cfg` at all — which also avoids having to delete it again before `git worktree remove`, since an untracked file makes that remove refuse. Copying `user.cfg` into the worktree works too; it is just more to undo.

⚠️ **`Build.success` is NOT proof of whose sources were built** — it lands in the same place either way. `CI_Build.bat:21-30` creates and writes `%~dp0..\Logs`, i.e. next to *the batch file you invoked*, exactly like `user.cfg`; so `Build.log` and all three markers go to the **primary checkout** even while the junction points at a worktree. **Verify the PBO instead**: check its timestamp in `%ModBuildDirectory%` and grep it for a string only your change introduces. (The `.pbo` is not compressed, so a plain byte search finds script text.)

⚠️ **Two `Le chemin d'accès spécifié est introuvable` lines at the end of every successful worktree build are normal and mean nothing is wrong.** `CI.bat:105` appends its "Successfully packaged all mods." summary to `%workDrive%%prefixLinkRoot%\Workbench\Logs\Build.log` and `:106` `type`s it — the only two writes in the build that go *through the junction* rather than beside the batch file. `Workbench/Logs/` is gitignored, so a fresh worktree has no such folder and both fail. The only real consequence is that the primary's `Build.log` is missing that final summary line. Do not "fix" it by creating the folder in the worktree; an untracked directory there makes `git worktree remove` refuse.
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

⚠️ **Two switches default OFF in `project.cfg` and almost certainly want to be ON in your `user.cfg`.** Both were unconditional until 2026-08-15; each is off by default because it is unrelated to building the mod and this repo is public.

| Key | Off (the default) | On |
|---|---|---|
| `UseSteamEmu` | `LaunchSteamClient.bat` starts the exe and writes nothing | writes the Goldberg Steam identity per slot, which is what stops several local clients kicking each other off one shared identity. **`LaunchLocalMP.bat 2`/`3` needs this** |
| `CopyExtraPBOs` | `CI0_CopyExtraPBO.bat` skips `Workbench/ExtraPBOs/` | copies those prebuilt third-party PBOs into the built mod, as the build always used to |

`Workbench/ExtraPBOs/` is gitignored (`*`), so an unconditional copy meant **a clone built a different mod than the maintainer's, invisibly** — the extra PBOs load exactly like the mod's own. That folder now carries a `README.md` naming each PBO, its author and its licence status; read it before turning the switch on for anything you publish.
2. `Workbench/Batchfiles/SetupMod.bat` — creates the `P:\Vigrid-BattleRoyale\` junction into this checkout, then copies every folder in `Workbench/Missions/` out to `%GameDirectory%Missions\`. The checkout folder name must equal `PrefixLinkRoot` (`Vigrid-BattleRoyale`).

**Re-run `SetupMod.bat` after a DayZ update** — an update wipes `%GameDirectory%Missions\`, which takes the offline test mission with it. It cannot be a junction into the checkout for the same reason, so `Workbench/Missions/` holds the masters and the copy is one-way. The copy is per folder and never deletes, so an unrelated mission already installed there survives; the build never sees these files, since `_EnumPaths` only enumerates `config.cpp`.

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
| `ClearStorage.bat` | Delete `storage_*` under `MPMission`. Skipped by the launcher when `KeepStorage=1`. |
| `BootTime.bat` | Report where the last server boot's time went, and append it to `Workbench/Logs/boottime.csv`. Run it *after* the server is up — the launcher starts it detached. |

Build result: `Workbench/Logs/Build.log`, plus marker files `Build.success` / `Build.failure` / `Build.deploy`. `CI.bat` hard-fails if `P:\` is not mounted.

**A failed signature now fails the build.** `CI1.bat`'s `:SIGN` used to ignore `DSSignFile.exe`'s exit code, and so did both of its call sites, so an **unsigned PBO reached `Build.success`** and a signature-verifying server rejected the mod with nothing in the log to say why. `:SIGN` returns 0/1, checks the `.bisign` actually appeared (the tool is not trusted to report its own failure), and `:PACK` forwards that to the `IF ERRORLEVEL 1 GOTO ABORT` the build loop already had.

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

**There are no unit tests and nothing compiles EnfusionScript outside the game — but there IS a static check suite, and it runs in CI.** `Tools/check.py` (stdlib only, ~4 s) holds 13 checks over the tracked sources; `.github/workflows/checks.yml` runs `python Tools/check.py -W` on every push to `main`, every PR and on demand. Run it locally the same way, or via `Workbench/Batchfiles/Check.bat`, which is the one batch file needing neither `user.cfg` nor a mounted `P:` because it only reads sources.

```bash
python Tools/check.py            # everything
python Tools/check.py --list     # name every check
python Tools/check.py --only rpc # one (repeatable, or comma-separated)
python Tools/check.py -W         # warnings fail too - what CI runs
```

| Check | Catches |
|---|---|
| `asset-paths` | every `Vigrid-BattleRoyale/...` asset reference resolves |
| `configs` | `CfgPatches` names unique, `requiredAddons` resolve, no stripped vanilla parents |
| `data` | tracked JSON parses; no UTF-8 BOM on any data file |
| `discipline` | standalone addons name no `BattleRoyale*` symbol; host calls are `#ifdef`-guarded |
| `enfusion` | no ternary operator, no multi-line `if`/`while` conditions |
| `extra-index` | every `Extra/` addon has a README, an index entry and a config |
| `guards` | every `Scripts/Server` file opens with `#ifdef SERVER` |
| `inputs` | every `Inputs.xml` parses, and its three sections agree |
| `menu-ids` | no two `MENU_*` constants share an id across the PBOs |
| `rpc` | every `AddRPC` name has a matching CF handler method |
| `settings-version` | a new settings field bumps `version`; a new `ref array` gets an `Upgrade()` branch |
| `stringtable` | every `STR_*` key referenced is defined, in its own addon's table |
| `version` | `BATTLEROYALE_VERSION` and `mod.cpp` agree |

**What it does NOT do is compile anything**, so a green run says nothing about whether the mod loads. Every check decides from source text alone, and what they cover is precisely the class of defect that fails **silently at runtime with a clean `.rpt`** — a dead RPC registration, a colliding menu id, an unguarded server file, a settings field that loads back empty. `-W` is affordable because the tree is clean, so a new warning must never be left to pass silently.

⚠️ **CI cannot build the mod and no hosted runner can** — binarize/rapify/pack need DayZ Tools, Windows and a mounted `P:`. A self-hosted Windows runner would be *worse than nothing*: `P:\Vigrid-BattleRoyale` is a single global junction, so a CI job would silently steal the build target from whatever interactive session was mid-build.

⚠️ **A check that stops catching anything still goes green.** `settings-version` was fixed in #309 after its `MEMBER` pattern — which has no notion of brace depth — read three ordinary **local variables** as brand-new settings fields simply because a diff re-added those lines. When changing a check, prove it in both directions: construct an input it must reject and confirm it still does, exactly as with the diag fixtures elsewhere in this file.

Validation loop for anything the suite cannot see: `Deploy.bat` → launch (`ClientEXE` defaults to `DayZDiag_x64.exe`) → read the `.rpt` in the profile directory for script errors.

**Reach for `LaunchOffline.bat` first, and only escalate to `LaunchLocalMP.bat N` when the test genuinely needs a server.** It is one process instead of two-to-four, it loads in ~90 s, and it gives a real controllable character — so the whole client half of the mod is testable in it: HUD and widget layout, menus and keybinds, the map addon, the compass, name tags, world markers, `#ifndef SERVER` code paths, and anything judged by eye. What it cannot reach is the mod itself: `SERVER` is undefined offline, so all of `Scripts/Server/**` compiles out — no `BattleRoyaleServer`, no states, no zones, no RPCs, no settings JSONs. Anything keyed on match state, a second player, or a server→client push needs MP.

`SPMission` must point at a mission whose `CreateCustomMission` returns a **`MissionGameplay`** — currently `.\Missions\empty.ChernarusPlus`, whose master is `Workbench/Missions/empty.ChernarusPlus/` in this repo and which `SetupMod.bat` deploys (an `init.c` and nothing else; the world comes from the folder suffix, so another map needs only a renamed copy). Community-Online-Tools then does the rest: its `modded class MissionGameplay.OnMissionStart` sees `IsMissionOffline()` and spawns you at a random `CfgWorlds` named location with a fixed loadout (military kit, plus a knife, Magnum, Shovel and Hatchet already on quickbar slots 0-3).

⚠️ **A `dayzOffline.*` mission — or any of the `BR-MissionFiles` ones — cannot work, and the failure looks like a hang.** Vanilla's own factory (`P:\scripts\5_mission\somemission.c:7`) returns `MissionServer` only for `IsMultiplayer() && IsServer()` and `MissionGameplay` otherwise; every stock mission overrides that with an **unconditional** `return new CustomMission()` where `CustomMission extends MissionServer`. Offline that means `MissionGameplay` is never constructed, nobody is ever selected, and the client sits on the on-screen **`IDLE MODE ACTIVE`** banner. The two log tells: `MissionGameplay::MissionGameplay` is **absent** from `script_*.log`, and the `.RPT` carries `[IdleMode] Entering IN` with no matching `Leaving OUT` (on a *dedicated server* that same line means the opposite — it is the readiness marker). Note the `pluginitemdiagnostic.c` → `GetPlugin()` stack trace in an offline `.rpt` is **not** this and is not an error at all: `pluginmanager.c:331-345` prints and `DumpStack()`s under `#ifdef DIAG_DEVELOPER`, then returns null. It was blamed for the freeze twice.

A bare `empty.*` mission calls no `CreateHive()`, so no loot and no infected spawn — copy `dayzOffline.chernarusplus` and change only its `CreateCustomMission` if a test needs the economy. And if `$profile:ExpansionMod\Loadouts\AdminLoadout.json` ever exists, COT applies that loadout instead.

⚠️ **Never screen-capture the machine and never send synthetic input to the game.** No
`CopyFromScreen` / `PrintWindow` / screenshot tool — the game window is not fullscreen, so a grab is
a picture of the whole desktop, not of the mod — and no `SendKeys` / `keybd_event` / `mouse_event` /
AutoHotkey aimed at the client. Building, deploying and launching through `Workbench/Batchfiles` is
the normal workflow and is unaffected; what is out of bounds is watching or driving the machine.
When a change can only be judged by eye, make the thing under test **open itself from script** (a
one-shot `EnterScriptedMenu`, or a `Show(true)` forced per frame from `Update()` so the widget's own
state feed cannot undo it), read the client `script_*.log`, and **ask Myst to look and report** —
they will supply a screenshot themselves if one helps. Note injected input does not work anyway:
DayZ reads raw input and discards it (measured 2026-08-11).

### Server boot time

⚠️ **The first boot after a build is not comparable to any other, and that is the first thing to
establish before chasing a "slowdown".** Measured 2026-08-14: three boots of one identical build
gave **3m36s, 2m27s and 1m48s** — but they were spread over an hour with a build in between. Three
boots taken back-to-back on warm cache gave **76.7 s, 76.2 s, 76.0 s**. So boot time is *stable to
under a second* when nothing else has touched the disk, and the 2× spread was the page cache paying
for ~130 MB of freshly written PBOs, not the mod. Compare warm against warm or the measurement is
meaningless — and the difference between "the mod got slower" and "the disk was cold" is invisible
without a baseline.

**`ClearLogs.bat` deletes `*.log` and `*.rpt` from the profile directory at the START of every
launch**, so each run destroys the evidence of the one before it. `BootTime.bat` exists to break
that cycle: it parses markers the engine already prints and appends a row to
`Workbench/Logs/boottime.csv`, which lives in the repo and therefore survives. Collect several
samples before believing a regression *or* a fix.

Where a cold ChernarusPlus boot actually goes (measured, 33,921 CE items):

| Phase | Share | Marker it ends at |
|---|---|---|
| engine + PBO load | ~16 s | `Hostname of server:` |
| world, scripts, mission `OnInit` | ~18 s | `[CE][TypeSetup]` |
| **Central Economy loot spawn** | **~33 s** | `[CE][LootRespawner] … Initially (re)spawned:` |
| CE vehicle respawn | ~6 s | last `[CE][VehicleRespawner]` |
| finalise + first save | ~3 s | `[IdleMode] Entering IN` |

The CE phase used to be 51–75 s of that, and the engine's own report of it (the `at N (sec)` on the
`LootRespawner` line) went **51 s → 30 s, stable across three runs**, once `SpawnWithAmmoAndMagazine`
stopped reading `serverDZ.cfg` once per item and stopped logging once per magazine.

**The mod's own boot work is free** — zone generation, all eight settings files, Party, KillFeed,
SafeZone and Map all complete inside a *single second*. Do not go looking for boot cost in
`BattleRoyaleServer.Init()`; it is not there. **The Central Economy is over half of boot**, and it
is the only phase the mod can influence, through the `EEOnCECreate` / `EEInit` hooks in
`Extra/SpawnWith*` and `Extra/SpawnWeaponChambered` that fire once per spawned item.

**`KeepStorage=1` is the big lever for an iteration loop.** The wipe is what forces CE to cold-spawn
the whole map every launch; keeping storage skips that phase outright. Off by default, because the
wipe is what makes a run reproducible. A loot-free `empty.*` MP mission removes the phase entirely
(same trick as the offline rig above — no `CreateHive()`, so no loot and no infected).

⚠️ **The script log is the cheapest proxy for per-item work, and it is enormous**: ~31,900 lines per
boot (44,700 before the two per-magazine `Print()`s came out of `SpawnWithAmmoAndMagazine`), of which
~26,650 are the weapon-chambering path — COT's `FillChamber` / `FillInnerMagazine` INFO logging,
triggered once per weapon by `Extra/SpawnWeaponChambered`, and the largest remaining block. By
contrast Expansion's `EXTRACE` contributes 221 lines and the four newer addons 31 between them, so
neither is worth suspecting. `boottime.csv` carries the line count per boot for exactly this reason:
a newly chatty hook shows up there first.

Runtime log verbosity (one at a time): `-br-warn`, `-br-info`, `-br-debug`, `-br-trace`, `-br-none`. On a server, `serverDZ.cfg` key `BRLogLevel` (1-4, negative disables) does the same. Diag builds default to trace via `#ifdef DIAG` → `BR_TRACE_ENABLED` in `BattleRoyaleConstants.c:10-20`.

Log with `BattleRoyaleUtils.Error/Warn/Info/Debug/Trace(string)` (`Scripts/Client/3_Game/BattleRoyaleUtils.c`) — not `Print`. On server+DIAG these mirror into in-game chat via the `ChatLog` RPC.

## EnfusionScript constraints

- **No ternary operator.** Use `if`/`else`.
- **No multi-line `if` conditions** — the whole condition must be on one line.
- **One declaration per variable name per method scope**, even across disjoint branches.
- **A single expression has a complexity ceiling** — around ten concatenated terms is rejected with `Formula too complex`, a hard compile error. The funnel-diagnostic log lines this repo favours are exactly the shape that trips it; build the string in steps once it carries more than about four fields. Packing succeeds regardless, so it only surfaces when the game loads the module.
- `modded class X` extends an existing class; `override` is required to replace a method.
- **`ref` is an ownership declaration, and it is the one mistake nothing catches** — a wrong one compiles, packs, boots and plays. Required on **member variables** and **template arguments** (`array<ref T>`); wrong on locals, parameters, return types and `new ref X()`. Full rule in the `dayz-modding` skill, §4.4.

The tree was swept clean in #76 / #309 — **120 removals across 29 files** (110 locals, 9 return types, 1 parameter, no `new ref`). ~410 of the remaining `ref` tokens are load-bearing, so **never run a find-and-replace on `ref `**: stripping it from `array<ref T>` frees the container's elements, and adding it to `array<PlayerBase>` makes the list try to destroy players. Four results of that sweep are non-obvious enough to be worth stating, and should not be undone:

- **`BRMasterControlsModule.OnRPC` no longer has a `#ifdef CF_BUGFIX_REF` / `#else` pair.** The two branches differed only by the `ref` on `ParamsReadContext ctx`, and CF defines `CF_BUGFIX_REF` unconditionally (`JM/CF/Scripts/config.cpp:50`) while being a hard dependency — so the `#else` was already dead code. It is one unconditional `override` now.
- ⚠️ **`BattleRoyaleGameData.LoadMission`'s `lockedAdmins` snapshot is the one site where being wrong is a security hole**, not a cosmetic bug: it is what stops a mission pack granting itself `admins_steamid64`. The local is strong without `ref` like every other local, but any future change there deserves an explicit test that a mission `general_settings.json` still cannot override the admin list.
- **`Timeable.AddTimer` returns a plain `Timer`, and that is safe for a non-obvious reason.** DayZ's `TimerQueue extends array<TimerBase>` is **non-owning**, so the engine's timer list keeps nothing alive — the return `ref` looked load-bearing and was not. All 22 call sites already store the result in a `ref` member or pass `loop = true` (which inserts into the owning `m_Timers`); two in `7_BattleRoyaleLastRound.c` carry the original author's comment *"we need to store the object in case it's automatically deconstructed ?"*, which is somebody hitting this rule the hard way. Verified live: a looping lobby timer fired three times at 45 s intervals.
- **`0_BattleRoyaleState.c:1130` still reads `ref array<PlayerBase> players` and is correct** — it sits inside a commented-out block (lines 1111–1152), which is also why a naive grep over-reports this file by one.

⚠️ **Any future mechanical sweep of this repo must be checked at byte level, not from `git diff`.** The tree is CRLF and `core.autocrlf=true`, so both sides are normalised before comparison and a rewrite that silently converts 29 files to LF still shows as a clean diff. Reading a file with newline translation on and writing it back does exactly that. Compare `b.count(b'\r\n')` against `b.count(b'\n')`, and assert every changed line differs from its original by exactly one removed token — 120 lines is well past what review by eye will catch.

## Architecture

### Script modules and the Client/Server split

The engine compiles in fixed stages: `1_Core → 2_GameLib → 3_Game → 4_World → 5_Mission`. `config.cpp` maps a folder to a stage via `class defs { class <stage>ScriptModule { files[] = {...} } }`. Files are registered **by directory**, so adding a new `.c` file needs no config change; adding a new stage folder does.

**`Scripts/Client` vs `Scripts/Server` is not a runtime split.** Both PBOs ship in the same mod and load on both sides. What actually gates execution is the preprocessor guard on line 1 of each file:

- `Scripts/Server/**` — every file opens with `#ifdef SERVER`.
- `Scripts/Client/**` — `#ifndef SERVER` for client-only, **or no guard at all** for shared code that compiles on both sides (e.g. `BattleRoyaleConstants.c`, `BattleRoyaleUtils.c`, `BattleRoyaleBase.c`, `MissionBaseWorld.c`, `BattleRoyalePlayArea.c`).

When adding a file, pick the folder for organisation and **always add the right guard.** An unguarded file under `Scripts/Client/` runs on the server too.

`BattleRoyale_Scripts_Server` lists `BattleRoyale_Scripts_Client` in `requiredAddons`, so server files compile *after* client files in every stage — that ordering is what lets server code `modded class` over client-declared classes.

Compile-time feature flags live in `Scripts/Client/config.cpp:36-43` (`defines[]`). Only `DAYZ_BATTLEROYALE` is active; `BLUE_ZONE` and `BR_TRACE_ENABLED` are commented out but gate real code. **`MOVING_ZONE` gates nothing** — it is a reserved name with no `#ifdef` anywhere in the tree or in its history, and uncommenting it together with the line below it is a rapify syntax error (neither carries a trailing comma). The unmerged `moving-zone` *branch* is unrelated to it. Other `#ifdef`s in use: `SERVER`, `DIAG`, `VIGRID_PARTY` (the in-repo party addon, see below), `JM_COT`, `VPPADMINTOOLS`, `EXPANSIONMODMISSIONS`.

⚠️ **`BR_MINIMAP` and `EXPANSION_MAP_ZONES` are gone (2026-08-15), and both names are worth recognising** because a lot of prose still refers to them. They gated this mod's own minimap and its zone circles drawn into DayZ Expansion's map; both were commented out, both had rotted (the minimap layout was missing two widgets `BattleRoyaleHud.Init` dereferences, and the Expansion trio would not compile without @DayZ-Expansion-Navigation), and `Extra/Map/` has replaced everything they did. The five files, the two defines, the three `Data/Inputs.xml` entries and the `MissionGameplay` / `BattleRoyaleClient` blocks were deleted rather than repaired. `Scripts/Client/3_Game/ExpansionClientSettings.c` went with them — nine of the fourteen settings it forced were Expansion **Navigation** marker options, and it forced them from `OnSave()`, permanently rewriting the player's global Expansion settings file. **Do not re-add a mod-side minimap**: `VigridMapMinimap` behind `VIGRID_MAP_MINIMAP` is the one that ships.

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

**The win condition is a test on GROUPS, and it lives in exactly one place: `BattleRoyaleState.IsOneSideLeft(players)`.** A surviving party has already won, so a match that waits for one of them to kill the other never ends. Without the party addon — or with it compiled in but the manager switched off in `party_settings.json` — `GetGroupCount()` degrades to one group per player and the helper mirrors the raw player test, which is why no caller needs an `#else` branch. It is static and takes the roster explicitly, because `SkipState()` has to ask it about the state it is handed rather than about itself. Five call sites: `5_BattleRoyaleStartMatch.IsComplete`, and `IsComplete` + `SkipState` on both `6_BattleRoyaleRound` and `7_BattleRoyaleLastRound`.

⚠️ **It was written out four separate times and one copy had drifted**, which is the whole reason it is centralised now. `BattleRoyaleStartMatch` carried only the player half, and that state owns the entire first-zone countdown (`round_duration_minutes / 2`) — so a party that cleared the field during it still held two live players, the state never completed, and **the win screen did not appear until the countdown expired on its own**. Fixed 2026-08-14; the before/after is one line in the server log, the kill's `RemovePlayer` and `Start Match State IsComplete!` now landing in the same second where they used to be 16 s apart. The same pass fixed `7_BattleRoyaleLastRound.SkipState`, whose two tests were **AND**ed, so a surviving pair satisfied `players > 1` and refused to skip — the final round activated and deactivated inside a single tick, visible as a doubled `RemoveAllPlayers`/`AddPlayer` pair either side of `Entering State 'Last Gameplay State'`.

Note the lobby's own "enough groups to *start*" tests in `1_BattleRoyaleDebug` are the inverse condition and deliberately stay separate.

`BattleRoyaleState` extends `Timeable` (`Scripts/Server/3_Game/Logic/Timeable.c`) — use `AddTimer(duration, this, "MethodName", params, looping)`; looping timers are stopped automatically by `Deactivate()`.

Long-running work uses script coroutines — `GetGame().GameScript.Call(this, "MethodName", NULL)` plus `Sleep()` (see `4_BattleRoyalePrepare.ProcessPlayers`), not the call queue.

### Teleports and stuck players

**A player wedged in geometry has their inventory locked, and that is why they used to spawn naked.** Vanilla `PlayerBase.OnCommandFallStart` / `ClimbStart` / `LadderStart` / `SwimStart` each take a `LOCK_FROM_SCRIPT` lock on the character's inventory and only release it in the matching `...Finish` (`playerbase.c:3964-4085`). A player stuck mid-jump never finishes the command, so the lock is still held when `4_BattleRoyalePrepare` dresses everyone — and **a locked inventory refuses `CreateAttachment` silently, returning NULL**. `LocalDestroyEntity` is not an inventory move and ignores the lock, so the lobby clothes still went away: the symptom is "stripped and never re-dressed", not "still in lobby clothes". `ClearStuckMovementState` ends the command and drains any leaked lock count before dressing, every creation result is now checked, and `ProcessPlayers` re-dresses anyone left with zero attachments after the teleport. **A ladder is the deterministic repro** — same lock, reproducible where wedging yourself in a prop is not.

**Never start a movement command outside `CommandHandler`.** Vanilla only ever does it there and always `return`s immediately after (`dayzplayerimplement.c:2366-2400`, four times in a row). Starting one from a juncture handler produces a player who *looks* right but whose movement input drives nothing until some real command transition resyncs them — one jump was the cure. `BR_NotifyTeleported` therefore only sets a flag; the overridden `CommandHandler` consumes it and issues the transition — a `Fall` if the controller reports airborne, otherwise an unconditional `Move`, which is that jump without the jump. `BattleRoyalePrepare` keeps `BR_ForceMoveCommandImmediate` because it needs the inventory unlock synchronously and cannot yield, which is safe only because input is disabled for that whole state.

Both teleports (match start and F2 unstuck) go through **one** sync juncture, `BR_SYNC_JUNCTURE_TELEPORT` (88). Its server half repositions; a **client** half is also required, in `Scripts/Client/4_World/.../PlayerBase.c`, because the server half is `#ifdef SERVER` and the client would otherwise keep predicting the old command. The client does receive the juncture — verified by instrumenting both sides.

⚠️ **Never call `hic.SetDisabled()` on a player — it was the "ADS at the sky" desync, measured and removed 2026-08-18.** A player's aim angles are integrated **per instance** from per-tick input deltas (`HumanInputController.GetAimChange`), never synced as absolutes: the owner's and the server's copies track in lockstep to five decimals — until `DisableInput`'s `SetDisabled(true)` froze the SERVER's copy at its last value while the owner's kept moving (the teleport restart then rode the two to opposite ±85 pitch clamps). After re-enable every delta mirrors again but the pitch baselines stay ~80° apart **for the rest of the match**; other players render the *server's* copy, so the victim ADSes level and reads as aiming at the sky/ground on every other screen, self-fixed only by sweeping the aim through the pitch clamp (the folk workaround players discovered). `SetDisabled` has zero call sites in all of vanilla (the `OverrideAimChangeX/Y` unvetted-symbol class); the freeze is fully carried by the `Override*` calls beside it plus the lobby SafeZone. Two supporting measurements, both from the `[AimTrace]` instrument (`PlayerBase.BR_LogAimState`, diag *Teleport Trace → Trace Aim* toggles): **remote proxies run no script `CommandHandler` at all** (zero trace lines), so a proxy holds no aim copy and "fix the proxy's aim" designs are unbuildable — one such fix (a `NotifyPlayerTeleported` broadcast restarting proxy commands) was built on the stale-proxy theory, refuted by the first traced run, and reverted; and **the juncture's command restarts do not reset the aim angles** on the instances that restart.

**Known bug, deliberately left in (2026-08-09): an F2 unstuck taken *while on a ladder in the lobby* still arrives playing the ladder animation, pinned until the player jumps once.** Everything else is clean — the match-start teleport handles a laddered player correctly, position and animation both, and neither path hovers any more. The difference between the two is the lead: `4_BattleRoyalePrepare` already ended the command server-side in `ClearStuckMovementState` and has input disabled for the whole state, while F2 has only the juncture. **One hypothesis has since been tested and refuted.** The theory was that the client consumes the forced `Move` before the corrected position arrives, restarts Move while still at the ladder, and vanilla re-enters the ladder command. Re-issuing the `Move` every frame of the settle window for as long as `GetCommand_Ladder()` or `GetCommand_Climb()` was running changed **nothing at all** — not the symptom, not even its texture — and everything else stayed clean. Had the ladder command genuinely still been running, forcing Move at frame rate would have looked like *something*. So the command state after the teleport is most likely already correct and **what is stuck is the animation graph, not the command** — which is also why a jump cures it, a jump being a real animation transition. That experiment was reverted rather than kept.

The untried lead is `HumanCommandLadder.Exit()` / `CanExit()` (`P:\scripts\3_game\human.c:644-664`) — vanilla's own way off a ladder, which plays the exit transition instead of replacing the command underneath it. Note `Exit()` plausibly only works while still *at* the ladder, so it would have to run before the teleport rather than in the juncture, and `Unstuck()` already defers 1-3 s, which is where the sequencing would go. **Instrument before writing any of it**: log instance type, `GetCurrentCommandID()` and whether `GetCommand_Ladder()` is null on both sides at juncture receipt and for a few ticks after. That single measurement distinguishes "command stuck" from "graph stuck" outright, and one instrumented run has already settled this subsystem once where reasoning had failed twice. Three explanations have now been wrong here; do not make a fourth without data.

**The controller does not re-check its ground contact after a scripted `SetPosition`, so a teleport must never rely on the engine noticing a drop.** `BR_TELEPORT_DROP_HEIGHT` was briefly 1 m on the theory that a player dropped in would be airborne, fall, and land — and that the landing would reset the animation graph. `PhysicsIsFalling` read "not airborne" on both sides with the metre applied, which was known at the time and read too narrowly ("the Fall branch is not what fixes the unstuck" rather than "nothing converts this drop into a fall"). What shipped was a character hovering a metre up after *every* teleport, on both paths, until the player's first input dropped them. It is now `0.05` — a seating epsilon so the capsule does not start inside the surface, nothing more. Two things replace it: `CommandHandler` ends every teleport with a real command transition (above), and `BattleRoyaleDebugState.FindUnstuckPosition` runs its lobby-centre fallback through `IsSafeForTeleport` instead of taking a raw `SurfaceY`, which is the wedging the metre of clearance was really hiding. `BR_TELEPORT_SETTLE_SECONDS` (0.75 s) is why the airborne check is a window rather than one tick: on the client the juncture can arrive before the corrected position does.

### Zones

`BattleRoyaleZone` (server, `BattleRoyaleZone.c`) is a static registry; all play areas are generated **once per process** into `static ref array<ref BattleRoyalePlayArea> m_PlayAreas`. **Zone 1 is the largest** and the last zone is the smallest — `GetZoneSettingsIndex()` indexes with `i_NumRounds - GetZoneNumber()`. Radii **start** at `zone_settings.json` `static_sizes`; the other `shrink_type` values are declared but marked unused.

⚠️ **Radii are no longer simply `static_sizes` (2026-08-15, #241).** `allow_zone_size_flex` — **on by default** — lets a squeezed circle grow for one match, so the authority on "how big is circle *i* this match" is **`s_ChainRadii` during generation and `m_PlayAreas[i].GetRadius()` afterwards**, never `static_sizes[i]`. Three things make that cheap:

- **Every consumer outside `BattleRoyaleZone.c` already reads radii through `BattleRoyalePlayArea.GetRadius()`**, so flex reaches the states, the RPCs and the client with no change anywhere else. Verified by grep, and it is why the whole feature is contained to the generator.
- **Inside the generator, everything geometric reads `s_ChainRadii`** — `GetStepReach`, `CanChainComplete`, `GetChainPressure`, `TryPlaceLevel`, `SweepPlaceLevel`, `CommitChain` — via the one `GetChainRadius(i)` accessor. `a_StaticSizes` survives only as the admin's untouched input and as the thing growth is measured *against*.
- ⚠️ **`s_ChainRadii` is filled in `Init()` BEFORE `ComputeAllowRadii()`, and that ordering is load-bearing.** `ComputeAllowRadii`, `BuildFeasiblePOIList` and `PickSeedCenter` are all *defined through* `GetStepReach`/`CanChainComplete`, so they read the array whether they mean to or not. Left NULL, `GetStepReach` returns 0 and `a_AllowRadius` collapses to `W/2 − r_max` — **4305 m instead of 7579 m on ChernarusPlus** — and the POI filter then rejects nearly every village. No error, no warning: the same "wrong and plausible" failure texture as the aliasing bug documented in `ComputeAllowRadii`. Because `Init()` resets to static first, those three pre-filters always see the admin's sizes.

**Generation runs smallest first, and this is the single most surprising fact in the subsystem.** `m_PlayAreas[i]` is built from `static_sizes[i]`, so index 0 is the tight final circle and each later index is a bigger circle *containing* the one before it — the `i == 0` branch is the **last-played** zone, which is why `end_in_villages` and `restrict_final_zone` both live there. Consequently `static_sizes`, `static_timers` and `min_players` are all ordered smallest-zone-first, and `num_zones` selects that many tiers **from the small end**: lowering it shortens a match by dropping the *largest* circles while always keeping the tight endgame one. Entries past `num_zones` are unused by design (at the defaults, `static_sizes[6] = 4500` never plays); an array *shorter* than `num_zones` is a misconfiguration and is clamped by `Validate()` (below). `Init()` logs the window in use.

Round duration comes from `static_timers` plus a per-circle offset in `s_PlayAreaDurationOffsets`, filled by `CommitChain` when a circle lands far from its parent so players have time to cross — and, since #241, when a circle *grew*. It is indexed exactly like the settings arrays and is a static parallel to `m_PlayAreas` — it must not become an instance field again, since the circles are shared by every zone object. **The feature was dead until 2026-08-11** — its threshold was 1500 m while the longest possible step at the shipped sizes is under 1000 m, so the array had always been all zeros. `BR_ZONE_OFFSET_MIN_DISTANCE` is 600 m now, capped by `BR_ZONE_OFFSET_MAX_SECONDS`.

⚠️ **EVERY TIMING TERM IS COMPUTED IN `CommitChain`, FROM THE FINISHED CHAIN, AND NEVER ACCUMULATED DURING PLACEMENT.** There is no `+=` into `s_PlayAreaDurationOffsets` anywhere and there must never be one. Two independent reasons, and the second is the one that bites:

- Backtracking: a single scratch slot cannot survive it, because a re-rolled level's offset has to be discarded.
- **`RunSelfTest` calls `BuildChain` 50–200 times *after* `CommitChain` has already run, and deliberately never re-commits.** So an accumulator anywhere upstream folds hundreds of throwaway chains' payments into the array the live match is about to read, pinning every entry at `BR_ZONE_OFFSET_MAX_SECONDS`. Anybody following the documented acceptance gate (`zone_selftest_runs > 0`) would silently get **+120 s on every round of every match** — a self-inflicted wound delivered by the instrument meant to catch problems. Growth is therefore recovered by *diffing* `s_ChainRadii` against `a_StaticSizes` in `CommitChain`, which is state a throwaway run can never reach.

⚠️ **Growth is credited to index `level − 1`, not `level`, and getting that backwards is the natural mistake.** During the round whose settings index is *j*, players are inside circle *j+1* and must reach circle *j* — exactly what `6_BattleRoyaleRound.Activate` sends as `UpdateCurrentPlayArea` and `UpdateFuturePlayArea` respectively. Worst-case travel is `|c_{j+1} − c_j| + r_{j+1} − r_j`, so growing `r_L` makes round `L−1` **harder** and round `L` slightly *easier*. Paying index `L` pays the round that got easier and starves the one that did not. Note also the `dist <= BR_ZONE_OFFSET_MIN_DISTANCE` test no longer `continue`s the whole iteration: that would take the growth term with it, and levels 1–2 (spans 105–422 m, which can never cross 600 m) are precisely where growth fires most.

**Geometry-derived round timers are available but OFF by default** (`derive_timers_from_geometry`, #241 part 3, gated additionally on `shrink_type == 3`). `CommitChain` fills `s_PlayAreaDerivedTimers` unconditionally — so the diag zone table can always show what turning it on *would* do — as `travel / BR_ZONE_TIMER_SPEED_MPS / BR_ZONE_LOCK_FRACTION + BR_ZONE_TIMER_FIGHT_SECONDS`, clamped, where `travel = |c_i − c_{i−1}| + r_i − r_{i−1}` off the **real placed distance and the radii in force**. Four things worth knowing:

- The derive branch deliberately **does not add `GetDurationOffset`** — the formula already prices both the real distance and the growth, so adding the offset double-counts.
- **The endgame in `7_BattleRoyaleLastRound` keeps its hand-authored `static_timers[0]`** even when it is on: it plays out an already-locked circle, so there is no travel to derive from. It also reads that entry *directly* rather than through `GetZoneTimer`, which is why `BattleRoyaleState.GetMatchDurationSeconds` reads it the same way — the two must not be able to disagree.
- ⚠️ **The OPENING round used to keep `static_timers` too, and since #284 it does not — it is priced as a LOOT round.** See *Round pacing* below; the old "index `n−1` is excluded" guard is still there but is now only a backstop.
- **`BR_ZONE_TIMER_FIGHT_SECONDS = 125` makes "turn it on" pacing-neutral, and that is arithmetic, not feel.** Tier 1 draws distance uniformly in `[0.25, 0.85] × span`, mean 0.55, so typical travel is `1.55 × span`; on ChernarusPlus stock sizes at 6 m/s and a 0.80 lock the travel term is 34/136/182/363/363 s, so the allowance reproducing `static_timers` 155/260/307/495/495 is **121/124/125/132/132**. A first pass used the *max* step instead of the mean and landed on 100, which is ~8% short on the match players actually get.
- Indices 3 and 4 share a 1125 m span at stock sizes, so each lands anywhere in ~393–534 s on one roll — a 2:20 swing between two rounds that look identical. That variance is arguably the feature; the max clamp warns when it binds, because a bound clamp silently reintroduces the under-time problem the derivation exists to remove.

**Round pacing: the OPENING round is a LOOT round, every other round is a FIGHT round (#284 point 4).** `CommitChain` fills a third parallel array, `s_PlayAreaOpeningTimers`, with `BR_ZONE_TIMER_LOOT_SECONDS + (r_i × BR_ZONE_TIMER_OPENING_SPREAD / BR_ZONE_TIMER_SPEED_MPS) / BR_ZONE_LOCK_FRACTION` — clamped like the others, and computed **for every index**, because which index is the opening one depends on the player count and is not known until the countdown. `145 + 3375 × 0.5 / 6 / 0.80 = 497 s` against the shipped `static_timers[5] = 495`, so it is near-neutral by arithmetic in the same way `BR_ZONE_TIMER_FIGHT_SECONDS = 125` is. Four things are load-bearing:

- **The opening round has NO inbound travel, and that is why it needs its own formula.** `4_BattleRoyalePrepare` spawns everybody *inside* the circle that round is going to lock (`spawns_settings.spawn_in_first_zone`), so the "travel from circle `i+1` into circle `i`" model every other round uses has nothing to measure. What it costs is looting plus the sprint a player still owes inside that circle — hence `SPREAD`, half the radius on average.
- ⚠️ **It fixes a real bug, which is why "just exclude index `n−1`" was never enough.** Index `n−1` is the opening round *only* when the match starts at zone 1. With a dynamic starting zone the opening round is index `n−Z`, and it was being handed `derived[n−Z]` — the travel from circle `n−Z+1`, **a circle that was skipped and never played**. A plausible number for a journey nobody made.
- ⚠️ **`GetZoneTimer(bool is_opening_round)` takes the flag as a PARAMETER and must keep doing so.** `BattleRoyaleState.BoundMatchDuration` calls it once per candidate tier to price a whole match; if `GetZoneTimer` asked `GetDynamicStartingZone` which tier was the opening one, the two would call each other forever. All three callers already know the answer — `6_BattleRoyaleRound.Activate` compares its own `zone_num` against the starting zone (a test that already existed two lines below), `5_BattleRoyaleStartMatch.GetFirstZoneLockExtraMs` is by definition pricing that round, and the duration bound is iterating candidates.
- **`6_BattleRoyaleRound` re-reads its round length in `Activate()`, not in `Init()`.** `Init()` runs from the constructor inside `BattleRoyaleServer.Init()` at boot, before anyone has connected, so it cannot know which round is the opening one. `Init()`'s value stays as the boot-time default the trace lines and the admin zone table read.

**The tier table itself can be derived from the map and the population (#284 points 1–3), and it is OFF by default** (`derive_zone_ladder`, `bound_match_duration`). `static_sizes` stays the admin's input and **the generator is completely untouched** — what becomes derived is *which* tier a given population opens on, *how many* tiers exist, and *how long* the whole match runs. That is deliberate: circles are generated once per process in `BattleRoyaleServer.Init()`, long before anyone connects, and the state list is sized from `num_zones` at the same moment, so anything player-count-driven has to ride the existing starting-tier selection rather than regenerate the chain.

Two calibration anchors, both **measured against the shipped config** rather than chosen:

- ⚠️ **`min_players` is linear in RADIUS, not in area.** `3375/33 = 2250/22 = 1125/11 ≈ 102.3` m per player, with the four smaller tiers all sitting on a floor of 10. So `zone_metres_per_player = 102.3` plus `zone_min_players_floor = 10` reproduces `{10, 10, 10, 11, 22, 33}` exactly — **but only at `zone_poi_density_weight = 0`**, which is therefore the neutrality check to run when tuning it.
- **`num_zones = 6` falls out of `0.22 × W`.** `0.22 × 15360 = 3379` keeps `static_sizes` up to 3375 and drops 4500 — exactly the shipped value, on both ChernarusPlus and Sakhal. It is the same 0.22 `Validate()`'s advisory already recommends, and it is derived **in `Validate()`** because `BattleRoyaleServer.Init()` reads `num_zones` to size the state list before a circle is placed.

Four things about the loot-density term are non-obvious:

- ⚠️ **The factor is RELATIVE TO EACH MAP'S OWN MEAN, so it does not capture "this map is loot-poor".** Measured 2026-08-21, opening circle at r=3375: ChernarusPlus enclosed 53 POIs against a map mean of 1.21 POI/km² → factor 1.22 → rated 40 players; Sakhal enclosed **9** POIs against a mean of 0.25 → factor ≈ 1.0 → rated **33**. So Sakhal, with a sixth of the POIs, is rated for *fewer* players per circle rather than being handed a bigger circle — the normalisation divides the map-wide poverty straight out. That is a deliberate limitation, not a bug: it measures "is this circle richer than the rest of *this* map", which is the per-match signal. **The cross-map knob is `zone_metres_per_player`**, and `zone_settings.json` supports a mission override, so a per-map value costs no code.

- ⚠️ **A loot-RICH circle gets a HIGHER `min_players`, and that reads backwards.** `GetDynamicStartingZone` walks largest circle first and takes the first one whose rating is below the population, so raising a circle's rating makes the walk *pass over* it and settle on a **smaller** opening circle. That is the wanted behaviour: a dense region feeds the same crowd on less ground. The instinct — "a rich circle supports more players, so pick it more readily" — has the direction of the walk backwards.
- ⚠️ **At the default weight the derived table does NOT match the authored one, and that is expected.** Every circle is nested around a village seed (`end_in_villages`), so every circle reads denser than the map mean and the factor is above 1 for all of them, most at the small end. That part is a smooth function of radius, i.e. bias, and is absorbed by lowering `zone_metres_per_player`; what is left — the run-to-run difference between a chain that landed somewhere dense and one that did not — is the actual signal.
- **`BuildDerivedLadder` runs unconditionally**, in `GenerateAll` right after `CommitChain`, on the same reasoning as `s_PlayAreaDerivedTimers`: ~1700 squared-distance tests with no native calls, and it means the boot report and the admin zone table can show what turning the setting on *would* do. It is **not** in `CommitChain` — these are not timing terms, and `s_POI` is only guaranteed populated after `BuildFeasiblePOIList`.

**`zone_ladder_selftest_players` is the acceptance gate for all of it**, the counterpart to `zone_selftest_runs`: it walks player counts `1..N` at boot and reports the achievable duration range, the tier each population opens on, the circle's radius, the POIs inside it, players-per-POI and the resulting match length — one row per *change*, plus a **tier histogram** and a count of how often the duration bound moved the answer. ⚠️ **The histogram is the point, not the rows.** This repo has twice shipped a derivation that compiled, ran and never once changed the answer — `BR_ZONE_OFFSET_MIN_DISTANCE` at 1500 against a longest possible step under 1000, and `TryGrowLevel` sitting after a tier that had already accepted a looser bar. Both were caught by a counter and neither was visible from the code; a histogram with one populated bucket says the ladder is dead code however good the formula is.

⚠️ **A healthy histogram is NOT sufficient, and #284's own acceptance run is the reason that second counter exists.** At the first-written defaults (`match_seconds_per_player 30`, window `[900, 2400]`) the bound moved the tier for **100 of 100** player counts and made the largest circle unreachable at every population — because a full six-circle match is ~2560 s, above the 2400 s ceiling. The histogram still showed two healthy-looking buckets; what it was showing was the *clock's* tier boundaries, not the player count's. Hence `duration bound moved the tier for N of M`, a warning past half, and the reported achievable range. The shipped defaults are now `45` over `[1200, 2700]`, which measures at 20-31 of 100 on both maps.

⚠️ **That reported range is over LEGAL starting tiers, `1..floor_zone`, not `1..num_zones`** — `min_zone_num` means the walk can never start past `floor_zone`. Written the other way first, the gate reported a 394 s "shortest match" that the very next line of its own output contradicted with 1242 s, and advised fixing a setting that was fine. A self test that lies quietly is worse than none.

**Measured 2026-08-21 with everything on, 200 placement runs plus a 1..100 ladder walk on each map:**

| | ChernarusPlus | Sakhal |
|---|---|---|
| placement | 200/200, T1 973 / T2 25 / T3 0, growth 2, sweep 0, no backtracking | 200/200, T1 842 / T2 86 / T3 53, growth 7, sweep 25, depth2 ×3 |
| ladder range | 1209 s (zone 3) – 2470 s (zone 1) | 1317 s – 2676 s |
| tier boundaries | zone 3 → zone 2 at 42, → zone 1 at 55 | 45, 60 |
| histogram | zone1 46 / zone2 13 / zone3 41 | 41 / 15 / 44 |
| bound moved | 22 of 100 | 31 of 100 |

Placement is unchanged from the pre-#284 baseline on both, which is the expected result: `static_sizes` is untouched and the generator never sees any of this.

⚠️ **`BattleRoyaleState.GetDynamicStartingZone` and its memo are now `static`.** The memo's own comment was always the argument for it (the answer depends only on `num_players` plus process-fixed config), and it has to be static regardless: the ladder self test asks the same question at boot, before any state instance exists. `RunLadderSelfTest` calls `ResetDynamicZoneMemo()` afterwards for the same reason `RunSelfTest` restores `s_ChainRadii` — a diagnostic must leave no trace on the match about to be played.

⚠️ **`BattleRoyaleZone.GetZone(x)` now RECURSES for its parent instead of reading `m_Zones` directly.** A zone's settings index is derived by walking its parent chain (`GetZoneNumber`), so a zone built with a NULL parent silently believes it is zone 1 and reads the wrong radius, timer and `min_players`. The map read it replaces returned exactly that for anybody who asked for zone 3 before zone 2 — no error, no warning. Every caller happens to ask in ascending order, and #284 added two more that also do, which is the kind of invariant that holds until it does not.

**Generation cannot dead-end, and that is a property of the geometry rather than of a retry budget.** The world-fit boxes `[r_i, W - r_i]²` are nested and all share the map centre, so for nested convex sets containing a common point, stepping toward that point weakly decreases the distance to every one of them. "Step the maximum allowed straight at the map centre" is therefore the *provably optimal* continuation of a chain, not a heuristic — which makes `CanChainComplete` an exact oracle, in pure arithmetic with no native calls. Two things follow:

- Used as an **acceptance test on every candidate**, every circle it accepts is provably extendable.
- The first greedy step from any accepted position is itself always acceptable — the **witness step**. So every level has a move that cannot be rejected on geometric grounds.

`TryPlaceLevel` escalates through three tiers (arc, distance window and land requirement all loosening), then one deterministic 96-probe sweep, then the witness step. **Since #241 there is one more rung, and it is spliced in at tier 1 rather than appended:** when tier 1's window fails, `TryGrowLevel` re-runs *that same window and that same land bar* at a bounded larger radius before tier 2 gets to loosen anything.

⚠️ **Where that rung goes was settled by measurement after two wrong answers, and the wrong answer is the intuitive one.** Growth changes the annulus itself rather than the acceptance bar, so it reads like a last resort that belongs after tier 3. Built that way it **never fired once** — `growth 0` in 1000 placements on Sakhal, twice: first with a `pressure > 0` gate (refused all 47 calls, because the failures that reach tier 3 on Sakhal are *water* failures, where the chain is not squeezed and pressure is 0 by construction), then with that gate removed (still 0 in 62 calls). The reason is structural and survives any gate: by tier 3 the ladder has already accepted **0.10 land over a 180° arc and failed even that**, so re-asking for `zone_min_land_fraction` (0.60) is a *stricter* bar than the tier that just failed. Tier 1 is the only position where growth is a fair test — same window, same land bar, differing in the radius alone — and it is also literally #19's condition. This is the `BR_ZONE_OFFSET_MIN_DISTANCE = 1500` dead-feature trap repeating, and `zone_selftest_runs` is what caught it; reading the code would not have. `BuildChain` backtracks to re-roll a parent when a level fails, and abandons a seed for a different village when one keeps costing placements — that is the "rewind" behaviour. Termination is a proof, not a budget: each level's counter rises monotonically while its parent is unchanged, and at `BR_ZONE_LEVEL_RETRIES` it takes the witness step.

**What this replaced:** a single forward pass that drew 500 random candidates per circle and, on running out, called `GetGame().RequestExit(0)` to take the server down so it would restart and roll again. That was never bad luck — per-step travel is capped at a fraction of `(r_i − r_{i−1})`, which telescopes to a fixed total, so a final circle seeded further than that from where the opening circle can legally sit could **never** be extended. The 500 attempts were provably wasted every time. There is no `RequestExit` in `BattleRoyaleZone.c` any more.

Two consequences worth knowing:

- **`end_in_villages` POIs are pre-filtered by the same oracle**, and the count is logged at boot (`306 in CfgWorlds, 285 after the avoid lists, 255 chain-feasible`) — confirmed still exactly those figures on 2026-08-15, against `usable disc r=7478 m`. It removes nothing usable: every POI it drops was already a seed that ended in a shutdown. That number is the one to watch when tuning `static_sizes` for a new map. Note the pre-filter is deliberately computed from **`static_sizes`, not the flexed radii** — growth only ever enlarges a circle and is only accepted when `CanChainComplete` still passes, so a seed feasible under the static sizes stays feasible, and keeping the filter static keeps it a stable per-map number. (Sakhal for comparison: `60 in CfgWorlds, 59 after the avoid lists, 58 chain-feasible`.)
- **The seed walk must start at a random POI.** Deriving the start from `seed_attempt` made it deterministic and the first acceptable village in `CfgWorlds` order won every match. The self test reported 200/200 with no backtracking, which reads as healthy and was the tell — it now also reports the spread of the final circle and warns when it collapses.

**`zone_selftest_runs` in `zone_settings.json` is the acceptance gate**, and it is a setting rather than a diag entry so it runs on a headless dedicated server. It generates N throwaway chains at boot, reports the failure / backtrack-depth / tier distribution and the spread, then plays normally. 200 runs inside one boot answers "can this configuration dead-end on this map", which relaunching the server twenty times never could — and it costs ~5-9 ms per generation. Measured 2026-08-11 at stock `static_sizes`: Sakhal 200/200, tiers T1 803 / T2 120 / T3 60 / sweep 21, witness step never needed, backtracking exercised twice at depth 2; ChernarusPlus 200/200, T1 953 / T2 47, no backtracking.

**It also reports a `radius flex` column (#241), and that column is the reason the feature is not dead code** — see the tier-ladder warning above. Re-measured 2026-08-15 with `allow_zone_size_flex` both ways, 200 runs each, stock sizes:

| | ChernarusPlus | Sakhal |
|---|---|---|
| flex **off** | 200/200, T1 939 / T2 60 / T3 1, sweep 0, no backtracking | 200/200, T1 803 / T2 127 / T3 51, sweep 47, depth2 ×4 |
| flex **on** | 200/200, T1 945 / T2 52 / T3 2, **growth 1** (8.75 m), sweep 0, no backtracking | 200/200, T1 807 / T2 115 / T3 60, **growth 5** (516 m), sweep 43, depth2 ×5 |

Growth is therefore real but **rare at stock settings — ~0.1% of placements on ChernarusPlus and ~0.5-0.9% on Sakhal.** That matches what #241 predicted: this is robustness for unusual `static_sizes` and new maps, not a live defect being fixed. Surface cost of turning it on is +1.3% on ChernarusPlus and +18% on Sakhal, i.e. 1.8 s → 2.1 s for 200 runs.

⚠️ **Sakhal's run-to-run variance is larger than most effects you will want to measure against it, and this is the number that proves it.** Two 200-run self tests of the *identical* build gave **sweep 43 vs 186**, 246k vs 617k surface calls, 1062 vs 1299 placements, and depth-2 backtracking ×5 vs ×18. So a single Sakhal self test cannot resolve a 1-in-1000 difference in the tier histogram, and a pre/post comparison on one run each is worthless. What *is* stable across every run is the part that matters: 200/200 completed, zero hard failures, and a final-circle spread covering most of the map. Judge a change on those, and use ChernarusPlus (11.2k-11.4k surface calls, no backtracking, run after run) when you need a quiet instrument.

`zone_generation_seed` (0 = off) replays one layout. Note it reseeds the **global** RNG, so a non-zero value also fixes loot, weather and spawn placement; at 0 a fresh seed is drawn and logged each boot, so a run is replayable without changing anything observable.

**Misconfiguration clamps rather than halting boot.** `BattleRoyaleZoneData.Validate()` — a new `BattleRoyaleDataBase` hook called after *both* the profile and mission passes — clamps `num_zones` to the shortest settings array, to the longest strictly-increasing prefix of `static_sizes` (a non-increasing pair makes the span ≤ 0 and silently breaks containment), and to what fits the world. It **must never `Save()`**: `Load()` re-saves before the mission pass, so persisting a clamp would overwrite the admin's file permanently. Verified — a boot with `num_zones: 12` clamps to 3, warns, reaches the lobby, and leaves `12` in the JSON.

**The distance window is a fraction of the SPAN, not of the new radius, and that is deliberate (#241 part 1, decided 2026-08-15).** #19 asked for the old centre to sit at 25–75% of the *new radius*; `DAYZBR_ZS_MIN/MAX_DISTANCE_PERCENT` are instead 0.25–0.85 of `span = r_i − r_{i−1}`, the legal containment reach. Keep it that way: the span **is** the containment constraint, so #19's second goal ("avoid the two zones' border collision") holds by construction rather than by tuning, and it is enforced independently by `FitsWorld` + `CanChainComplete`. The max was also deliberately raised 0.75 → 0.85 — a quarter of the legal reach was otherwise discarded for free, and off-centre circles create useful rotation pressure. No code change was made for this; it is recorded so the next reader does not "fix" it back.

**Size the opening circle to the map: `r_max ≈ 0.22 × W`.** PUBG's Erangel is 8 km with a ~2 km first circle. Past `0.25 × W` the opening circle's centre is pinned near the map centre every match (`Validate()` warns), because `W/2 − r_max` is all the freedom it has. `zone_settings.json` supports a mission override and the mission is per-map, so per-map sizes need no code; `scale_sizes_to_world` (off by default) does it automatically, holding the final circle fixed and scaling only the span above it — a flat multiply would shrink the endgame, whose size is a function of how many players are left, not of the map.

⚠️ **Sakhal is 15360 m, not 8192** — its difficulty is water, not the world box. Do not re-derive the geometry from a wrong world size.

Within a round, the new circle only becomes the damaging boundary at `BR_ZONE_LOCK_FRACTION` (0.80) of the round timer (`LockNewZone`); before that `GetActiveZone()` returns the previous zone. **The endgame is a separate figure** — `7_BattleRoyaleLastRound` locks at `BR_ZONE_ENDGAME_LOCK_FRACTION` (0.5), so the two must not be collapsed into one constant. Both were inline literals until #241; they are named because `derive_timers_from_geometry` divides by the first one to size a round, and a derivation that could drift from the state consuming it would be sizing a travel window players never get. Damage is applied per player from `PlayerBase.OnScheduledTick` → `BattleRoyaleServer.OnPlayerTick` → `GetCurrentState().OnPlayerTick`, scaled by zone index.

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

**Match state is never sent as a state id.** The server pushes discrete facts (`StartMatch`, `SetPlayerCount`, `SetCountdownMs`, `UpdateCurrentPlayArea`, `UpdateFuturePlayArea`, `SetFade`, `SetInput`, `SetTopPosition`, …). Each lands as a plain field on the client singleton **`BattleRoyaleRPC`** (`Scripts/Client/3_Game/BattleRoyale/RPC/BattleRoyaleRPC.c`), and `BattleRoyaleClient.Update()` polls those fields every frame, diffing against `br_previous_*` locals to detect edges.

**The countdown clock is the one field that is NOT a value the client holds — it is a DEADLINE the client mints.** `SetCountdownMs` carries **milliseconds remaining**, and `BattleRoyaleRPC.SetCountdownMs` immediately turns it into `countdown_deadline_ms = GetGame().GetTime() + payload`; `BattleRoyaleClient.Update()` subtracts that from the current time every frame. Five things about it are load-bearing, and each was a bug before it was a rule:

- **The client must never count down locally.** It used to be told a whole number of seconds once per phase and decrement it from a 1 Hz `CallQueue` tick, so the quantisation error accumulated across a three-to-five minute round — *and each client accumulated its own*. The zone locked several seconds before the HUD reached `00:00`, and no two screens agreed. `OnSecond` is gone; there is nothing left that ticks.
- **The payload is milliseconds precisely because the server re-asserts it.** `BattleRoyaleState.ResendGameInfo` fires every 5 s. At whole-second resolution each resend would move the latched deadline by up to half a second, so the displayed digit would visibly repeat or skip — trading smooth-but-wrong for correct-but-jittery. In ms a resend is a no-op correction.
- **Latching at packet arrival is what removes the edge.** The reader has nothing to edge-detect: it recomputes unconditionally and is right whether a push landed this frame, five seconds ago, or never. Same rule as the map's marker layer, where a snapshot that raised no edge waited out a one-second watchdog.
- **Every send goes through `BattleRoyaleState.SendCountdown(Timer, int zone_extra_ms = 0)`**, which pushes the RPC *and* records which timer the countdown belongs to so the 5 s resend knows what to re-assert. Pass `NULL` to clear it. Its `IsRunning()` test is not defensive: a one-shot `Timer` that has already fired sets `m_time = 0` **before** invoking its callback (`TimerBase.Tick`, `P:\scripts\3_game\tools\tools.c`), so `GetRemaining()` on a fired timer hands back its **full duration** — drop the guard and the countdown restarts from the top every 5 s. The five call sites are `5_BattleRoyaleStartMatch.Activate`, `6_BattleRoyaleRound.Activate` + `.LockNewZone`, and `7_BattleRoyaleLastRound.Activate` + `.LockFinalZone`. Each `Activate` send sits **below** its `AddTimer`, since the helper reads the remaining time off the timer.
- ⚠️ **The payload is `Param2<int,int>`: the countdown, and the deadline the HUD colours the CLOCK against — and those are NOT the same number in every state.** During `5_BattleRoyaleStartMatch` the countdown runs out when the first *round* starts, after which the player still gets `BR_ZONE_LOCK_FRACTION` of round 1 before the circle bites: 536 s of real allowance behind a 140 s readout at the shipped defaults. Colouring against the readout over-warns by ~3.6× — red, meaning "you cannot make it", whose correct response is the wrong play. Three things follow:
  - **It is expressed as an EXTRA to add to the countdown, defaulting to 0**, so four of the five call sites say nothing and send exactly the countdown. Only `StartMatch` passes one (`GetFirstZoneLockExtraMs`). That default is what keeps the colour bit-for-bit unchanged in every other phase.
  - **It rides this RPC rather than one of its own.** The two are halves of one fact: sent together they can never disagree, `ResendGameInfo` has one thing to keep in step rather than two, and there is no second `AddRPC`/handler/`Reset()` invariant. `i_CountdownZoneExtraMs` is stored on the state so the 5 s resend re-asserts it — drop that and every resend flattens the warm-up's deadline back onto the countdown.
  - **It is deliberately NOT "milliseconds until the circle bites".** At `LockNewZone` the circle already bites, so that reading is 0 and would pin the clock red for the rest of the match. *"The deadline the colour is measured against"* is one meaning that stays true at all five sites. The client falls back to `countdown_deadline_ms` when it is unset, which is what makes the whole thing additive for offline and diag sessions.

Note `2_BattleRoyaleCountReached` holds an `m_StartTimer` but has never driven the HUD countdown, so the pre-match lobby countdown shows no timer widget — `SendCountdown( m_StartTimer )` would add one.

⚠️ **Nothing tells a client where the safe circle is until `6_BattleRoyaleRound.LockNewZone`, and every HUD affordance used to be gated on that.** `5_BattleRoyaleStartMatch.ShowFirstZone` sends only `UpdateFuturePlayArea`, and the first round's `UpdateCurrentPlayArea` is guarded on `GetPreviousZone()`, which is NULL — so `m_CurrentPlayArea` stays NULL for the warm-up **plus the first 80% of round 1**, ~9 minutes at the defaults. That is *correct*: before the lock nothing is lethal. What was wrong is that `BattleRoyaleClient.Update` read it as "no zone information at all" and called `HideDistance()`, which also killed the clock colour, since `BattleRoyaleHud.SetDistance` is the only thing that ever paints `m_CountdownTextWidget` / `m_ImageClock`. A future circle with no current one now measures against the future one. Two rules fall out:
- **The out-of-zone PPE tint stays keyed to `m_CurrentPlayArea` alone.** Pointing it at the future circle would start the you-are-dying signal up to nine minutes early. The two `GetZoneDistanceFrom` calls look redundant and are not.
- **`BattleRoyaleHud.ShowDistance(false)` repaints the clock white.** The clock lives in `CountdownPanel` but was coloured from the *distance* panel's code path, so a red clock stayed red for the rest of the session once the arrow went away — which `7_BattleRoyaleLastRound.LockFinalZone` does while a countdown is still running.
- **The diag fixture needs `Fake Zones: Pre-Lock`** (`BattleRoyaleDiag.zones_fake_no_current`). The plain fake rig sets *both* circles and therefore cannot reach this branch at all — the same trap as the leaderboard fixture that fitted its viewport.

Two secondary channels: `ScriptRPC` with the `BattleRoyaleCOTStateMachineRPC` enum (`Scripts/Client/2_GameLib/BattleRoyaleEnums.c`) for the COT admin module, and sync juncture id `88` for teleports (`Scripts/Server/4_World/Entities/ManBase/PlayerBase.c`).

### Settings (JSON)

Server-side only (`Scripts/Server/3_Game/Config/`, all `#ifdef SERVER`). `BattleRoyaleConfig` is a singleton holding `map<string, ref BattleRoyaleDataBase>`; reach it with `BattleRoyaleConfig.GetConfig().GetGameData()` etc.

| File in `$profile:Vigrid-BattleRoyale\` | Class | Registry key | Scope |
|---|---|---|---|
| `general_settings.json` | `BattleRoyaleGameData` | `GameData` | general match-flow settings not owned by another file below; also `admins_steamid64` (mission-locked) and the three spectate switches |
| `lobby_settings.json` | `BattleRoyaleLobbyData` | `LobbyData` | pre-match lobby flow: ready-up, autostart, spawn selection, forced team size |
| `zone_settings.json` | `BattleRoyaleZoneData` | `ZoneData` | zone geometry, shrink timing, zone damage, shrink-notification timing, per-match radius flex, derived round timers, the derived tier ladder and the match-duration bound |
| `voice_settings.json` | `BattleRoyaleVoiceData` | `VoiceData` | party-only voice while frozen, speaking-list panel |
| `spawns_settings.json` | `BattleRoyaleSpawnsData` | `SpawnsData` | lobby spawn point and match spawn placement |
| `pois_settings.json` | `BattleRoyalePOIsData` | `POIsData` | POI position overrides |
| `server_settings.json` | `BattleRoyaleServerData` | `ServerData` | Vigrid API/webhook + autolock infra + Steam name lookup — **no mission override** |
| `leaderboard_settings.json` | `BattleRoyaleLeaderboardData` | `LeaderboardData` | scoring curve + persistence knobs — **no mission override**, integrity-sensitive |

The mission override (`$mission:Vigrid-BattleRoyale\`) is **not a merge** — `JsonFileLoader<T>.LoadFile()` is called twice into the same instance (`Load()` then `LoadMission()`), so only keys present in the mission JSON get overwritten. `Upgrade()` runs inside `Load()`, before the mission pass, and `Save()` only ever writes the profile path.

A field can also be locked out of mission override *within* an otherwise-overridable file: `LoadMission()` snapshots it before the deserialize call and restores it right after. `BattleRoyaleGameData.admins_steamid64` is the example — general_settings.json supports mission overrides, but the admin list is a server-operator concern, not mission content, so it's exempted. Reach for the same idiom for any future field that needs this.

Each class carries `int version` plus an `Upgrade()` migration. `Load()` re-saves after reading, so new fields appear in existing profile JSONs on next boot. Moving a field to a different settings file is *not* treated as a migration — the field starts from its new class's default and the old key is left inert in the old file.

### Player names

A player who never set a name in the launcher connects as `Survivor`, and the engine turns a second one into `Survivor (2)`. **`PlayerIdentity` cannot be renamed** — every accessor is a getter and there is no `SetName` (`P:\scripts\3_game\gameplay.c`) — so the corrected name lives mod-side in `BattleRoyaleNameService` (`Scripts/Server/4_World/Names/`, all `#ifdef SERVER`), keyed on SteamID64, resolved from the Steam Web API by `SteamNameWebhook` and gated on `enable_steam_name_lookup`. Vanilla surfaces that read the identity directly — in-game chat, the vanilla player list — are out of reach and still show `Survivor`.

**Two maps, and the split is the whole design.** `s_Overrides` is what is *in force*: every `Resolve*()` reads it and nothing else, so a uid absent from it is shown under the name it connected with. `s_Cache` is what is *known*: every persona ever resolved, dated, persisted to `$profile:Vigrid-BattleRoyale\steam_names.json`. A cache entry is promoted into `s_Overrides` **only by a connect that is actually wearing a placeholder name** — they were one map until it was noticed that a player who once joined as `Survivor` and has since set a name of their own was *still* being shown the resolved one, because the warm-cache branch of `RequestForPlayer` ran before the placeholder test and "we have an answer for this uid" was standing in for "this uid still needs one". Three consequences worth keeping:

- **`RequestForPlayer` must run before `player_name` is seeded** in `BattleRoyaleServer.OnPlayerConnected` — it is where the override is dropped, and a `ResolveIdentity()` taken first would bake the stale name in with nothing left to undo it.
- **The clear has to reach clients too.** A client already online holds `resolved_by_uid[uid]` for its whole session, so `ClearOverride` broadcasts `SetResolvedName` with an **empty name** — the wire contract for "drop any override for this uid", handled by `BattleRoyaleRPC.ClearResolvedName`. A client connecting *later* needs nothing: `SendAllResolvedNames` walks the online players against `s_Overrides`.
- **The disk entry is kept** when a player stops using a placeholder. It is still a true steamid→persona record and re-applies for free if they go back.

`steam_name_cache_max_age_hours` (`server_settings.json` v4, default 168, **0 = never**) ages the cache: past that, a connect that was going to use the cache **applies the cached name immediately and queues a refresh anyway**, so nobody waits on the request and a changed persona is picked up. An undated entry — a v1 cache file, migrated in — counts as stale, so each is re-asked exactly once. The `s_Requested` guard covers the refresh path too: it lets one lookup out per process and stops a permanently unanswerable one (a private profile) re-queuing on every reconnect.

Application is `WriteThrough`: the mod's own `PlayerBase.player_name`, vanilla's protected `m_CachedPlayerName` (via `BR_SetCachedName`), and COT's `JMPlayerInstance.m_Name`. That middle one is the lever that reaches code this mod does not own — `Party` and `KillFeed` both prefer `GetCachedName()` and pick the corrected name up without ever naming a `BattleRoyale*` symbol. `PluginAdminLog` is **not** covered by it and needs its own `modded class` (`BattleRoyaleAdminLog.c`), because vanilla only falls back to the cache once the identity is gone.

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
- ⚠️ **`#ifdef VIGRID_PARTY` says the addon is COMPILED IN, not that parties are in play** — and since Party ships in this repo the `#else` branch of every one of those guards is dead on every real server. `party_settings.json` still has an `enabled` flag, and with it off each grouping query falls back to the solo partition: `GetGroupCount()` returns one group per player, i.e. a number always identical to the player count. That degradation is *correct* for the logic tests (`<= 1`, `> 1` — round-end and skip conditions), which is why almost every call site can ignore this. It is wrong for anything that **displays** a group figure, where the same number twice under a group icon is worse than no figure at all. Ask **`VigridPartyAPI.IsReady()`** before believing a group count you are about to show. This is the mechanism behind #158: it left `BattleRoyaleHud.SetCount`'s `BR_HUD_GROUPS_NONE` sentinel unreachable, and it was invisible because the guard *looks* like it covers the case.
- **With the manager off, the three binds go inert client-side, and the gate is `VigridPartyAPI.IsClientReady()`** (#286). `enabled` already reaches the client: `VigridPartyManager.OnPlayerConnected` calls `SendSettings` *above* every gate, so `VP_Settings` carries the truth on connect and on each `VP_RequestSync` — no new wire format was needed, and this is `IsClientReady()`'s first caller. **P is gated in `PartyMissionGameplay.HandlePartyOpen`, never in the menu's `OnShow`**: a menu that opens and immediately closes has already eaten the keypress and already run `LockControls`. `HandlePartyClose` carries the same test with no key pressed, because the client's copy of `enabled` **defaults TRUE** until the first `VP_Settings` lands — a press inside that window would otherwise leave the menu open for good. T is silent for `!enabled` and keeps `STR_PARTY_PING_DISABLED` only for `!ping_enabled`, since naming the ping switch on a server with no party system points at the wrong setting. ⚠️ **Offline this path is unreachable** — no `VP_Settings` ever arrives, so `enabled` sits at its default and the diag fake-session rig is unaffected; testing it needs a server with `party_settings.json` → `enabled: false`.
- Identity is `PlayerIdentity.GetPlainId()` (SteamID64) throughout — never `GetPlayerId()`, a session index the engine reuses after a disconnect. Note `MissionServer.PlayerDisconnected` is handed `GetId()` (hashed), so the manager keeps a `GetId()` → `GetPlainId()` table.
- `VigridPartyAPI.SetFormationLocked(true)` is called from `1_BattleRoyaleDebug.Deactivate()`. While locked, every composition change is refused — otherwise a player leaving mid-match would raise the group count and stall the round-end condition.
- Files: `$profile:Vigrid-Party\party_settings.json` and `parties.json`. Parties are persisted because the server process restarts between matches.
- **`GetPlayerByUid` is memoized per millisecond** (`VigridPartyManager.c`) — a `uid → PlayerBase` index over `GetGame().GetPlayers()`, rebuilt on the millisecond and forcibly on `OnPlayerConnected` / `OnPlayerDisconnected`. It replaced a full population walk *per call*, which the recurring paths were paying once per member: `PushTeamState` twice per member of every party every `state_push_interval_ms`, `BroadcastRoster` twice (once directly, once through `NameOfUid`). **The client twin `VigridPartyAPI.FindLocalPlayer` also keys on the population count; do not add that here** — nothing in the engine reports a server population count, and both `GetPlayers` and `GetPlayerIndentities` fill an `out` array, so reading it means doing the walk the index removes. The two invalidation hooks are what replace it, and neither is optional: `OnPlayerConnected` broadcasts a roster resolving **the joiner themselves**, and `OnPlayerDisconnected` runs `FirstOnlineOther` to pick a new leader. Measured live: 1.11 walks per push against 4 lookups.

**⚠️ Known deviation, deliberately left in (2026-08-14): eight call sites in `VigridPartyManager.c` pass an array read straight into a call** — `GetPlayerByUid(party.member_uids.Get(i))` at `:627`, `:823`, `:1079`, `:1178`, `:1295`, `:1434`, `:1875`, and `DropMemberIndex(...)` at `:588`. That is the container-aliasing shape measured to silently read the **wrong array** in `BattleRoyaleZone` (`a_AllowRadius.Get(i+1) + GetStepReach(i+1)` returned `a_StaticSizes` entries), and which this same file already guards against at `:1380-1383` — `PushTeamState`'s payload loop reads the uid into a local first and says why, while its *recipient* loop three lines later does not. **None has been observed misbehaving**, and the triggering conditions are uncharacterised, so this is hardening against a real-but-poorly-understood engine behaviour rather than a diagnosed bug. Fix is mechanical (read the element into a local on its own line before the call) but unverifiable by observation — nothing changes visibly — so it rests entirely on matching the rule. Note the line numbers moved when the memo above was added; re-grep rather than trusting them.

**Forced team size (duos, trios, …).** `lobby_settings.json` `min_party_size` (v3, default **1** = off). At 2+, `1_BattleRoyaleDebug.Deactivate()` calls `VigridPartyAPI.AutoGroup` on the line *above* `SetFormationLocked(true)` and fills everyone into a party of at least that size. `VigridPartyAutoGroup.Plan` decides, `VigridPartyManager.AutoGroupPopulation` applies. Order: top up undersized existing parties → form new parties of exactly `min_size` → deal with the remainder per `min_party_remainder` (0 absorb / 1 short-handed party / 2 leave solo). A party already at `min_size` is never touched and never split. Five things are load-bearing:

- ⚠️ **`BR_AUTO_GROUP_MIN_GROUPS` (2) is a floor on the resulting group count and it OUTRANKS `min_party_size`.** Every round-end and skip condition ends the match at `GetGroupCount() <= 1`, so a pass that merged the lobby into one party would end the match on its first tick. Each merge is gated individually because they cost different amounts — folding one solo into a party costs 1 group, fusing `min_size` solos costs `min_size - 1` — which is why a blocked *new party* `break`s into the remainder step rather than returning: the floor can forbid the expensive merge while still allowing the cheap one.
- **Auto-placed members must not persist.** They are recorded in `VigridParty.auto_uids`, session-scoped and absent from `VigridPartyStoreEntry` exactly like `pings`, and `VigridPartyStore.Save()` subtracts them — so a fully auto-formed party never reaches `parties.json` and a real duo that was topped up is stored as the duo it was. `Save()` also has to repair the leader: `leader_transfer_on_disconnect` promotes the first *online* member, which can be an auto one.
- **The pool is shuffled, and that is the whole of "the leader is random"** — `VigridParty.Add` makes the first member in the leader and the plan fills each new party in pool order. Topping up never moves an existing party's leadership.
- **A party is sized by how many of its members are actually in the population**, not `party.Count()`. One whose third player never connected is a duo this match, and topping that duo up is the point.
- **`auto_group_selftest`** (lobby_settings.json, 0 = off) runs the planner over synthetic populations at lobby open and checks two invariants — conservation, and that a pass starting above the floor never finishes below it. Same reasoning as `zone_selftest_runs`: the cases that can strand a player are combinatorial and `LaunchLocalMP.bat` tops out at three clients, which cannot produce a four-way split, a top-up that also leaves a remainder, or the `max_party_size` overflow. It runs from `BattleRoyaleDebug.Activate()` rather than `BattleRoyaleServer.Init()` because nothing pins the order of the two modded `MissionServer.OnInit` overrides, so the party manager may not exist yet at `Init()`.
- RPC namespaces `RPC-VigridParty` / `RPC-VigridParty-Server`, message names `VP_*`. CF's `AddRPC` dispatches by **method name**, so a handler method must be named exactly like its registered string.
- Keybinds live in `Party/Data/Inputs.xml`, declared by a second `inputs=` in `Party/config.cpp`: `UAVigridPartyMenu` (P), `UAVigridPartyPing` (T), `UAVigridPartyPingClear` (Y). Read them with `GetUApi().GetInputByName(...)`, not the generated constants. **Y also toggles Community-Online-Tools' sidebar** (`UACOTToggleButtons`, plain `kY` in COT's own `Inputs.xml`) — an admin pressing it gets both; rebind if that bites.
- **`PartyMissionGameplay.OnUpdate` is the same three-part shape as the map's**: `HandlePartyClose()` (returns `bool`, short-circuits) → a single `if (m_UIManager.GetMenu()) return;` gate → `HandlePingInput()` + `HandlePartyOpen()`. Esc is **polled here as `UAUIBack`**, not left to the engine — while any scripted menu is open `MissionGameplay.OnUpdate` never reaches its `Pause()` branch, so Esc over the menu is a dead key. The gate is "any menu at all", never a list of ids: the list it replaced named vanilla's `MENU_MAP` rather than the Vigrid one, so P/T/Y still fired under the Vigrid map, the inventory and the in-game Esc menu — and `EnterScriptedMenu` passed `GetMenu()` as the **parent**, which made whatever was open the party menu's parent and handed focus back to it on close. The parent is `NULL`. It stays id-agnostic on purpose: Party must not name `MENU_VIGRID_MAP`. Unlike the map, `VigridPartyMenu` keeps `UseKeyboard() == true` and so keeps vanilla's three-device `LockControls()` — the player is meant to be frozen while it is open, so none of the map's `Supress()` work applies here.
- **Every player-facing message goes through vanilla `NotificationSystem.AddNotificationExtended`, never chat** (#275) — a player who has chat off must still see an invitation. Four sites in `VigridPartyClient.c`, funnelled through one `NotifyExtended` helper; nothing else in the addon talks to the player. Five properties of that system are load-bearing:
  - **`AddNotificationExtended` dereferences its static `m_Instance` with no null check** of its own, and this runs every frame from `Update()` — an unwind there would take the name tags, the pings and the HUD panel with it. Hence the one guard, even though `DayZGame.OnInitialize` builds the instance long before any mission exists. **Its fallback must not be chat.**
  - **`show_time` is quantised to whole seconds** by `Update`'s 1.0 s accumulator, and the element then fades for a further 3 s, so time on screen ≈ the figure plus three. A fractional duration buys nothing.
  - **Only five are visible**; the sixth onward is deferred and released **LIFO at about one per second**.
  - **`Detail` carries `wrap 1` and `Title` does not**, so anything of variable length goes in the detail line.
  - **`AddNotification` (the enum form) is unusable here** — it needs a `NotificationType` that only a `modded enum` *plus* an entry in **vanilla's own** `scripts/data/notifications.json` can supply, and an unregistered type renders the literal `please_add_a_title`.

  Two consequences of the five-slot cap are written into the code. The invitation is deliberately **one** notification, prompt as title and keybind hint as detail: with two, the LIFO deferral drops the **hint**, telling a player they were invited without telling them which key answers it. And `Announce` **rate-limits repeats of the same key** (`VIGRID_PARTY_NOTIFY_REPEAT_MS`), because it is the only message a player can trigger at will — `PlacePing` answers `PING_DISABLED` / `PING_NO_PARTY` *before* its cooldown check and `PING_NO_TARGET` before the cooldown is set, and `ClearPings` has no cooldown at all, so a held key mints one per press. `DrainNotifications` by contrast drains the whole queue in one pass on purpose: the server emits one message per event per recipient and the largest co-arriving set in the tree is two (`DISBANDED` beside `NEW_LEADER` on a leader's disconnect), so five is unreachable from the wire and pacing would only delay every message.

  **No settings toggle, deliberately.** `party_settings.json` is a *server* file and "I turned chat off" is a *client* preference, so a server field cannot express it; the right shape if it is ever wanted is a client profile option. And the only thing a toggle could switch back to is the chat this replaced. The icon needs **no `imageSets[]` entry** — `"set:dayz_gui image:..."` is handed to `ImageWidget.LoadImageFile` at runtime, which `VigridPartyHud` already relies on. ⚠️ **The fake-session rig cannot reach `DrainNotifications`**: every other `Debug*` path applies itself locally and only `VP_Notify` fills that queue, so *Party → **Fake Notifications*** exists to push a burst of eight — more than five on purpose, since a fixture that fits cannot reach the deferral it is meant to exercise.

**Both of the party menu's columns are sorted by displayed name, and the sort is a DISPLAY ORDER — nothing reorders `list_*` or `roster_*`.** A roster slot index *is* the member's palette colour in five other surfaces (nametag, HUD row, map marker, ping, compass caret) and slot 0 is what "the leader is the first member" means server-side, so `BuildOnlineOrder` / `BuildMemberOrder` produce an array of **data indices** instead; the pooled rows keep their creation order and only the content written into them moves, which is also why nothing has to be relinked under the `WrapSpacer`. Four decisions worth not undoing:

- **The leader is pinned at the top**, the rest sorted below. A party is a handful of people, so findability is not what that column needs — what it needs is the one row that is not interchangeable, since the leader is who every Invite comes from and whose identity decides what the buttons on your own row do.
- **Lexical, not natural order.** Real player names do not end in digits; the `Fake 1, Fake 10, Fake 2` reading that invites a natural sort was an artefact of the old fixture, which now uses non-numeric names.
- **Ties break on uid.** Two players called `Survivor` is the ordinary case with `enable_steam_name_lookup` off, and a merely stable sort would leave exactly those two churning in arrival order — the players hardest to tell apart.
- **The member column sorts on `GetMemberName`, never `roster_names`**, since an offline member's name can arrive as a stringtable key and only that accessor resolves it. The online column sorts on `list_names` as-is: the `BattleRoyaleNameService` correction is applied server-side, so the string already on the wire is the one on screen.

The comparison is written out by hand (`CompareStrings`): EnfusionScript's `string` exposes no comparison at all, and the one native sort there is (`array<string>.Sort`) reorders the array in place, which is the one thing that must not happen here.

⚠️ **The diag fixture used to be sorted by construction and so could not fail this test** — `"Fake 1".."Fake N"` generated in index order, the same trap as the 40-row leaderboard fixture authored in rank order. `DebugSetRoster` / `DebugSetPlayerList` now draw from two jumbled, mixed-case name pools carrying a deliberate duplicate pair. Note the fixture still cannot separate the leader pin from "no sort at all" on its own, because you are slot 0 *and* the leader: press **Promote** on a teammate, or take the **Invite Me** route, which lands you at slot 1 in somebody else's party.

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

#### The audience counter

A living player is shown how many people are currently spectating **them** — an eye-and-number row
on the HUD, hidden entirely at zero. It is the **last** row of `MatchInfoIconsPanel`, below the
zone arrow, and that is deliberate: the grid reflows when a child is hidden, so a row that comes and
goes mid-match shifts everything under it — put at the bottom it can move nothing, which is what
keeps the clock and the distance arrow still. Gated on `show_spectator_count`
(`general_settings.json` v6, defaults **on**); `spectate_enabled` already decides whether anyone can
spectate at all, so on a server that never offers it this is never reached.

⚠️ **The filter is `entry.is_admin` — the SESSION TYPE — and never membership of
`admins_steamid64`.** An admin in *admin* spectate must not be counted: an operator watching is not
part of the game, and surfacing it tells a player the exact moment they are being observed, the same
leak the invisible anchor body and `SetMemberHidden` exist to prevent. But an admin who competed,
died and took **ordinary** spectate is a real audience member and *does* count — key it on the
roster instead and the number shown depends on who is on an admin list rather than on what sessions
exist. `is_admin` is written in exactly one place (`BeginAdminSpectate`) and means precisely "this is
an operator session". Two further exclusions: `pending_enter` (still on the death screen — the camera
is not attached, they may press Quit, and `OnDeath`'s `target_uid` is the *provisional* one
`BeginSpectate` re-resolves) and `target_uid == ""` (the T5 orbit, which has nobody to charge it to).

⚠️ **`PushAudienceCounts()` is called from the TOP of `Tick()`, above BOTH of its early returns, and
that position is load-bearing.** `m_Spectators.Count() == 0` is the *last spectator leaving* — the
one moment the watched player has to be told zero — and `m_Ended` is `EndAll()` having cleared the
table, so below either guard the count could be raised and never lowered, and **the winner would
hold "12 watching" over the win screen for the rest of the session.** Nothing else needs a special
case: `EndAll()` empties `m_Spectators`, so the tally simply comes back empty and the drain half
pushes the zeros.

It is a **recompute plus a diff**, not an incrementally maintained number, because the count is a
many-to-one aggregate: a `Retarget` moves one spectator and changes **two** players' counts with no
entry added or dropped, `BeginSpectate` clears `pending_enter`, and an admin session supersedes an
ordinary one. `m_AudienceSent` (watched uid → count last pushed) exists solely because a recompute
can see who *has* an audience but not who has just **stopped** having one. The admin switch gates the
tally rather than the pass, so turning it off mid-match pushes everyone a zero instead of freezing
the row on its last value.

**Nothing identifying goes on the wire** — `SetAudienceCount` is a `Param1<int>` sent per identity,
so a player learns they are being watched without learning who is dead. Diag: *HUD & Menus* → **Force
HUD** + **Fake Audience** is the only way to reach the element offline, where `SERVER` is undefined
and no spectator can exist; its range reaches 0 because 0 is the hidden state. Server side, **Log
Spectators** dumps `m_AudienceSent`.

#### Admin spectate

A second, separate session type on the same machinery: a free camera, target cycling, and a
name/health overlay, for `admins_steamid64` members only. Gated on `admin_spectate_enabled`
(`general_settings.json` v5, defaults **on** — membership of the admin list is the real gate, so
defaulting it off would only mean every operator edits a file before an admin tool works).

**One rule governs the whole feature: admin spectate requires a NON-PARTICIPANT — alive, holding a
body, and absent from `m_Players`.** A competing admin is refused, because a competitor who can
freecam the map is indistinguishable from a cheat. There are exactly two ways to be a
non-participant, and **the admin respawn is the bridge** between them:

```
Admin competing in the match        F3 -> refused ("still in the match")
        | dies
Dead admin: death screen/spectate   F3 or the ADMIN MODE button -> respawn, then camera
        v
NON-PARTICIPANT ADMIN               F3 toggles camera <-> body, <-/-> cycle, F5 mode
   ^
Admin who connected mid-match       already lands here (OnPlayerConnected)
```

`BattleRoyaleSpectators.AdminEligibility()` **is** that rule, returning one of four
`BR_ADMIN_*` verdicts, and every admin RPC consults it rather than re-deriving it. Keys are F3
(toggle), F5 (mode), ←/→ (cycle), all in `Data/Inputs.xml`.

Five things worth knowing:

- **`AdminRespawn` creates an entity, which invariant 2 otherwise forbids — and does not break it.**
  The new body is never added to `m_Players`, so the roster count, every `IsComplete()` and
  `br_position` are untouched; the admin's placement, leaderboard entry, death record and corpse all
  stand. It mirrors vanilla `MissionServer.CreateCharacter` (`missionserver.c:486-495`) inline rather
  than calling it, because that writes the mission's `m_player` and because this mod overrides
  `EquipCharacter` to apply the *lobby* loadout. **The two recorded crashes do not apply**:
  `CreateObjectEx` faulted at 0x0, and `CreatePlayer` faulted at 0x9 with a **NULL** identity, which
  is out of contract. This passes a real one. The other recorded objection — "the body would be
  ALIVE and outside the state, so `OnPlayerTick` force-logs-it-out" — was about the carrier body;
  here that is the goal, and `IsLateJoinExempt` now covers it.
- **`IsLateJoinExempt` consults `admins_steamid64` directly**, not just `a_LateJoinExempt`. That
  array is only populated by `OnPlayerConnected`'s mid-match branch, so an admin who connected during
  the *lobby* and later held no state was never on it and got kicked. Latent bug, fixed here because
  the respawn depends on it.
- **`ResolveAdminTarget` is a separate resolver, not a sixth tier.** `ResolveTarget` opens with
  `m_Deaths.Get(spectator_uid)` to find the spectator's own party and killer chain; a non-participant
  admin has no death record, so all five tiers degrade to the T5 orbit **without saying so**. The
  admin order is: the killer of whoever just died (if alive) → next living player in cycle order →
  orbit. Cycle order is `GetPlayers()` **sorted by uid**, so it cannot renumber itself under the
  admin as the roster compacts.
- **The camera carries the admin's own body under itself** (`CarryAnchorBody`, fed by a 2 Hz
  `AdminSpectateCamPos` RPC), which is what moves the replication bubble and defeats the ~1 km limit
  described above. Unlike `CarryCorpse` it skips `DropAllItems` — it is the admin's own gear, handed
  back on exit. **The push must run in every mode, not just FREE.** It was FREE-only at first, so a
  following admin never updated `cam_pos`, the carry measured zero drift from the respawn point and
  never fired, and the bubble stayed where they respawned — the symptom being that the admin opens
  the camera and sees *no players at all*.
- ⚠️ **A carried LIVE body is visible and its teleports replicate — AND THIS IS STILL UNSOLVED.**
  The "a carried body's replicated position does not move" measurement in the section above was
  taken on a **corpse** and does **not** carry over: a live body is still simulated, so other
  players watch an admin-shaped character slide across the map with the camera, and the admin sees
  their own body standing next to whoever they are following (observed 2026-08-11).

  **`SetInvisibleRecursive(true)` called server-side does NOT fix it** — tried, shipped, and
  measured still visible on 2026-08-11. The likely reason is that `SetInvisible` is a local render
  flag rather than replicated state: COT only ever calls it **client-side**, on the local player's
  own model (`JM/COT/.../Player/DayZPlayerImplement.c:208`). It is left in the code because it is
  harmless, not because it works.

  **The known-good route is COT's `PlayerBase.COTSetInvisibility()`**, whose `JMInvisibilityType`
  includes `DisableSimulation` as well as `Interactive` (`JMPlayerModule.c:1572-1628`) — i.e. real
  invisibility needs the simulation disabled, not just a render flag. COT is a hard dependency and
  this is a plain API call behind `#ifdef JM_COT`, so it carries no licence obligation (COT is
  CC BY-SA 4.0; *calling* it is not adaptation). Untried as of this writing.

- **A spectating admin is hidden from their own party's state feed**, via
  `VigridPartyAPI.SetMemberHidden(uid, bool)` — the fix for a leak where a respawned admin showed to
  their teammates as a live member with a compass caret pointing at wherever the camera had carried
  their body (observed 2026-08-11, fixed 2026-08-12). Note `SetHudSuppressed` had *looked* like the
  fix and only ever addressed the **admin's own** screen; the leak was on everyone else's, which
  nothing client-side can reach because it is the server's push putting the position on the wire.
  Three things about it:
  - **A hidden member is presented exactly as an OFFLINE one** — zeroed position, zeroed flags — and
    that is the whole implementation rather than a new flag. Every consumer already routes through
    `IsMemberVisible` / `IsMemberOnline`, both keyed off the `ONLINE` bit, so this suppresses the
    world nametag, the compass caret and the map's team layer at once with no client change. They
    stay a full member otherwise: on the roster, listed by name, still counted by `GetGroupCount`.
    The visible cost is that teammates see them as "Offline".
  - **Party is told nothing about spectating.** The API is documented as "a host mod can put a player
    somewhere that is not where they are playing from", which keeps the discipline rule intact.
  - ⚠️ **The clear is the dangerous half** — a member left hidden reads as permanently offline for
    the rest of the match. Every removal now funnels through `BattleRoyaleSpectators.DropEntry`,
    which releases it; there were four exit paths and a fifth in `EndAll`, which skips
    `EndAdminSpectate` for an admin whose identity has already gone and then `Clear()`s the map. Add
    no sixth: drop entries through `DropEntry` and nowhere else.

  Verified live 2026-08-12, and the useful part is *which* paths were exercised. The hide fires
  before `BeginAdminSpectate` returns, so the first `VP_TeamState` after a respawn already carries
  the admin as offline and there is no window for a caret. It also fired on an entry with **no
  preceding `AdminRespawn`** — the mid-match-joiner branch — which confirms it hangs off the session
  rather than off the respawn. The release was seen on **two non-clean paths**: a disconnect, and a
  match end, both landing `SetMemberHidden … visible` immediately before the entry was dropped.
  Those are the ones that would have stranded a member as permanently offline.
- ⚠️ **Do not hand the body back in the same frame as the exit teleport.** It crashed the client in
  vanilla `DayZPlayerCamera1stPerson.UpdateUDAngleUnlocked` (`dayzplayercamera_base.c:132`): the body
  has never been simulated — created by `CreatePlayer` and dropped from the selection immediately —
  so vanilla's camera initialises against a player that is still mid-juncture. `SelectPlayer` is
  deferred `BR_ADMIN_EXIT_SELECT_DELAY_MS` behind the teleport, in `FinishAdminExit`.
- **Party's HUD is suppressed for an admin session only**, through
  `VigridPartyClientAPI.SetHudSuppressed()` — a switch the host flips, so `Party/` still names no
  `BattleRoyale*` symbol. Without it the party nameplates stack with the admin ones over the same
  character and the roster panel describes a party the admin is not currently playing in. An
  *ordinary* spectator keeps their party HUD: following a teammate with it live is the wanted
  behaviour.
- ⚠️ **The focus counter is the trap.** `EnterSpectate`'s three `ChangeGameFocus(-1)` calls exist to
  undo the `LockControls(true)` that `SimulateDeath` performed. An admin entering spectate **alive**
  never ran `SimulateDeath`, so releasing anyway drives the additive counter negative — which breaks
  input with no error, no log line and nothing on screen. They are gated on `b_DeathLocked`, set by
  `ShowDeadScreen` via `BattleRoyaleRPC.death_locked` (the same 4_World→5_Mission stage hop
  `dead_placement` uses) and consumed exactly once. `LeaveSpectate` deliberately adds **no**
  symmetric `+1`.

The overlay (`BattleRoyaleSpectatorTags.c` + `spectator_tag.layout`) shows name, health bar,
distance, kills, party colour and a highlight on the current target. It is fed by the
`SetAdminPlayerList` RPC — **per-identity to registered admin spectators, never broadcast**, because
it carries SteamID64s, which `SetLeaderboard` deliberately keeps off the wire for exactly that
reason. That push is also why **tags work at any range while models do not**: positions come from the
server, bones do not.

**Names are coloured per PARTY, and that field is NOT `VigridPartyPalette` (#276).** The wire carries a
match-local party index (`admin_parties`, was `admin_slots`) which
`BattleRoyaleTeamColour.ForParty` turns into a generated pastel. Five things are load-bearing:

- ⚠️ **The bug was a SEMANTIC one on a field that already existed and already drew.** The server filled
  it with `VigridPartyAPI.GetMemberIndex` — the player's slot *inside* their own party — so every
  party's first member came out amber and the overlay answered "which of my teammates is that" to
  somebody who is in none of the parties. Nothing was missing; the wrong index was being sent.
- **The index is keyed on `GetPartyId()`**, a stable string, into `m_PartyColourIndex`, assigned
  first-seen. Deliberately **not** a position in `GetGroups(roster)`, which re-partitions the living
  population every push — a team's colour would change whenever somebody in an earlier group died. A
  party reduced to one living member keeps its colour: they are the last of a team, not a solo.
- **The palette is not reused because it cannot answer this question.** It is eight hues wrapping on
  `slot % 8`, right for a party of at most 16 that you are in, wrong for a match holding thirty
  parties where two sharing amber *is* the question. Both still exist and neither should be pointed
  at the other's job.
- **The walk is the GOLDEN ANGLE with six sat/val tiers, and the tier count is not a round number.**
  A golden-angle walk puts its closest hue pairs at Fibonacci offsets (8, 13, 21), and a tier count
  dividing one of them leaves that pair separated by hue alone. At the first-written 3 tiers, parties
  21 apart measured **dE 2.9** in CIE Lab — the just-noticeable threshold, i.e. indistinguishable. Six
  divides none of them. The twelve tier values were *searched*, not picked; measured over 30 parties:
  worst pair anywhere **dE 13.0**, worst adjacent pair **dE 46.4**, nearest to the solo white
  **dE 20.6**. They are one package — changing one saturation moves the worst-case pair elsewhere.
- ⚠️ **Thirty distinguishable colours do not exist and the code says so.** Categorical colour tops out
  around ten to twelve. What is guaranteed is that no two parties share a colour and that adjacent
  indices are far apart, which is what "are those two on the same side" needs. Do not "fix" this by
  adding hues. `BR_SPECTATE_TAG_TARGET_COLOUR` is now a **fully saturated** amber precisely because
  every generated colour is a pastel (sat ≤ 0.50), so the follow highlight can never collide.

**A nearby body gets a name too (#278)**, from a second RPC, `SetAdminDeadList` — names and positions
only, no uids and no party index. Four things about it:

- ⚠️ **The positions are where each player FELL, not where the body is now**, and that is correct
  rather than lazy. `CarryCorpse` moves a spectator's corpse hundreds of metres, but a carried body's
  *replicated* position does not follow (measured, above), so no client renders it anywhere — tagging
  the live server position would put a name in empty air and leave the fight unlabelled.
- That is also what makes the set **append-only**, which is why it rides a 2 s clock with a count
  comparison rather than the 500 ms player push. A keepalive covers an admin entering spectate in a
  lull. 59 names and positions at 2 Hz would be the largest recurring payload in the mod.
- **Proximity is a rule, not a clutter cap**: `BR_SPECTATE_CORPSE_TAG_RANGE_M` is 40 m against the
  skeleton overlay's 500. The skeleton says a fight happened over there; the tag says who it was.
- Corpse tags are dim, carry no health bar, sort *below* every living tag, fade over the last 8 m and
  are **not edge-clamped** — an off-screen body is clutter where an off-screen living player is worth
  knowing about. They are deliberately not team-coloured.

**Both screen-space overlays are suppressed while the fullscreen map is open (#279)** — the tags and
the COT skeleton canvas, via `BattleRoyaleClient.IsFullscreenMapOpen()`. ⚠️ **The skeleton guard sits
BELOW the canvas `Clear()`, not above it**: a `CanvasWidget` keeps its draw list, so an earlier return
burns the last frame of skeletons into the map for as long as it stays open. The map plots the same
players itself — see *Map → the admin player layer*.

**Skeletons are COT's renderer, called rather than copied — but from OUR loop, not COT's.** F6
(`UADayZBRSpectateSkeleton`) is the one admin key with **no server half**, because it changes only
what this client draws. `BattleRoyaleClient.UpdateSkeletonOverlay` runs each frame and calls
`JMESPSkeleton.Draw(human, canvas, thickness)` — a public static that reads bone positions in world
space and writes into COT's `JMESPCanvas`.

⚠️ **Two builds went into driving COT's own loop instead, and neither drew anything.** The first set
`JMESPModule.SetDrawPlayerSkeletonsEnabled(true)`; the second added `esp.EnableUpdate()`. Each time
the log proved the flag was set, the `ESP.View` permission granted and the canvas present — so
`JMESPModule.OnUpdate`, which is what actually draws, was simply never ticked for us. Do not go back
to the flag; it is not the mechanism, and the tidy reading of COT's source that says it should be is
the same reading that cost those two builds.

Four things to keep:

- **Range is ours.** COT culls each subject at `ESPRadius` — 200 m by default, compared against the
  projection's *depth*, and an admin's own COT setting — which is well inside where a spectating
  admin watches from and would likely have suppressed everything even had the loop been ticking.
  `BR_SPECTATE_SKELETON_RANGE_M` is 500. COT's own `ESPRadius` is deliberately **not** written to; it
  outlives this session and is not ours to change.
- **Clearing is ours.** A `CanvasWidget` keeps what was drawn until something clears it, so the
  canvas is cleared every frame *before* the redraw, and again on toggle-off and in `LeaveSpectate`.
  Skeletons left running would follow the admin back into their own body and give a non-participant
  an x-ray view of the match — the exact thing the non-participant rule exists to prevent.
- **The living and the dead are filtered differently, because only one of them can be roster-checked.**
  A LIVING subject must be on the `admin_uids` roster the server already pushes for the overlay tags,
  and that is what keeps the admin's **own anchor body** out of it: the body is created server-side
  and reaches its owner's client as a *remote* entity — the connection's selected object is the
  spectator camera, not the body — so `ClientData.m_PlayerBaseList` holds it like anybody else, and
  testing it against `GetGame().GetPlayer()` cannot work, because that inequality is precisely why it
  was inserted. A non-participant is absent from `admin_uids` by construction. A CORPSE is drawn
  unconditionally: `admin_uids` carries only the living, so a body fails that test by definition, and
  there is nothing to gate it on anyway — a corpse marks where a fight happened, which is the point.
  Corpses do stay in `ClientData.m_PlayerBaseList` (the mod never deletes a body), verified rather
  than assumed. Measured with one corpse and one live player in view:
  `population=3 notplaying=1 corpses=1 drawn=2`.
- ⚠️ **Corpses are drawn by US, and the colour is the whole reason.** `JMESPSkeleton.Draw` takes no
  colour — it derives one from `GetHealthLevel()`, and a dead body is `STATE_RUINED`, which COT
  paints `0xFF232323`. Near-black is the worst possible colour for the one job a corpse marker has.
  `JMESPCanvas.DrawLine` *does* take a colour, so the corpse pass goes straight to the canvas at
  `BR_SPECTATE_SKELETON_CORPSE_COLOUR` and 2 px (thicker because a body lies flat and foreshortens).
  Living players still go through COT's renderer, whose health colouring is worth having.
  **The bone chain is derived from vanilla's rig, not transcribed from COT's `s_Limbs`** — calling
  `JMESPSkeleton.Draw` is interoperation, copying its table would be adaptation. The pairs come from
  the bone names vanilla registers per damage zone in `BleedingSourcesManagerBase.Init`
  (`P:\scripts\4_world\classes\bleedingsources\bleedingsourcesmanagerbase.c:23-61`); it shows, since
  ours carries the shoulders and the full spine and stops at the toes rather than the finger bones.
  `BR_SPECTATE_SKELETON_CORPSES` compiles the pass out.
- **COT's `ESP.View` permission is no longer consulted**, since nothing calls COT's gated setter any
  more. The gate is this mod's own: `admins_steamid64`, non-participant, and in a spectate session.

This is also the licence answer: porting `JMESPSkeleton` is blocked — COT is CC BY-SA 4.0, this repo
is DSPL-SA, and BY-SA §3(b) forbids relicensing adapted material under added restrictions — but
*calling* a published API is interoperation, not adaptation, and carries no obligation at all.

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

Killer attribution goes through `BattleRoyaleKillAttribution` — see *Kill attribution* below.
`BattleRoyaleSpectators.ResolveKillerUid` is now a one-line delegate to it. `RecordDeath` is
**first-write-wins**, which is what makes the documented double `RemovePlayer` and the
unconscious-disconnect path harmless; `RecordDeathWithKillerUid` is its general form, because a
killer known only as a uid may have no `PlayerBase` left at all.

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

`LanguageCore/stringtable.csv`, keys prefixed `STR_BR_`. The `Party/` addon carries its own `stringtable.csv` at its PBO root with `STR_PARTY_*` keys — the engine loads one per addon — so party strings do not go here. Five reference styles:
- Layouts: `text "#STR_BR_..."`
- Client script: `SetText("#STR_BR_...")`
- **Server script: the bare key with no `#`.** `MessagePlayerUntranslated()` / `MessagePlayersUntranslated()` (`0_BattleRoyaleState.c:541-583`) ship the key over the `NotificationMessage` RPC and the client localizes it in `BattleRoyaleRPC.NotificationMessage()`, which also substitutes the `READY_KEY` / `UNSTUCK_KEY` placeholders with live keybinds. `StringLocaliser` takes the bare key too.
- **`Inputs.xml` `loc=`: the bare key, no `#`** — the keybind label in Options → Controls, and the `<sorting>` category name. Verified live 2026-08-14; the reference is DayZ-Expansion, whose every `Data/Inputs.xml` uses `loc="STR_EXPANSION_..."`. A raw English label here is not an error, it just ships untranslated, so nothing flags it but the eye.
- **Data JSON: `"#STR_BR_..."`, and it does resolve.** `Data/credits.json` names its section headings this way, exactly as vanilla's `P:\scripts\data\credits.json` does — vanilla feeds `SectionName` straight to `SetText` (`creditsdepartmentelement.c:53`).

**`SetText` resolves EVERY `#token` in the string, not just a leading one, and not only a whole-string key.** Concatenating around a key is fine and is what vanilla itself does:

```c
SetText("Client #main_menu_version" + " " + version);   // resolves - key mid-string
SetText("#STR_BR_MM_SEARCHING " + attempt);             // resolves - key terminated by the space
```

Vanilla ships `"#main_menu_version" + " " + version` in seven places (`P:\scripts\5_mission\gui\ingamemenu.c:85` is the method this mod overrides), `"#craft "` / `"#build "` with trailing text, and `"#server_browser_show / #server_browser_hide"` — **two keys in one literal**, the second mid-string. ⚠️ **This was written up backwards on 2026-08-14** ("a key only resolves as the entire string"), four call sites were rewritten to work around a bug that never existed, and Myst caught it by observing the version line had always been fine. If you are about to "fix" a concatenated key, don't — check vanilla first.

The real reason to reach for **`Widget.TranslateString("#KEY")`** is narrower: when you need `string.Format` to substitute into the *localized* text (`DayZPlayerImplement.ShowDeadScreen` — the `%1` only exists once the key is resolved), or when the string is handed to code that is **not** a widget at all, such as a webhook body.

The `stringtable` check (`Tools/Checks/stringtable.py`, via `Workbench/Batchfiles/Check.bat`) catches a key that is referenced but undefined, filed in the wrong addon's table, or defined and never referenced — across `*.c`, `*.layout`, `*.xml`, `*.cpp` and `*.json`. ⚠️ Note it prunes orphans in a direction that bites: a key goes unreferenced the moment its last call site is hardcoded, so "delete the unused key" is often exactly backwards. `STR_BR_MM_LOGIN` was deleted as unused while the button beside it read `"LOGIN..."`; treat an orphan as *find the missing call site* first.

### UI

Layouts live in `GUI/layouts/`. The dominant pattern is imperative — `GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/....layout")` then `FindAnyWidget("Name")`; most layouts have no `scriptclass`. `SpawnSelectionMenu` is a `UIScriptedMenu` (`MENU_SPAWN_SELECTION = 75` in `Scripts/Client/3_Game/Constants.c`, instantiated in `MissionBase.CreateScriptedMenu`). The only declarative `scriptclass` binding is the COT `master_controls.layout` → `BRMasterControlsForm`.

Keybinds are declared in `Data/Inputs.xml` (`UADayZBRReadyUp` = F1, `UADayZBRUnstuck` = F2, `UADayZBRLeaderboard` = F4, plus the admin-spectate set `UADayZBRAdminSpectate` = F3, `UADayZBRSpectateMode` = F5, `UADayZBRSpectateNext`/`Prev` = →/←, `UADayZBRSpectateSkeleton` = F6), registered via `inputs = "Vigrid-BattleRoyale/Data/Inputs.xml"` in `Scripts/Client/config.cpp`. The admin keys are refused server-side for anyone outside `admins_steamid64`, so the client-side check on them is presentation only — **except F6, which has no server half at all** and is gated by COT's own `ESP.View` permission instead (see *Spectating → Admin spectate*).

#### Lobby name tags

Every living non-teammate wears their name over their head while the players are still gathered
before the match — `BattleRoyaleLobbyTags.c` plus `lobby_tags.layout` / `lobby_tag.layout`, driven
each frame from `BattleRoyaleClient.Update`.

**It replaced a "point at somebody to read their name" tag, and that tag was vanilla's own.**
`IngameHud.RefreshPlayerTags` / `ShowPlayerTag` ship `#ifdef PLATFORM_PS4` and never run on PC; the
mod's `modded class IngameHud` used to call them directly whenever `!match_started`, and repaint the
text with the resolved name. It names one player at a time, chosen by a 25 m raycast down the camera
axis, and draws it at a fixed spot beside the crosshair rather than over the character.

Four things worth keeping:

- ⚠️ **`!match_started` was the wrong phase test, and this is a general trap.** `match_started` is a
  one-way latch set by the `StartMatch` **broadcast**, so a client that connects *after* that
  broadcast never receives it and reads `false` for its whole session. In practice that is always an
  admin — everyone else is kicked — which is exactly how the lobby-only tag ended up live for an
  admin spectating a running match. The replacement is `BattleRoyaleRPC.lobby_phase`, a discrete fact
  pushed **on every state transition** (one call in `BattleRoyaleServer.Update`, so no state has to
  remember) **and per-identity in `PlayerLoadedIn`**, which is what a late joiner needs. Any
  broadcast-latched flag has this bug latent in it; check for a per-connect seed before trusting one.
- The window is `BattleRoyaleServer.IsLobbyPhase()` — the current state casting to
  `BattleRoyaleDebugState`, which covers the lobby *and* the pre-match countdown and nothing else.
  Deliberately narrower than `!match_started` was: spawn selection and the drop are outside it, so no
  name hangs over anybody at their spawn point.
- **Positions are local, names come over `SetLobbyNames` keyed by network id.** In the lobby everyone
  is in the same clearing and therefore inside the bubble, so `ClientData.m_PlayerBaseList` gives
  exact per-frame positions; the server sends who to name and what to call them, with the
  recipient's own teammates already removed, so no party composition and no SteamID64s go on the wire.
- ⚠️ **`PlayerBase.GetIdentity()` IS populated client-side for a remote player — measured
  2026-08-12, `noidentity=0` sustained.** It is written down because the opposite was asserted
  confidently enough to cause a rewrite: when this overlay was first reported broken, the theory was
  that remote identities are null client-side and that `VigridPartyAPI.FindLocalPlayer` and
  `BattleRoyaleSpectatorTags` only appear to work because each falls back to a server-pushed
  position. That reasoning is tidy and **wrong**. The overlay had never been broken at all — the
  report was about a different tag. The network-id transport that replaced it works and is what
  ships, but it is not *needed*; the client-side version is strictly smaller. Do not re-derive the
  identity claim from the shape of the code.
- **Party members are excluded server-side**, before the packet is built — they already carry the
  party's own coloured tags, and two labels over one character is the stacking that had to be fixed
  once already for the admin overlay. Doing it there means `BattleRoyaleLobbyTags` names no Party
  symbol at all.
- **The funnel diagnostic earned its place and is kept.** `[Lobby] tags rows=… noentity=… noidentity=…
  far=… offscreen=… drawn=…`, 2 s apart while the overlay is active. It is what turned a three-way
  argument into a one-line answer, and it is what should be reached for first next time.

`IngameHud.BR_SuppressPointTag` now hides the point tag whenever the mod draws its own names — in the
lobby, and for an admin spectator. **It hides the root `m_PlayerTag`**, which is what makes it work
against DayZ Expansion's NameTags addon too: Expansion's own `modded IngameHud` drives the same
vanilla-owned root and parents its icon inside it, and neither vanilla nor Expansion ever calls
`Show(true)` on that root — they fade the text alpha. So the hide wins whichever order the two modded
`Update`s run in, which matters because nothing in `requiredAddons` pins that order down.

**The two scrolling lists — `LeaderboardMenu` and `VigridPartyMenu` — share one construct, and all three of its parts are needed.** A `ScrollWidget` owns its child's geometry, so each wraps a `WrapSpacer` carrying `"Size To Content V"` (plus `"Scrollbar V"` on the scroll itself); the script must therefore **never `SetPos` a row**, and must **`Unlink()` surplus rows rather than hide them** — a spacer lays out the children it *has*, so a hidden row keeps its slot and leaves dead scroll below a shorter refresh. Both also implement `OnMouseWheel` → `VScrollStep`, following vanilla's own `ScrollBarContainer`. Note this binds only under a spacer: `VigridPartyHud`, `BattleRoyaleSpeakingList` and `Extra/KillFeed`'s row pools free-position with `SetPos` into plain panels and correctly keep hiding their surplus.

⚠️ **A list that "does not scroll" is usually a list that fits.** The leaderboard was diagnosed twice as a widget bug when its fake data was 12 rows against a ~16-row viewport — content height and scrollbar state were both already correct. The diag fixtures are deliberately oversized for this reason (`BRDiagFillBoard` at 40 solo / 25 group, `Fake Online Players` defaulting to 20 of a possible 60): **a fixture that fits cannot reach the feature it exists to exercise.** Count rows against the viewport before changing widget code.

⚠️ **`LeaderboardMenu` pools rows across THREE tabs but only TWO row layouts.** The last-match table uses `match_summary_row.layout`, so `SwitchBoard` must `TrimPool(0)` before repainting — a widget built for one layout and reused for the other answers NULL to every `FindAnyWidget`, which paints a **silently blank table** rather than erroring. The original two-tab code had no such hazard because both its tabs shared one layout.

⚠️ **A grouping panel with the default `ignorepointer 0` EATS CLICKS on everything under it.** The two panels that swap the leaderboard's tab bodies are full dialog size and declared *after* the tab buttons, so Solo / Group / Last Match silently stopped responding — no error, no log line — while `CloseButton`, declared after the panels, kept working. That asymmetry is the fingerprint. **`ignorepointer` does NOT propagate to children**: vanilla's `day_z_respawn_dialogue.layout` stacks it on three nested parents and its `ButtonWidget`s inside them work fine, so a pure grouping node should always carry it. Note the diagnosis came from the *server* log — a `RequestLeaderboard` retry firing once a second while still on the solo board proved `Update()` was running and the board never changing. `LeaderboardMenu.OnClick` now traces the clicked widget name, as `DeathScreenMenu` already did.

**The vanilla right-hand HUD is trimmed.** `modded class IngameHud` (`Scripts/Client/5_Mission/GUI/IngameHud.c`) hides the thirst, hunger and temperature notifiers plus the `NotifierDivider` beside Blood, and shifts `BadgesSpacer` / `BadgesPanel` right to close the resulting gap. `Extra/PreventPlayerModifiers/` already makes `ThirstMdfr.OnTick` and `HungerMdfr.OnTick` return immediately, so those three icons never move for a whole match — they are pinned decoration. Gated on `BR_HIDE_SURVIVAL_NOTIFIERS` (`BattleRoyaleConstants.c`), compile-time because the settings files are server-side only and this is a client cosmetic.

Three things about that hook are load-bearing:

- **`Show(false)` targets the parent panel (`Thirsty`, `Hungry`, `Temperature`), never the `Icon<Name>` image inside it.** Vanilla's `InitBadgesAndNotifiers` unconditionally `Show(true)`s every `Icon*` widget and `DisplayTendency` keeps tinting them; a hidden *parent* is not drawn whatever happens to its children, so nothing has to be re-asserted per frame. `NotifierDivider` is the one widget here that no vanilla script references at all — only `BadgeNotifierDivider` is managed, by `IngameHudVisibility`'s `NO_BADGE` flag.
- **The hook is `InitBadgesAndNotifiers()`, not `Init()`.** `Init()` calls it once at startup and `respawndialogue.c` calls it again after a respawn, so one override covers both. That is also why the badge reposition uses **absolute** x (`BR_HUD_BADGES_SPACER_X` / `_PANEL_X`) rather than a delta — a second pass must be a no-op.
- Everything under `HudPanel` is `halign right_ref`, so `position x` is the distance from the parent's **right** edge and hiding a middle widget leaves a hole rather than reflowing. Vanilla's right-to-left order is `Health` 0, `Blood` 43, `NotifierDivider` 86, `Temperature` 96, `Hungry` 139, `Thirsty` 182, `BadgesSpacer` 213, `BadgesPanel` 252. The badge group moves right by 143 so its divider lands on the old 86.

**The injured-leg badge is suppressed too, and a BADGE needs a different seam from a notifier.** `BR_HIDE_INJURED_LEGS_BADGE` covers it, and the override is `DisplayBadge( int key, int value )` — forcing `value = 0` for `NTFKEY_LEGS` and passing the call to `super`. Four things make it the shape it is:

- **A `Show(false)` in `InitBadgesAndNotifiers` cannot work here**, which is the trap, because it is exactly what the three notifiers beside it do. Those are static widgets; badges are dynamic. Vanilla's `DisplayBadge` re-walks *every* badge on *every* call and `Show(true)`s any whose stored value is positive, so a one-time hide is silently undone by the first fall that scratches a leg.
- **`super` must still run.** `DisplayBadge` is also what maintains `EHudContextFlags.NO_BADGE`, the only thing driving `BadgeNotifierDivider` (`IngameHudVisibility`'s element link map). Suppress the badge by any route that skips `super` and that divider stands beside a badge nobody can see. Forcing the value to 0 rather than dropping the call also keeps `m_BadgesWidgetDisplay` honest.
- **No layout work, unlike the notifiers.** `BadgesPanel` is a `WrapSpacerWidgetClass` (`day_z_hud.layout:514`), so a hidden child collapses and the siblings reflow — the opposite of the `halign right_ref` hole described above, and why `BR_HUD_BADGES_*` are untouched.
- ⚠️ **"Just prevent the leg damage instead" is the wrong fix and was considered.** Leg *health* is not leg *breakage*: `InjuredLegNotfr` tiers `1 - min(GetHealth01("LeftLeg"), GetHealth01("RightLeg"))` at 0.05/0.35/0.65 and only consults `GetBrokenLegs()` to bail out, so the badge lights at a 5% scratch even though `Extra/PreventPlayerModifiers` makes a fracture impossible. Blocking that damage would need a config patch to the leg zone, whose `transferToGlobalCoef=0.25` (`P:\dz\characters\data\config.cpp:787`) is **how a bullet in the leg hurts you at all** — it would turn legs into bullet sponges. `DamageAllLegs()` is only the *extra* leg damage from falls, car exits, landmines and bear traps; ordinary gunfire damages the zone natively, so emptying it removes the harmless sources and keeps the common one. And leg health drives nothing else in this build: the limping override reads it only under the `BROKEN_LEGS` force mask (`injuryhandler.c:137-148`), which is unreachable here. The badge is its only consumer, so hiding the badge is the complete fix, not a cosmetic patch over a real one.
- The separate `Fracture` badge (`NTFKEY_FRACTURE`) is deliberately left alone — it is the one that matters again the moment `Extra/PreventPlayerModifiers` is disabled, and it costs nothing while that addon is in the build.

### Kill attribution

**One resolver answers "whose kill was this": `BattleRoyaleKillAttribution`
(`Scripts/Server/4_World/Entities/`).** Four consumers used to derive it independently and disagreed
— the spectator killer chain, the `player.kill` webhook, the kill credit, and
`PlayerBase.EEHitBy`'s `last_unconscious_source`. Two facts drive it, and both have caused bugs:

- **`EEKilled`'s `source` is the WEAPON** for every gun and melee kill. `ResolvePlayerSource` does
  the hierarchy-parent step, with the `EntityAI.Cast` **null-checked** — a source that is not an
  `EntityAI` (a building, a vehicle part) used to dereference NULL at two separate call sites.
- **For an explosive or a trap the source is the DEVICE**, which has no hierarchy parent once armed.
  The responsible player is knowable only because the device recorded them at arm time, as a **plain
  string** — which is exactly what lets a kill outlive its owner's death *and* their disconnect.

**Devices are modded at the vanilla PARENTS, `ExplosivesBase` and `TrapBase`.** Naming the leaves
(`Grenade_Base`, `LandMineTrap`) meant Claymore, IED and Plastic Explosive — the archetypal traps —
credited their classname instead of their owner. `ExplosivesBase.OnPlacementComplete` is the one hook
that covers arming as well as placement: `ActionArmExplosive.OnFinishProgressServer` calls
`OnPlacementComplete(action_data.m_Player, …)`. `Grenade_Base` therefore carries **only `OnUnpin`**;
re-declaring `m_ActivatorId` on it would shadow the parent's. `ActionTriggerRemotely` re-attributes
to whoever pressed the detonator, and **sets the activator before `super`**, since `super` is what
detonates.

⚠️ **The `EEKilled(Object killer)` override on a device cannot use a bare `PlayerBase` cast** — the
same weapon-not-shooter rule applies, so the cast this replaced could never succeed and shooting an
armed charge silently kept crediting whoever placed it.

⚠️ **`OnPlacementComplete` must be hooked on the PARENT; `OnActivatedByItem` must be hooked on the
LEAF. The two sit inches apart and behave oppositely.** Vanilla's four explosives — `Grenade_Base`,
`ClaymoreMine`, `ImprovisedExplosive`, `Plastic_Explosive` — **none** of which calls
`super.OnActivatedByItem`, so an override of it on `ExplosivesBase` is dead code for every explosive
in the game. It was written there first and looked right. The symptom is a device that records its
owner correctly and still credits `<environment>`.

**A grenade rigged to a tripwire is the case that needs it**, and it is the one real path where the
device that KILLS is not the device that knew the owner: `TripwireTrap.SetInactive` calls
`attachment.OnActivatedByItem(this)` and then **drops** the attachment, so the grenade does the
damage while the trap holds the activator. Vanilla's `Grenade_Base.OnActivatedByItem` answers by
calling `Unpin()`, which does reach the mod's `OnUnpin` — but the grenade's hierarchy root is the
*trap* by then, not a player, so it resolves nobody. `BR_InheritActivatorFrom` copies the activator
across, called before `super` because `super` starts the fuse. Same handoff applies to an IED with
grenades attached (`ImprovisedExplosive.OnActivatedByItem` activates its own attachments).

**Kill credit lives in `BattleRoyaleKillLedger`, a uid → kills map, and `br_kills` is now a mirror
of it.** The explosive branch of `0_BattleRoyaleState.OnPlayerKilled` wrote the webhook JSON and then
fell out of the `if`, so **no grenade or mine kill had ever scored** — not on the HUD counter, the
admin spectator tags, or the ladder. Both branches funnel through `CreditKill` now, which is a no-op
on an empty uid. A **corpse** still resolves through `FindPlayerByUid` (the mod never deletes bodies),
so a dead-but-connected killer watches their own counter tick.

**A posthumous kill has to AMEND the ladder, not add to it.** `RecordExit` fires and locks the uid
into `m_Recorded` the moment its owner leaves the match — which is before their grenade lands.
`BattleRoyaleLeaderboard.AmendKills` recomputes `MatchPoints` against the stored
`BattleRoyaleMatchRecord` (board, rank, ranked, kills, points) and applies the **deltas**; entries are
cumulative across matches, so re-adding would double-pay. A uid with no match record needs nothing —
`RecordExit` reads the ledger itself and gets the right answer.

**The `AddPlayerKill` RPC now carries the authoritative TOTAL, not an increment.** The client handler
ignored its payload and did a local `+= 1`, which drifts the moment a kill is scored while that client
controls no entity — a spectating killer whose grenade landed after they died.

⚠️ **`EnScript.GetClassVar` DOES reach a var declared on a modded PARENT class — measured 2026-08-12
— but its return code is meaningless.** This matters because `Extra/KillFeed` reads the activator
reflectively (the discipline rule forbids it naming a BattleRoyale symbol), and those vars now live on
`ExplosivesBase` rather than on `Grenade_Base`. A round-trip through a `Grenade_Base` instance
returned `PROBE-OK`, so the seam is sound. The return code was **0 for all four cases**: a var
declared directly on the class, a var that exists nowhere, a successful `SetClassVar`, and a
successful `GetClassVar`. Never branch on it — the same trap as `DiagMenu.BindCallback`. An empty
string back is *not* evidence of failure, which is why the first probe (a bare read of a fresh
grenade) was ambiguous and had to be redone as a round-trip.

### Death recap and the last-match summary

Two halves of one feature. On death the player gets a one-line recap; back in the **lobby of the next
match** they get a full card and the previous match's standings, on a third tab of the F4 leaderboard.

**The summary is persisted and read in the next lobby rather than shown at match end, and that is the
whole design.** The server process restarts between matches, so everybody — including the winner, who
`KickWinner` drops 15 s after they win — reconnects into a fresh lobby. It is the only place a summary
can reach every player, it makes the death-screen change a single line, and it gives reconnect
resilience for free (the card is keyed by uid out of a file, so dropping mid-match and rejoining still
finds you). That last property is why the uid must stay in the file.

**One resolver answers the recap: `BattleRoyaleKillAttribution.ResolveKillDetails`**, beside the
existing `ResolveKillerUid` and for the same reason that class exists — weapon-and-range already had
*two* independent derivations in the tree (`Extra/KillFeed/KillFeedDeath.c` and the webhook JSON block
in `0_BattleRoyaleState.OnPlayerKilled`), and a third inline copy would repeat the original mistake. It
is 4_World because `PlayerBase` and `BattleRoyaleMatchStats` both need it. The webhook's own copy is
deliberately **not** refactored onto it: that is an external Vigrid API contract and a
behaviour-preserving change there is not locally verifiable.

`BattleRoyaleKillCause` (2_GameLib, unguarded) is **BR-owned and numerically independent of
`KillFeedCause`** — that addon is optional by contract and can be deleted from the build. Do not
"align" the two. The zone is the one environmental cause that can be named, and only because the two
`DecreaseHealthCoef` sites drop a timestamp that `ConsumeZoneHint` **consumes**; a hint left behind
would mislabel the player's next environmental death.

**`BattleRoyaleDeathRecord` lives in 4_World**, not beside `BattleRoyaleSpectators` which owns and
writes it, because `BattleRoyaleMatchStats.RecordExit` copies the recap onto the summary row and a
4_World method cannot name a 5_Mission type. Every recap field is written **inside `RecordDeath`'s
existing first-write-wins guard** — the unconscious-disconnect path fires a second `EEKilled` whose
source is the *victim*, so a second pass would overwrite good attribution with `ENVIRONMENT`.

#### Damage: measured as a delta, and the two bugs that took real matches to find

`TotalDamageResult` reports **computed** damage, not applied, and is getter-only. Damage is therefore
measured as the health/blood the hit actually removed, latched in `EEOnDamageCalculated` (which runs
first) and subtracted in `EEHitBy` after its `super`. **Both pools**, `Math.Max` of the two fractions
normalised by the victim's own maxima: DayZ death is `Health <= 0` **or** `Blood <= 0` and a firearm
body shot is mostly a *blood* hit, so health alone under-reports every gunfight and reports **zero**
for a bleed-out kill. The unit is "fraction of a player removed", so one full-health kill scores
exactly 100 — which is also the arithmetic that catches under-counting.

⚠️ **The guard must be "were they alive BEFORE this hit", not `IsAlive()`.** Vanilla's `IsAlive()` is
`!IsDamageDestroyed()` — purely health-based — and `EEHitBy` runs *after* the engine applies damage,
so a killing blow reads as "already dead" and the one hit that decided the fight is discarded. Caught
because a knife kill on a full-health player scored **76**: the per-hit deltas cannot sum below the
health actually lost. The pre-hit latch answers it exactly, and still refuses corpse-shooting.

⚠️ **Pair the latch to its hit with a CONSUMED FLAG, never a clock comparison.** This was
`br_prehit_ms != GetGame().GetTime()` — millisecond-exact equality — and it dropped hits at random,
because `GetTime()` is live ms and the engine applies damage, processes bleeding and runs the shock
transfer between the two hooks. The same player killing the same opponent with the same weapon scored
**100 in one match and 0 in the next**. The 200 ms age check that remains is only a safety net for the
orphan case (another mod cancels the damage, so nobody consumes the latch); "no latch at all" is a
**separate** rejection reason, because it is a different fault with a different fix.

**Known gap: bleed-out damage is credited to nobody**, since blood loss drains through the bleeding
manager rather than through `EEHitBy` with a player source.

#### Persistence: ONE write

`$profile:Vigrid-BattleRoyale\last_match.json`, written **once**, in `9_BattleRoyaleRestart.Activate()`.

The tempting alternative — the leaderboard's debounced flush — is wrong here, and not for performance
reasons: it would make the file do two jobs with opposite lifetimes. A partial mid-match write holds
players whose `br_position` is *groups remaining right now* rather than a finishing place, so every
survivor reads as 4th, and the same flush cycle overwrites the backup, losing the previous match too.
The ladder can afford that trade because it is cumulative across months; this is one cosmetic match.
Writing once also makes the in-memory `m_Previous` immutable for the process lifetime — which is what
makes serving it safe with no sequence number — and turns the copy-aside into a free one-deep history.

⚠️ **Not in `8_BattleRoyaleWin`.** The winner's `RecordExit` does not run until `KickWinner`, 15 s
*after* that state activates, so a write there produces a file with no placement and no survival time
for the winner — the one row everybody looks at first.

**Sorted by place on write AND on load.** `RecordExit` appends in death order, so unsorted the table
renders the winner underneath the loser. The write-time sort cannot repair a file produced by an older
build or written by hand, and everything read off that disk is untrusted anyway. The winner is read
*before* the sort, because it is the last **exit**, not the best place.

**I/O profile: nothing touches the disk while anybody is playing.** `JsonFileLoader` is synchronous —
EnfusionScript has no async I/O — so every save blocks the main thread, which is what makes *when*
matter more than how big. The load is one small file (~11 KB at 40 rows, ~18 KB at the 64 cap) during
lobby entry; the save is after the match has ended, 10 s ahead of `RequestExit`. Per hit the ledger is
pure in-memory map work, and its per-hit trace is gated on `CheckLogLevel` so the string is not even
built at production log levels. For contrast, the thing that *does* write mid-match is the
pre-existing `BattleRoyaleLeaderboard` flush every `BR_LEADERBOARD_FLUSH_DEBOUNCE_MS` (15 s),
serialising up to 5000 entries — that is the one to suspect if periodic hitches ever appear.

#### The wire

`RequestLastMatch` (no payload, actor from `sender`, its **own** cooldown map — sharing the
leaderboard's means a fast tab switch is silently refused) answered per identity by
`SetLastMatchTable` + `SetLastMatchRecap`. **No SteamID64 on any of it**: `self_index` identifies the
local row, and is `-1` for anyone who did not play — the *common* case in a lobby, which must hide the
card rather than paint a zeroed one.

⚠️ **Never call `RequestLeaderboard` with `BR_LEADERBOARD_BOARD_LASTMATCH`.** `ServeRequest` treats
anything that is not `GROUP` as `SOLO`, so board 2 is answered with the solo ladder tagged as solo
while consuming the requester's cooldown — the tab just looks dead.

`grouped` comes from `VigridPartyAPI.IsReady()`, never from `#ifdef VIGRID_PARTY`, or a server with
the party manager disabled labels a player count as a squad count (#158 again). The squad block is
summed **client-side** from the table, which is only sound while the table is complete — hence the
`TRUNCATED` flag, on which the client hides the block rather than showing a quietly wrong figure.

`SetLastMatchRecap` has **two send points and one handler** — pushed at death, and again when the
lobby asks. They cannot be confused within a session because a session spans exactly one server
process; the one exception is an admin, who is exempt from the late-join kick and can see match N's
recap under match N−1's header.

#### Testing it

`DIAG_DEVELOPER` entries under *HUD & Menus*: **Fake Last Match**, **Recap Cause**, **Did Not Play**
and **Open Death Screen** — the last of which must set `suppress_alive_close`, because offline the
player is always alive and `DeathScreenMenu.Tick()` closes on `IsAlive()`, which reads as a broken
layout.

⚠️ **Build the fixture so it can FAIL.** The 40-row fixture was written in rank order — already sorted
by construction — so it could never have caught the missing sort; a real two-player match did, on the
first try. This is the same trap as the leaderboard fixture that fitted its viewport and so could not
test scrolling. Whatever property is under test, construct the fixture to violate it.

⚠️ **Log every rejection, with its own reason.** A guard that only logs on success cannot distinguish
"nothing arrived" from "everything arrived and was thrown away" — both are no line at all. A damage
total of zero was ambiguous for two full test matches until one build logged each early `return`; it
named the guilty guard on the first hit.

### Kill feed (`Extra/KillFeed/`)

A standalone addon replacing the third-party `nulledkillfeed.pbo`. It builds into `extra_killfeed.pbo` and defines `KILLFEED`. It hooks vanilla `PlayerBase.EEKilled` itself, so it works on any DayZ server — Battle Royale is not required.

**Same discipline rule as `Party/`: nothing under `Extra/KillFeed/` may reference a `BattleRoyale*` symbol.** It carries its own logger (`KillFeedLog`), settings (`$profile:KillFeed\killfeed_settings.json`), `stringtable.csv` (`STR_KF_*`), layouts and RPC namespace (`RPC-KillFeed`). `KILLFEED_PREFIX` in `KillFeedConstants.c` is the single place the asset path appears.

**`ResolveActivatorName` asks for the recorded NAME first, and that ordering is the point.** Mapping the activator's uid back to a name by scanning `GetGame().GetPlayers()` can only answer for somebody still connected and still holding an identity — but an explosive routinely outlives its owner, which is the one case worth naming. A thrower who died or left yielded `""`, degrading their kill to an environmental death that `show_environment_deaths` could then suppress outright. `IsExplosive` tests the vanilla parents `ExplosivesBase` / `TrapBase`, not two concrete classes. Both reads stay defensive `EnScript.GetClassVar` calls — see *Kill attribution* for why that works and why its return code must not be checked.

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

- **M** opens a fullscreen `UIScriptedMenu` (`MENU_VIGRID_MAP = 178`), **Esc** or a second **M** closes it. Pan and zoom are the engine's own — **never override `OnMouseWheel`** or native zoom dies. `ClampZoom()` holds the range each frame because there is no zoom event to hook. **The zoom is remembered for the session and reapplied on open** (the position still recentres on the player every time): the menu is `new`ed per open and destroyed on close, so it lives in `VigridMapMenu.s_LastScale`, a static, and is deliberately *not* a field in `map_client.json` — persisting it would cost a synchronous JSON write every time the map is shut. It is recorded from `Update()` off the same per-frame `GetScale()` read `ClampZoom` already does, gated on the view having settled; neither teardown hook is a safe substitute, since `OnHide` need not pair with `OnShow` and the destructor runs as the widget tree goes away. ⚠️ **A `MapWidget` ignores `SetMapPos`/`SetScale` until it has been laid out**, so the pair issued in `Init()` is dropped and the map drew at the engine's default position and zoom until the +100 ms `DelayedCenter` — "opens off-centre, then jumps", pre-existing and made louder by a restored zoom, since the jump then moves the scale too. `SettleView()` re-issues the view every frame and `Update()` holds `ClampZoom` and every canvas off until it has settled, with `DelayedCenter` demoted to a backstop so the worst case is the old behaviour. **`SettleView`'s latch test is measured never to pass** — the backstop wins every open — so which half actually fixes it is unestablished; the method header carries the one-line hypothesis (`GetMapPos` answers at Y 0 while the target carries terrain height) and its acceptance test. A premature latch is the one way to bring the artefact back.
- **Left-click** places your marker, **right-click** clears it. One per player, permanent, party-shared, and the placer is the only one who can remove it. Both click guards from `BRMapHandler` are load-bearing: without the moved-mouse test every pan drops a marker.
- **The map does not stop the player, and Esc has to be polled by hand.** `VigridMapMenu` declares `UseKeyboard() == false`, so `LockControls` takes only the mouse focus and movement keys still reach the game; vanilla `MissionGameplay` carries a branch written for exactly `!UseKeyboard() && UseMouse()` that disables the mouse as a *game* input, so panning and clicking cannot fire the weapon underneath. **Two vanilla-looking hooks are both wrong here, and each cost a build to find out.** `AddActiveInputRestriction(EInputRestrictors.MAP)` is vanilla's own "this player has a map open" restriction, but its entire body is `UAWalkRunForced.ForceEnable(true)` — it pins the player to walking speed. And the exclude group `{"map"}`, which is what Expansion's map menu uses, is nothing but `<include name="menu" />`, and `"menu"` includes `"movement"` — it takes WASD straight back off. (Expansion gets away with it because its map keyboard-locks anyway. The vanilla exclude groups live in **`P:\bin\specific.xml`**, not in any `inputs.xml`; that file is the reference when composing one.) **And an exclude group of your own is wrong too, which is the third and least obvious one.** Gameplay actions do have to be stopped — vanilla's mouse-disable for `!UseKeyboard` menus calls `Input.DisableKey`, which is only the low-level device, while fire, melee and user actions are read engine-side from the `UApi` binding, so a left click to place a marker also swung the weapon. But `AddActiveInputExcludes` and `RemoveActiveInputExcludes` **both** end in `GetUApi().UpdateControls()` ("call this on each change of exclusion"), which rebuilds the control state and drops the **held** state of every input including `UATurbo` — closing the map mid-sprint dumps the player out of sprint until Shift is re-pressed. That is inherent to adding or removing a group at all, not to its membership, and it is the same mechanism that walks a vanilla player when they open the inventory. `VigridMapMenu.SuppressGameplayInputs` calls `Supress()` on the individual inputs once per frame from `Update()` instead: nothing global, nothing to tear down, and safe to call from a menu `Update` whose ordering against `MissionGameplay.OnUpdate` is undefined, because `Supress` is forward-looking ("supress press event for next frame"). Never point it at a movement key — the rest of that doc line is "while not pressed ATM, **otherwise until release**". Esc is polled as `UAUIBack` in `MapMissionGameplay.HandleMapClose` because while any scripted menu is open `MissionGameplay.OnUpdate` never reaches its `Pause()` branch, so Esc is a dead key rather than a competing one; every vanilla menu answers this the same way. Opening is guarded on `m_UIManager.GetMenu()` being null rather than on a list of menu ids — the list drifted, and passing `GetMenu()` as the parent made whatever was open the map's parent menu.
- **`Supress()` does not reach the aim axes, so the mouse still turned the camera under the map — and this is the one place an exclude group is right.** It is the fourth wrong-looking-right hook, and it also qualifies the previous bullet's "disables the mouse as a *game* input": vanilla's `Input.DisableKey` block covers mouse buttons 0-4 **and axes 0-5**, and it does run (there is no early `return` before `missiongameplay.c:616`) — but both player cameras read the aim engine-side off the input controller, `GetAimChange()` in third person and `GetAimDelta()` in first (`dayzplayercamera_base.c`, `dayzplayercamera3rdperson.c:441`), so the low-level disable never reaches it. `Supress()` cannot substitute either — it is a *press event* concept and the aim axes are analog. **`HumanInputController.OverrideAimChangeX/Y(ENABLED, 0)` was tried first and measured not to work**: the camera kept turning while an edge log confirmed the calls were reaching the live controller every open and close. The tell was available in advance and missed — `OverrideRaise` and `Override3rdIsRightShoulder` have real vanilla call sites and do work, while `OverrideAimChangeX/Y` have **none anywhere in `P:\scripts`**, only the proto declaration at `human.c:240`. What works is `AddActiveInputExcludes({"aiming"})` / `RemoveActiveInputExcludes` in **`MapMissionGameplay.UpdateAimSuppression`**. `"aiming"` (`bin/specific.xml:149`) is exactly the four aim inputs and does **not** include `"movement"` — which is the whole reason it is usable where vanilla's `{"map"}` is not. **The `UpdateControls()` held-input reset described above is still real and is simply accepted here**: opening or closing the map mid-sprint drops the player out of sprint until Shift is re-pressed. It is edge-triggered, not per-frame (each call rebuilds the control state), and it lives in the mission update rather than the menu so the *remove* edge cannot be missed — a leaked exclude group would leave the player permanently unable to aim. It is called **above** the `m_VigridMap` guard for that reason.
- **N** toggles the HUD minimap, which is **opt-in — off by default**. Effective visibility is `VIGRID_MAP_MINIMAP && minimap_allowed && minimap_enabled`, and each switch can only opt further out than the one before it. The first is the **build's** — a define in `Extra/Map/config.cpp`; comment it out and `VigridMapMinimap`, its widgets and the N handler are compiled out entirely, leaving the fullscreen map untouched. The second is the admin's (pushed over `VM_Settings`, ships **on**), the third the player's, in `$profile:Vigrid-Map\map_client.json` (ships **off**). So out of the box the key works but nothing is shown until it is pressed; the layout is still built on first `Update` and just kept hidden, so toggling on later costs nothing. Changing the `minimap_enabled` initialiser only affects players with no prefs file yet — anyone who already toggled keeps their saved choice. Two things deliberately survive a minimap-less build: `minimap_allowed` on the wire and in `map_settings.json`, because a *client* build flag must not change the wire format, and the N entry in `Data/Inputs.xml`, because XML cannot be conditional — it still lists under Options → Controls doing nothing.
- **K** toggles the HUD compass strip (`VigridMapCompass`), gated exactly like the minimap — `VIGRID_MAP_COMPASS && compass_allowed && compass_enabled` — but the **player default is ON**, since it is a thin band answering a question the HUD could not otherwise answer at all. A 620×38 px strip flush against the top of the screen showing a **90° window**: cardinals every 45°, numeric degrees every 30°, an unlabelled tick every 15°, the bearing read out below, and carets in the bottom lane for the next zone, teammates and party pings. Its three lanes are packed tight — ticks 0-11, labels 11-33, carets 33-38 — because slack at the bottom reads as a misaligned box whenever no caret is up. Labels come in three size tiers, and **each tier is its own widget with its own font face** (`metron-bold28` / `-bold22` / `-bold14`), because glyph size is fixed by the declared face and there is no `SetFont`; `PickLabel` shows one and hides the other two. **`SetTextExactSize` was measured to do nothing** — 28/18/13 on one widget rendered 28/28/28 under `GetTextSize` — and its one call site in all of `P:\scripts` was the tell that was missed. Every length is authored against a 1920-wide screen and multiplied by `parent_w / VIGRID_MAP_COMPASS_REFERENCE_W`, re-applied by `ApplyScale` only when the viewport moves. Heading is the **camera** bearing, the same one line the minimap dart uses; the regression test is that a full 360 returns to the same reading. Two structural choices worth keeping: the entry pool is indexed **by bearing** (entry *i* is permanently the *i*×15° mark) so labels are localised once at creation and never per frame, and the strip is redrawn **every frame** rather than the minimap's 10 Hz, because a band sliding under a fixed cursor is exactly where 10 Hz reads as stutter. Elements fade out over the last 8° instead of being clipped, which is why losing the clipping container below cost nothing.

**A widget's DECLARED position and size are scaled by viewport/1920; `SetPos` and `SetSize` are in real screen pixels.** Measured 2026-08-11 with `GetScreenPos` on a 1280-wide client: a child declared at `position 23` reported `15.33` (×0.667), and the same widget after an explicit `SetPos(23)` reported `23`. **Mixing the two silently misplaces things, and it does not look like a coordinate bug** — it looks like an *angle* bug, because the spacing stays perfectly correct while the whole group shifts. It cost two builds on the compass: first a fixed `620 px` `hexactsize 1` container, whose children were laid out in 620-px units while the container itself rendered ~430 px wide, putting the strip 95 px off centre; then, after reparenting to the full-screen root, a residual 8 px because the pooled entry root *declared* 48 px wide actually rendered 32, so subtracting the declared half-width over-shot. **The rule that falls out: for anything script-positions, position and size it entirely from script and treat the layout's numbers as Workbench-only placeholders.** The one container shape that behaves is the full-screen `size 1 1` / `hexactsize 0` frame — `m_Root.GetScreenSize()` reports real pixels and children `SetPos`ed in those pixels land correctly, which is what `VigridMapMarkers3D` and `VigridPartyNametags` were already relying on.

**Nothing can be drawn over a `MapWidget` with script-created widgets.** `CreateWidgets(path, parent)` returns a valid widget, `SetPos` puts it in the right place, it tracks pan and zoom — and it is never rendered, with no warning and no failed image load. What *does* work is a widget **declared in the layout as a child of the MapWidget**, which is what `SpawnSelectionMenu`'s heat map already does. Four canvases in `map_menu.layout`, z-ordered by `priority`: `LineCanvas` 999, `ZoneCanvas` 1000, `MarkerCanvas` 1001, `TeamCanvas` 1002, plus `AdminCanvas` 1003.

⚠️ **CORRECTED 2026-08-18: the discriminator is SCRIPT-CREATION, NOT the widget class and NOT the parent — and "there is no text on the map" was wrong.** This file, `Extra/Map/README.md` and three code comments all used to reason from "canvas draws lines only, therefore no text is possible here". A canvas indeed has only `DrawLine` and `Clear`, but that never implied text was impossible — only that *a canvas* could not carry it. A **`TextWidget` declared as a child of the `MapWidget` renders perfectly**, which is how the admin layer's name labels ship. Two builds were spent on the wrong reading:

- parenting a script-created `TextWidget` to the `MapWidget` — never rendered, as the old rule predicted;
- parenting it to a declared full-screen **sibling** frame above the map by `priority` — **also never rendered**, which the old rule did *not* predict and is what killed the "sibling" theory. The funnel read `labelled=12 offscreen=0 pool=12 layer=1` with a blank screen: created, positioned, shown, unclipped, invisible.

The cost of a declared pool is that it **cannot grow at runtime** — `map_menu.layout` declares 32 `AdminName<n>` labels and the bind loop in `VigridMapMenu.Init` stops at the first gap, so the layout is the single source of truth for the size. Glyphs stay uncapped, so a busy match loses names before it loses positions.

**Glyph vocabulary**, all in `VigridMapRender` and all sized in screen pixels rather than metres: zone rings and centre dots, a **ring with a cross** for a placed marker, a **hollow triangle** for a teammate, a lighter **diamond** for a party ping, and a **notched dart** for you — on **both** maps. The circle is spent twice already, so both live-position glyphs are straight-edged, and triangle-vs-diamond needs separating because they are the pair that could still collide at small size. That was originally done on three axes — vertex count, stroke weight, opacity — but the ping's 1 px stroke proved not reliably visible over satellite imagery at 12 px, so it was raised to match the triangle's 2 px. **Vertex count and opacity (0.75 vs 1.0) are now the only things telling them apart**, so `VIGRID_MAP_PING_ALPHA` must not be raised to 1.0 without giving the ping a different silhouette. If they ever do read alike, the tested fallback is a six-pointed asterisk for pings — three lines through a common centre, no enclosed area, unmistakable for any polygon.

Ping alpha has one more trap: on the **world** marker the crosshair-fade floor `VIGRID_PARTY_PING_CENTER_MIN_ALPHA` *multiplies* with `VIGRID_PARTY_PING_BASE_ALPHA`, so a floor that looks reasonable on its own can be far darker in practice — 0.25 × 0.75 left a ping at 0.19 alpha exactly when the player was looking straight at the thing they had just placed. It is 0.55 now.

**Both maps draw "you" as the same dart, and the fullscreen one used to be an axis-aligned plus (reversed 2026-08-11).** The plus was defended as easier to *find* on a big map, which is true and beside the point: the question a player opens a map to ask is which way they are facing, and the plus could not answer it. Findability is carried by size instead — `VIGRID_MAP_SELF_PX` is 16, the largest glyph on the map, against the teammate triangle's 14, and white, which no `VigridPartyPalette` slot is. Three things about the dart are load-bearing: its angle is the **camera** bearing, never `player.GetYawPitchRoll()` (body yaw snaps in steps and does not return to its start after a 360, so the arrow drifts); it is **drawn rather than a rotated `ImageWidget`** — `icon_arrow` points *down* at rest, reads ambiguously at small size because both ends look like a point, and vanilla's dedicated `Marker_Arrow.edds` does not resolve from a mod PBO at all (silently — no `.rpt` error); and the concave notch is what makes the direction unmistakable. The two call sites differ in **what they anchor to**: the minimap re-centres on the camera each tick and so passes the camera position for both position and angle, while `VigridMapMenu.RenderSelfGlyph` takes the position from `player.GetPosition()` and only the angle from the camera — the map is panned by the player, and in third person the camera is metres behind the body. Note the `"aiming"` exclude group means the fullscreen dart holds the heading you had when you opened the map; live turning can only be observed on the minimap.

**The repaint gate is split, and that is not an optimisation detail.** Zones and markers are edge-triggered on `m_RenderDirty || transform_moved || watchdog_due`. Teammates have **no edge** — Party's roster sequence moves when the party changes shape, never when somebody walks — so `TeamCanvas` repaints on a 10 Hz clock instead. **The probe results must be assigned unconditionally**, outside the repaint branch: left inside it, a team-only frame leaves the probe stale, `transform_moved` latches true, and the static layers silently repaint at frame rate. Every canvas must `Clear()` before any early return, or the last frame burns in permanently.

⚠️ **An edge-triggered layer is only as good as the edge, and the edge must be raised unconditionally.** `VigridMapClient.TrackSnapshot()` bumps `m_MarkerSeq` on every incoming `VM_Markers`, from `Update()`, above everything else. It used to live *inside* `ResolvePending`, below that method's early return for "nothing pending" — so a snapshot arriving with no outstanding request raised no edge at all and the map fell back on the 1 s `VIGRID_MAP_REPAINT_WATCHDOG_MS`. Two ordinary cases landed there: **a teammate's marker appearing**, and **the confirmation of your own removal** (`ClearMarker` retires the prediction before the echo arrives, so it is never pending when its own snapshot lands). That second was diagnosed for a long time as network latency and is not: **the fingerprint is that the minimap (10 Hz) and the world markers (every frame) update visibly before the fullscreen map does**, since neither is edge-triggered. When a repaint feels slow, check what raises its edge before you measure the wire.

**Marker placement is predicted client-side, for all three interactions.** `m_PendingIntent` (`VIGRID_MAP_PENDING_NONE`/`_PLACE`/`_REMOVE`) — a place, a **move** and a **clear** all draw immediately and reconcile against the server's snapshot. It covered only "place on an empty map" until 2026-08-11, because `GetDrawCount` appended the prediction only when `GetOwnIndex() == -1`, which is false for ever after the first confirmed marker. `ResolveDrawIndex` holds all the index arithmetic, so the three renderers read the merged draw list unchanged. Two rules that are load-bearing:

- **Retirement is content-based** — the prediction drops when the set *contains what was asked for* (within `VIGRID_MAP_PENDING_MATCH_EPSILON_M`, or contains nothing of ours for a removal), never merely because a snapshot arrived. The existence test it replaced was correct only for a first placement: on a move you already own a marker at the *old* position, so the 5 s resync or a teammate's placement would retire the prediction and rubber-band the marker back.
- **Every refusal must answer** (`MapMissionServer.RejectRequest`), with an empty key when there is nothing to tell the player. Silence reads as acceptance until the 2 s TTL. **A corrective snapshot cannot serve here and was tried first**: a refusal does not bump `m_SetVersion`, so the push is indistinguishable from the resync and the content-based test above correctly reads it as "still waiting".

The Battle Royale mod talks to it **only** through `VigridMapAPI` (`Scripts/4_World/VigridMapAPI.c`), each call site wrapped in `#ifdef VIGRID_MAP`:

```c
#ifdef VIGRID_MAP
    VigridMapAPI.SetZones( cur_center, cur_radius, next_center, next_radius );
#endif
```

- Client: `SetZones` / `ClearZones` — push, not pull, because the addon may not reach into `BattleRoyaleClient`. Called every frame from `BattleRoyaleClient.Update` (it diffs internally) and `ClearZones()` from the destructor.
- Client: `SetHotZones` / `ClearHotZones` — **hot zones ride the same push contract**, from the same call site and the same destructor. They are static, admin-configured circles from `zone_settings.json` (`hot_zone_centers` / `hot_zone_radii` / `hot_zone_margin_m`, v5) marking regions of interest, and they are **purely cosmetic** — nothing reads them but the three renderers. Drawn filled-and-outlined on all three surfaces, always *under* the play-area rings, so a hot zone can never obscure the circle a player has to run to. Four things are load-bearing:
  - **Two different filters, and confusing them is the trap.** `BattleRoyaleServer.b_HotZoneRefSet` is the **phase** gate: nothing is sent to anybody until a state calls `SetHotZoneReference`, which is what keeps hot zones off the map for the whole lobby *and* the pre-match countdown without a single phase test at the three draw sites. The distance test in `SendHotZones` is the **relevance** one: only circles touching the starting play area (plus `hot_zone_margin_m`) are sent, because a ring in the far corner of the map is noise nobody will ever visit.
  - **The reference circle is cached on the server, not re-derived per send.** It needs `GetDynamicStartingZone` and `i_NumStartingPlayers`, both of which live on `BattleRoyaleState` — and `PlayerLoadedIn` has no state to ask. `3_BattleRoyaleSpawnSelection.Activate()` sets it (before `ShowSpawnSelection`, so the menu's first frame already has the data), and `5_BattleRoyaleStartMatch.Activate()` re-asserts it — **that second call is not redundant**, because spawn selection is optional and a server with `enable_spawn_selection_menu` off would otherwise never show a hot zone at all.
  - They carry their **own** sequence number (`GetHotZoneSeq`, and `BattleRoyaleRPC.hot_zone_seq` on the BR side) because they arrive on a separate RPC that often lands while a map is already open — without folding it into the edge check they would wait out the 1 s watchdog.
  - Delivery is **per-identity from `BattleRoyaleServer.PlayerLoadedIn`**, not a broadcast, which is what covers a mid-match joiner. Verified live: two clients received on their own load-in 13 s apart, which a broadcast could not have produced.

  `BattleRoyaleZoneData.Validate()` truncates the two arrays to equal length and drops anything undrawable, so no consumer re-checks the pair. Note the arrays are **per-mission overridable** like everything else in that file — and because the override is per-key rather than a merge, a mission that sets `hot_zone_centers` and forgets `hot_zone_radii` produces exactly the length mismatch `Validate()` clamps.
- Client: `SetSelfPositionOverride` / `ClearSelfPositionOverride` — **where to draw "you" when that is not where your body is (#277).** Stated as a general contract, like `VigridPartyAPI.SetMemberHidden`: *a host mod can put the local player somewhere that is not where their body is.* The map addon has no concept of a spectator and must not try to detect one. `VigridMapMenu.ResolveSelfPos` feeds **both** `RenderSelfGlyph` and `CenterOnPlayer` — the second matters, or opening the map while spectating still centres on the anchor body even with the glyph in the right place. ⚠️ **Only the POSITION is overridden, never the heading**: the heading already comes from the camera on both maps and is therefore already right while spectating. ⚠️ **And it is asserted, not inferred** — "use the camera when it differs from the body" would put every third-person player's glyph several metres behind themselves. The **minimap needed no change at all**, because it re-centres on the camera every tick and never asks the player; that asymmetry is what the bug was.
- Client: `SetAdminPlayers` / `ClearAdminPlayers` — **an arbitrary set of named, coloured people to plot (#279).** In practice an admin spectator's overview of a whole match. Four things are load-bearing:
  - **The addon is told nothing about what they are.** Colours arrive as resolved ARGB ints, precisely so no concept of a party crosses the seam — unlike teammates, which need `VigridMapTeam`. **No indirection file is needed here**, because nothing on this side reads another addon; the host pushes.
  - **Positions are the host's SERVER positions, which is the whole point.** An admin watches from far outside the network bubble, so a layer built from `ClientData` would show a handful of players at most. It is also why nothing is interpolated: a glyph 4 km away steps by about a pixel.
  - **A hollow square**, the last silhouette not spent on the marker, zone, teammate, ping and self dart — and the only one with a flat top edge, which the eye reads before it can count vertices. It carries **no heading**: the push has no yaw, and implying a facing it was never told is worse than admitting it does not know.
  - **It has an edge AND a clock**, unlike every other layer. The sequence catches the set growing or shrinking; the team clock covers positions moving, since a 2 Hz repaint of moving glyphs visibly steps.
- ⚠️ **The name labels are a DECLARED pool of `TextWidget`s inside the `MapWidget`** — `AdminName0`..`AdminName31` in `map_menu.layout`. Script binds them in `Init`, then only positions and fills them; it never creates one. See the corrected rule above for the two builds that established this, and note the **pool cannot grow at runtime**. They position in the same **canvas-local** pixels as the glyph beneath them, since both are children of the same map, and clipping is the `MapWidget`'s own. Each is a `TextWidget` with a **shadow** rather than a panel backdrop: half the widgets, and it sidesteps the `PanelWidget`-with-a-colour-and-no-`style` trap that paints nothing.
- **The `[Admin]` funnel line is what found both of this layer's bugs and is worth keeping.** "Glyphs but no names" is indistinguishable by eye from five causes — the zoom gate, an empty name, the pool failing to bind, a label clipped out, or the widget not rendering. Two readings named two different ones outright: `labelled=0 wantnames=false pool=0` was the zoom gate with its comparison inverted (scale runs **0 fully in to 1 fully out**, so the test is `<=`, not `>=`), and `labelled=12 wantnames=true pool=12` was everything running with nothing drawn. Neither was reachable by reading the code. `scale` is logged raw and not just its verdict, because otherwise `wantnames=false` cannot distinguish a wrong threshold from a wrong comparison.
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
- **An incremental `Deploy.bat` does not always delete the PBO of an addon you just disabled.** It cleans orphans only sometimes, so a `config.cpp` → `.disabled` rename can leave the previous PBO in `%ModBuildDirectory%` and the addon still loads — which silently invalidates a discipline negative-build. Check the output folder and delete the `.pbo` plus its `.bisign` by hand. **The same applies to `CopyExtraPBOs`**: turning it off does not remove the third-party PBOs a previous build already copied there.
- **`Tools/edds.py` reads and rewrites the Enfusion `.edds` texture container**, whose format is documented in that file's header (decoded from scratch; two traps — the chunk table runs *smallest mip first*, and the LZ4 blocks inside a chunk are *linked*). `selftest` is its acceptance gate and is the thing to run first. It is what took the twelve loading screens from 120 MB of uncompressed BGRA8 to 15.4 MB of DXT1.
- `Extra/RandomMenuGear/` re-dresses the main-menu intro character in a random outfit plus a slung rifle and a melee weapon, re-rolled on every menu show. It hooks vanilla `IntroSceneCharacter.CreateNewCharacterById` (creation, prev/next arrows) and `MainMenu.OnShow` (returning from a submenu — that path calls `OnChangeCharacter(false)` and never recreates the character). It is **not** a fix for the broken character save that makes the menu character render naked; it only decorates the spawned object. Gear is applied with `GameInventory.CreateAttachmentEx` and deliberately never written into `MenuDefaultCharacterData` — that map is serialized to the server on connect and saved locally, so writing to it would leak menu gear into the real spawn loadout. Same discipline rule as `Party/` and `Extra/KillFeed/`: no `BattleRoyale*` symbol may be referenced.
- Spectating is entered **in place on death** — no disconnect, no reconnect — behind `spectate_enabled` in `general_settings.json`, which defaults **off**. **It has a known ~1 km limitation**: the network bubble stays on the spectator's corpse, so a target further than that is not replicated and the spectator sees a nametag with no character. Measured both directions 2026-08-10. See *Architecture → Spectating*. The orphaned `GUI/layouts/hud/spectator/player.layout` is still unreferenced: there is no spectator HUD, only a notification naming the current target.
- `Workbench/version` (`0.8.100368`) is a DayZ build number read by nothing. The mod version is `BATTLEROYALE_VERSION`.
