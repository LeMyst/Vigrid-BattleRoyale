# Spawn Weapon Chambered

Weapons arrive with a round in the chamber and, where possible, a magazine attached — so a gun picked
up mid-fight can be fired immediately rather than needing to be loaded and racked first.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_spawnweaponchambered.pbo`                        |
| **Side**        | server (`#ifdef SERVER`)                                |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

`modded class Weapon_Base` overrides `EEInit()`, calls `super.EEInit()` first, then:

```c
FillChamber( "", WeaponWithAmmoFlags.CHAMBER );   // "" = pick a valid ammo type at random

Magazine mag = GetMagazine(0);
if ( mag == NULL )
    SpawnAmmo();                                  // vanilla: fill internal mag, else attach one, else fill chamber
```

Unlike the CE-only spawn addons in this folder, this hooks `EEInit`, so it fires for **every**
`Weapon_Base` created server-side — loot spawns, admin-spawned weapons, scripted spawns and player
loadouts alike.

## Caveats

- The magazine check uses muzzle index `0`, so on a double-barrel or other multi-muzzle weapon only the
  first muzzle is inspected.
- It runs immediately after `super.EEInit()`, which itself only *queues* `AssembleGun` on the game call
  queue — chambering therefore happens before that queued assembly runs.
- `Extra/DefaultFullAuto` also does `modded class Weapon_Base { override void EEInit() }`. The two are
  compatible: both call `super` first and touch disjoint state (chamber vs. fire mode).
- Stacks with `Extra/SpawnWithAmmoAndMagazine`, which drops **loose** magazines on the ground next to
  CE-spawned weapons. This addon loads the gun itself; that one supplies the reloads.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely.
