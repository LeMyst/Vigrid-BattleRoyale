# Server boot time

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

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
