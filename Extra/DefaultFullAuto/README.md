# Default Full Auto

Every weapon that has a full-auto fire mode spawns already switched to it, so a player who picks a
rifle up mid-fight does not have to remember to toggle the selector first.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_defaultfullauto.pbo`                             |
| **Side**        | server (`#ifdef SERVER`)                                |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

`modded class Weapon_Base` overrides `EEInit()`, calls `super.EEInit()` first, then walks every muzzle
on the weapon looking for an auto-fire mode:

```c
for (int i = 0; i < GetMuzzleCount(); i++)
{
    int newMuzzleMode = GetCurrentMode(i);          // fallback: whatever the config default was
    for (int j = 0; j < GetMuzzleModeCount(i); j++)
    {
        SetCurrentMode(i, j);
        if (GetCurrentModeAutoFire(i)) { newMuzzleMode = j; break; }
    }
    SetCurrentMode(i, newMuzzleMode);
}
```

The scan works by *mutating* the current mode and reading `GetCurrentModeAutoFire` back, then
committing the winner on the last line. Weapons with no auto mode keep their config default, so
semi-only guns are untouched.

Because this hooks `EEInit` rather than `EEOnCECreate`, it applies to **every** weapon created
server-side — loot spawns, admin-spawned guns and scripted loadouts alike.

## Caveats

- `Extra/SpawnWeaponChambered` also does `modded class Weapon_Base { override void EEInit() }`. The two
  are compatible: both call `super.EEInit()` first and they touch disjoint state (fire mode vs. chamber).

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely.
