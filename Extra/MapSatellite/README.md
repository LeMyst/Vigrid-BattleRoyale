# Vigrid Map Satellite

Turns on the satellite layer of the in-game map, retunes the map's overlay layers so they work over
photographic imagery instead of a pale contour drawing, and makes place names legible over it.

It is **config only** — there is not a single line of script in this addon, and no assets. A handful
of numbers, a colour block and two label classes.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_mapsatellite.pbo`                               |
| **Side**        | both — it is a config patch, so it ships everywhere    |
| **Stages**      | none — no scripts                                      |
| **`defines[]`** | none                                                   |
| **Requires**    | `DZ_Data`, `DZ_Gear_Navigation`                        |
| **Standalone**  | yes — no `BattleRoyale*` symbol, and no `Extra/Map` dependency either |

## Why it exists

The satellite map everyone was used to was never the terrain's doing: it came from
**@DayZ-Expansion-Navigation**, which sets `maxSatelliteAlpha = 1.0` where vanilla ships `0`. Dropping
Navigation — which `Extra/Map/` makes possible — silently swapped the map back to topographic. This
addon turns it back on.

The imagery itself is the terrain's own and already shipped with the world
(`dz\worlds\<world>\data\layers\s_XXX_YYY_lco.paa`). Nothing is bundled here.

## Provenance

**Every value here is derived from vanilla, not from Expansion.** Earlier revisions of this file were
Expansion's config block "copied whole". Expansion's source is licensed **CC BY-NC-ND 4.0** — the
*NoDerivatives* term is squarely at odds with copying their config and altering values in it, and
*NonCommercial* matters the moment a server takes donations. It was re-derived from Bohemia's own
config on 2026-08-09.

Nothing was lost in the process. What was actually wanted from Expansion was a **fact** about the
engine — where the place-name shadow key lives — and a **technique**: *zero the alpha on the fill
layers that fight a photo, keep the linear features that still read.* Neither is anyone's expression,
and both are applied to vanilla's own hues.

The rule every colour obeys, which also makes the diff against `P:\DZ\data\config.cpp:1717`
reviewable at a glance:

> **Keep Bohemia's RGB. Change only the alpha, and only where a layer occludes the imagery.**

Anything vanilla already gets right is simply **not restated** — roads, tracks, rails, power lines,
the grid, the `ptsPerSquare*` densities, the fonts, the `Legend` block and the icon subclasses are
all absent on purpose and inherited untouched. A shorter file is a safer one here.

## ⚠️ The one rule

**If you redeclare an existing class, carry its parent — or do not redeclare it. Never declare
`class RscMapControl` in this file.**

Doing that hard-froze every client on 2026-08-07, proven by bisection on 2026-08-08. Vanilla ships
`class RscMapControl: MapDefaults {};`, and redeclaring it **without restating the parent replaces it
rather than merge-patching it** — so the real map control kept satellite and lost everything else,
including the `ptsPerSquare*` tessellation densities. Zero points per square is the kind of value that
turns a polygon tessellator into an infinite loop, which is what a permanent hang with no error, at
every zoom, on every `MapWidget`, while the server runs on untouched, actually looks like.

**This inheritance rule applies to every config override in the repo, not just this one.** The file
header carries the full evidence, including the two hypotheses that were refuted by a build each
(`maxUserMapAlpha = 1`, and the absent `scaleMin`/`scaleMax`/`scaleDefault` — both harmless).

It is why the `CfgLocationTypes` patch touches **only `Name` and `NameIcon`**: both are *parentless*
in vanilla, so there is no parent to drop. Every other label class derives from one of them and
inherits the changes for free. Touch a derived class and you must write `class Village: Name`, never
`class Village`.

## What it sets

Two **global, top-level** classes, both *outside* `CfgWorlds` — an override written against
`CfgWorlds` is inert. Between them they cover every terrain and every `MapWidget`: the fullscreen
map, the HUD minimap, the BR HUD minimap and the spawn-selection screen.

| class | declared in | covers |
|---|---|---|
| `MapDefaults` | `P:\DZ\data\config.cpp:1717` | the raster and vector layers |
| `CfgLocationTypes` | `P:\DZ\gear\navigation\config.cpp:14` | the place-name labels |

Two things only become visible once satellite renders, and they need different fixes:

- **Vanilla's overlay layers fight imagery.** `colorForest` is bright green at α 0.5 and `colorSea` is
  fully opaque, both tuned for a pale contour drawing. Fill layers that duplicate what the photo
  already shows are taken to α 0 or near it; linear features that still read — roads, rails, power
  lines, the grid — are left at vanilla.
- **Place names needed a shadow, not a colour.** Contrast comes from `shadow = 1` on
  `CfgLocationTypes::Name`, plus the larger `MetronBook-Bold58` face (Metron is a bitmap font, so
  raising `textSize` on the 28px face just goes soft). Amber on top of that is a preference, not a
  contrast hack.

> ⚠️ **Correction, 2026-08-09.** This file and `CLAUDE.md` both used to state that *"`MapDefaults`
> exposes no outline or shadow key for place names, so contrast has to come from hue."* **That was
> wrong.** Place names are not styled by `MapDefaults` at all — `colorNames` / `sizeExNames` govern
> grid and mountpoint labels. Settlement names come from `CfgLocationTypes`, which has had a `shadow`
> key all along. The amber was the right instinct aimed at the wrong key, which is why the names
> stayed unreadable no matter what was set.

`alphaFadeStartScale` / `alphaFadeEndScale` stay at `2` (satellite at every zoom) rather than
vanilla's `1`. Dropping to `1` would fade satellite out when zoomed out and fall back to the topo
layer — whose alphas have deliberately been dialled down, so it would fade to a near-blank map. **The
fade values and the colour block are a package.**

`scaleMin` / `scaleMax` / `scaleDefault` are **not** set. Vanilla does not declare them,
`VigridMapMenu.ClampZoom()` re-clamps `GetScale()` to 0.06 – 1.0 every frame regardless, and their
absence was already proven harmless by one of the 2026-08-08 bisection builds.

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
