# Reduce Car Damage

Scales down the damage vehicles take in collisions, and the damage their crew takes, so a minor
bump no longer wrecks a car. **Third-party code:** written by DaemonForge, published at
<https://github.com/daemonforge/DayzSA-ReduceCarDamage/> under the **Apache License 2.0**, and
built here from source instead of being bundled as a prebuilt PBO.

|                 |                                                                       |
|-----------------|-----------------------------------------------------------------------|
| **PBO**         | `extra_reducecardamage.pbo`                                           |
| **Side**        | compiles on both — the damage code runs only where `IsServer()`       |
| **Stages**      | `4_World`                                                             |
| **`defines[]`** | none                                                                  |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                            |
| **Licence**     | Apache 2.0, in `LICENSE` and `NOTICE` here, **not** this mod's DSPL-SA |

## Where it came from

Until 2026-11-18 this was `Workbench\ExtraPBOs\ReduceCarDamage.pbo`, the author's PBO signed with
their own `daemonforge` key, copied into the mod by `CI0_CopyExtraPBO.bat`. DayZ 1.30 stopped
compiling it (see below), so it was unpacked here byte for byte and changed in two places. The
original PBO is kept, out of the copy, in `Workbench\ExtraPBOs\replaced\`.

## Changes from the original

Apache 2.0 asks for changed files to say so; both carry a *Changed for Vigrid-BattleRoyale*
comment at the change.

- `Scripts/4_World/ReduceCarDamage/rcdsettingsloader.c`, line 23:
  `ref ReduceCarDamageSettings data = new ref ReduceCarDamageSettings;` became
  `ReduceCarDamageSettings data = new ReduceCarDamageSettings;`. DayZ 1.30 stops on it with
  `Unexpected statement 'ref'` and `Can't compile "World" script module!` unless the game runs with
  `-newErrorsAreWarnings=1`. A local variable keeps its object alive anyway, and the static
  `settings` keeps it after the call.
- `config.cpp`: the script path `ReduceCarDamage/scripts/4_world` became
  `Vigrid-BattleRoyale/Extra/ReduceCarDamage/Scripts/4_World`, since the build derives the prefix
  from this folder's path, not the original PBO's `ReduceCarDamage`. The folder `scripts` became
  `Scripts`, as in the other addons. The `CfgPatches` and `CfgMods` class names stay
  `ReduceCarDamage`.

`ReduceCarDamage.c`, `LICENSE` and `NOTICE` are unchanged.

## How it works

`modded class CarScript` replaces `OnContact()` without calling `super`. On the server it scales
the contact damage, skips crashes below a minimum, and scales the crew's health and shock loss;
it can log each crash.

## Runtime configuration

`$profile:ReduceCarDamage\config.json`, written with these defaults the first time a server (or an
offline game) starts without one:

| Key | Default | Meaning |
|---|---|---|
| `dmgModifier` | `0.8` | Multiplier on the vehicle's crash damage |
| `plyrdmgModifier` | `0.8` | Multiplier on the crew's health loss; also divides the damage that kills the crew outright |
| `mindmg` | `1350` | Crashes below this damage do nothing |
| `subtractmindmg` | `true` | Subtract `mindmg` from the damage before scaling it |
| `logcrashes` | `false` | Print each crash to the script log |

## Caveats

- **`OnContact()` is replaced, not extended.** In 1.30, vanilla's `Transport.OnContact()` only
  gathers contacts into `m_ContactCache`, and `CarScript.CheckContactCache()` applies the damage
  each tick from the change in momentum. Without `super`, none of that runs for cars: their crash
  damage is this addon's older formula on the contact impulse. Boats are not touched.
- **Loading the author's own mod as well** defines every class twice. Use one or the other.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely.
