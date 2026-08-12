# Prevent Weapon Raise

Stops the weapon from being forced upward when the muzzle is close to a wall or doorway. Vanilla's
lift-weapon behaviour is a realism touch that reads as unresponsive in close-quarters fights, so it is
removed here.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_preventweaponraise.pbo`                          |
| **Side**        | client (`#ifndef SERVER`)                               |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

`modded class PlayerBase` overrides `CheckLiftWeapon()` and does **not** call `super`, discarding
vanilla's obstruction raycast entirely. All that is left is the un-lift path:

```c
override void CheckLiftWeapon()
{
    if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT)
    {
        // Never lift weapon and sync it to false if already lifted
        if (m_LiftWeapon_player)
            SendLiftWeaponSync(false);
    }
}
```

So the lift state is never set, and if something else already set it, it is synced back to `false`.

Despite the addon's name, this affects **only** the obstruction lift. Raising the weapon normally,
aiming down sights and melee are all untouched.

## Caveats

- Vanilla's `CheckLiftWeapon` also computes and syncs a separate weapon-obstruction value. This
  override never writes it, so a nonzero obstruction value set before the addon loaded can be left
  stale.
- Skipping the check means a player can fire with the muzzle inside geometry. That is the intended
  trade-off, but it does allow shooting through thin cover in ways vanilla prevents.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely.
