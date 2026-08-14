# Spawn With Ammo And Magazine

Loot firearms come with ammunition next to them. Without this, a rifle found in a building is dead
weight until a magazine turns up somewhere else — a poor fit for a mode where the first minutes decide
the match.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_spawnwithammoandmagazine.pbo`                    |
| **Side**        | server (`#ifdef SERVER`)                                |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

`modded class ItemBase` overrides `EEOnCECreate()` and calls `super` first, so vanilla's own
CE-spawn initialisation still runs. It then acts only when `IsWeapon()` is true — real firearms.

**If the weapon takes magazines**, it rolls a count between `min_spawn` and `max_spawn` (default 1–2)
and creates that many magazines:

```c
GetGame().CreateObjectEx(magazineType, GetPosition(), ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH);
```

**If it takes none** (a break-action or bolt gun with no detachable mag), it reads the weapon's own
`chamberableFrom` config array and drops loose ammo piles instead, deleting any pile that has no
economy profile.

Note both paths use `GetPosition()` — the ammunition lands **on the ground at the weapon**, not inside
its inventory. If you want the gun itself loaded, that is `Extra/SpawnWeaponChambered`'s job; the two
stack fine.

Because this hooks `EEOnCECreate`, it applies **only to Central-Economy loot spawns** — not to
admin-spawned or scripted weapons.

## Configuration

Three `serverDZ.cfg` keys, read only by this addon. `VigridSpawnAmmoConfig` reads all three **once per
process**, on the first CE weapon: `EEOnCECreate` fires for every item the economy creates — 33,921 on
a cold ChernarusPlus boot — and the original code read `BRDisableSpawnWithAmmo` *before* the
`IsWeapon()` test, so every tin can paid for a config lookup during the phase that dominates boot time.
serverDZ.cfg is parsed at boot and never changes under a running server, so nothing is lost.

| Key | Effect |
|---|---|
| `BRDisableSpawnWithAmmo` | `1` disables the addon entirely |
| `BRMinSpawnAmmo` | above `0` overrides the minimum; below `0` forces the minimum to `0`; `0` keeps the default of 1 |
| `BRMaxSpawnAmmo` | above `0` overrides the maximum; `0` or below keeps the default of 2. Raised to the minimum if it ends up lower |

## Caveats

- **Magazines over 100 rounds are skipped** — drums and other high-capacity magazines never spawn. The
  loop retries a different type, up to `spawnCount * 10` attempts.
- The remaining `Print()` calls are the two rare diagnostics (a skipped high-capacity magazine, and a
  shortfall after all attempts). The per-magazine pair that used to sit on the hot path is gone: it
  wrote 6,344 of the 44,700 lines in one boot's script log, and one of the two wrapped the real
  `CreateObjectEx` call inside a `Print()` argument.
- In the no-magazine branch the loop bound `Math.RandomIntInclusive(min_spawn, max_spawn)` is
  **re-rolled on every iteration**, which biases the number of ammo piles toward the minimum.
- Shares `modded class ItemBase { override void EEOnCECreate() }` with `Extra/SpawnWithBattery`; both
  call `super`, so they chain safely.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely. To keep
the addon but turn it off at runtime, set `BRDisableSpawnWithAmmo = 1;` in `serverDZ.cfg` instead.
