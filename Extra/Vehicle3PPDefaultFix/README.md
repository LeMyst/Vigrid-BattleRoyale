# Vehicle 3PP Default Fix

Guarantees that the common vanilla cars and boats are always allowed a third-person camera under the
third-party **Vehicle3PP** mod, regardless of what its own whitelist happens to contain.

|                 |                                                                        |
|-----------------|------------------------------------------------------------------------|
| **PBO**         | `extra_vehicle3ppdefaultfix.pbo`                                        |
| **Side**        | compiles on both — the body is fenced at runtime by `IsDedicatedServer()` |
| **Stages**      | `4_World`                                                               |
| **`defines[]`** | none — but the code is gated on `Vehicle3PP` (see below)                 |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                               |

## Requires the Vehicle3PP mod

Both source files open with `#ifdef Vehicle3PP` rather than the usual `#ifdef SERVER`. That define is
supplied by the external Vehicle3PP mod's own `defines[]`, so **without that mod loaded this addon
compiles to nothing** and is harmless. It is the only consumer of that define in this repository.

## How it works

`modded class CarScript` and `modded class BoatScript` each override `EEInit()`, run their body first
and call `super.EEInit()` last. On a dedicated server they inject a hardcoded list into Vehicle3PP's
whitelist if it is not already there, then re-run that mod's whitelist evaluation:

```c
array<string> vehicles = GetVehicle3PPConfig().GetWhitelist();
foreach (string forced_vehicle : ForcedWhitelist)
{
    if (vehicles.Find(forced_vehicle) == -1)
        vehicles.Insert(forced_vehicle);
}
```

Forced entries:

| File | Vehicles |
|---|---|
| `CarScript.c` | `OffroadHatchback`, `CivilianSedan`, `Hatchback_02`, `Sedan_02`, `Truck_01_Covered`, `Offroad_02` |
| `BoatScript.c` | `Boat_01_Black`, `Boat_01_Blue`, `Boat_01_Orange`, `Boat_01_Camo` |

If the whitelist ends up empty, 3PP is allowed for everything.

## Caveats

- **Vehicle3PP is not listed in `requiredAddons[]`** — only `DZ_Scripts` is, even though the code
  depends on that mod's `GetVehicle3PPConfig()`.
- The two files are byte-identical apart from the class name and the vehicle list, and each is a ~45
  line copy of Vehicle3PP's own `EEInit`. If that mod changes its implementation, this copy will
  silently diverge.
- The whitelist is dereferenced (`Find` / `Insert`) *before* the code's own `if (!vehicles || ...)`
  null check, so a null return from `GetWhitelist()` would crash before reaching the guard.
  `GetVehicle3PPConfig()` is likewise unchecked.
- Only vanilla classnames are forced — modded vehicles are not covered.
- `m_Allow3PPCamera` and `m_DriverOnly` are redeclared here while the upstream mod also declares them
  on the same class. If you upgrade Vehicle3PP, verify these still resolve to the upstream netsync
  fields rather than becoming shadow copies.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely. Removing
the Vehicle3PP mod also neutralises it without any change here.
