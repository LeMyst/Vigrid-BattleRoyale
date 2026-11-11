"""Generates GUI/layouts/spawn_selection.layout (CRLF). Lengths in 1920-wide reference units."""
import _layoutgen

out = "spawn_selection.layout"

TEXT = "0.925 0.929 0.933 1"
TEXT2 = "0.788 0.804 0.82 1"
MUTED = "0.608 0.631 0.651 1"
DIM = "0.502 0.525 0.549 1"
ACCENT = "0.851 0.62 0.161 1"
ACCENT_HI = "0.941 0.722 0.29 1"
PARTY = "0.239 0.725 0.651 1"
PARTY_HI = "0.361 0.827 0.753 1"
ZONE = "0.431 0.608 0.961 1"
HOT_FILL = "1 0.196 0.196 0.24"
HOT_LINE = "1 0.196 0.196 0.78"
HEAT = "0.69 0.486 0.941"
WINDOW = "0.067 0.078 0.09 0.94"
SURFACE = "0.082 0.094 0.106 1"
LINE = "1 1 1 0.08"


def exact(x, y, w, h, extra=None):
    return [f"position {x} {y}", f"size {w} {h}", "hexactpos 1", "vexactpos 1", "hexactsize 1", "vexactsize 1"] + (extra or [])


def node(cls, name, props, children=None, depth=0):
    pad = " " * depth
    lines = [f"{pad}{cls} {name} {{"]
    for p in props:
        lines.append(f"{pad} {p}")
    if children:
        lines.append(f"{pad} {{")
        for c in children:
            lines.extend(c(depth + 2))
        lines.append(f"{pad} }}")
    lines.append(f"{pad}}}")
    return lines


def panel(name, geom, color, extra=None, children=None, pointer=False):
    props = ["ignorepointer " + ("0" if pointer else "1"), "inheritalpha 0"] + geom + [
        f"color {color}", "style rover_sim_colorable"] + (extra or [])
    return lambda d: node("PanelWidgetClass", name, props, children, d)


def text(name, geom, color, font, size, halign="left", valign="center", value='""', extra=None):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [
        f"color {color}", f"text {value}", f'font "gui/fonts/{font}"', '"exact text" 1',
        f'"exact text size" {size}', f'"text halign" {halign}', f'"text valign" {valign}'] + (extra or [])
    return lambda d: node("TextWidgetClass", name, props, None, d)


def mtext(name, geom, color, font, size, value):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [
        f"color {color}", f"text {value}", f'font "gui/fonts/{font}"', '"exact text" 1', f'"exact text size" {size}',
        '"size to text h" 0', '"size to text v" 0', '"text halign" left', '"text valign" top', "wrap 1"]
    return lambda d: node("MultilineTextWidgetClass", name, props, None, d)


def frame(name, geom, children, extra=None):
    props = ["ignorepointer 1"] + geom + (extra or [])
    return lambda d: node("FrameWidgetClass", name, props, children, d)


def outline(prefix, w, h, t, color, extra=None):
    e = extra or []
    return [
        panel(prefix + "Top", exact(0, 0, w, t), color, e),
        panel(prefix + "Bottom", exact(0, h - t, w, t), color, e),
        panel(prefix + "Left", exact(0, 0, t, h), color, e),
        panel(prefix + "Right", exact(w - t, 0, t, h), color, e),
    ]


# --- timer: big number + label, centred together by script, and a draining bar
TW, TH = 600, 96
timer = panel("TimerPanel", exact(0, 36, TW, TH, ["halign center_ref", "priority 1200"]), LINE, children=[
    panel("TimerInner", exact(1, 1, TW - 2, TH - 2), WINDOW, children=[
        text("TimerNumber", exact(0, 8, 120, 72), ACCENT_HI, "metron-bold48", 48),
        text("TimerLabel", exact(0, 8, 560, 72), TEXT2, "metron22", 22),
        panel("TimerBarBack", exact(0, TH - 8, TW - 2, 6), LINE),
        panel("TimerBar", exact(0, TH - 8, TW - 2, 6), ACCENT),
    ])])

# --- side panel
P = 33
IW = 450 - 2 * P
legend_y = 372
step = 36


def swatch_row(i, name, swatch_children=None, color=None):
    y = legend_y + step * i
    kids = []
    if color:
        kids.append(panel(name + "Swatch", exact(P, y + 3, 21, 21), color))
    if swatch_children:
        kids.append(frame(name + "Swatch", exact(P, y + 3, 21, 21), swatch_children))
    kids.append(text(name + "Text", exact(P + 36, y, IW - 36, 27), TEXT2, "metron16", 16,
                     value=f'"#STR_BR_SPAWN_LEGEND_{name.upper()}"'))
    return kids


ramp_y = legend_y + step * 5
ramp = [text("LegendFew", exact(P, ramp_y, 66, 27), DIM, "metron16", 16, value='"#STR_BR_SPAWN_LEGEND_FEW"')]
for i, a in enumerate([0.14, 0.3, 0.5, 0.75]):
    ramp.append(panel(f"LegendRamp{i}", exact(P + 72 + 57 * i, ramp_y + 8, 57, 12), f"{HEAT} {a}"))
ramp.append(text("LegendMany", exact(P + 72 + 57 * 4 + 9, ramp_y, 90, 27), DIM, "metron16", 16,
                 value='"#STR_BR_SPAWN_LEGEND_MANY"'))


def key_row(i, key, action):
    y = P + 42 * (2 - i)
    bottom = ["valign bottom_ref"]
    cap = frame(f"KeyCap{i}", exact(P, y, 132, 33, bottom),
                outline(f"KeyCap{i}", 132, 33, 1, "1 1 1 0.16") +
                [panel(f"KeyCap{i}Base", exact(0, 30, 132, 3), "1 1 1 0.16"),
                 text(f"KeyCap{i}Text", exact(0, 0, 132, 31), TEXT2, "metron-bold16", 16, "center",
                      value=f'"#STR_BR_SPAWN_KEY_{key}"')])
    act = text(f"KeyAction{i}", exact(P + 147, y, IW - 147, 33, bottom), MUTED, "metron16", 16,
               value=f'"#STR_BR_SPAWN_ACT_{action}"')
    return [cap, act]


side_children = [
    text("SpawnTitle", exact(P, P, IW, 36), TEXT, "metron-bold28", 28, value='"#STR_BR_SPAWN_TITLE"'),
    mtext("SpawnHint", exact(P, 81, IW, 96), MUTED, "metron16", 16, '"#STR_BR_SPAWN_HINT"'),
    panel("PickCard", exact(P, 195, IW, 111), "1 1 1 0.06", children=[
        panel("PickCardInner", exact(1, 1, IW - 2, 109), SURFACE, children=[
            text("PickLabel", exact(20, 14, IW - 42, 24), DIM, "metron-bold16", 16, value='"#STR_BR_SPAWN_YOUR_PICK"'),
            text("PickName", exact(20, 40, IW - 42, 32), TEXT, "metron-bold22", 22, value='"#STR_BR_SPAWN_NO_PICK"'),
            text("PickCoords", exact(20, 74, IW - 42, 24), MUTED, "metron16", 16),
        ])]),
    text("LegendLabel", exact(P, 336, IW, 24), DIM, "metron-bold16", 16, value='"#STR_BR_SPAWN_LEGEND"'),
] + swatch_row(0, "You", color=ACCENT) \
  + swatch_row(1, "Party", color=PARTY) \
  + swatch_row(2, "Zone", swatch_children=outline("LegendZone", 21, 21, 3, ZONE)) \
  + swatch_row(3, "Hot", swatch_children=[panel("LegendHotFill", exact(0, 0, 21, 21), HOT_FILL)] + outline("LegendHot", 21, 21, 2, HOT_LINE)) \
  + [text("LegendPicksText", exact(P, legend_y + step * 4, IW, 27), TEXT2, "metron16", 16, value='"#STR_BR_SPAWN_LEGEND_PICKS"')] \
  + ramp \
  + key_row(0, "LMB", "PLACE") + key_row(1, "WHEEL", "ZOOM") + key_row(2, "DRAG", "PAN")

# Blocks the pointer, so a click on the panel never selects a spawn behind it.
side = panel("SidePanel", ["position 36 0.0333", "size 450 0.9333", "hexactpos 1", "vexactpos 0", "hexactsize 1",
                           "vexactsize 0", "priority 1200"], WINDOW, children=side_children, pointer=True)

# --- party member name tags, placed on the map by script
tags = []
for i in range(6):
    tags.append(panel(f"MateTag{i}", exact(0, 0, 160, 30), "0.067 0.078 0.09 0.85", ["visible 0", "priority 1100"], children=[
        text(f"MateTag{i}Text", exact(9, 0, 142, 30), PARTY_HI, "metron-bold16", 16)]))

map_widget = lambda d: node("MapWidgetClass", "SpawnMap", [
    "visible 1", "clipchildren 1", "inheritalpha 0", "position 0 0", "size 1 1", "hexactpos 0", "vexactpos 0",
    "hexactsize 0", "vexactsize 0", 'scriptclass ""'], [
    lambda dd: node("CanvasWidgetClass", "CanvasSpawnMap", [
        "visible 1", "ignorepointer 1", "position 0 0", "size 1 1", "halign center_ref", "valign center_ref",
        "hexactpos 0", "vexactpos 0", "hexactsize 0", "vexactsize 0", "priority 1000"], None, dd),
    lambda dd: node("CanvasWidgetClass", "CanvasHeatmap", [
        "visible 1", "ignorepointer 1", "position 0 0", "size 1 1", "halign center_ref", "valign center_ref",
        "hexactpos 0", "vexactpos 0", "hexactsize 0", "vexactsize 0", "priority 999"], None, dd),
] + tags + [timer, side], d)

root = node("FrameWidgetClass", "ContainerWidget", [
    "clipchildren 0", "position 0 0", "size 1 1", "hexactpos 0", "vexactpos 0", "hexactsize 0", "vexactsize 0",
    "priority 50"], [map_widget], 0)

header = [
    "// Spawn selection. Generated layout, driven by SpawnSelectionMenu.",
    "// Lengths in 1920-wide reference units: declared geometry is scaled by viewport/1920.",
    "// Everything drawn over the map is DECLARED as a child of SpawnMap: nothing else renders over a",
    "// MapWidget (Docs/map.md).",
]
_layoutgen.emit(out, header + root)
