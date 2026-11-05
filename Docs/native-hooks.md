# Native hooks (dzhooks)

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

[dzhooks](https://lemyst.github.io/dayz-serverkit/composer/) is a native hook library for the dedicated server: `dzhooks.so` via `LD_PRELOAD` on Linux, `XAPOFX1_5.dll` dropped next to `DayZServer_x64.exe` on Windows. You build it yourself from the composer's source package. **It is not a dependency.** The mod builds, loads and plays identically without it, and nothing here is `#ifdef`-guarded, because nothing here names a symbol the hook provides.

## Which modules the mod uses

Of the composer's ten modules, **one integration is the mod's job** and the rest are purely operator-side:

| Module | Mod involvement |
|---|---|
| `scriptlock` | **Driven by the mod** through the sentinel file below. Locks like `#lock`, so it refuses everyone, admins included. |
| `adminbypass` (opt-in) | **Driven by the mod** through the same file. While it is locked only `whitelist.txt` IDs can connect, and nobody already in game is kicked. |
| `lockdrain` | None. It changes what `#lock` / `LockSession` does; the sentinel-file modules set the lock bit directly and do not drain. |
| `serverlock` (opt-in) | Not used yet. It needs the `DZHook_API` companion mod and a `proto native`, see *Not done* below. |
| `highpriority`, `reservedslots`, `userlistreload`, `maxplayers`, `reconnectpriority` | None. These are queue and slot policy driven by the operator's own `.txt` files. |
| `ce_reset` (opt-in) | Not used, see *Not done* below. |

## The lock file

`BattleRoyaleNativeLock` (`Scripts/Server/4_World/NativeHooks/`) writes **`$profile:Vigrid-BattleRoyale\server_lock.txt`**:

- **`1` in `2_BattleRoyaleCountReached.Activate`**, beside the Vigrid API and autolock webhooks, i.e. the moment the match countdown starts and the roster freezes.
- **`0` in `BattleRoyaleServer.Init`**, on every boot.

`server_settings.json` → `write_lock_file` (default `true`) turns the writes off. Writing the file without the hook costs nothing, so the opt-in is the hook's own environment variable: point one module at the file.

```bat
REM Windows - XAPOFX1_5.dll next to DayZServer_x64.exe, then before launching:
set DZHOOK_LOCKFILE=F:\ServerProfile\Vigrid-BattleRoyale\server_lock.txt

REM ...or, to let admins in mid-match (see below):
set DZHOOK_SCRIPTLOCK=0
set DZHOOK_ADMINBYPASS=1
set DZHOOK_ADMINBYPASS_FILE=F:\ServerProfile\Vigrid-BattleRoyale\server_lock.txt
```

```bash
# Linux
env LD_PRELOAD="$PWD/dzhooks.so" DZHOOK_LOCKFILE="$PWD/profiles/Vigrid-BattleRoyale/server_lock.txt" ./DayZServer ...
```

Pick **one** of the two modules. With both watching the file, `scriptlock` refuses everyone and `adminbypass`'s exception is moot. `scriptlock` defaults on whenever it is compiled in, so set `DZHOOK_SCRIPTLOCK=0` when you use `adminbypass`, or leave `scriptlock` out of the build.

Three properties are load-bearing:

- ⚠️ **The file outlives the process, and the boot write is what makes that safe.** A match ends in `RequestExit` with `1` still in the file, and so does a crash. Without the `0` in `Init` the next boot would sit locked forever with an empty lobby. **Nothing unlocks at match end, on purpose:** the server is about to exit, and unlocking would let players connect into the restart. Do not "fix" the asymmetry.
- **Turning `write_lock_file` off does not clear the file.** "Off" means "never touch it", so a server switched off mid-match keeps whatever was last written. If the hook still watches it, delete the file or write `0` to it by hand.
- **A torn write cannot unlock.** `FileMode.WRITE` truncates first, so the hook's 0.5 s poll can catch an empty file. Both modules read only the first character, and treat EOF as "no instruction" rather than as `0`.

### Admins joining mid-match: pick `adminbypass`

[`late-joiners.md`](late-joiners.md) lets an `admins_steamid64` member join mid-match and places them at the live circle. **`scriptlock` makes that path unreachable**, because the lock bit is tested in `ctrlReceive` before the engine knows who is connecting. So `adminbypass` is the right module for a server whose admins spectate. Its bypass list is the engine's own **`whitelist.txt`**, not `admins_steamid64`, and it uses the engine's 44-character ID (the one in the `.ADM` log, `PlayerIdentity.GetId()`), not the SteamID64. Keep the two lists in step by hand.

The late-join kick stays in place regardless. With a lock it is only the backstop for anyone who gets through: a whitelisted non-admin, or a connection already in flight when the lock landed.

### Versions and the local rig

The hook's signatures and offsets are pinned to one `DayZServer` build (**1.30.164289** at the time of writing). Run the package's `check_version.py` after every DayZ update; a stale offset crashes the server. **They target the retail exe, not `DayZDiag_x64.exe`**, so only `Release.bat LaunchServer.bat` can exercise them locally. On the diag rig the file is still written, which is enough to test the mod's half: check `[NativeLock] wrote 0` / `wrote 1` in the server `script_*.log` and the file contents.

## Not done, and why

- **`serverlock`'s `ServerLock()` native.** On top of the lock it drains the login queue, if `lockdrain` is also loaded. It needs the `DZHook_API` companion mod, and a `proto native` that nothing binds is a compile failure. So it has to be guarded, and that companion's own `#define DZHOOK_SERVERLOCK` is a *script* define, whose visibility across script modules and mod load order is unproven. `#ifdef DZHook_API` (the auto-defined CfgMods class name) is visible everywhere but does not prove `serverlock` itself was compiled in. The queue drain also matters little here: the lock lands while the lobby is full, and anyone queued at that point is refused on their next attempt anyway.
- **`ce_reset`'s `CE_ResetLoot()` / `CE_Cleanup*()`.** These would let a match restart in-process instead of through `RequestExit`. That means resetting every server singleton (states, zones, ledger, parties, spectators, chickens, name service) to save a restart that [`boot-time.md`](boot-time.md) already measures as cheap once warm. The module is also flagged experimental and must run on an empty server.
