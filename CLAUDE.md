# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A DayZ Standalone mod (Bohemia Interactive Enfusion engine) implementing battle-royale gameplay, written in **EnfusionScript** (`.c` files). Originally by Kegan, maintained by Myst. Version `0.2.1-Vigrid`, defined in `Scripts/Client/2_GameLib/BattleRoyaleConstants.c` and mirrored in `mod.cpp`.

Hard dependencies (mod load order set in `Workbench/project.cfg`): [CF](https://github.com/Arkensor/DayZ-CommunityFramework), [Dabs Framework](https://github.com/InclementDab/DayZ-Dabs-Framework), [Community-Online-Tools](https://github.com/Jacob-Mango/DayZ-CommunityOnlineTools). Each link is the mod's source repository — consult it for a dependency's API rather than guessing.

⚠️ **[DayZ-Expansion](https://github.com/salutesh/DayZ-Expansion-Scripts) is NO LONGER a hard dependency — the mod compiles and runs on both sides without it.** **A `requiredAddons` entry for an ABSENT addon is NOT harmless, and this file asserted the opposite until 2026-08-22.** `Scripts/Client/config.cpp` listed `DayZExpansion_Scripts` deliberately, to order this mod's `modded class`es after Expansion's; on a dedicated server with Expansion not loaded that killed the server outright at addon load, with the modal *"Addon 'BattleRoyale_Scripts_Client' requires addon 'DayZExpansion_Scripts'"* and **an RPT that stops after the header** — no addon list, no `script_*.log`, nothing to read, because scripts never got as far as compiling. That signature is the fingerprint of a `CfgPatches` dependency failure and is worth recognising on sight; note the error goes to a modal / stdout, so a server run as a service shows nothing at all.

The entry is gone. **Ordering against Expansion is now done by `-mod=` order instead** — `project.cfg` lists `@Vigrid-BattleRoyale` last, after the three Expansion mods, where it used to sit before them and let `requiredAddons` do the work. Two lessons generalise. `Tools/check.py`'s `configs` check *did* resolve the name — `DayZExpansion_Scripts` was simply sitting in `Tools/allowlists/external_addons.txt`, added in good faith under the same wrong belief, so **the allowlist entry is what made the check complicit** rather than blind. It now carries a second lock, `Tools/allowlists/optional_addons.txt`, tested to reject the name **even when re-allowlisted**. And — the part worth carrying furthest — **no local launch in this repo could ever have caught it**, for a reason that has nothing to do with Expansion: see *Testing → the local rig demotes fatal errors* below. What remains optional and guarded:

| Reached through | Guard | Gives up when absent |
|---|---|---|
| `m_NewsFeed`, `Expansion_OnGeneralSettingsUpdated` | `#ifdef DZ_Expansion` | Expansion's news feed panel is no longer hidden in the Esc menu |
| `GetExpansionClientVersion()` | `#ifdef DZ_Expansion_Core` | the Expansion version segment in the two version lines |
| `ExpansionMissionModule.CallAirdrop` | `#ifdef EXPANSIONMODMISSIONS` | airdrops |
| `ExpansionNotificationSettings` (Extra/KillFeed) | `#ifdef EXPANSIONMODKILLFEED` | suppressing Expansion's own kill feed |

⚠️ **`DZ_Expansion` and `DZ_Expansion_Core` are Expansion's `CfgMods` CLASS NAMES, not `defines[]` entries.** The engine auto-defines the class name of every loaded mod, which is the only way to `#ifdef` a mod that publishes no flags — and Expansion publishes none. Same mechanism that defines `DabsFramework` and `JM_CommunityFramework`, neither of which appears in any `defines[]` either.

## Workflow

**Always work in a git worktree.** Before the first edit for any new feature or fix, call the `EnterWorktree` tool — it creates a branch plus a worktree under `.claude/worktrees/` and switches the session into it. Never commit directly to `main`.

- Skip it for read-only work: answering questions, searching, reading logs.
- Skip it if the session is already inside `.claude/worktrees/` — worktrees do not nest.
- **A worktree builds itself: run `Deploy.bat` from the WORKTREE's own `Workbench/Batchfiles`.** The build packs **the checkout its batch files live in** (`srcRoot`, set by `_Config.bat`), and never reads a source through the `P:\Vigrid-BattleRoyale\` junction any more. It used to: `CI.bat` did `pushd "%workDrive%%prefixLinkRoot%\"`, so building worktree code meant re-pointing that one global junction. Claude Code's sandbox refuses to remove that path, and a junction left pointing at the wrong checkout silently packed the wrong sources. Three things make a worktree self-sufficient:
  - **Gitignored local state is read from the primary checkout.** `_Config.bat` asks git for the common dir (`git rev-parse --git-common-dir`, which names the primary's `.git` from any worktree) and exports `brPrimaryWorkbench`. A checkout with no `user.cfg` uses the primary's, and says so. `CI0_CopyExtraPBO.bat` does the same for `ExtraPBOs\`, whose only tracked file is its README. Without that a worktree build with `CopyExtraPBOs=1` dropped the two extra PBOs as orphans and shipped a different mod from a primary build. `project.cfg` always comes from the checkout being built, because it is tracked and may be what changed. ⚠️ The `=` in `--path-format=absolute` has to be written `^=` inside `for /f`. A bare `=` is an argument separator there, git gets a valueless flag, and the fallback fails without a word.
  - **`_EnumPaths.bat` applies its dot-folder filter only BELOW the current directory.** A worktree's own root is under `.claude\worktrees\`, so testing the whole path would discard every file it is meant to find. Building the primary still never picks up worktree sources: they are below its root and do contain `\.`. Measured: 25 configs found from the primary, with two worktrees inside it.
  - **The PBO prefix is derived by cutting `srcRoot` off the front of the path** (`CI1.bat`, `relDir`), not by deleting every `\Vigrid-BattleRoyale\` from it, which is what the junction made safe.

  `BR_SOURCE_ROOT=<checkout>` overrides `srcRoot` for one run: `set BR_SOURCE_ROOT=...\.claude\worktrees\other` then `Deploy.bat` packs that checkout with the invoking checkout's batch files. **The junction is still required**, and `SetupMod.bat` still creates it, pointing at the primary. Binarize resolves cross-PBO model and texture references through `P:\`, so a worktree that changes a texture another PBO's `.p3d` references still binarizes against the primary's copy. No script, layout or config read goes that way.

⚠️ **`Build.success` is NOT proof of whose sources were built.** `Build.log` and the three markers land in `Workbench\Logs\` beside *the batch files you invoked*, which with `BR_SOURCE_ROOT` is not the checkout that was packed. **Verify the PBO instead**: check its timestamp in `%ModBuildDirectory%` and grep it for a string only your change introduces. (The `.pbo` is not compressed, so a plain byte search finds script text.) Every `Mirroring "..."` line in `Build.log` also names the source folder in full now.

**Switching the source checkout repacks every PBO** (~50 s instead of a few). The mirror under `P:\Temp\Vigrid-BattleRoyale\` is shared, and file times differ between checkouts, so robocopy sees every folder as changed. That is correct, not a bug: the old junction build *hid* a source switch from the incremental check, which is how stale lists went unnoticed.
- Finish the work with a commit on the worktree branch; push and open a draft PR when the change should go to the remote.

**Most of the hard-won DayZ knowledge here is written down three times** — in this file and `Docs/`, in the
project memories, and in the global skills under `C:\Users\Myst\.claude\skills\` (`dayz-modding`,
`dayz-source`, `dayz-ui`, `dayz-multiplayer`, `slow-loop-debugging`). That is deliberate — the skill
carries the rule, the memory keeps the dated measurement and the retractions, this file and `Docs/` keep the
mod-specific detail — but it means **a correction has to be applied to every copy**. Several
findings here were overturned after they were first written up, and the wrong version outlived the
correction in a file nobody re-read. So when a memory-consolidation pass runs, or whenever a rule is
re-diagnosed, **grep `C:\Users\Myst\.claude\skills\`, this file and `Docs/` too**, not just the memory
directory.

**The GitHub wiki is a fourth copy, written for server operators rather than developers.** It lives at
<https://github.com/LeMyst/Vigrid-BattleRoyale/wiki>. That is a separate git repo,
`LeMyst/Vigrid-BattleRoyale.wiki.git`, cloned beside this one as `..\Vigrid-BattleRoyale.wiki\`. It is
plain Markdown pushed straight to `master`, with no PR. It restates, page by page, behaviour that is
otherwise only in code:
- every settings field, with its default, unit and real effect;
- the lobby start conditions;
- the match phases;
- the `-mod=` order;
- the dzhooks modules and their log lines;
- the COT permissions;
- every keybind.

It also carries four example setups whose JSON is copy-paste ready. **So a change to any of these is not
done until the matching wiki page says so too.** Each settings file has its own page (`General-Settings`,
`Lobby-Settings`, `Zone-Settings`, …, `Party-Settings`, `Addon-Settings`), and an example that sets a
renamed or removed key now hands operators a dead key. A new settings field means a new table row, plus a
`version` bump on that page.

It was first written against `78a5850` (2026-11-08), and `Home` names the commit it describes; move that
reference when you resync. **`README.md` deliberately carries no operator reference any more**: its
*Running a server* section is a table of links into the wiki. Its old copies of the config table, the
zone tuning notes and the `-br-*` flags had all drifted from the code, so add new operator detail to
the wiki, not back into the README. Two other details:
- The wiki deliberately documents current behaviour, defects included. The defects themselves are filed
  under *Wiki pass (2026-11-08)* in the local `TODO.md`, and each one names the page to update once it is
  fixed.
- **Before pushing a wiki edit, re-run the same three checks the first version passed:**
  - every internal link and anchor resolves;
  - every fenced ```json block parses as strict JSON;
  - every key in those blocks exists as a member of the matching settings class on `main` — and every
    member of each class appears on its page.

  Prove the checker in both directions with a deliberately wrong key, as with any check here.

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
2. `Workbench/Batchfiles/SetupMod.bat` — creates the `P:\Vigrid-BattleRoyale\` junction into the primary checkout, also when run from a worktree (binarize resolves cross-PBO references through it; sources are no longer read through it), then copies every folder in `Workbench/Missions/` out to `%GameDirectory%Missions\`. The junction is named after `PrefixLinkRoot` (`Vigrid-BattleRoyale`); the checkout folder's own name no longer matters.

**Re-run `SetupMod.bat` after a DayZ update** — an update wipes `%GameDirectory%Missions\`, which takes the offline test mission with it. It cannot be a junction into the checkout for the same reason, so `Workbench/Missions/` holds the masters and the copy is one-way. The copy is per folder and never deletes, so an unrelated mission already installed there survives; the build never sees these files, since `_EnumPaths` only enumerates `config.cpp`.

**All batch files must be run with cwd = `Workbench/Batchfiles`** — several `call` siblings by relative path.

| Command | Purpose |
|---|---|
| `Deploy.bat` | Build. Thin wrapper over `CI.bat` → `CI_Build.bat` → `CI1.bat`. Binarize + rapify + pack + sign into `%ModBuildDirectory%@Vigrid-BattleRoyale\`. 2-5 min. |
| `Deploy.bat rebuildAll` | Wipe the output mod folder and rebuild everything. |
| `LaunchOffline.bat` | Single-player with `SPMission`. |
| `LaunchServer.bat` | Dedicated server (clears logs + `storage_*` first). |
| `LaunchLocalMP.bat [1\|2\|3]` | Server + N local clients. `LaunchLocalMP2.bat` / `MP3.bat` are wrappers for `2` / `3`. |
| `Release.bat <Launcher.bat> [args]` | Run any launcher against the **retail** exes instead of the diag ones — `Release.bat LaunchServer.bat`. The only local way to reproduce a fatal config/addon error; see *Testing* below. |
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
| `_Config.bat <tag>` | The **only** way config is loaded. Transliterates `project.cfg` then `user.cfg` into `Workbench/combined.cfg.<tag>.bat` (`set "K=V"` per line, `#` and `;` comments skipped) and calls it. `user.cfg` wins by load order. The `<tag>` keeps concurrent callers off each other's artifact. Also exports `srcRoot` (the checkout to pack: this one, or `BR_SOURCE_ROOT`) and `brPrimaryWorkbench`, and falls back to the primary's `user.cfg` when this checkout has none. |
| `_Require.bat <Key>` | Echo a config value; non-zero exit if empty. |
| `_EnumPaths.bat <pattern> <outputFile>` | The **only** way the build enumerates sources. Recursive `dir /B /S` from the caller's cwd, minus every path containing a dot-prefixed folder (`.claude`, `.git`, `.idea`, …) **below that cwd**. The part above it is not tested, which is what lets a worktree under `.claude\worktrees\` build itself. Exits 0 when nothing matches. |
| `_SafeDir.bat <path>` | Gate in front of every recursive delete. Normalises the path and exports it as `%brSafeDir%`. Exit `0` safe, `1` unsafe — a drive root, UNC, bare `X:`, leading `\`, or an exact match on a system dir like `%USERPROFILE%` — `2` well-formed but absent (caller skips quietly). Used by `ClearLogs.bat` / `ClearStorage.bat`, whose paths come from the gitignored `user.cfg`. |
| `_LaunchClient.bat <A\|B\|C>` | Resolve the slot's `Player[B\|C]SteamID` / `Player[B\|C]Name` / `Client[B\|C]ProfileDirectory` and start it. |
| `_LaunchServer.bat <SteamID>` | Clear logs + storage, then start the dedicated server. |
| `_ReleaseEXE.bat <MP\|SP>` | Swap `ClientEXE` / `ServerEXE` for the retail ones and strip the diag-only launch parameters, then export the lot back into `SetupLaunch.bat`. Called only when `ReleaseEXE=1`. |
| `SetupLaunch.bat <MP\|SP>` | Preamble for every `Launch*.bat`: config, validation, mod list, kill running game. Deliberately no `setlocal` — it exports into the caller. |

Each `config.cpp` with no ancestor `config.cpp` becomes one PBO — currently 8 mod PBOs (`Data`, `GUI`, `LanguageCore`, `Models_Shapes`, `Sounds`, `Scripts_Client`, `Scripts_Server`, `Party`) plus 17 `extra_*`. Renaming a top-level folder renames its PBO. PBO names are lowercased by the build, so `Extra/KillFeed` packs as `extra_killfeed.pbo`.

## Testing

**There are no unit tests and nothing compiles EnfusionScript outside the game — but there IS a static check suite, and it runs in CI.** `Tools/check.py` (stdlib only, ~5 s) holds the checks tabled below, over the tracked sources; `.github/workflows/checks.yml` runs `python Tools/check.py -W` on every push to `main`, every PR and on demand. Run it locally the same way, or via `Workbench/Batchfiles/Check.bat`, which is the one batch file needing neither `user.cfg` nor a mounted `P:` because it only reads sources.

```bash
python Tools/check.py            # everything
python Tools/check.py --list     # name every check
python Tools/check.py --only rpc # one (repeatable, or comma-separated)
python Tools/check.py -W         # warnings fail too - what CI runs
```

| Check | Catches |
|---|---|
| `asset-paths` | every `Vigrid-BattleRoyale/...` asset reference resolves, in scripts, layouts, imagesets, JSON, `config.cpp` (including `files[]` folders and `samples[]`) and `mod.cpp` |
| `configs` | `CfgPatches` names unique, `requiredAddons` resolve to a built addon (never a `config.cpp.disabled` one) **and name no optional mod's addon**, no stripped vanilla parents |
| `client-only-types` | no server-compiled file names a `#ifndef SERVER` class — a bug an offline client cannot see |
| `data` | tracked JSON parses; no UTF-8 BOM on any data file |
| `diag-guards` | no retail-compiled file names a `#ifdef DIAG_DEVELOPER` class or method — the defect no local launch can see |
| `discipline` | standalone addons name no `BattleRoyale*` symbol; host calls are `#ifdef`-guarded |
| `enfusion` | no ternary operator, no multi-line `if`/`while` conditions |
| `preprocessor` | every `#if*` has its `#endif` in the same file; no stray `#else` / `#endif` |
| `extra-index` | every `Extra/` addon has a README, an index entry and a config |
| `guards` | every `Scripts/Server` file opens with `#ifdef SERVER` |
| `inputs` | every `Inputs.xml` parses, and its three sections agree |
| `menu-ids` | no two `MENU_*` constants share an id across the PBOs |
| `rpc` | every `AddRPC` name, literal or `const string`, has a CF handler method; every `SendRPC` has an `AddRPC` on the same namespace |
| `settings-version` | a new settings field bumps `version`; a new `ref array` gets an `Upgrade()` branch |
| `stringtable` | every `STR_*` key referenced is defined, in its own addon's table |
| `version` | `BATTLEROYALE_VERSION` and `mod.cpp` agree |

**What it does NOT do is compile anything**, so a green run says nothing about whether the mod loads. Every check decides from source text alone, and what they cover is precisely the class of defect that fails **silently at runtime with a clean `.rpt`** — a dead RPC registration, a colliding menu id, an unguarded server file, a settings field that loads back empty. `-W` is affordable because the tree is clean, so a new warning must never be left to pass silently.

⚠️ **CI cannot build the mod and no hosted runner can** — binarize/rapify/pack need DayZ Tools, Windows and a mounted `P:`. A self-hosted Windows runner would be *worse than nothing*: the build mirror under `P:\Temp\Vigrid-BattleRoyale\` and the output under `%ModBuildDirectory%` are single global folders, so a CI job would silently overwrite whatever an interactive session was mid-build.

⚠️ **A check that stops catching anything still goes green.** `settings-version` was fixed in #309 after its `MEMBER` pattern — which has no notion of brace depth — read three ordinary **local variables** as brand-new settings fields simply because a diff re-added those lines. When changing a check, prove it in both directions: construct an input it must reject and confirm it still does, exactly as with the diag fixtures elsewhere in this file.

⚠️ **THE ENTIRE LOCAL RIG RUNS `DayZDiag_x64.exe` WITH `-newErrorsAreWarnings=1`, ON BOTH SIDES, SO NO LOCAL LAUNCH CAN REPRODUCE A FATAL CONFIG OR ADDON ERROR.** `project.cfg:7-8` set *both* `ClientEXE` and `ServerEXE` to the diag exe, and `user_sample.cfg:31-32` put `-newErrorsAreWarnings=1` on both `ServerLaunchParams` and `ClientLaunchParams`. Production runs `DayZServer_x64.exe` with neither.

The measurement that established this (2026-08-22): the `DayZExpansion_Scripts` `requiredAddons` entry killed a remote server outright, and a local server booted **the very same afternoon, with Expansion genuinely absent from its mod list** — `-mod=@Vigrid-BattleRoyale;@CF;@Community-Online-Tools;@Dabs Framework;@Vehicle3PP`, no Expansion anywhere — and produced a full RPT, a full `script_*.log` and a playable server. **The bug was present in every local run and left no trace in any of them.** `grep -ril "requires addon"` over `ServerProfileDirectory` and every client profile returns *nothing*: the error is not merely demoted to a warning you could go looking for, it is demoted and then **not logged at all**.

So: a clean diag `.rpt` is evidence about *script*, and no evidence at all about whether the mod will load on a real server. The only two things that can catch this class of defect are `Tools/check.py` and an actual `DayZServer_x64.exe` boot.

**`Release.bat` is that second one, and it needs no `user.cfg` edit.** It wraps any launcher — `Release.bat LaunchServer.bat`, `Release.bat LaunchLocalMP.bat 2`, `Release.bat LaunchOffline.bat` — by setting `BR_RELEASE=1`, which `SetupLaunch.bat` turns into `ReleaseEXE=1`; `_ReleaseEXE.bat` then swaps in `ReleaseClientEXE` / `ReleaseServerEXE` (`DayZ_x64.exe` / `DayZServer_x64.exe`) and **strips every diag-only launch parameter — `-newErrorsAreWarnings` and `-filePatching` — from both sides**, naming each one it drops. Reach for it before shipping anything that touches `config.cpp`, `requiredAddons`, `defines[]` or a `#ifdef` guard. Four things are worth knowing:

- **The strip is a rebuild, not a string replace.** `%VAR:search=replace%` splits at the *first* `=`, so it cannot express removing `-newErrorsAreWarnings=1`, whose own value contains one. `_ReleaseEXE.bat` tokenises the parameter string and drops any token containing a diag needle instead — which is also why every needle in `brDiagFlags` must itself be `=`-free.
- **The retail server exe is usually in a different folder from the diag one**, which lives in the client install that `ServerDirectory` often points at. `ReleaseServerDirectory` / `ReleaseClientDirectory` override the two directories for release launches only; a missing exe is a hard error naming the exact path it looked at.
- ⚠️ **`-br-autoconnect` is `#ifdef DIAG_DEVELOPER` and is therefore gone**, so retail clients stop at the main menu and have to be connected by hand (Direct connect, `127.0.0.1`). The whole diag menu and all its fixtures go with it — so a release run tests *loading*, not the diag rig.
- **`ReleaseEXE=1` in `user.cfg` is the persistent form**, but prefer the wrapper: `Release.bat` uses `setlocal`, so the next launch is a diag one again and nobody is left wondering why the diag menu vanished.

**The defect class it exists for, with a worked example.** `BattleRoyaleState.BR_DiagTeleport` sat inside a `#ifdef DIAG_DEVELOPER` block while `BRMasterControlsModule`'s *production* "teleport to the live circle" admin action called it unguarded — right under a comment asserting it was ungated. `DIAG_DEVELOPER` is defined on every local launch, so it compiled cleanly every time; the first retail server took the whole Mission module down with `Undefined function 'BattleRoyaleState.BR_DiagTeleport'`, followed by a cascade of `Bad type 'Param2'` / `Can't find class Param3` / `Bad type 'map'` lines. **That cascade is a red herring — the template types are fine; fix the FIRST error and the rest go.** Fixed 2026-08-22 by moving the method out of the guard (its doc comment now says why), leaving `BR_DiagTeleportRing` inside it since all of *its* callers are diag. The rule is **gate the callers, never the shared helper**, and `Tools/check.py`'s `diag-guards` check now enforces it. Three things about that check are worth knowing:

- ⚠️ **It is exact for classes and APPROXIMATE for methods.** Enfusion class names are globally unique, so a class declared only inside a diag guard really is absent from a retail build; method names are per-class and the check has no type information, so a diag-only method colliding with a **vanilla** method that shipping code calls would be a false positive. `Tools/allowlists/diag_refs.txt` is for exactly that and nothing else — a symbol from this repo landing there is almost certainly the real bug.
- **Two exclusions carry the whole signal-to-noise ratio**, and a prototype without them emitted 137 KB of findings, none real. A name declared *unguarded anywhere* in the tree is not diag-only; and **a declaration line does not end in `;` while a call statement does**, which is what stops `int level = GetGame().ServerConfigGetInt(...);` reading as a declaration of `ServerConfigGetInt`.
- ⚠️ **It skips addons parked as `config.cpp.disabled`.** They produce no PBO, so nothing under them is compiled and a declaration there cannot make a symbol diag-only anywhere — `Extra/DumpItemHeights/` declares a diag-only `Close()`, which collided with vanilla `UIScriptedMenu.Close()` and reported eight shipping call sites as broken.

Proven in both directions, as a new check must be: re-introducing the real bug reports `BRMasterControlsModule.c:484` and nothing else, a synthetic unguarded reference to the `BattleRoyaleDiag` class reports that, and the ~120 genuinely diag-only symbols called only from diag code stay silent.

⚠️ **`user.cfg` is read last-wins, and Myst's carries a dozen commented-out `AdditionalMPMods` lines with two or three live ones.** The effective mod list is the *last uncommented* assignment, not the first, and not the one that looks canonical. Read it with `grep -n "^AdditionalMPMods"` and take the bottom hit — assuming the wrong line is how "Expansion is always loaded locally" got asserted in this file and was wrong within the hour.

Validation loop for anything the suite cannot see: `Deploy.bat` → launch (`ClientEXE` defaults to `DayZDiag_x64.exe`) → read the `.rpt` in the profile directory for script errors.

**Reach for `LaunchOffline.bat` first, and only escalate to `LaunchLocalMP.bat N` when the test genuinely needs a server.** It is one process instead of two-to-four, it loads in ~90 s, and it gives a real controllable character — so the whole client half of the mod is testable in it: HUD and widget layout, menus and keybinds, the map addon, the compass, name tags, world markers, `#ifndef SERVER` code paths, and anything judged by eye. What it cannot reach is the mod itself: `SERVER` is undefined offline, so all of `Scripts/Server/**` compiles out — no `BattleRoyaleServer`, no states, no zones, no RPCs, no settings JSONs. Anything keyed on match state, a second player, or a server→client push needs MP.

⚠️ **Offline cannot validate GUARDS EITHER, and that is stronger than "it skips the server files".** With `SERVER` undefined, a `#ifndef SERVER` type is *always* present, so unguarded code that names one compiles perfectly offline and takes down a whole module on a dedicated server — `loadingscreenbackground.c(9): Bad type 'BattleRoyaleShuffleBag'` was exactly this, and `BattleRoyaleSpectatorCamera` carries a comment about the same thing happening to `BattleRoyaleRPC` before it. The `client-only-types` check exists to catch this without a server boot; it does not remove the need to actually boot one before believing a change is safe.

⚠️ **A dependency on another mod can also be a bare method call that no grep for that mod's name will find.** `PlayerBase.GetIdentityName()` is declared on Expansion Core's modded `PlayerBase` and reads exactly like vanilla API at the call site; it sat in `0_BattleRoyaleState` for a long time and only surfaced the first time a server booted without Expansion. **The only reliable audit is to compile with the other mod absent** — `AdditionalSPMods` / `AdditionalMPMods` in `user.cfg` is where you take it out.

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

## Subsystem notes (`Docs/`)

**Most of the per-subsystem detail lives in `Docs/`, not here — read the matching file before changing code in that area.** It was split out of this file on 2026-11-02 because the whole thing had grown to 260k characters, past what Claude Code will load as one instruction file. Nothing was condensed: each section moved verbatim. `Docs/` has no `config.cpp`, so it never reaches a PBO.

| File | Read before touching |
|---|---|
| [`Docs/boot-time.md`](Docs/boot-time.md) | server boot time, `BootTime.bat`, `boottime.csv`, Central Economy spawn cost, `KeepStorage` |
| [`Docs/late-joiners.md`](Docs/late-joiners.md) | `OnPlayerConnected`, `ScheduleLateJoinKick` / `ArmLateJoiner`, `PlayerLoadedIn`, admins joining mid-match |
| [`Docs/lobby-chickens.md`](Docs/lobby-chickens.md) | `BattleRoyaleChickens`, `TickLobbyChickens`, the win-state chicken rain |
| [`Docs/teleports.md`](Docs/teleports.md) | match-start and F2 unstuck teleports, sync juncture 88, inventory locks, `ClearStuckMovementState`, the `SetDisabled` aim desync, the open ladder bug |
| [`Docs/zones.md`](Docs/zones.md) | `BattleRoyaleZone*`, `zone_settings.json`, chain generation, radius flex, round timers, the tier ladder, the building census, `zone_selftest_runs` |
| [`Docs/player-names.md`](Docs/player-names.md) | `BattleRoyaleNameService`, Steam name lookup, `Survivor` placeholders |
| [`Docs/party.md`](Docs/party.md) | anything under `Party/` or calling `VigridPartyAPI`: grouping, forced team size, the party menu, pings |
| [`Docs/spectating.md`](Docs/spectating.md) | the death screen, `BattleRoyaleSpectators`, the spectator camera, corpse carry, the audience counter, admin spectate, its overlays and skeletons |
| [`Docs/toasts.md`](Docs/toasts.md) | `BattleRoyaleToasts`, every player-facing notification |
| [`Docs/ui.md`](Docs/ui.md) | lobby name tags, the leaderboard / party-menu scrolling lists, the trimmed vanilla HUD and injured-leg badge |
| [`Docs/kill-attribution.md`](Docs/kill-attribution.md) | `BattleRoyaleKillAttribution`, explosive and trap owners, `BattleRoyaleKillLedger` |
| [`Docs/death-recap.md`](Docs/death-recap.md) | the death recap, damage measurement, `last_match.json`, the last-match leaderboard tab |
| [`Docs/killfeed.md`](Docs/killfeed.md) | `Extra/KillFeed/` |
| [`Docs/safezone.md`](Docs/safezone.md) | `Extra/SafeZone/` |
| [`Docs/autorun.md`](Docs/autorun.md) | `Extra/AutoRun/` |
| [`Docs/map.md`](Docs/map.md) | `Extra/Map/`, `Extra/MapSatellite/`, and every `VigridMapAPI` call |
| [`Docs/earplugs.md`](Docs/earplugs.md) | `Extra/EarPlugs/` |
| [`Docs/native-hooks.md`](Docs/native-hooks.md) | the optional dzhooks native hook library, `BattleRoyaleNativeLock`, `server_lock.txt`, `write_lock_file`, `BattleRoyaleNativeBubble`, `BattleRoyaleNativeAnnounce`, `BattleRoyaleNativeRefill` and the server-browser name |
| [`Docs/in-place-reset.md`](Docs/in-place-reset.md) | `BattleRoyaleInPlaceReset`, the `in_place_reset*` settings, `9_BattleRoyaleRestart`, `BattleRoyaleCorpses`, any `ResetForNewMatch`, and any new static that holds match state |

What stays in this file is what nearly every change needs: build and test, the EnfusionScript rules, the script-module split, the match state machine, networking, settings, localization and the key map. When a new subsystem note outgrows a few paragraphs here, give it a file in `Docs/` and a row above.

## Architecture

### Script modules and the Client/Server split

The engine compiles in fixed stages: `1_Core → 2_GameLib → 3_Game → 4_World → 5_Mission`. `config.cpp` maps a folder to a stage via `class defs { class <stage>ScriptModule { files[] = {...} } }`. Files are registered **by directory**, so adding a new `.c` file needs no config change; adding a new stage folder does.

**`Scripts/Client` vs `Scripts/Server` is not a runtime split.** Both PBOs ship in the same mod and load on both sides. What actually gates execution is the preprocessor guard on line 1 of each file:

- `Scripts/Server/**` — every file opens with `#ifdef SERVER`.
- `Scripts/Client/**` — `#ifndef SERVER` for client-only, **or no guard at all** for shared code that compiles on both sides (e.g. `BattleRoyaleConstants.c`, `BattleRoyaleUtils.c`, `BattleRoyaleBase.c`, `MissionBaseWorld.c`, `BattleRoyalePlayArea.c`).

When adding a file, pick the folder for organisation and **always add the right guard.** An unguarded file under `Scripts/Client/` runs on the server too.

`BattleRoyale_Scripts_Server` lists `BattleRoyale_Scripts_Client` in `requiredAddons`, so server files compile *after* client files in every stage — that ordering is what lets server code `modded class` over client-declared classes.

Compile-time feature flags live in `Scripts/Client/config.cpp:45-50` (`defines[]`). Only `DAYZ_BATTLEROYALE` is active; `BLUE_ZONE` and `BR_TRACE_ENABLED` are commented out but gate real code. Re-enabling `BLUE_ZONE` also means putting `BlueZone` in that file's `CfgVehicles` back to `scope=2`: it is `1` while the define is off, so admin spawners do not offer a class with no script behind it. **`MOVING_ZONE` was deleted on 2026-11-08.** It gated nothing: there was no `#ifdef` for it anywhere in the tree or its history, and with no trailing comma it was a rapify trap. The unmerged `moving-zone` *branch* is unrelated to it. Other `#ifdef`s in use: `SERVER`, `DIAG`, `VIGRID_PARTY` (the in-repo party addon, see below), `JM_COT`, `VPPADMINTOOLS`, `EXPANSIONMODMISSIONS`.

⚠️ **`BR_MINIMAP` and `EXPANSION_MAP_ZONES` are gone (2026-08-15), and both names are worth recognising** because a lot of prose still refers to them. They gated this mod's own minimap and its zone circles drawn into DayZ Expansion's map; both were commented out, both had rotted (the minimap layout was missing two widgets `BattleRoyaleHud.Init` dereferences, and the Expansion trio would not compile without @DayZ-Expansion-Navigation), and `Extra/Map/` has replaced everything they did. The five files, the two defines, the three `Data/Inputs.xml` entries and the `MissionGameplay` / `BattleRoyaleClient` blocks were deleted rather than repaired. `Scripts/Client/3_Game/ExpansionClientSettings.c` went with them — nine of the fourteen settings it forced were Expansion **Navigation** marker options, and it forced them from `OnSave()`, permanently rewriting the player's global Expansion settings file. **Do not re-add a mod-side minimap**: `VigridMapMinimap` behind `VIGRID_MAP_MINIMAP` is the one that ships.

### Entry points

`MissionBaseWorld` (`Scripts/Client/4_World/MissionBaseWorld.c`) holds `ref BattleRoyaleBase m_BattleRoyale`, and the free function **`GetBR()`** in the same file is how any code reaches it on either side.

- Server: `BattleRoyaleServer` (`Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleServer.c`), constructed in `MissionServer.OnInit()`, ticked from `MissionServer.OnUpdate`. Also `BattleRoyaleServer.GetInstance()`.
- Client: `BattleRoyaleClient` (`Scripts/Client/5_Mission/BattleRoyale/Client/BattleRoyaleClient.c`), constructed in `MissionGameplay.OnInit()`, ticked from `MissionGameplay.OnUpdate`.

### Match state machine (server)

States are an **ordered `array<ref BattleRoyaleState>`** built in `BattleRoyaleServer.Init()` (`BattleRoyaleServer.c:88-133`). There is no state enum — the numeric filename prefix in `Scripts/Server/5_Mission/BattleRoyale/Server/States/` encodes the order:

`1_BattleRoyaleDebug` (lobby / ready-up) → `2_BattleRoyaleCountReached` (countdown, lock server) → `3_BattleRoyaleSpawnSelection` (only if `enable_spawn_selection_menu`) → `4_BattleRoyalePrepare` (loadout + teleport) → `5_BattleRoyaleStartMatch` (warmup) → `6_BattleRoyaleRound` × `num_zones` → `7_BattleRoyaleLastRound` → `8_BattleRoyaleWin` → `9_BattleRoyaleRestart` (`RequestExit`). By default a match is one-shot and the server process restarts between matches. **With `in_place_reset` on (and the dzhooks `ce_refill` module present), `9_BattleRoyaleRestart` hands over to `BattleRoyaleInPlaceReset` instead**, which refills the loot, replaces `BattleRoyaleServer` and reopens the lobby in the same process - see [`Docs/in-place-reset.md`](Docs/in-place-reset.md). So "one match per process" is no longer a safe assumption: any new static or singleton that holds match state needs a reset there.

**The completion protocol is inverted.** `BattleRoyaleState.IsComplete()` returns `!IsActive()` (`0_BattleRoyaleState.c:85-92`), so a state signals "done" by calling **`Deactivate()`** on itself — typically from a timer callback, or from an overridden `IsComplete()` that tests its condition, calls `Deactivate()`, then `return super.IsComplete()`.

`BattleRoyaleServer.Update()` is the only driver: it `Update()`s every state each frame and checks `IsComplete()` at 10 Hz, then deactivates → migrates the player list → activates the next non-skipped state (`SkipState()` lets a state opt out entirely; rounds use it for the dynamic starting zone).

**The win condition is a test on GROUPS, and it lives in exactly one place: `BattleRoyaleState.IsOneSideLeft(players)`.** A surviving party has already won, so a match that waits for one of them to kill the other never ends. Without the party addon — or with it compiled in but the manager switched off in `party_settings.json` — `GetGroupCount()` degrades to one group per player and the helper mirrors the raw player test, which is why no caller needs an `#else` branch. It is static and takes the roster explicitly, because `SkipState()` has to ask it about the state it is handed rather than about itself. Five call sites: `5_BattleRoyaleStartMatch.IsComplete`, and `IsComplete` + `SkipState` on both `6_BattleRoyaleRound` and `7_BattleRoyaleLastRound`.

⚠️ **It was written out four separate times and one copy had drifted**, which is the whole reason it is centralised now. `BattleRoyaleStartMatch` carried only the player half, and that state owns the entire first-zone countdown (`round_duration_minutes / 2`) — so a party that cleared the field during it still held two live players, the state never completed, and **the win screen did not appear until the countdown expired on its own**. Fixed 2026-08-14; the before/after is one line in the server log, the kill's `RemovePlayer` and `Start Match State IsComplete!` now landing in the same second where they used to be 16 s apart. The same pass fixed `7_BattleRoyaleLastRound.SkipState`, whose two tests were **AND**ed, so a surviving pair satisfied `players > 1` and refused to skip — the final round activated and deactivated inside a single tick, visible as a doubled `RemoveAllPlayers`/`AddPlayer` pair either side of `Entering State 'Last Gameplay State'`.

Note the lobby's own "enough groups to *start*" tests in `1_BattleRoyaleDebug` are the inverse condition and deliberately stay separate.

`BattleRoyaleState` extends `Timeable` (`Scripts/Server/3_Game/Logic/Timeable.c`) — use `AddTimer(duration, this, "MethodName", params, looping)`; looping timers are stopped automatically by `Deactivate()`.

Long-running work uses script coroutines — `GetGame().GameScript.Call(this, "MethodName", NULL)` plus `Sleep()` (see `4_BattleRoyalePrepare.ProcessPlayers`), not the call queue.

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
| `lobby_settings.json` | `BattleRoyaleLobbyData` | `LobbyData` | pre-match lobby flow: ready-up, autostart, spawn selection, forced team size, lobby chickens |
| `zone_settings.json` | `BattleRoyaleZoneData` | `ZoneData` | zone geometry, shrink timing, zone damage, shrink-notification timing, per-match radius flex, derived round timers, the derived tier ladder and the match-duration bound |
| `voice_settings.json` | `BattleRoyaleVoiceData` | `VoiceData` | party-only voice while frozen, speaking-list panel |
| `spawns_settings.json` | `BattleRoyaleSpawnsData` | `SpawnsData` | lobby spawn point and match spawn placement |
| `pois_settings.json` | `BattleRoyalePOIsData` | `POIsData` | POI position overrides |
| `server_settings.json` | `BattleRoyaleServerData` | `ServerData` | Vigrid API/webhook + autolock infra + Steam name lookup — **no mission override** |
| `leaderboard_settings.json` | `BattleRoyaleLeaderboardData` | `LeaderboardData` | scoring curve + persistence knobs — **no mission override**, integrity-sensitive |

The mission override (`$mission:Vigrid-BattleRoyale\`) is **not a merge** — `JsonFileLoader<T>.LoadFile()` is called twice into the same instance (`Load()` then `LoadMission()`), so only keys present in the mission JSON get overwritten. `Upgrade()` runs inside `Load()`, before the mission pass, and `Save()` only ever writes the profile path.

A field can also be locked out of mission override *within* an otherwise-overridable file: `LoadMission()` snapshots it before the deserialize call and restores it right after. `BattleRoyaleGameData.admins_steamid64` is the example — general_settings.json supports mission overrides, but the admin list is a server-operator concern, not mission content, so it's exempted. Reach for the same idiom for any future field that needs this.

Each class carries `int version` plus an `Upgrade()` migration. `Load()` re-saves after reading, so new fields appear in existing profile JSONs on next boot. Moving a field to a different settings file is *not* treated as a migration — the field starts from its new class's default and the old key is left inert in the old file.

### Localization

`LanguageCore/stringtable.csv`, keys prefixed `STR_BR_`. The `Party/` addon carries its own `stringtable.csv` at its PBO root with `STR_PARTY_*` keys — the engine loads one per addon — so party strings do not go here, and the same goes for every `Extra/` addon that has strings at all (`STR_KF_*`, `STR_MAP_*`, `STR_AUTORUN_*`). Five reference styles:
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

Keybinds are declared in `Data/Inputs.xml` (`UADayZBRReadyUp` = F1, `UADayZBRUnstuck` = F2, `UADayZBRLeaderboard` = F4, plus the admin-spectate set `UADayZBRAdminSpectate` = F3, `UADayZBRSpectateMode` = F5, `UADayZBRSpectateNext`/`Prev` = →/←, `UADayZBRSpectateSkeleton` = F6), registered via `inputs = "Vigrid-BattleRoyale/Data/Inputs.xml"` in `Scripts/Client/config.cpp`. The admin keys are refused server-side for anyone outside `admins_steamid64`, so the client-side check on them is presentation only — **except F6, which has no server half at all** and is gated by COT's own `ESP.View` permission instead (see [`Docs/spectating.md`](Docs/spectating.md) → *Admin spectate*).

**Five `inputs=` declarations now ship**, which is supported — @DayZ-Expansion ships three. The full key map across them, so a sixth does not collide: **F1–F6** and **←/→** (Battle Royale, `Data/Inputs.xml`), **P/T/Y** (Party), **M/N/K** (Map), **Z** (AutoRun), **J** (EarPlugs). Check Community-Online-Tools' own `Inputs.xml` before claiming another letter — that is what collided with Party's `Y`.

⚠️ **"Unbound in vanilla" is a stronger claim than it looks, and only K, O and P actually qualify.** The two letters this mod took beyond those — `Z` for auto-run and `J` for ear plugs — are both bound in `P:\bin\preset_keymouseprimary.xml`, and both are safe for the same reason: their bindings sit under blocks that are unreachable in normal gameplay. `kZ` is `UAMoveDown` under **DEBUG SPECIFIC / Freefly Camera**; `kJ` is `UABuldSlow` under **BULDOZER SPECIFIC**, whose only script reference is the suppression list at `scriptconsole.c:531`. Check that file rather than assuming a letter is free — grep for `name="kX"` and read which comment block it falls under.

### Vigrid API / webhooks

`Scripts/Server/3_Game/BattleRoyale/Webhook/` — REST via `GetRestApi().GetRestContext(BATTLEROYALE_API_ENDPOINT)`. Every call site is gated on `BattleRoyaleConfig.GetConfig().GetServerData().enable_vigrid_api`; `use_autolock` is a separate, independent gate that is *not* covered by it. Each webhook pairs with a `RestCallback` subclass that retries from `i_TryLeft`.

Client-side matchmaking is `MatchMakingWebhook` (`Scripts/Client/3_Game/BattleRoyale/Webhook/`), used by the modded `MainMenu`.

## Asset paths

Script and JSON references use the PBO-relative form with forward slashes: `Vigrid-BattleRoyale/GUI/layouts/...`. `config.cpp` `samples[]` uses backslashes with a leading slash. Profile/mission paths use `$profile:` / `$mission:` with **escaped** backslashes (`"$profile:Vigrid-BattleRoyale\\"`). The prefix comes from `PrefixLinkRoot` in `Workbench/project.cfg`.

Note the imageset's internal name is `battleroyale_gui`, not the filename `dayzbr_gui` — hence `"set:battleroyale_gui image:..."`.

**The atlas carries the mod's own HUD icons as well as the two logos**: `GroupIcon`, four zone arrows (`ZoneArrowNeedle` in use, plus `Dart` / `Classic` / `Chevron`) and three clocks (`ClockSimple` in use, plus `Ticks` / `Solid`), all 64x64 in the free row at y=256. The alternates are there so a swap is one name in `br_hud.layout` and no rebuild of the texture.

⚠️ **Editing `dayzbr_gui.edds` is a script job, not an Object Builder one** — `Tools/edds.py` parses and re-emits the container (see *Notes*). Two rules, both learned the hard way:

- **Patch each mip from its own original; never regenerate the chain from a new mip 0.** The engine samples the lower mips when an icon draws small, so rebuilding them with a different filter quietly restyles the two logos at small sizes for no reason. Below ~mip 6 a 64 px icon is sub-pixel and is left alone.
- **Give every glyph a MARGIN off its cell edge.** Vanilla does — `icon_player` is 15 px of ink in a 20 px cell — and drawn edge to edge a shape has no slack for the rounding the HUD introduces scaling 64 down to its drawn size, so the touching side renders visibly flattened. It matters most for `ZoneIcon`, which is **rotated**, so any side can be the clipped one.

⚠️ **A HUD icon's declared size must fit the row its `GridSpacer` parent gives it, and that row SCALES with the viewport while an exact size does not.** The three icons in `MatchInfoIconsPanel` are `28 28` rather than `32 32` for exactly this reason: the row measures 20.23 px at 1280x720 and a declared 32 renders 21.33 into it. Re-measure with `BattleRoyaleHud.DiagIconRects` (`[HudRect]`, once per `ShowHud`) after any change to that panel — reading the rects back beats reasoning about the layout, which got this wrong twice.

## Adding things

- **A gameplay phase** — subclass `BattleRoyaleState` in `States/` with the next numeric prefix, override `GetName`/`Activate`/`Deactivate`/`IsComplete`/`OnPlayerTick`, and insert it at the right index in `BattleRoyaleServer.Init()`.
- **A field in an existing settings file** — add the member to the data class, bump `version`, add an `Upgrade()` branch if migration is needed. Nothing else. **If the new field is a `ref array`, the `Upgrade()` branch is not optional:** a field initialiser survives deserialization for scalars but *not* for arrays, so on every server that already has the JSON the array loads back empty and the feature silently does nothing. Refill it in `Upgrade()` when it comes back empty — and only when empty, so an admin who deliberately cleared it keeps their choice. `BattleRoyaleGameData` and `BattleRoyaleServerData.placeholder_player_names` both do this.
- **A whole new settings file** — new `BattleRoyaleDataBase` subclass + one `m_Configs.Insert()` in `BattleRoyaleConfig.Init()` (marked `//--- adding a new config? copy below`) + a typed accessor.
- **A server → client value** — field + handler + `AddRPC` in the `BattleRoyaleRPC` ctor + a line in its `Reset()`, then read it from `BattleRoyaleClient.Update()`.
- **A client → server command** — `AddRPC` in the `BattleRoyaleServer` ctor, or in a state's `Activate()` paired with `RemoveRPC` in `Deactivate()`.
- **A standalone tweak** — new folder under `Extra/` with its own `config.cpp` and `Scripts/<stage>/`; it becomes its own PBO. Rename `config.cpp` → `config.cpp.disabled` to exclude it from the build (see `Extra/DisableFogChernarusPlus/`).

## Notes

- `Extra/` holds 19 independent single-purpose sub-addons, each its own PBO. 17 are built; `Extra/DisableFogChernarusPlus/` and `Extra/DumpItemHeights/` are parked as `config.cpp.disabled` and produce no PBO. Most are small script tweaks; the exceptions are `Extra/KillFeed/`, `Extra/SafeZone/`, `Extra/Map/`, `Extra/AutoRun/` and `Extra/EarPlugs/`, self-contained addons documented in [`Docs/killfeed.md`](Docs/killfeed.md), [`Docs/safezone.md`](Docs/safezone.md), [`Docs/map.md`](Docs/map.md), [`Docs/autorun.md`](Docs/autorun.md) and [`Docs/earplugs.md`](Docs/earplugs.md). Each folder carries its own `README.md`, indexed by `Extra/README.md`.
- **An incremental `Deploy.bat` deletes the PBO of an addon you just disabled — but only since 2026-11-08.** `CI0_DeleteOrphaned.bat` deletes every `addons\*.pbo` (plus its `<pbo>.<KeyName>.bisign`) that has no marker in `%WorkDrive%Temp\PboNames\@Vigrid-BattleRoyale\`, and nothing ever emptied that folder, so every PBO the mod had *ever* built kept its marker: a `config.cpp` → `.disabled` rename left the addon loading, silently invalidating a discipline negative-build, and a PBO from a worktree build survived the next primary build. `CI1.bat` now empties the folder before the build loop writes this build's markers. The cleanup still does not run on a build that aborts, or with `skipPboCleanup`. **`CopyExtraPBOs` is only half covered**: turning it off, the next build deletes the third-party `.pbo`s (they never get a marker), but leaves their `.bisign`s in `addons\` — they carry their authors' key names, not `KeyName` — and their `.bikey`s in `keys\`. Delete those by hand, or run `Deploy.bat rebuildAll`.
- **`Tools/edds.py` reads and rewrites the Enfusion `.edds` texture container**, whose format is documented in that file's header (decoded from scratch; two traps — the chunk table runs *smallest mip first*, and the LZ4 blocks inside a chunk are *linked*). `selftest` is its acceptance gate and is the thing to run first. It is what took the twelve loading screens from 120 MB of uncompressed BGRA8 to 15.4 MB of DXT1.
- `Extra/RandomMenuGear/` re-dresses the main-menu intro character in a random outfit plus a slung rifle and a melee weapon, re-rolled on every menu show. It hooks vanilla `IntroSceneCharacter.CreateNewCharacterById` (creation, prev/next arrows) and `MainMenu.OnShow` (returning from a submenu — that path calls `OnChangeCharacter(false)` and never recreates the character). It is **not** a fix for the broken character save that makes the menu character render naked; it only decorates the spawned object. Gear is applied with `GameInventory.CreateAttachmentEx` and deliberately never written into `MenuDefaultCharacterData` — that map is serialized to the server on connect and saved locally, so writing to it would leak menu gear into the real spawn loadout. Same discipline rule as `Party/` and `Extra/KillFeed/`: no `BattleRoyale*` symbol may be referenced.
- Spectating is entered **in place on death** — no disconnect, no reconnect — behind `spectate_enabled` in `general_settings.json`, which defaults **off**. **It has a known ~1 km limitation**: the network bubble stays on the spectator's corpse, so a target further than that is not replicated and the spectator sees a nametag with no character. Measured both directions 2026-08-10. The dzhooks `spectatorbubble` hook (serverkit PR #24, merged 2026-11-07) removes the limit: it makes the bubble follow the camera, and then the mod stops carrying the corpse. Tested on the diag rig 2026-11-07, with a target replicated at 2590 m. See [`Docs/spectating.md`](Docs/spectating.md) and [`Docs/native-hooks.md`](Docs/native-hooks.md). The orphaned `GUI/layouts/hud/spectator/player.layout` is still unreferenced: there is no spectator HUD, only a notification naming the current target.
- `Workbench/version` (`0.8.100368`) is a DayZ build number read by nothing. The mod version is `BATTLEROYALE_VERSION`.
