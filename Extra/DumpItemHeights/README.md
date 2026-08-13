# Dump Item Heights

Writes every loot item's bounding box to a CSV, once, on an offline Diag client. The `size_y` column
is the model's vertical extent — the number to compare against `<point height="..."/>` in
`mapgroupproto.xml`.

|                 |                                                                    |
|-----------------|--------------------------------------------------------------------|
| **PBO**         | `extra_dumpitemheights.pbo`                                        |
| **Side**        | client (`#ifndef SERVER` + `#ifdef DIAG_DEVELOPER`)                |
| **Stages**      | `5_Mission`                                                        |
| **`defines[]`** | `DUMP_ITEM_HEIGHTS` — declared for consistency only, nothing reads it |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                          |

> **This is a throwaway tool.** Build it, run it once, then rename `config.cpp` →
> `config.cpp.disabled` and rebuild.

## Why it exists

A loot item's height lives in the `.p3d`, not in config. There is no `boundingSphere`, `bounding
box` or `envCargo` key anywhere in `P:\dz`, and `itemSize[]` is a 2D inventory-grid slot count rather
than physical dimensions — vanilla's own reader of it, `ItemBase.GetItemSize()`
(`P:\scripts\4_world\entities\itembase.c:2695`), is commented out and returns `1`. So the only way to
get the number is to instantiate each item and ask the engine.

## How it works

`modded class MissionGameplay` arms the dumper in `OnMissionStart()` and ticks it from `OnUpdate()`.

**Three gates**, all of which must pass before a single object is created:

| # | Gate | Effect |
|---|---|---|
| 1 | `#ifndef SERVER` | client only |
| 2 | `#ifdef DIAG_DEVELOPER` | `DayZDiag_x64` only — the whole PBO is inert in a release client |
| 3 | `!GetGame().IsMultiplayer()` (runtime, in `Arm()`) | offline only, i.e. `LaunchOffline.bat` |

Gate 3 is `!IsMultiplayer()` rather than `IsMissionOffline()` because the latter is
Community-Online-Tools', not vanilla's.

**Once per file.** A finished run deletes its progress file, so a CSV with no `item_heights_progress.txt`
beside it means "already done" and `Arm()` returns early. Delete the CSV to force a fresh run.

**Resumable, because `CreateObjectEx` can hard-crash the client and there is no `try`/`catch`.** The
index about to be created is written to `item_heights_progress.txt` before every attempt. If the
client dies, the next launch resumes at the following index, appends to the existing CSV and records
the offender in it as a `# resumed - index N (ClassName) crashed the previous run` comment. One
relaunch per bad class instead of losing the whole run.

**Chunked, not synchronous.** There are a few thousand `Inventory_Base` classes; creating and
deleting all of them inside one frame is a multi-minute freeze the engine may decide is a hang.
`Update()` processes `ITEMS_PER_FRAME` (25) config indices per frame after a 5 s settle.

**Per item:** skip `scope < 2`, skip anything that is not `IsKindOf(name, "Inventory_Base")`, then

```c
Object obj = GetGame().CreateObjectEx(classname, test_pos, ECE_LOCAL | ECE_CREATEPHYSICS);
```

`ECE_CREATEPHYSICS` is load-bearing — without a collision envelope `GetCollisionBox` returns false
and every row silently degrades to the looser render bounds. `ECE_TRACE` and `ECE_UPDATEPATHGRAPH`
are deliberately absent: the box is measured in **model space** so the spawn position is irrelevant,
and vanilla's own preview spawner carries a literal `//! Don't use ECE_UPDATEPATHGRAPH !!!` warning
(`P:\scripts\5_mission\gui\scriptconsoleitemstab.c:697`).

The box comes from a three-step fallback chain, and which step fired is recorded per row:

| `source` | Call | Notes |
|---|---|---|
| `collision` | `obj.GetCollisionBox(min_max)` | the good one — returns `bool` |
| `memorypoint` | `box_placing_min` / `box_placing_max` memory points | vanilla's own fallback, `hologram.c:1067`; authored placement bounds |
| `clipping` | `obj.ClippingInfo(min_max)` | render bounds, loosest. `proto float` returning a radius, so it has **no failure signal** and is the last resort by construction |

`GetCollisionRadius()` goes out as its own `radius` column regardless — a free sanity check on rows
that fell through to `clipping`.

## Output

`<ClientProfileDirectory>\item_heights.csv`

```
classname,scope,size_x,size_y,size_z,min_y,max_y,radius,source
```

plus a trailer line `# dumped=N skipped_scope=N skipped_kind=N skipped_create=N`. Values are rounded
to millimetres.

Progress goes to the client `script_*.log` (not the `.rpt`, which stops recording once the world has
loaded) as `[DumpItemHeights] i=N/M dumped=K`, one line per chunk.

## Measured results (2026-08-13, ChernarusPlus, full mod list)

```
# dumped=1859 skipped_scope=3251 skipped_kind=561 skipped_create=0
```

1859 rows out of 5671 `CfgVehicles` children. `source` came out **1698 `collision` / 161 `clipping` /
0 `memorypoint`** — `collision` being the large majority is the check that `ECE_CREATEPHYSICS` is
actually taking effect. Sanity spot-checks: `TunaCan` 0.025, `Apple` 0.101, `WaterBottle` 0.27,
`FirefighterAxe` 0.95, `Barrel_Blue` 0.81.

## Caveats

- ⚠️ **`scope < 2` is not an optimisation, it is a crash guard.** `scope=1` classes are inheritable
  bases that are not usable on their own, and they inherit `model=""` from `Inventory_Base`.
  `CreateObjectEx` on one **hard-crashes the client** — `Access violation. Illegal read ... at 0x31c`
  inside the engine, uncatchable. Measured 2026-08-13 on `ItemOptics` (`CfgVehicles` index 56). Every
  item that actually spawns as loot is `scope=2`, so nothing real is lost.
- ⚠️ **`source=clipping` rows are low confidence, and 13 of them are all-zero.** Clipping info is
  render bounds, and it has no failure signal — a class with no geometry at all silently yields a
  zero box (`ExpansionBanknote*_InsanityStack`). Some are worse than useless: the base-building kits
  (`ShelterKit`, `WatchtowerKit`, `FenceKit`, `TerritoryFlagKit`) all report an identical `size_y` of
  3.664, which is the *deployed structure's* bounds rather than the kit item's. **Filter on
  `source == "collision"` for anything you intend to trust**, and treat a zero row as "no data", not
  as a flat item.
- ⚠️ **Firearms and magazines are NOT in this dump.** They live in `CfgWeapons` and `CfgMagazines`,
  which are separate config trees — this addon walks `CfgVehicles` only, and `IsKindOf` is
  CfgVehicles-only by construction. `M4A1`, `AKM`, `Mag_STANAG_30Rnd` and `Ammo_556x45` are all
  absent. If point heights are needed for weapon spawns, `CFG_PATH` and `KIND_FILTER` have to grow a
  second pass over those trees (no `IsKindOf` filter there — just `scope >= 2`).
- **Loot only.** `Inventory_Base` covers clothing, optics, containers, food, traps and explosives —
  everything under it. It does **not** cover `House` / `HouseNoDestruct` (buildings, and a surprising
  number of camping, cooking and radio items inherit `HouseNoDestruct`) or `Man`.
- Vanilla's preview spawner calls `dBodyDestroy` on its throwaway object; this addon deliberately
  does **not**. That object lives for many frames, this one is deleted before the frame ends, and
  destroying the physics body risks taking away the very collision data being read.
- The tool spawns and deletes real entities. It is `ECE_LOCAL`, so nothing is registered with the
  network or the central economy, but it is still not something to run on a live server — which is
  what the three gates are for.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely. Check
`%ModBuildDirectory%@Vigrid-BattleRoyale\` afterwards — an incremental build does not reliably delete
an orphaned PBO, so `extra_dumpitemheights.pbo` and its `.bisign` may need removing by hand.
