# Disable Fog (Chernarus+)

Removes volumetric and camera fog from the Chernarus+ terrain, so distant players are not hidden by
weather in a mode where sightlines decide fights.

> **This addon is currently disabled and does not ship.** Its config file is named
> `config.cpp.disabled`, so the build never sees it and no PBO is produced.

|                 |                                                                 |
|-----------------|-----------------------------------------------------------------|
| **PBO**         | none — disabled (would be `extra_disablefogchernarusplus.pbo`)    |
| **Side**        | n/a — no script at all; a config patch loads wherever the PBO does |
| **Stages**      | none — this addon registers no script modules                     |
| **`defines[]`** | none                                                              |
| **Standalone**  | yes — no scripts, no `BattleRoyale*` symbol referenced             |

## Why it is parked

**It targets Chernarus+ only, and this mod runs on Vigrid.** The patch hardcodes the world classes
`CAWorld` and `ChernarusPlus`, so on Vigrid it does nothing at all. It is kept rather than deleted
because it is still correct for anyone running the mod on Chernarus+, and because it is the repo's
reference example of the disable-by-rename technique.

## How it works

There is no script. The whole addon is a `CfgWorlds` override that zeroes the fog parameters on both
the shared base world class and Chernarus+ itself:

```cpp
class CAWorld: DefaultWorld       // and, identically, class ChernarusPlus: CAWorld
{
    class Weather
    {
        class VolFog
        {
            CameraFog  = 0;
            UseDynamic = 0;
            Item1[] = {0,0,0,0,0};
            Item2[] = {0,0,0,0,0};
        };
    };
};
```

`requiredAddons[]` is `DZ_Worlds_Chernarusplus_World` — the only addon in `Extra/` that does not
require `DZ_Scripts`, since it has no script to compile.

## Caveats

- Patching `CAWorld` means **every** CAWorld-derived terrain that does not re-specify its own `VolFog`
  inherits the zeroed values, not just Chernarus+.
- Fog is a gameplay-balance lever as much as a visual one. Turning it off removes a concealment
  mechanic; that is the point, but it is worth being deliberate about.

## Enabling

Rename `config.cpp.disabled` → `config.cpp` and rebuild. The folder will then be packed as
`extra_disablefogchernarusplus.pbo`.
