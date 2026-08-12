# Vigrid Map

A standalone in-game map: a fullscreen pannable map, a HUD minimap, player-placed markers shared with
your party, and the Battle Royale play-area circles when a host mod pushes them in. It replaces DayZ
Expansion Navigation.

It hooks nothing of the host mod's, so it works on **any** DayZ server — Battle Royale is not required.

|                 |                                                                    |
|-----------------|--------------------------------------------------------------------|
| **PBO**         | `extra_map.pbo`                                                    |
| **Side**        | both — client draws, server owns the marker set                    |
| **Stages**      | `3_Game`, `4_World`, `5_Mission`                                   |
| **`defines[]`** | `VIGRID_MAP`, `VIGRID_MAP_MINIMAP`                                 |
| **Requires**    | `DZ_Data`, `DZ_Scripts`, `JM_CF_Scripts` (CF's RPC manager)        |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                         |

## Controls

| Key | What |
|---|---|
| **M** | Open the fullscreen map; press again to close |
| **Esc** | Close the fullscreen map |
| **Left click** | Place your marker (one per player, replaces your previous) |
| **Right click** | Clear your marker |
| **N** | Toggle the HUD minimap — **off by default**, see below |

Pan and zoom are the engine's own. **You can keep running while the map is open**, and clicking the map
does not fire the weapon underneath it.

## What is drawn

Every overlay is a `CanvasWidget` declared as a child of the `MapWidget`, drawing in screen space.
Canvas offers only `DrawLine` and `Clear`, so there is no text anywhere on the map and every glyph is a
fan of strokes:

| Glyph | Means |
|---|---|
| Ring with a cross | A placed marker — yours in its own colour, a teammate's in their party slot colour |
| Circle + centre dot | A play-area zone (current and next), pushed in by the host mod |
| Dashed line | From you to the next zone's centre |
| Hollow triangle | A teammate |
| Lighter diamond | A party ping |
| Axis-aligned plus | You, on the fullscreen map — deliberately does **not** rotate |
| Notched dart | You, on the minimap — carries your heading |

## Settings

| Where | Keys |
|---|---|
| `$profile:Vigrid-Map\map_settings.json` (server) | `enabled`, `minimap_allowed`, `label_max_length` |
| `$profile:Vigrid-Map\map_client.json` (client) | `minimap_enabled` — the player's own toggle, written by **N** |

The minimap is gated by three independent switches, each able only to opt further out than the one
before it:

1. **`VIGRID_MAP_MINIMAP`** — the build's. Comment the define out of `config.cpp` and the minimap class,
   its widgets and the **N** handler are gone from the PBO entirely; the fullscreen map is untouched.
2. **`minimap_allowed`** — the admin's, pushed to clients over `VM_Settings`. Ships **on**.
3. **`minimap_enabled`** — the player's, persisted locally. Ships **off**, so the minimap is opt-in.

Two things deliberately survive a minimap-less build: `minimap_allowed` on the wire and in the settings
file, because a *client* build flag must not change the wire format; and the **N** entry in
`Data/Inputs.xml`, because XML cannot be conditional — it still lists under Options → Controls bound to
nothing.

## Public API

The host mod talks to this addon **only** through `VigridMapAPI`, every call site wrapped in
`#ifdef VIGRID_MAP`:

```c
// client - push, not pull; the addon may not reach into the host mod
static void VigridMapAPI.SetZones(vector cur_center, float cur_radius, vector next_center, float next_radius)
static void VigridMapAPI.ClearZones()

// server
static void VigridMapAPI.ClearAllMarkers()
static void VigridMapAPI.SetMarkersActive(bool active)
```

Party is reached the other way, and **only** through `VigridMapTeam` — the addon's sole
`#ifdef VIGRID_PARTY` code. Every body has an `#else` returning an empty answer, so disabling `Party/`
leaves a working map with teammates and pings simply absent.

## Design notes

**Nothing can be drawn over a `MapWidget` with script-created widgets.** `CreateWidgets(path, parent)`
returns a valid widget and `SetPos` puts it in the right place — and it is never rendered, with no
warning and no failed image load. A `CanvasWidget` **declared in the layout** as a child of the map is
the only overlay that works.

**Markers are server-authoritative.** The owner is always `sender.GetPlainId()`, the whole visible set
is pushed as a snapshot rather than deltas (tiny, and idempotent under packet loss), with a 5 s resync
so joining a party mid-match works. A marker records its placer's **party slot at placement time**, so
it keeps its colour when the placer disconnects.

**The repaint gate is split.** Zones and markers are edge-triggered; teammates have no edge — a party's
roster sequence moves when the party changes shape, never when somebody walks — so that layer repaints
on a 10 Hz clock. Every canvas must `Clear()` before any early return, or the last frame burns in.

**The two "you" glyphs differ on purpose.** The fullscreen map's plus does not rotate, because a
rotating "you" is harder to *find* on a big map; the minimap's dart carries heading, which is the whole
reason to glance at it. The dart's angle is the **camera** bearing, never `GetYawPitchRoll()` — body yaw
snaps in steps and does not return to its start after a 360.

**The fullscreen map does not stop the player.** `VigridMapMenu` declares `UseKeyboard() == false`, so
only the mouse focus is taken. Do **not** add `AddActiveInputRestriction(EInputRestrictors.MAP)`: it
looks like the right hook, but its entire body force-enables walk, pinning the player to walking speed.

**The price of that is that every non-keyboard input stays live, and it is paid in two places.**
`VigridMapMenu.SuppressGameplayInputs` covers the buttons — fire, melee, use-action, gear, quickbar,
freelook, zoom — one `Supress()` per input, per frame. It cannot cover the four **aim axes**:
`Supress()` is a press event concept, and both player cameras read the aim engine-side from the input
controller (`GetAimChange` / `GetAimDelta`), so the mouse still turned the camera under the map.
`MapMissionGameplay.UpdateAimSuppression` handles them with an exclude group of exactly `{"aiming"}` —
**the one place in this addon where an exclude group is the right tool.** `"aiming"`
(`P:\bin\specific.xml:149`) is precisely the four aim inputs and contains **no movement**, which is why
it works here where vanilla's own `{"map"}` does not: that one is `<include name="menu" />`, and
`"menu"` includes `"movement"`.

**Measured, do not retry: `HumanInputController.OverrideAimChangeX/Y(ENABLED, 0)` does not work.** It
was the first attempt, and it looked right — the host mod drives `OverrideMovementSpeed`,
`OverrideRaise` and `OverrideFreeLook` from that same family and those all work. The camera kept
turning anyway, with an edge log confirming the calls reached the live controller on every open and
close. The tell was there in advance: `OverrideRaise` and `Override3rdIsRightShoulder` have real
vanilla call sites, while `OverrideAimChangeX/Y` have **none anywhere in `P:\scripts`** — only the
proto declaration at `human.c:240`.

The `UpdateControls()` cost in the caveat below is real and is **accepted**: opening or closing the map
mid-sprint drops the player out of sprint until Shift is re-pressed. A camera that spins while you read
the map is worse. Two things keep it contained — it is **edge-triggered**, never per-frame, since each
call rebuilds the control state; and it lives in the **mission** update rather than the menu, so the
*remove* edge cannot be missed the way an `OnShow`/`OnHide` pairing can when a menu is torn down
without `OnHide`. A leaked exclude group would leave the player permanently unable to aim.

## Caveats

- **Never override `OnMouseWheel`** on the map widget — native zoom dies. `ClampZoom()` holds the range
  each frame instead, because there is no zoom event to hook.
- **An input exclude group resets every held input, so `{"aiming"}` is the only one used here.** Both
  `AddActiveInputExcludes` and `RemoveActiveInputExcludes` end in `GetUApi().UpdateControls()`, which
  rebuilds the control state and drops the **held** state of every input including `UATurbo` — so
  opening or closing the map mid-sprint dumps the player out of sprint until Shift is re-pressed. The
  reset comes from the group being added or removed at all, not from what is in it, and it is the same
  mechanism that walks a vanilla player when they open their inventory. That is why the *buttons* use
  per-frame `Supress()` instead: it touches nothing global and costs nothing on close. The aim axes
  have no such option, so they pay the reset — do not extend the group's membership to anything
  `Supress()` could have handled.
- Satellite imagery is **not** part of this addon. It comes from `Extra/MapSatellite/`, which patches
  the engine's `MapDefaults`. Every overlay here draws in screen space over whatever the `MapWidget`
  renders, so nothing depends on which raster layer is underneath.
- **Y also toggles Community-Online-Tools' sidebar.** That is Party's ping-clear bind, not this
  addon's, but it shows up while reading the map.

## Logging

`-map-trace` / `-map-debug` / `-map-info` / `-map-warn` / `-map-none` on the command line, or
`MapLogLevel` in `serverDZ.cfg`. Diag builds default to trace.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild. The host mod's call sites are all
`#ifdef VIGRID_MAP`, so it still builds — there is simply no map.
