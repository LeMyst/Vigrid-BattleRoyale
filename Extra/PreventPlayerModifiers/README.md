# Prevent Player Modifiers

Turns off the vanilla survival simulation — hunger, thirst, illness and broken legs. A battle royale
match lasts minutes, so metabolism and disease add bookkeeping without adding decisions, and a broken
leg is effectively a death sentence with no time to heal.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_preventplayermodifiers.pbo`                      |
| **Side**        | mixed — see Caveats                                     |
| **Stages**      | `4_World`                                               |
| **`defines[]`** | none                                                    |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

## How it works

Eight files, nine modded classes, three different techniques:

| Class | Override | Effect |
|---|---|---|
| `HungerMdfr` | `OnTick` → empty | No energy drain, no hungry sound state, no starvation damage. The modifier still activates; it just does nothing. |
| `ThirstMdfr` | `OnTick` → empty | Same, for water and dehydration damage. |
| `CommonColdMdfr` | `ActivateCondition` → `false` | Never contract common cold. |
| `InfluenzaMdfr` | `ActivateCondition` → `false` | Never contract influenza. |
| `PneumoniaMdfr` | `ActivateCondition` → `false` | Never contract pneumonia. |
| `WoundInfectStage1Mdfr` | `ActivateCondition` → `false` | Never contract wound infection. Vanilla's stage-1 deactivate condition is the inverse of activate, so an already-active infection also clears. |
| `WoundInfectStage2Mdfr` | `ActivateCondition` → `false` | As above, second stage. |
| `BrokenLegsMdfr` | `OnActivate`, `OnReconnect`, `Activate` → all empty | Legs never break: no `SetBrokenLegs`, no fracture notifier, and the modifier cannot enter the active set at all. |
| `HeatComfortAnimHandler` | `Update` → returns immediately | No heat-comfort symptom animations (shivering, rattling, overheating). Not a modifier — this is the client-side animation handler. |

## Not covered

These vanilla diseases are left fully active, in case you expected a clean sweep:

`Cholera`, `Salmonella`, `Contamination1/2/3`, `HeavyMetal`, `BrainDisease`.

Bleeding, blood loss and shock are also untouched — they are core combat mechanics, not survival ones.

## Caveats

- **The `#ifdef SERVER` guard is inconsistent.** Only `BrokenLegs.c` has one; the other seven files
  have no guard and therefore compile and run client-side too. For `HeatComfortAnimHandler` that is
  arguably load-bearing (it is a client-side handler), but for the `*Mdfr` classes the modifier manager
  is server-authoritative, so the client copies are redundant. Tracked in `TODO.md`.
- The three disease files declare `override protected bool ActivateCondition`, but vanilla
  `CommonColdMdfr.ActivateCondition` is declared **without** `protected`. Worth knowing if this ever
  produces a compile warning.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely and full
vanilla survival mechanics return. There is no runtime switch — remove individual `.c` files if you
want to keep only some of the effects.
