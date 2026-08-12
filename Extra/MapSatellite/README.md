# Vigrid Map Satellite

Turns on the satellite layer of the in-game map, and retunes the map's overlay colours so they work
over photographic imagery instead of a pale contour drawing.

It is **config only** — there is not a single line of script in this addon, and no assets. Ten numbers
and a colour block.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_mapsatellite.pbo`                               |
| **Side**        | both — it is a config patch, so it ships everywhere    |
| **Stages**      | none — no scripts                                      |
| **`defines[]`** | none                                                   |
| **Requires**    | `DZ_Data`                                              |
| **Standalone**  | yes — no `BattleRoyale*` symbol, and no `Extra/Map` dependency either |

## Why it exists

The satellite map everyone was used to was never the terrain's doing: it came from
**@DayZ-Expansion-Navigation**, which sets `maxSatelliteAlpha = 1.0` where vanilla ships `0`. Dropping
Navigation — which `Extra/Map/` makes possible — silently swapped the map back to topographic. This
addon is those numbers.

The imagery itself is the terrain's own and already shipped with the world
(`dz\worlds\<world>\data\layers\s_XXX_YYY_lco.paa`). Nothing is bundled here.

## ⚠️ The one rule

**Patch `MapDefaults` only. Never declare `class RscMapControl` in this file.**

Doing that hard-froze every client on 2026-08-07, proven by bisection on 2026-08-08. Vanilla ships
`class RscMapControl: MapDefaults {};`, and redeclaring it **without restating the parent replaces it
rather than merge-patching it** — so the real map control kept satellite and lost everything else,
including the `ptsPerSquare*` tessellation densities. Zero points per square is the kind of value that
turns a polygon tessellator into an infinite loop, which is what a permanent hang with no error, at
every zoom, on every `MapWidget`, while the server runs on untouched, actually looks like.

**This inheritance rule applies to every config override in the repo, not just this one.** The file
header carries the full evidence, including the two hypotheses that were refuted by a build each
(`maxUserMapAlpha = 1`, and the absent `scaleMin`/`scaleMax`/`scaleDefault` — both harmless).

## What it sets

The settings live in the **global, top-level `class MapDefaults`**, *not* under `CfgWorlds`. An
override written against `CfgWorlds` is inert. One block covers every terrain and every `MapWidget`:
the fullscreen map, the HUD minimap and the Battle Royale spawn-selection screen.

Two things only become visible once satellite renders, both handled here:

- **Vanilla's overlay colours fight imagery.** `colorForest` is bright green at α 0.5 and `colorSea` is
  fully opaque, both tuned for a pale contour map. Expansion's colour block is copied whole.
- **`MapDefaults` exposes no outline or shadow key for place names** — only `fontNames`, `sizeExNames`
  and `colorNames` — so label contrast has to come from hue. Amber is used because neither black nor
  white survives both Sakhal's snowfields and its dark forest.

`alphaFadeStartScale` / `alphaFadeEndScale` stay at Expansion's `2` (satellite at every zoom) rather
than vanilla's `1`. Dropping to `1` would fade satellite out when zoomed out and fall back to the topo
layer — whose colours have deliberately been dialled down, so it would fade to a near-blank map. **The
fade values and the colour block are a package.**

## Runtime configuration

**None, and it cannot have any.** `MapWidget` exposes no satellite API and the key appears nowhere in
the vanilla scripts, so the `config.cpp` ↔ `config.cpp.disabled` rename is the whole control surface.

## Relationship to `Extra/Map/`

Independent. This addon changes what the `MapWidget` *renders*; `Extra/Map/` draws its overlays in
screen space on `CanvasWidget` children on top of it. Either works without the other, and the two
raster layers are interchangeable as far as the map addon is concerned. The choice is purely a look.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild, and the map goes back to the vanilla
topographic layer. **Check the output folder afterwards** — an incremental `Deploy.bat` does not
reliably delete the PBO of an addon you just disabled, so delete `extra_mapsatellite.pbo` and its
`.bisign` by hand if they are still there.
