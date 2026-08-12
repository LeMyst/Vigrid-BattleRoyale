# Limit Unconscious Time

Caps how long a player can be knocked out. In vanilla, unconsciousness lasts until shock regenerates
past the wake threshold, which in a battle royale means a player can be out of the match for minutes
with nothing to do. This force-wakes them after **5 seconds**.

|                 |                                                    |
|-----------------|----------------------------------------------------|
| **PBO**         | `extra_limitunconscioustime.pbo`                    |
| **Side**        | server (`#ifdef SERVER`)                            |
| **Stages**      | `4_World`                                           |
| **`defines[]`** | none                                                |
| **Standalone**  | **no** — depends on the main mod (see Caveats)      |

## How it works

`modded class UnconsciousnessMdfr` overrides three methods on the vanilla unconsciousness modifier.

**`ActivateCondition`** — stamps `player.m_UnconsciousStartTime` with the current time when the player
goes down, then defers to vanilla's own condition (shock at or below 25).

**`DeactivateCondition`** — the actual limit:

```c
if (GetGame().GetTime() - player.m_UnconsciousStartTime > (UNCONSCIOUSNESS_MAX_TIME * 1000))
{
    player.SetHealth("", "Shock", SHOCK_DAMAGE_AFTER_UNCONSCIOUSNESS);
    return true;    // wake up
}
return super.DeactivateCondition(player);
```

Two constants drive it (both at the top of `Unconsciousness.c`, marked with a `// TODO: move to config`):

| Constant | Value | Meaning |
|---|---|---|
| `UNCONSCIOUSNESS_MAX_TIME` | `5.0` | seconds before a forced wake-up |
| `SHOCK_DAMAGE_AFTER_UNCONSCIOUSNESS` | `PlayerConstants.CONSCIOUS_THRESHOLD + 0.1` = **50.1** | shock the player is set to on waking — just past the threshold, so they wake and stay awake |

`GetGame().GetTime()` is in milliseconds, hence the `* 1000`. Below the limit, vanilla rules still
apply (shock at or above 50 **and** a regular pulse).

## Configuration

Unconsciousness can be switched off entirely — two independent kill-switches, either one is enough.
Both are read only by this addon and are documented nowhere else:

| Where | Key | Effect |
|---|---|---|
| `serverDZ.cfg` | `BRDisableUnconsciousness = 1;` | disables unconsciousness |
| server command line | `-br-disable-unconsciousness` | same |

When disabled, a player who would have gone down instead stays conscious and is **forced prone**:

```c
HumanCommandMove hcm = player.GetCommand_Move();
if (hcm) hcm.ForceStance(DayZPlayerConstants.STANCEIDX_PRONE);
```

## Caveats

- **Not standalone.** `requiredAddons[]` lists only `DZ_Scripts`, but the code reads
  `player.m_UnconsciousStartTime` (declared by the main mod's `PlayerBase`,
  `Scripts/Server/4_World/Entities/ManBase/PlayerBase.c`) and calls `BattleRoyaleUtils`. It works today
  by incidental load order, not by contract — `TODO.md` tracks adding `BattleRoyale_Scripts_Server` to
  `requiredAddons[]`.
- In the **disabled** path the player keeps their low shock, so the vanilla condition keeps
  re-evaluating on every modifier tick and re-issues the prone command each time.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely and
vanilla unconsciousness duration returns. To keep the addon but turn the mechanic off at runtime, use
the `serverDZ.cfg` key above instead.
