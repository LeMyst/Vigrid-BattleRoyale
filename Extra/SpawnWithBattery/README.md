# Spawn With Battery

Battery-powered loot spawns with a battery already fitted, so a found radio, flashlight or scope works
the moment you pick it up instead of needing a separate scavenger hunt.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_spawnwithbattery.pbo`                            |
| **Side**        | server (`#ifdef SERVER`)                                |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

`modded class ItemBase` overrides `EEOnCECreate()`, calls `super` first, then acts on any item with an
energy manager:

```c
if( HasEnergyManager() )
{
    ItemBase item;
    item = ItemBase.Cast( GetInventory().CreateInInventory( "Battery9V" ) );
    ApplyRandomHealthAndEnergy(item);
    item = ItemBase.Cast( GetInventory().CreateInInventory( "CarBattery" ) );
    ApplyRandomHealthAndEnergy(item);
}
```

Both battery types are attempted on every such item; whichever the item cannot accept simply returns
null and is skipped by the null check inside the helper. The battery is then randomised:

| Property | Range |
|---|---|
| Health | 66–100 % of max (`HEALTH_MIN_FACTOR = 0.66`) |
| Charge | 50–90 % (`ENERGY_MIN = 0.5`, `ENERGY_MAX = 0.9`) |

Because this hooks `EEOnCECreate`, it applies **only to Central-Economy loot spawns** — not to
admin-spawned items, and not to a vehicle's `CarBattery` attachment.

## Caveats

- Battery classnames `Battery9V` and `CarBattery` are hardcoded, so modded battery types are not
  covered.
- The helper `ApplyRandomHealthAndEnergy` and its three `static const` fields are declared on
  `ItemBase` itself, not inside the override — they are added to **every** `ItemBase` in the game while
  this PBO is loaded. Worth knowing if another mod ever declares the same names.
- Shares `modded class ItemBase { override void EEOnCECreate() }` with
  `Extra/SpawnWithAmmoAndMagazine`; both call `super`, so they chain safely.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely.
