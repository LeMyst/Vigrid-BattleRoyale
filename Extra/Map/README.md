# Vigrid Map

A standalone in-game map: a fullscreen pannable map, a HUD minimap, a HUD compass strip, player-placed
markers shared with your party, and the Battle Royale play-area circles when a host mod pushes them in.
It replaces DayZ Expansion Navigation.

It hooks nothing of the host mod's, so it works on **any** DayZ server — Battle Royale is not required.

|                 |                                                                    |
|-----------------|--------------------------------------------------------------------|
| **PBO**         | `extra_map.pbo`                                                    |
| **Side**        | both — client draws, server owns the marker set                    |
| **Stages**      | `3_Game`, `4_World`, `5_Mission`                                   |
| **`defines[]`** | `VIGRID_MAP`, `VIGRID_MAP_MINIMAP`, `VIGRID_MAP_COMPASS`           |
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
| **K** | Toggle the HUD compass strip — **on by default**, see below |

Pan and zoom are the engine's own. **You can keep running while the map is open**, and clicking the map
does not fire the weapon underneath it.

Placing, moving and clearing a marker all take effect **the moment you click**, not when the server
answers — see *Design notes*. If the server refuses a placement outright, a message says so in the strip
above the map instead.

## What is drawn

Every overlay is a `CanvasWidget` declared as a child of the `MapWidget`, drawing in screen space.
Canvas offers only `DrawLine` and `Clear`, so there is no text anywhere **on** the map and every glyph
is a fan of strokes. (The refusal message is the one piece of text, and it is a `TextWidget` sibling of
the `MapWidget` sitting in the strip above it, not an overlay on it.)

| Glyph | Means |
|---|---|
| Ring with a cross | A placed marker — yours in its own colour, a teammate's in their party slot colour |
| Circle + centre dot | A play-area zone (current and next), pushed in by the host mod |
| Red circle | A hot zone — a static region of interest, pushed in by the host mod. Drawn under the play-area rings, so it never obscures the circle you have to reach |
| Dashed line | From you to the near edge of the zone you have to reach — the next one when there is one, the current one otherwise. Only drawn while you are outside it, so its length is the distance you still have to cover |
| Hollow triangle | A teammate |
| Lighter diamond | A party ping |
| Notched dart | You, on both maps — carries your heading, and the largest glyph on either |

## The compass strip

A 620×42 band sitting **flush against the top of the screen**, showing a **90° window** — cardinal
letters every 45°, numeric degrees every 30°, an unlabelled tick every 15°, and the exact bearing read
out below it.

Labels come in three sizes so the directions a player actually calls out read first: **N/E/S/W in
`metron-bold28`, the diagonals in `-bold22`, the numeric degrees in `-bold14`.** Measured in game at
22 px and 8 px cap height for the outer two tiers.

**Each tier is its own widget, and it has to be.** A widget's glyph size is fixed by the font face it
declares and there is no `SetFont`, so `compass_entry.layout` carries three label widgets and
`VigridMapCompass.PickLabel` shows exactly one per entry. `SetTextExactSize` looks like the obvious
alternative and **was measured to do nothing** — 28/18/13 requested on one widget rendered 28/28/28,
which is why the tiers were briefly indistinguishable. Metron is a bitmap font shipped at 12, 14, 16,
22, 28, 48 and 58, so a new tier has to be picked from that set.

**Every length is authored against a 1920-wide screen and scaled by the measured viewport**
(`VIGRID_MAP_COMPASS_REFERENCE_W`). `SetPos`/`SetSize` take real pixels while the engine scales a
widget's *declared* geometry — including its font size — by viewport/1920 on its own. Scaling the
lanes by the same factor is what keeps the two in proportion; without it the strip stayed 42 px tall
at every resolution while the glyphs inside it grew, and at fullscreen the bearing readout collided
with the labels. The
heading comes from the **camera**, never the player's body yaw, for the reasons written up on
`VigridMapMinimap.DrawHeadingArrow`: body yaw snaps in discrete steps and does not return to its start
after a full turn. **The regression test is to spin a 360 and check the strip comes back to the same
reading.**

Three kinds of caret ride in the bottom lane, all read through the same APIs the map uses:

| Caret | Means |
|---|---|
| Wide blue bar | The next play-area zone, when a host mod has pushed one in |
| Full-height slot-coloured bar | A teammate |
| Half-height slot-coloured bar, 0.75 alpha | A party ping |

A teammate and a ping necessarily share their owner's slot colour, so **height and opacity are the only
things separating them** — the same two axes that separate the map's triangle from its diamond. Don't
collapse either without giving the ping a different silhouette.

Unlike the minimap, the strip is redrawn **every frame** rather than at 10 Hz: it slides continuously
under a fixed cursor, which is exactly where a 10 Hz update reads as stutter.

**Elements fade out over the last 8° of the window rather than being clipped.** Every widget is
positioned from script against the measured root size, in real screen pixels — see the header of
`GUI/layouts/compass.layout` for the measurement behind that, and for why a fixed-size container is the
wrong shape here.

## Settings

| Where | Keys |
|---|---|
| `$profile:Vigrid-Map\map_settings.json` (server) | `enabled`, `minimap_allowed`, `compass_allowed`, `label_max_length` |
| `$profile:Vigrid-Map\map_client.json` (client) | `minimap_enabled` (**N**) and `compass_enabled` (**K**) — the player's own toggles |

The minimap and the compass are each gated by three independent switches, each able only to opt further
out than the one before it:

1. **`VIGRID_MAP_MINIMAP`** / **`VIGRID_MAP_COMPASS`** — the build's. Comment the define out of
   `config.cpp` and that feature's class, its widgets and its keybind handler are gone from the PBO
   entirely; the fullscreen map is untouched either way.
2. **`minimap_allowed`** / **`compass_allowed`** — the admin's, pushed to clients over `VM_Settings`.
   Both ship **on**.
3. **`minimap_enabled`** / **`compass_enabled`** — the player's, persisted locally. The minimap ships
   **off**, so it is opt-in; the compass ships **on**, because it is a thin strip answering a question
   the HUD could not otherwise answer at all.

Two things deliberately survive a build with either define removed: the `*_allowed` field on the wire
and in the settings file, because a *client* build flag must not change the wire format; and the **N** /
**K** entries in `Data/Inputs.xml`, because XML cannot be conditional — they still list under
Options → Controls bound to nothing.

## Public API

The host mod talks to this addon **only** through `VigridMapAPI`, every call site wrapped in
`#ifdef VIGRID_MAP`:

```c
// client - push, not pull; the addon may not reach into the host mod
static void VigridMapAPI.SetZones(vector cur_center, float cur_radius, vector next_center, float next_radius)
static void VigridMapAPI.ClearZones()

// client - static circles marking regions of interest. Decoration only: this addon draws
// them and knows nothing else about them. Safe to call every frame, like SetZones - the pair
// is diffed internally and only walked when it actually changed.
static void VigridMapAPI.SetHotZones(array<vector> centers, array<float> radii)
static void VigridMapAPI.ClearHotZones()

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

**The round trip that costs is hidden by an optimistic prediction, and it covers all three
interactions.** `VigridMapClient` draws the result of a click immediately and retires the prediction
when the server's snapshot agrees. It originally covered only *placing on an empty map*, which meant
the first click of a session was instant and every one after it waited for the wire: a **move** kept
redrawing the old position, and a **right-click** kept drawing a marker the player had already deleted.
`m_PendingIntent` is what fixes that — the store keys one marker per owner, so a second click is a move
and the prediction must *override* the confirmed entry rather than sit beside it, and a removal must
*suppress* one. All the index arithmetic lives in `ResolveDrawIndex`; the three renderers read the
merged draw list and needed no changes.

**Retirement is content-based, and must stay that way.** The prediction is dropped when the set
actually contains what was asked for (a marker within a metre of the requested position; for a removal,
no marker at all) — not merely when *a* snapshot arrives. The old existence test was right only for a
first placement: on a move the player already owns a marker, at the old position, so an unrelated bump
such as the 5 s resync would retire the prediction and rubber-band the marker back.

**Every refusal answers.** `MapMissionServer.RejectRequest` sends `VM_Rejected` for all of them,
carrying an empty key when there is nothing worth telling the player (the place cooldown, a click off
the world edge). The signal is that an answer came at all — the client draws optimistically, so silence
reads as acceptance until the 2 s TTL, and then the marker jumps back unexplained. A **corrective
snapshot was tried first and cannot work**: a refusal does not bump `m_SetVersion`, so the push is
indistinguishable from the resync, and the content-based test above correctly reads it as "still
waiting". `VIGRID_MAP_CLICK_DEBOUNCE_MS` is also deliberately longer than the server's
`VIGRID_MAP_PLACE_COOLDOWN_MS`, so the client is the stricter gate.

**The repaint gate is split.** Zones and markers are edge-triggered; teammates have no edge — a party's
roster sequence moves when the party changes shape, never when somebody walks — so that layer repaints
on a 10 Hz clock. Every canvas must `Clear()` before any early return, or the last frame burns in.

**The marker edge is raised unconditionally from an incoming snapshot** (`VigridMapClient.TrackSnapshot`),
and that is load-bearing rather than tidy. It used to sit below `ResolvePending`'s early return for
"nothing pending", so a snapshot arriving with no request outstanding raised no edge and the fullscreen
map fell back on the **1 s repaint watchdog**. Two common cases landed there: a teammate's marker
appearing, and the confirmation of your own removal. That second was much larger than the round trip it
was mistaken for, and the fingerprint is that the minimap (10 Hz) and the world markers (every frame)
update well before the map does.

**Both maps draw "you" as the same dart.** The fullscreen map used an axis-aligned plus until
2026-08-11, on the argument that a rotating "you" is harder to *find* on a big map — true, and beside
the point, since the question a map gets opened for is which way you are facing. Findability is carried
by size instead: 16 px against the teammate triangle's 14, in white, which no party slot colour is. The
dart's angle is the **camera** bearing, never `GetYawPitchRoll()` — body yaw snaps in steps and does not
return to its start after a 360. The two call sites differ only in what they anchor to: the minimap
re-centres on the camera and passes it for both, while the fullscreen map takes the position from the
body and only the angle from the camera. With the `"aiming"` exclude group active, the fullscreen dart
holds the heading you had when you opened the map.

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
