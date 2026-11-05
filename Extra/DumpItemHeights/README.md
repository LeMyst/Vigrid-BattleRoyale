# Dump Item Heights

Writes every loot item's bounding box to a CSV, once, on an offline Diag client, across all three
loot config trees — `CfgVehicles`, `CfgWeapons` and `CfgMagazines`. The `size_y` column is the
model's vertical extent — the number to compare against `<point height="..."/>` in
`mapgroupproto.xml`.

|                 |                                                                    |
|-----------------|--------------------------------------------------------------------|
| **PBO**         | *none — parked* (`extra_dumpitemheights.pbo` when enabled)         |
| **Side**        | client (`#ifndef SERVER` + `#ifdef DIAG_DEVELOPER`)                |
| **Stages**      | `5_Mission`                                                        |
| **`defines[]`** | `DUMP_ITEM_HEIGHTS` — declared for consistency only, nothing reads it |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                          |

> ⚠️ **This addon does not ship.** Its `config.cpp` is renamed to `config.cpp.disabled`, so the build
> skips the folder entirely and no PBO is produced. See *Enabling* at the bottom.

## Why it is parked

It is a throwaway tool, not a gameplay feature: it was written to answer one question, it answered
it, and leaving it enabled would mean every client build carries a module that spawns and deletes a
few thousand entities on an offline launch. The measured output is recorded below, so the dump only
needs re-running when the item set changes — a DayZ update, or a mod added or removed.

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

**Three trees, walked in order:** `CfgVehicles` → `CfgWeapons` → `CfgMagazines`. Loot is not all in
one tree — firearms are in `CfgWeapons`, magazines and loose rounds in `CfgMagazines`, and only items
and clothing in `CfgVehicles`. All three spawn at loot points, so all three need heights.

**Per item:** skip `scope < 2`, then — **for `CfgVehicles` only** — skip anything that is not
`IsKindOf(name, "Inventory_Base")`. That filter resolves against `CfgVehicles` by construction
(`IsKindOf` walks `ConfigGetFullPath("CfgVehicles " + name)`), so applying it to the other two trees
would reject every class in them; nothing there needs filtering anyway, as it is all loot. Then

```c
Object obj = GetGame().CreateObjectEx(classname, test_pos, ECE_LOCAL | ECE_CREATEPHYSICS);
```

`ECE_CREATEPHYSICS` is load-bearing — without a collision envelope `GetCollisionBox` returns false
and every row silently degrades to the looser render bounds. `ECE_TRACE` and `ECE_UPDATEPATHGRAPH`
are deliberately absent: the box is measured in **model space** so the spawn position is irrelevant,
and vanilla's own preview spawner carries a literal `//! Don't use ECE_UPDATEPATHGRAPH !!!` warning
(`P:\scripts\5_mission\gui\scriptconsoleitemstab.c:697`).

The box comes from a two-step chain, and every row then has to pass a trust gate:

| `source` | Meaning | `trusted` |
|---|---|---|
| `collision` | `GetCollisionBox` returned true and the box survived inspection | **1** |
| `memorypoint` | `box_placing_min` / `box_placing_max`, vanilla's own fallback (`hologram.c:1067`) | **1** |
| `nocollision` | no collision box and no memory points — size written as **0** | 0 |
| `suspect2m` | a box came back with an axis within 0.01 of exactly 2.0 — a placeholder, not geometry | 0 |
| `zero` | a box came back with `size_y <= 0` | 0 |

**`ClippingInfo` is deliberately not a fallback value.** It is recorded in its own `clip_y` column for
diagnostics and never written into `size_*` — see the measurement below for why.

## Output

`<ClientProfileDirectory>\item_heights.csv`

```
classname,tree,scope,trusted,size_x,size_y,size_z,min_y,max_y,clip_y,radius,source
```

| Column | Meaning |
|---|---|
| `classname` | the config class — the same string used in `types.xml` |
| `tree` | which config tree it came from: `CfgVehicles`, `CfgWeapons` or `CfgMagazines` |
| `scope` | config visibility. Always `2`, since `scope < 2` is filtered out |
| **`trusted`** | **1 = the numbers are real, 0 = they are not. Consumers must filter on this.** |
| `size_x` / `size_y` / `size_z` | bounding-box width / **height** / depth in metres, model space. **Zero when `trusted=0` and `source=nocollision`** |
| `min_y` / `max_y` | how far the geometry reaches below / above the model origin. `size_y = max_y - min_y` |
| `clip_y` | `ClippingInfo`'s height. **Diagnostics only — do not use as a height**, it over-reports by ~5× |
| `radius` | `GetCollisionRadius()`, the bounding *sphere*. Unreliable (`AKM` reports 2.619) — diagnostics only |
| `source` | which measurement produced the box, or why it was rejected — see the table above |

`min_y` is worth reading alongside `size_y`: `min_y ≈ 0` means the origin sits at the item's base,
while `min_y ≈ -size_y / 2` means it sits at the centre and half the item hangs below the point it is
placed at.

Plus a trailer line `# dumped=N skipped_scope=N skipped_kind=N skipped_create=N`. Values are rounded
to millimetres.

Progress goes to the client `script_*.log` (not the `.rpt`, which stops recording once the world has
loaded) as `[DumpItemHeights] i=N/M dumped=K`, one line per chunk.

## ⚠️ Why `trusted` exists — the measurement that produced it

The first version of this addon published a bounding box for every row with no trust signal, and a
minority of them were wrong. `Izh43Shotgun` came out as **2.785 m** tall (its siblings are ~0.19), so
a consuming tool compared it against `<point height="…"/>`, found no loot point that tall, and
reported the weapon as unspawnable — while in game it spawns fine.

A dedicated probe settled it on 2026-08-17. **Two plausible theories were killed:**

1. **It is not a timing artefact.** The obvious explanation was that the box is read in the same
   frame the object is created, before geometry has streamed in. Subjects were created once, kept
   alive and re-measured on a widening schedule — 12 samples over 480 frames / ~30 s. **Every subject
   gave exactly one distinct reading**, suspects and controls alike. So "read it a frame later"
   cannot help, and neither can "run it twice and diff" — the wrong answers are perfectly
   reproducible.
2. **It is not the spawn flags.** Five variants — the current mask, vanilla's Script Console mask
   (`+ECE_TRACE`), no physics at all, `+ECE_SETUP`, and a second instance created after the first —
   were **bit-identical** for every class. The box cannot be recovered by creating the object
   differently.

**What it actually is — two independent failure modes:**

- **`ClippingInfo` is not a usable fallback.** Measured against known-good *controls*, it
  over-reports by roughly 5×: `AKM` `clip_y` **0.986** against a true **0.169**, `Izh18Shotgun`
  **0.87** against **0.191**. So the old build's `clipping` rows were not merely imprecise, they were
  wrong — *including* the ones that looked plausible (`Derringer_Black` 0.299, `GP25` 0.294). It is
  now recorded in `clip_y` and never used as a size.
- **`GetCollisionBox` can return `true` and still hand back a placeholder.** `Groza`
  (2 / 2.192 / 2.001) and `P1` (2.034 / 0.139 / 2.001) both did. Real geometry does not land on
  2.000, hence the `suspect2m` check.

**Which classes lose their box**: every `CfgWeapons` row that fails is a non-standard-muzzle weapon —
`Izh43Shotgun` and `SawedoffIzh43Shotgun` and all three `Derringer`s (double barrels), `GP25` and
`M203` (grenade launchers), `RPG7`, `Crossbow` ×5, `DartGun`, `Shockpistol`, `SawedoffB95`. Those are
exactly the weapons whose `InitMuzzleArray` vanilla expects to be overridden, and `Izh43Shotgun` was
caught throwing `WeaponStableState.ValidateMuzzleArray` from inside `Weapon_Base`'s constructor
*during* `CreateObjectEx`. **Whether that throw is what prevents the collision geometry being
attached, or the model simply has none, was not established** — and the fix does not depend on it,
because either way the box is unobtainable and the only honest answer is to say so.

## Measured results (2026-08-17, ChernarusPlus, full mod list)

```
# dumped=2064 skipped_scope=3333 skipped_kind=596 skipped_create=0
# trusted=1869 nocollision=177 suspect2m=5 zero=13
```

| Tree | Children | Dumped | `trusted=1` | `trusted=0` |
|---|---:|---:|---:|---:|
| `CfgVehicles` | 5671 | 1824 | 1682 | 142 |
| `CfgWeapons` | 194 | 117 | 94 | 23 |
| `CfgMagazines` | 128 | 123 | 93 | 30 |
| | | **2064** | **1869** | **195** |

`memorypoint` never fires in practice. Sanity spot-checks, all `trusted=1`: `TunaCan` 0.025,
`Apple` 0.101, `WaterBottle` 0.27, `FirefighterAxe` 0.95, `Barrel_Blue` 0.81, `Mosin9130` 0.184
(× 1.236 long), `M4A1` 0.22, `Mag_STANAG_30Rnd` 0.211. Note a firearm's length is `size_x` — `size_y`
is the height of it lying flat, which is the clearance a loot point actually has to give it.

**Expect `ValidateMuzzleArray` entries in `crash_*.log`, and ignore them.** They are non-fatal vanilla
script errors, logged rather than fatal because `LaunchOffline.bat` passes `newErrorsAreWarnings 1`.
The run completes and the row accounting sums exactly to `dumped`.

## Caveats

- ⚠️ **`scope < 2` is not an optimisation, it is a crash guard.** `scope=1` classes are inheritable
  bases that are not usable on their own, and they inherit `model=""` from `Inventory_Base`.
  `CreateObjectEx` on one **hard-crashes the client** — `Access violation. Illegal read ... at 0x31c`
  inside the engine, uncatchable. Measured 2026-08-13 on `ItemOptics` (`CfgVehicles` index 56). Every
  item that actually spawns as loot is `scope=2`, so nothing real is lost.
- ⚠️ **`trusted=1` means "this is the engine's collision box", not "this is the visual height".** The
  gate catches the two failure modes described above; it does **not** verify that a returned box is
  semantically the object. `BarrelHoles_*` (2.822), `OvenIndoor` (2.834) and `FireplaceIndoor`
  (2.347) are `collision`-sourced and pass the gate, but a barrel is not 2.8 m tall — those boxes
  look like they include an interaction volume. They are deployables rather than ordinary loot, so
  they rarely matter, but a consumer that reports "unspawnable" should treat a tall `trusted=1` row
  with the same suspicion as a tall anything.
- ⚠️ **`suspect2m` has a known false positive.** `ShelterFabric` and `ShelterLeather` measure
  0.003 × 2.001 × 1.002, which is a *perfectly plausible 2 × 1 m tarp*, and they are rejected anyway
  because the rule cannot tell a real 2.0 from a placeholder 2.0. Rejecting a good row is the safe
  direction; publishing a bad one is not.
- **Character heads are excluded** (`IsKindOf(name, "Head")`). They pass the `Inventory_Base` test but
  are not loot, never appear in `types.xml`, and measure the whole character rig (~2.17 m). 35 rows.
- **Loot only.** Within `CfgVehicles`, `Inventory_Base` covers clothing, optics, containers, food,
  traps and explosives — everything under it. It does **not** cover `House` / `HouseNoDestruct`
  (buildings, and a surprising number of camping, cooking and radio items inherit `HouseNoDestruct`)
  or `Man`. `CfgWeapons` and `CfgMagazines` are taken whole, filtered on `scope >= 2` alone.
- Vanilla's preview spawner calls `dBodyDestroy` on its throwaway object; this addon deliberately
  does **not**. That object lives for many frames, this one is deleted before the frame ends, and
  destroying the physics body risks taking away the very collision data being read.
- The tool spawns and deletes real entities. It is `ECE_LOCAL`, so nothing is registered with the
  network or the central economy, but it is still not something to run on a live server — which is
  what the three gates are for.

## Enabling

Rename `config.cpp.disabled` → `config.cpp` and rebuild; the folder becomes its own PBO again. Then
delete `<ClientProfileDirectory>\item_heights.csv` (a run with the CSV already present skips) and
launch `LaunchOffline.bat`.

To park it again, rename back and rebuild — then ⚠️ **delete the orphaned PBO by hand.** A rebuild
after the rename does **not** remove it: measured 2026-08-13, `Deploy.bat` reported success and
`extra_dumpitemheights.pbo` was still sitting in `%ModBuildDirectory%@Vigrid-BattleRoyale\Addons\`
with its pre-rename timestamp, so the addon would have carried on loading despite being "disabled".
Both files have to go:

```
extra_dumpitemheights.pbo
extra_dumpitemheights.pbo.battleroyale.bisign
```

The PBO count in that folder is the check — 23 with this addon parked.
