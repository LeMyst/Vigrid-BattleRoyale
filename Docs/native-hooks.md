# Native hooks (dzhooks)

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

[dzhooks](https://lemyst.github.io/dayz-serverkit/composer/) is a native hook library for the dedicated server: `dzhooks.so` via `LD_PRELOAD` on Linux, `XAPOFX1_5.dll` dropped next to `DayZServer_x64.exe` on Windows. You build it yourself from the composer's source package. **It is not a dependency.** The mod builds, loads and plays identically without it. The one symbol it names from the hook, the `serverlock` natives, is behind `#ifdef DZHOOK_SERVERLOCK`, a define the hook injects itself (see *The `serverlock` natives* below).

## Which modules the mod uses

Of the composer's ten modules, **the lock is the mod's job** and the rest are purely operator-side:

| Module | Mod involvement |
|---|---|
| `scriptlock` | **Driven by the mod** through the sentinel file below, when `serverlock` is absent or off (`DZHOOK_SERVERLOCK=0`). Locks like `#lock`, so it refuses everyone, admins included. |
| `adminbypass` (opt-in) | **Driven by the mod** through the same file, so also only without `serverlock`. While it is locked only `whitelist.txt` IDs can connect, and nobody already in game is kicked. |
| `serverlock` (on by default) | **Preferred. Driven by the mod** through `ServerLock()` / `ServerUnlock()` whenever the hook provides them, and the file is then not used to lock. Same lock bit as `scriptlock`, so it refuses admins too. **Never with `adminbypass`.** |
| `scriptdefine` | Needed by `serverlock`: it is what makes `DZHOOK_SERVERLOCK` visible to the mod's scripts. |
| `lockdrain` | None. It changes what `#lock` / `LockSession` does. `scriptlock` and `serverlock` set the lock bit directly and `adminbypass` uses the whitelist, so none of them drains. |
| `highpriority`, `reservedslots`, `userlistreload`, `maxplayers`, `reconnectpriority` | None. These are queue and slot policy driven by the operator's own `.txt` files. |

## The lock file

**The file is the fallback.** When the hook provides `serverlock`, the mod locks through its natives instead (see *The `serverlock` natives* below) and only ever writes `0` here.

Without `serverlock`, `BattleRoyaleNativeLock` (`Scripts/Server/4_World/NativeHooks/`) writes **`$profile:Vigrid-BattleRoyale\server_lock.txt`**:

- **`1` in `2_BattleRoyaleCountReached.Activate`**, beside the Vigrid API and autolock webhooks, i.e. the moment the match countdown starts and the roster freezes.
- **`0` in `BattleRoyaleServer.Init`**, on every boot.

`server_settings.json` → `write_lock_file` (default `true`) turns the writes off. Writing the file without the hook costs nothing, so the opt-in is the hook's own environment variable: point one module at the file.

```bat
REM Windows - XAPOFX1_5.dll next to DayZServer_x64.exe, then before launching.
REM serverlock is on by default and would take over, so switch it off first:
set DZHOOK_SERVERLOCK=0
set DZHOOK_LOCKFILE=F:\ServerProfile\Vigrid-BattleRoyale\server_lock.txt

REM ...or, to let admins in mid-match (see below) - and do NOT also set DZHOOK_LOCKFILE:
set DZHOOK_ADMINBYPASS=1
set DZHOOK_ADMINBYPASS_FILE=F:\ServerProfile\Vigrid-BattleRoyale\server_lock.txt
```

```bash
# Linux
env LD_PRELOAD="$PWD/dzhooks.so" DZHOOK_SERVERLOCK=0 DZHOOK_LOCKFILE="$PWD/profiles/Vigrid-BattleRoyale/server_lock.txt" ./DayZServer ...
```

Point **one** of the two modules at the file. With both watching it, `scriptlock` refuses everyone and `adminbypass`'s exception is moot.

**`DZHOOK_<MODULE>=0` switches a compiled-in module off, on Windows too, since serverkit `8bef3af`** (2026-11-05, included in `9c40081`). Older Windows builds ignored it and installed every module regardless (serverkit #1), which this doc reported on 2026-11-05. With an older DLL, keep `scriptlock` off the file by leaving `DZHOOK_LOCKFILE` unset, or leave it out of the build.

⚠️ **With the file, switch `serverlock` off: `DZHOOK_SERVERLOCK=0`.** It is on by default since serverkit `8921f20`, and while it is present the mod locks through it and never writes `1` to the file, so neither `scriptlock` nor `adminbypass` would ever lock.

Three properties are load-bearing:

- ⚠️ **The file outlives the process, and the boot write is what makes that safe.** A match ends in `RequestExit` with `1` still in the file, and so does a crash. Without the `0` in `Init` the next boot would sit locked forever with an empty lobby. **Nothing unlocks at match end, on purpose:** the server is about to exit, and unlocking would let players connect into the restart. Do not "fix" the asymmetry.
- **Turning `write_lock_file` off does not clear the file.** "Off" means "never touch it", so a server switched off mid-match keeps whatever was last written. If the hook still watches it, delete the file or write `0` to it by hand.
- **A torn write cannot unlock.** `FileMode.WRITE` truncates first, so the hook's 0.5 s poll can catch an empty file. Both modules read only the first character, and treat EOF as "no instruction" rather than as `0`.

### Admins joining mid-match: pick `adminbypass`

[`late-joiners.md`](late-joiners.md) lets an `admins_steamid64` member join mid-match and places them at the live circle. **`scriptlock` makes that path unreachable**, because the lock bit is tested in `ctrlReceive` before the engine knows who is connecting. So `adminbypass` is the right module for a server whose admins spectate. Its bypass list is the engine's own **`whitelist.txt`**, not `admins_steamid64`, and it uses the engine's 44-character ID (the one in the `.ADM` log, `PlayerIdentity.GetId()`), not the SteamID64. Keep the two lists in step by hand.

**Verified 2026-11-05** on retail `DayZServer_x64.exe` 1.30.164.289, with `DZHOOK_ADMINBYPASS=1` and `DZHOOK_ADMINBYPASS_FILE` on the mod's file. The test used one client, whose ID was first in `whitelist.txt` and then removed:

| Claim | Evidence |
|---|---|
| A whitelisted admin gets in while the lock is on | `lock ON` 18:29:41, then `is connecting` 18:43:34 and `is connected` 18:43:56 |
| Nobody already in game is kicked | ID removed and the lock cycled 0 → 1 at 18:44:20 (which re-arms the whitelist reload). The player stayed connected for minutes |
| Anyone off the list is refused | reconnect → `0x0004000C` ("not in whitelist", kick reason `0xc`) |

While locked, the server advertises a `whitelisting` tag in its Steam query info. The module's source calls that harmless, and it was: the client connected with it present.

Three things about the bypass list:

- **It lives in the server folder** (`DayZServer\whitelist.txt`, beside the exe), not the profile. The mod cannot write it, because script file access stops at `$profile:` / `$mission:` / `$saves:`. So keeping it in step with `admins_steamid64` stays a manual job.
- **An entry is the 44-character ID from the `.ADM` log**, one per line, with an optional `// comment` after it.
- **`enableWhitelist = 0` in `serverDZ.cfg` is correct, and is what this needs.** The module switches the whitelist on itself while locked, and restores the server's own setting on `0`. A server that was already whitelisting keeps its native behaviour, including the online kick the module otherwise suppresses.

The late-join kick stays in place regardless. With a lock it is only the backstop for anyone who gets through: a whitelisted non-admin, or a connection already in flight when the lock landed.

### Versions and the local rig

The hook's signatures and offsets are pinned to one `DayZServer` build (**1.30.164289** at the time of writing). Run the package's `check_version.py` after every DayZ update; a stale offset crashes the server. **They were written against the retail exe.** Since serverkit `9c40081`, `serverlock` and `scriptdefine` resolve on `DayZDiag_x64.exe` as server too (measured 2026-11-06), and a second client was refused there on 2026-11-07 (serverkit #20). ⚠️ **On the diag rig the server and the clients run from the same folder, so the clients load the DLL too.** `9c40081` installed every module in a client and the client crashed; since serverkit `8921f20` the DLL installs only in `DayZServer*` or with `-server` on the command line, and stays a plain proxy in a client. With an older DLL, rename it once the server has loaded it, then start the clients. `scriptlock` and `adminbypass` were only ever tested on retail, so `Release.bat LaunchServer.bat` stays the way to exercise those. On the diag rig the file is still written, which is enough to test the mod's half: check `[NativeLock] wrote 0` / `wrote 1` in the server `script_*.log`, and the file contents. Those lines, and `[NativeLock] ServerLock() called` / `ServerUnlock() called`, go through a plain `Print`, so they show on retail too. Through `BattleRoyaleUtils` they did not: the retail log level is 0, which filters `Warn` and `Info` alike, and the only level it keeps, `Error`, is fatal.

**Testing a hook needs a person at the client, and three things about the retail client trip a test:**

- **This mod's Play button joins the `-connect` / `-port` the client was started with** (`MainMenu.ConnectFromCLI`). Start it without them and Play reports `0x00010001`, "server unreachable", while the server is up and answering. A Steam A2S query on port 27016 tells the two apart in a second.
- **Start the client through `DayZ_BE.exe`, not `DayZ_x64.exe`.** The local `serverDZ.cfg` runs server-side BattlEye, so a client without it is kicked with `0x000400F0` 10-20 s after joining. `BattlEye = 0;` appended at the end of the file, after `class Missions`, was ignored: the server still advertised the `battleye` tag.
- **Test the hook by writing the byte by hand**, not through a match. The countdown's write is one line, and a full match needs several retail clients, none of which can autoconnect.

## The `serverlock` natives

**The mod's preferred lock, and on by default** in any DLL built with it since serverkit `8921f20` (2026-11-07). Before that it was opt-in with `DZHOOK_SERVERLOCK=1`. When `DZHOOK_SERVERLOCK` is defined, `BattleRoyaleNativeLock.Set` calls **`ServerLock()` when the countdown starts and `ServerUnlock()` at boot**, and does not lock through the file. It has the same effect as `scriptlock` (lock bit `0x04` of the server params, refusing everyone) and three advantages:

- **No file and no path to configure.** A DLL built with `serverlock` and `scriptdefine` is all it takes.
- **The bit dies with the process.** A crash mid-match cannot leave the next boot locked, whatever `write_lock_file` says.
- **It is synchronous.** It locks inside the countdown's `Activate`, not on the next 0.5 s poll.

```bat
REM Windows - a DLL built with serverlock AND scriptdefine. Nothing to set since serverkit 8921f20;
REM an older DLL needs:
set DZHOOK_SERVERLOCK=1
```

**How the guard works.** The natives are declared at file scope in `BattleRoyaleNativeLock.c`, under `#ifdef DZHOOK_SERVERLOCK`, and called only under the same guard. An unbound `proto native` is a compile failure, so the two must never drift apart. The define comes from the hook: `serverlock` registers it when it installs, and `scriptdefine` injects it into every script module through `enf::CParser::AddStaticDef`, whatever the `-mod=` order. No companion mod is involved; there used to be one, `DZHook_API`, and it is gone. Without the DLL, or with `DZHOOK_SERVERLOCK` unset, the define is absent and both the declarations and the calls compile out.

**Which to choose:**

| The server needs | Use |
|---|---|
| a plain lock at the countdown (the default) | `serverlock`: nothing to set, and no `DZHOOK_LOCKFILE` |
| admins able to join mid-match | `adminbypass` with the file, and **`DZHOOK_SERVERLOCK=0`** |
| something outside the mod (an operator script, a panel) driving the lock too | `scriptlock` with the file, and `DZHOOK_SERVERLOCK=0` |

⚠️ **Do not combine it with `adminbypass`.** `adminbypass` locks by switching the whitelist on and leaves bit `0x04` clear. `ServerLock()` sets that bit, and the engine tests it in `ctrlReceive` *before* the whitelist, so the admins `adminbypass` exists to let in are refused. And with `serverlock` present the mod never writes `1` to the file, so `adminbypass` would not even switch its whitelist on.

**The boot `0` is still written when the file exists.** A server moved from the file to `serverlock` may keep `DZHOOK_LOCKFILE` set, with a `1` left by its last file-driven match. Nothing else would ever clear it, and `scriptlock` would hold that server locked. When the file does not exist, nothing is created.

`write_lock_file` does not gate the natives. That switch is about the file; `DZHOOK_SERVERLOCK=0` is the hook-side off switch for the natives.

**Verified** on retail `DayZServer_x64.exe` 1.30.164.289 and on `DayZDiag_x64.exe` as server, with serverkit `9c40081`:
- **2026-11-06, from a probe build:** `DZHOOK_SERVERLOCK` reached both `4_World` and `5_Mission`, the file-scope declarations compiled and bound, and the hook logged `[serverlock] ServerLock() -> locked` / `ServerUnlock() -> open`.
- **This integration, on retail:** the boot logged `[NativeLock] ServerUnlock() called` with the hook's `ServerUnlock() -> open`, and the countdown (reached with COT's *Next state*) logged `[NativeLock] ServerLock() called` with `[serverlock] ServerLock() -> locked` in the same millisecond. Without `DZHOOK_SERVERLOCK` the natives compile out and the script log is clean.

## Not done, and why

- **`ce_reset`'s `CE_ResetLoot()` / `CE_Cleanup*()`.** The idea was to restart a match in-process instead of through `RequestExit`, and it is **dead**, not deferred. Five instrumented builds of the hook (serverkit issue #16, retail and DayZDiag, November 2026) established three things:
  - **There is no instant wipe.** `TimeShift` expires every lifetime, but CE's own throttled loop does the deleting, which would take about an hour for the whole map. No budget or forced drain changes that.
  - **There is no reachable loot respawn.** No "Loot" event list exists, and the loot spawner is not reachable from the CE root object.
  - **The natives add nothing** over vanilla script's `GetCEApi().TimeShift`: `CE_Cleanup*` measured as no-ops.

  serverkit PR #19 retired the module (`8445556`, 2026-11-07); it is no longer in the composer. The process restart between matches stays; [`boot-time.md`](boot-time.md) measures it as cheap once warm.
