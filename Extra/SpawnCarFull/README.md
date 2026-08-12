# Spawn Car Full

Vehicles spawn ready to drive. Vanilla gives a freshly spawned car 0–35 % fuel and nothing else, so a
found vehicle usually needs parts and jerrycans before it moves — too slow to matter inside a single
battle royale match.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_spawncarfull.pbo`                                |
| **Side**        | server (`#ifdef SERVER`)                                |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

Two classes, each overriding `EEOnCECreate()`.

**`CarScript`** — fuel is randomised, coolant and oil are filled completely:

```c
Fill( CarFluid.FUEL,    GetFluidCapacity( CarFluid.FUEL ) * Math.RandomFloatInclusive(0.30, 1) );
Fill( CarFluid.COOLANT, GetFluidCapacity( CarFluid.COOLANT ) );
Fill( CarFluid.OIL,     GetFluidCapacity( CarFluid.OIL ) );
```

**`BoatScript`** — boats only have a fuel fluid, so that is all there is to fill:

```c
Fill( BoatFluid.FUEL, GetFluidCapacity( BoatFluid.FUEL ) * Math.RandomFloatInclusive(0.30, 1) );
```

Net effect versus vanilla: **30–100 % fuel** instead of 0–35 %, plus full coolant and oil on cars.

Because this hooks `EEOnCECreate`, it applies **only to Central-Economy vehicle spawns** — not to
admin-spawned or scripted vehicles.

## Caveats

- Despite the in-file comment "Fill the car to max", **fuel is random 30–100 %**, not full. Only
  coolant and oil are maxed.
- Neither override calls `super.EEOnCECreate()`. That drops the vanilla base implementation and any
  other mod's CE-spawn initialisation further along the chain, DayZ-Expansion included.
- Attachments — battery, spark plug, radiator, wheels — are untouched. Whether a vehicle is actually
  driveable still depends on your loot configuration. (`Extra/SpawnWithBattery` covers items with an
  energy manager, not vehicle battery slots.)

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely.
