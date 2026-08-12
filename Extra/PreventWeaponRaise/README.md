# Prevent Weapon Raise

Stops the weapon from being forced upward when the muzzle is close to a wall or doorway. Vanilla's
lift-weapon behaviour is a realism touch that reads as unresponsive in close-quarters fights, so it is
removed here — unless the server admin asks for it back, see *Configuration*.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_preventweaponraise.pbo`                          |
| **Side**        | both (unguarded, with `#ifdef SERVER` halves inside)    |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

`modded class PlayerBase` overrides `CheckLiftWeapon()` and does **not** call `super`, discarding
vanilla's obstruction raycast entirely. All that is left is the un-lift path:

```c
override void CheckLiftWeapon()
{
    //--- Admin opted out: hand the whole thing back to vanilla.
    if (m_VigridPreventWeaponRaiseDisabled)
    {
        super.CheckLiftWeapon();
        return;
    }

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

## Configuration

One key, read only by this addon:

| Where | Key | Effect |
|---|---|---|
| `serverDZ.cfg` | `BRDisablePreventWeaponRaise = 1;` | turns the addon off — vanilla's obstruction lift comes back |

An absent key reads as `0`, which is indistinguishable from an explicit `0` — both leave the addon
active, so a server that has never heard of the key keeps the behaviour it has today.

## Design notes

**The switch is server-owned but the behaviour is client-side, and bridging that is the whole of the
implementation.** `ServerConfigGetInt` is a server-only API — vanilla documents it as *"Server config
parsing. Returns 0 if not found"*, and a client has no `serverDZ.cfg` to find anything in, so it
answers `0` for every key. Reading the switch directly from the client-side hook therefore cannot
work, and fails **silently**: the key appears to be ignored with no error anywhere.

So the server reads it and mirrors the answer onto each player as the netsync bool
`m_VigridPreventWeaponRaiseDisabled`, exactly the way [SafeZone](../SafeZone/README.md) carries its
truce flag. Three consequences:

- **`PlayerBase.c` is deliberately unguarded** so both sides compile it. `RegisterNetSyncVariableBool`
  sits in the shared `Init()` override precisely so registration order is byte-identical on both
  sides — mandatory for netsync. Compiling the override server side costs nothing: vanilla's whole
  `CheckLiftWeapon` body is inside an `INSTANCETYPE_CLIENT` branch, so both paths are no-ops there.
- **Netsync rather than an RPC broadcast**, so a player connecting later is covered for free;
  `OnConnect` / `OnReconnect` re-assert it unconditionally.
- **The key is read once per process**, in `VigridPreventWeaponRaiseState`. `CheckLiftWeapon` runs
  every frame from `HandleWeapons` inside the command handler, so nothing but a field read belongs in
  it — the netsync flag costs nothing there, a string-keyed config lookup would not.

The flag defaults to `false` (suppression active), so a client that has not received its first sync
yet behaves exactly as this addon always has.

## Caveats

- Vanilla's `CheckLiftWeapon` also computes and syncs a separate weapon-obstruction value. This
  override never writes it, so a nonzero obstruction value set before the addon loaded can be left
  stale.
- Skipping the check means a player can fire with the muzzle inside geometry. That is the intended
  trade-off, but it does allow shooting through thin cover in ways vanilla prevents.

## Disabling

`BRDisablePreventWeaponRaise = 1;` in `serverDZ.cfg` is the runtime switch and needs no rebuild — use
that one on a server running the published mod. To drop the addon from the build entirely, rename
`config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped.
