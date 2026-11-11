"""Generates leaderboard.layout, leaderboard_row.layout and match_summary_row.layout (CRLF).
Lengths in 1920-wide reference units (the Claude Design board x1.5)."""
import _layoutgen


TEXT = "0.925 0.929 0.933 1"
TEXT2 = "0.788 0.804 0.82 1"
MUTED = "0.608 0.631 0.651 1"
DIM = "0.502 0.525 0.549 1"
ACCENT = "0.851 0.62 0.161 1"
ACCENT_HI = "0.941 0.722 0.29 1"
PARTY = "0.239 0.725 0.651 1"
DANGER_HI = "0.949 0.467 0.482 1"
WINDOW = "0.067 0.078 0.09 0.96"
SURFACE = "0.082 0.094 0.106 1"
LINE = "1 1 1 0.06"
BORDER = "1 1 1 0.08"
CLEAR = "1 1 1 0"


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


def panel(name, geom, color, extra=None, children=None):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [f"color {color}", "style rover_sim_colorable"] + (extra or [])
    return lambda d: node("PanelWidgetClass", name, props, children, d)


def text(name, geom, color, font, size, halign="left", valign="center", value='""', extra=None):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [
        f"color {color}", f"text {value}", f'font "gui/fonts/{font}"', '"exact text" 1',
        f'"exact text size" {size}', f'"text halign" {halign}', f'"text valign" {valign}'] + (extra or [])
    return lambda d: node("TextWidgetClass", name, props, None, d)


def mtext(name, geom, color, font, size, value='""'):
    props = ["ignorepointer 1", "inheritalpha 0"] + geom + [
        f"color {color}", f"text {value}", f'font "gui/fonts/{font}"', '"exact text" 1', f'"exact text size" {size}',
        '"size to text h" 0', '"size to text v" 0', '"text halign" left', '"text valign" top', "wrap 1"]
    return lambda d: node("MultilineTextWidgetClass", name, props, None, d)


def frame(name, geom, children, extra=None):
    props = ["ignorepointer 1"] + geom + (extra or [])
    return lambda d: node("FrameWidgetClass", name, props, children, d)


def keycap(name, x, y, w, h, value, font="metron-bold16", size=16):
    """Fill plus a heavier bottom edge. Both children are relative, so script only resizes the cap."""
    kids = [
        lambda d: node("PanelWidgetClass", name + "Base", [
            "ignorepointer 1", "inheritalpha 0", "position 0 0", "size 1 3", "halign left_ref", "valign bottom_ref",
            "hexactpos 1", "vexactpos 1", "hexactsize 0", "vexactsize 1", "color 1 1 1 0.18",
            "style rover_sim_colorable"], None, d),
        lambda d: node("TextWidgetClass", name + "Text", [
            "ignorepointer 1", "inheritalpha 0", "position 0 0", "size 1 1", "hexactpos 1", "vexactpos 1",
            "hexactsize 0", "vexactsize 0", f"color {TEXT2}", f"text {value}", f'font "gui/fonts/{font}"',
            '"exact text" 1', f'"exact text size" {size}', '"text halign" center', '"text valign" center'], None, d),
    ]
    return panel(name, exact(x, y, w, h), "1 1 1 0.07", children=kids)


def outline(prefix, w, h, t, color):
    return [
        panel(prefix + "Top", exact(0, 0, w, t), color),
        panel(prefix + "Bottom", exact(0, h - t, w, t), color),
        panel(prefix + "Left", exact(0, 0, t, h), color),
        panel(prefix + "Right", exact(w - t, 0, t, h), color),
    ]


def button(name, geom, children):
    props = geom + ["color 0 0 0 0", "style Empty", '"no focus" 0']
    return lambda d: node("ButtonWidgetClass", name, props, children, d)


def scroll(name, rows_name, geom, row_h):
    spacer = lambda d: node("WrapSpacerWidgetClass", rows_name, [
        "ignorepointer 1", "position 0 0", f"size 1 {row_h}", "hexactpos 1", "vexactpos 1", "hexactsize 0",
        "vexactsize 1", '"Size To Content V" 1'], None, d)
    return lambda d: node("ScrollWidgetClass", name, geom + ['"Scrollbar V" 1'], [spacer], d)


def write(name, header, root_lines):
    _layoutgen.emit(name, header + root_lines)


DW, DH = 1350, 888          # the dialog, border included
IW, IH = DW - 2, DH - 2     # inside the border
HEAD_H, TABS_Y, TABS_H = 114, 116, 72
BODY_Y = TABS_Y + TABS_H + 2
BODY_H = IH - BODY_Y

# --- ladder columns: rank | player | matches | wins | kills | points, 42 in from each side
LCOL = [("Rank", 42, 96, "left"), ("Player", 138, 618, "left"), ("Matches", 756, 144, "right"),
        ("Wins", 900, 120, "right"), ("Kills", 1020, 120, "right"), ("Points", 1140, 168, "right")]
ROW_H = 60
FOOT_H = 72

# --- last-match standings, right of a 480 aside
ASIDE_W = 480
RIGHT_X = ASIDE_W + 2
RW = IW - RIGHT_X
MCOL = [("Place", 42, 96, "left"), ("Player", 138, 314, "left"), ("Kills", 452, 108, "right"),
        ("Damage", 560, 132, "right"), ("Survived", 692, 132, "right")]
MROW_H = 57
STRIP_H = 72


def tab(name, key, x, w):
    return button(name + "Button", exact(x, 0, w, TABS_H), [
        text(name + "Label", exact(0, 0, w, TABS_H - 3), MUTED, "metron-bold22", 22, "center", value=f'"#{key}"'),
        panel(name + "Accent", exact(0, TABS_H - 3, w, 3), ACCENT, ["visible 0"]),
    ])


header = [
    panel("TitleMark", exact(42, 33, 6, 48), ACCENT),
    text("Title", exact(66, 24, 900, 39), TEXT, "metron-bold28", 28, value='"#STR_BR_LEADERBOARD_TITLE"'),
    text("SeasonText", exact(66, 63, 1000, 27), MUTED, "metron16", 16),
    button("CloseButton", exact(IW - 30 - 210, 24, 210, 66),
           outline("CloseEdge", 210, 66, 2, "1 1 1 0.1") + [
               keycap("CloseKey", 21, 18, 54, 30, '"F4"'),
               text("CloseLabel", exact(90, 0, 105, 66), TEXT2, "metron-bold16", 16, value='"#STR_BR_LEADERBOARD_CLOSE"')]),
    panel("HeaderLine", exact(0, HEAD_H, IW, 2), LINE),
    frame("TabBar", exact(0, TABS_Y, IW, TABS_H), [
        tab("SoloTab", "STR_BR_LEADERBOARD_SOLO", 30, 150),
        tab("GroupTab", "STR_BR_LEADERBOARD_GROUP", 186, 150),
        tab("LastMatchTab", "STR_BR_LASTMATCH_TAB", 342, 270),
        keycap("TabKeyQ", 0, 21, 36, 30, '"Q"'),
        keycap("TabKeyE", 0, 21, 36, 30, '"E"'),
        text("TabHint", exact(0, 21, 300, 30), DIM, "metron16", 16, value='"#STR_BR_LB_SWITCH_TAB"'),
    ]),
    panel("TabLine", exact(0, TABS_Y + TABS_H, IW, 2), LINE),
]


def col_headers(prefix, cols, keyfmt, y, highlight=None):
    out = []
    for key, x, w, align in cols:
        color = TEXT2 if key == highlight else DIM
        out.append(text(prefix + key, exact(x, y, w, 54), color, "metron-bold16", 16, align,
                        value=f'"#{keyfmt.format(key.upper())}"'))
    return out


def foot_cell(name, i, font, size, color):
    key, x, w, align = LCOL[i]
    return text(name, exact(x, 0, w, FOOT_H), color, font, size, align)


ladder = frame("LadderPanel", exact(0, BODY_Y, IW, BODY_H), col_headers("Header", LCOL, "STR_BR_LEADERBOARD_{}", 0, "Points") + [
    panel("LadderHeaderLine", exact(0, 54, IW, 2), LINE),
    scroll("LeaderboardScroll", "LeaderboardRows", exact(0, 56, IW, BODY_H - 56 - FOOT_H), ROW_H),
    text("EmptyText", exact(0, 56, IW, BODY_H - 56 - FOOT_H), MUTED, "metron22", 22, "center",
         value='"#STR_BR_LEADERBOARD_EMPTY"', extra=["visible 0"]),
    panel("SelfBanner", exact(0, BODY_H - FOOT_H, IW, FOOT_H), "0.851 0.62 0.161 0.12", children=[
        panel("SelfBannerLine", exact(0, 0, IW, 2), "0.851 0.62 0.161 0.4"),
        foot_cell("SelfRank", 0, "metron-bold28", 28, ACCENT_HI),
        foot_cell("SelfName", 1, "metron-bold22", 22, TEXT),
        panel("SelfBadge", exact(0, 22, 60, 28), "0.851 0.62 0.161 0.22", children=[
            text("SelfBadgeText", exact(0, 0, 60, 28), ACCENT_HI, "metron-bold16", 16, "center", value='"#STR_BR_LB_YOU"')]),
        foot_cell("SelfMatches", 2, "metron22", 22, TEXT2),
        foot_cell("SelfWins", 3, "metron22", 22, TEXT),
        foot_cell("SelfKills", 4, "metron22", 22, TEXT),
        foot_cell("SelfPoints", 5, "metron-bold28", 28, ACCENT_HI),
        text("SelfText", exact(42, 0, IW - 84, FOOT_H), TEXT2, "metron22", 22),
    ]),
])

# --- last match: aside
AX, AWI = 42, ASIDE_W - 42 - 36
CELL_W = (AWI - 3) // 2


def stat_cell(name, label_key, x, y, w, h):
    return panel(name + "Cell", exact(x, y, w, h), SURFACE, children=[
        text(name + "Label", exact(21, 15, w - 42, 24), MUTED, "metron16", 16, value=f'"#{label_key}"'),
        text(name + "Value", exact(21, 42, w - 42, 42), TEXT, "metron-bold28", 28, value='"-"'),
    ])


def chip(name):
    return panel(name, exact(0, 0, 120, 33), "1 1 1 0.07", ["visible 0"], [
        text(name + "Text", exact(0, 0, 120, 33), TEXT2, "metron16", 16, "center")])


RECAP_Y, RECAP_H = 417, 237
recap_inner = [
    text("RecapHeader", exact(21, 18, AWI - 44, 24), DANGER_HI, "metron-bold16", 16, value='"#STR_BR_LASTMATCH_RECAP"'),
    text("RecapLine", exact(21, 48, AWI - 44, 33), TEXT, "metron-bold22", 22),
    chip("WeaponChip"),
    chip("DistanceChip"),
    mtext("RecapDetail", exact(21, 135, AWI - 44, 90), MUTED, "metron16", 16),
]

aside = [
    frame("CardSelf", exact(0, 0, ASIDE_W, BODY_H), [
        text("PlaceLabel", exact(AX, 36, AWI, 24), DIM, "metron-bold16", 16, value='"#STR_BR_LASTMATCH_PLACEMENT"'),
        text("PlaceNumber", exact(AX, 60, 200, 102), ACCENT_HI, "sdf_metronbook72", 96),
        text("PlaceOf", exact(AX + 100, 108, AWI - 100, 42), MUTED, "metron28", 28),
        panel("CardStats", exact(AX, 189, AWI, 199), LINE, children=[
            stat_cell("CardKills", "STR_BR_LASTMATCH_KILLS", 1, 1, CELL_W, 98),
            stat_cell("CardDamage", "STR_BR_LASTMATCH_DAMAGE", CELL_W + 2, 1, AWI - CELL_W - 3, 98),
            stat_cell("CardHits", "STR_BR_LASTMATCH_HITS", 1, 100, CELL_W, 98),
            stat_cell("CardTime", "STR_BR_LASTMATCH_TIME", CELL_W + 2, 100, AWI - CELL_W - 3, 98),
        ]),
        panel("CardRecap", exact(AX, RECAP_Y, AWI, RECAP_H), "0.898 0.282 0.302 0.28", children=[
            panel("CardRecapInner", exact(1, 1, AWI - 2, RECAP_H - 2), "0.133 0.094 0.106 1", children=recap_inner)]),
    ]),
    mtext("AsideNote", exact(AX, 36, AWI, 150), MUTED, "metron22", 22, '"#STR_BR_LASTMATCH_NOT_PLAYED"'),
    panel("AsideLine", exact(ASIDE_W, 0, 2, BODY_H), LINE),
]

strip = frame("CardSquad", exact(RIGHT_X, 0, RW, STRIP_H), [
    text("SquadHeader", exact(42, 0, 300, STRIP_H), DIM, "metron-bold16", 16, value='"#STR_BR_LASTMATCH_YOUR_SQUAD"'),
    text("SquadKillsLabel", exact(0, 0, 200, STRIP_H), MUTED, "metron16", 16, value='"#STR_BR_LASTMATCH_SQUAD_KILLS"'),
    text("CardSquadKillsValue", exact(0, 0, 80, STRIP_H), TEXT, "metron-bold16", 16),
    text("SquadDamageLabel", exact(0, 0, 200, STRIP_H), MUTED, "metron16", 16, value='"#STR_BR_LASTMATCH_SQUAD_DAMAGE"'),
    text("CardSquadDamageValue", exact(0, 0, 80, STRIP_H), TEXT, "metron-bold16", 16),
    text("SquadBestLabel", exact(0, 0, 200, STRIP_H), MUTED, "metron16", 16, value='"#STR_BR_LASTMATCH_SQUAD_BEST"'),
    text("CardSquadBestValue", exact(0, 0, 80, STRIP_H), TEXT, "metron-bold16", 16),
    panel("SquadLine", exact(0, STRIP_H, RW, 2), LINE),
])

standings = frame("Standings", exact(RIGHT_X, STRIP_H + 2, RW, BODY_H - STRIP_H - 2),
                  col_headers("LastMatchHeader", MCOL, "STR_BR_LASTMATCH_COL_{}", 0) + [
    panel("StandingsHeaderLine", exact(0, 54, RW, 2), LINE),
    scroll("LastMatchScroll", "LastMatchRows", exact(0, 56, RW, BODY_H - STRIP_H - 2 - 56), MROW_H),
    text("LastMatchEmpty", exact(0, 56, RW, 200), MUTED, "metron22", 22, "center",
         value='"#STR_BR_LASTMATCH_EMPTY"', extra=["visible 0"]),
])

lastmatch = frame("LastMatchPanel", exact(0, BODY_Y, IW, BODY_H), aside + [strip, standings], ["visible 0"])

# --- the hint row under the dialog, laid out by script
hints = frame("Hints", exact(0, DH + 18, DW, 33), [
    panel("HintSquadSwatch", exact(0, 10, 12, 12), PARTY, ["visible 0"]),
    text("HintSquadText", exact(0, 0, 300, 33), MUTED, "metron16", 16, value='"#STR_BR_SPAWN_LEGEND_PARTY"', extra=["visible 0"]),
    keycap("HintWheelKey", 0, 0, 90, 33, '"#STR_BR_SPAWN_KEY_WHEEL"'),
    text("HintWheelText", exact(0, 0, 200, 33), MUTED, "metron16", 16, value='"#STR_BR_LB_SCROLL"'),
    keycap("HintF4Key", 0, 0, 54, 33, '"F4"'),
    keycap("HintEscKey", 0, 0, 60, 33, '"Esc"'),
    text("HintCloseText", exact(0, 0, 200, 33), MUTED, "metron16", 16, value='"#STR_BR_LEADERBOARD_CLOSE"'),
])

dialog = panel("DialogEdge", exact(0, 0, DW, DH), BORDER, children=[
    panel("Dialog", exact(1, 1, IW, IH), WINDOW, children=header + [ladder, lastmatch])])

shell = frame("Shell", exact(0, 0, DW, DH + 51, ["halign center_ref", "valign center_ref"]), [dialog, hints])

root_node = node("PanelWidgetClass", "LeaderboardRoot", [
    "inheritalpha 0", "position 0 0", "size 1 1", "hexactpos 0", "vexactpos 0", "hexactsize 0", "vexactsize 0",
    "color 0.024 0.031 0.039 0.62", "style rover_sim_colorable"], [shell], 0)

write("leaderboard.layout", [
    "// The leaderboard (F4). Generated layout, driven by LeaderboardMenu.",
    "// Lengths in 1920-wide reference units: declared geometry is scaled by viewport/1920.",
    "// Grouping nodes are FrameWidgets with ignorepointer 1: a full-size panel with the default",
    "// ignorepointer 0 eats every click on the tabs declared before it (Docs/ui.md).",
], root_node)


# --- one ladder row
def row_cell(name, i, font, size, color):
    key, x, w, align = LCOL[i]
    return text(name, exact(x, 0, w, ROW_H), color, font, size, align)


write("leaderboard_row.layout", [
    "// One ladder row. Colours (stripe, own row, medals) and the badge position are set by",
    "// LeaderboardMenu; the row itself is laid out by the WrapSpacer and never positioned.",
], node("PanelWidgetClass", "LeaderboardRowRoot", ["visible 1", "ignorepointer 1", "inheritalpha 0"] + exact(0, 0, IW, ROW_H) + [
    f"color {CLEAR}", "style rover_sim_colorable"], [
    row_cell("RowRank", 0, "metron-bold28", 28, DIM),
    row_cell("RowName", 1, "metron22", 22, TEXT),
    panel("RowBadge", exact(0, 16, 60, 28), "0.851 0.62 0.161 0.18", ["visible 0"], [
        text("RowBadgeText", exact(0, 0, 60, 28), ACCENT_HI, "metron-bold16", 16, "center", value='"#STR_BR_LB_YOU"')]),
    row_cell("RowMatches", 2, "metron22", 22, MUTED),
    row_cell("RowWins", 3, "metron22", 22, TEXT),
    row_cell("RowKills", 4, "metron22", 22, TEXT),
    row_cell("RowPoints", 5, "metron-bold22", 22, TEXT),
], 0))


# --- one last-match row
def mrow_cell(name, i, font, size, color, dx=0):
    key, x, w, align = MCOL[i]
    return text(name, exact(x + dx, 0, w - dx, MROW_H), color, font, size, align)


write("match_summary_row.layout", [
    "// One row of the Last Match standings. Colours and the squad mark are set by LeaderboardMenu.",
], node("PanelWidgetClass", "SummaryRowRoot", ["visible 1", "ignorepointer 1", "inheritalpha 0"] + exact(0, 0, RW, MROW_H) + [
    f"color {CLEAR}", "style rover_sim_colorable"], [
    mrow_cell("RowPlace", 0, "metron-bold22", 22, DIM),
    panel("RowSquad", exact(MCOL[1][1], 22, 12, 12), PARTY, ["visible 0"]),
    mrow_cell("RowName", 1, "metron22", 22, TEXT, 24),
    mrow_cell("RowKills", 2, "metron22", 22, TEXT),
    mrow_cell("RowDamage", 3, "metron22", 22, TEXT2),
    mrow_cell("RowSurvived", 4, "metron22", 22, MUTED),
], 0))
