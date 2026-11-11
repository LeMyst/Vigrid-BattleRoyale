"""Generates GUI/layouts/death_screen.layout (CRLF). Lengths in 1920-wide reference units."""
import _layoutgen

out = "death_screen.layout"

TEXT = "0.925 0.929 0.933 1"
TEXT2 = "0.788 0.804 0.82 1"
MUTED = "0.608 0.631 0.651 1"
DANGER = "0.898 0.282 0.302 1"
DANGER_HI = "0.949 0.467 0.482 1"
ACCENT = "0.851 0.62 0.161 1"
WINDOW = "0.067 0.078 0.09 0.9"
INK = "0.067 0.078 0.09 1"


def exact(x, y, w, h):
    return [f"position {x} {y}", f"size {w} {h}", "hexactpos 1", "vexactpos 1", "hexactsize 1", "vexactsize 1"]


def rel(x, y, w, h):
    return [f"position {x} {y}", f"size {w} {h}", "hexactpos 0", "vexactpos 0", "hexactsize 0", "vexactsize 0"]


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


def panel(name, geom, color, extra=None, children=None):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [f"color {color}", "style rover_sim_colorable"] + (extra or [])
    return lambda d: node("PanelWidgetClass", name, props, children, d)


def text(name, geom, color, font, size, halign="left", valign="center", value='""', extra=None):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [
        f"color {color}", f"text {value}", f'font "gui/fonts/{font}"', '"exact text" 1',
        f'"exact text size" {size}', f'"text halign" {halign}', f'"text valign" {valign}'] + (extra or [])
    return lambda d: node("TextWidgetClass", name, props, None, d)


def mtext(name, geom, color, font, size):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [
        f"color {color}", 'text ""', f'font "gui/fonts/{font}"', '"exact text" 1', f'"exact text size" {size}',
        '"size to text h" 0', '"size to text v" 0', '"text halign" left', '"text valign" top', "wrap 1"]
    return lambda d: node("MultilineTextWidgetClass", name, props, None, d)


def frame(name, geom, children, extra=None):
    props = ["ignorepointer 1"] + geom + (extra or [])
    return lambda d: node("FrameWidgetClass", name, props, children, d)


def image(name, geom, img, color):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [
        f"color {color}", f'image0 "{img}"', "mode blend", '"src alpha" 1', '"no wrap" 1', "stretch 1"]
    return lambda d: node("ImageWidgetClass", name, props, None, d)


def outline_button(name, label_key, x):
    w, h, t = 240, 78, 2
    kids = [
        panel(name + "Bg", exact(0, 0, w, h), "1 1 1 0"),
        panel(name + "Top", exact(0, 0, w, t), "1 1 1 0.2"),
        panel(name + "Bottom", exact(0, h - t, w, t), "1 1 1 0.2"),
        panel(name + "Left", exact(0, 0, t, h), "1 1 1 0.2"),
        panel(name + "Right", exact(w - t, 0, t, h), "1 1 1 0.2"),
        text(name + "Label", exact(0, 0, w, h), TEXT2, "metron-bold22", 22, "center", value=f'"#{label_key}"'),
    ]
    props = exact(x, 0, w, h) + ["color 0 0 0 0", "style Empty", '"no focus" 0']
    return lambda d: node("ButtonWidgetClass", name, props, kids, d)


def spectate_button():
    w, h = 525, 78
    kids = [
        panel("SpectateBg", exact(0, 0, w, h), ACCENT),
        text("SpectateLabel", exact(0, 0, w, h), INK, "metron-bold22", 22, "center", value='"#STR_BR_DEAD_SPECTATE"'),
        panel("SpectateProgress", exact(0, h - 6, w, 6), "0.067 0.078 0.09 0.45"),
    ]
    props = exact(0, 0, w, h) + ["color 0 0 0 0", "style Empty", '"no focus" 0']
    return lambda d: node("ButtonWidgetClass", "SpectateButton", props, kids, d)


def stat_cell(i, x, w, label_key):
    kids = [
        text(f"StatLabel{i}", exact(24, 18, w - 48, 24), MUTED, "metron16", 16, value=f'"#{label_key}"'),
        text(f"StatValue{i}", exact(24, 44, w - 48, 50), TEXT, "metron-bold48", 48, value='"-"'),
    ]
    return panel(f"StatCell{i}", exact(x, 1, w, 102), WINDOW, children=kids)


def chip(name):
    kids = [text(name + "Text", exact(0, 0, 120, 33), TEXT2, "metron16", 16, "center")]
    return panel(name, exact(0, 0, 120, 33), "1 1 1 0.06", extra=["visible 0"], children=kids)


card_inner = [
    panel("KillerIconBox", exact(29, 26, 60, 60), "0.898 0.282 0.302 0.16", children=[
        image("KillerIcon", exact(15, 15, 30, 30), "set:dayz_crosshairs image:cross_32x32", DANGER_HI)]),
    text("KillerLabel", exact(107, 28, 640, 24), MUTED, "metron16", 16),
    text("KillerName", exact(107, 52, 640, 36), TEXT, "metron-bold28", 28),
    chip("WeaponChip"),
    chip("DistanceChip"),
    text("DetailLeft", exact(29, 107, 360, 27), MUTED, "metron16", 16),
    text("DetailRight", exact(389, 107, 360, 27), MUTED, "metron16", 16, "right"),
    panel("BarBack", exact(29, 143, 720, 12), "1 1 1 0.06"),
    panel("BarHealth", exact(29, 143, 0, 12), TEXT2),
    panel("BarDealt", exact(29, 143, 0, 12), "0.898 0.282 0.302 0.6"),
]

column = [
    panel("EyebrowLine", exact(0, 12, 36, 3), DANGER),
    text("EyebrowText", exact(51, 0, 700, 27), DANGER_HI, "metron-bold16", 16, value='"#STR_BR_DEAD_ELIMINATED"'),
    text("PlacementText", exact(0, 33, 400, 162), TEXT, "sdf_metronbook72", 180),
    text("FieldText", exact(0, 126, 500, 54), MUTED, "metron48", 48),
    mtext("FlavourText", exact(0, 204, 780, 60), MUTED, "metron-italic22", 22),
    panel("KillerCard", exact(0, 306, 780, 183), "1 1 1 0.08", extra=["visible 0"], children=[
        panel("KillerCardInner", exact(1, 1, 778, 181), WINDOW, children=card_inner)]),
    panel("StatsGrid", exact(0, 531, 780, 104), "1 1 1 0.06", children=[
        stat_cell(0, 1, 258, "STR_BR_LASTMATCH_KILLS"),
        stat_cell(1, 260, 258, "STR_BR_LASTMATCH_DAMAGE"),
        stat_cell(2, 519, 260, "STR_BR_LASTMATCH_TIME")]),
    frame("ButtonRow", exact(0, 677, 780, 78), [
        spectate_button(),
        outline_button("AdminButton", "STR_BR_DEAD_ADMIN", 285),
        outline_button("QuitButton", "STR_BR_DEAD_QUIT", 540)]),
    text("StatusText", exact(0, 770, 780, 30), MUTED, "metron22", 22),
]

# The design's left gradient (0.92 -> 0.75 at 70% -> 0), approximated by a solid panel and sixteen
# fading strips: a layout has no gradient fill.
side = [panel("SidePanel", rel(0, 0, 0.4, 1), "0.031 0.035 0.043 0.85")]
N = 16
for i in range(N):
    a = round(0.85 * (N - i) / (N + 1), 3)
    side.append(panel(f"SideFade{i}", rel(round(0.4 + 0.015 * i, 3), 0, 0.015, 1), f"0.031 0.035 0.043 {a}"))

root_props = ["keepsafezone 0", "position -0.02 -0.02", "size 1.04 1.04", "hexactpos 0", "vexactpos 0",
              "hexactsize 0", "vexactsize 0", "priority 900", "color 0.078 0.024 0.031 0.7",
              "style rover_sim_colorable"]
root = node("PanelWidgetClass", "DeathRoot", root_props,
            side + [frame("Column", exact(180, 0, 780, 800), column, ["valign center_ref"])], 0)

header = [
    "// The death screen. Generated layout, painted by DeathScreenMenu.",
    "// Lengths in 1920-wide reference units: declared geometry is scaled by viewport/1920. Column",
    "// children below the killer card are re-placed from script when the card shrinks.",
]
_layoutgen.emit(out, header + root)
