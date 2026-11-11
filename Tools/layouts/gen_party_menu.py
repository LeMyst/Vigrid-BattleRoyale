"""Generates Party/GUI/layouts/party_menu.layout and its two row layouts (CRLF).
Lengths in 1920-wide reference units (the Claude Design board x1.5)."""
import _layoutgen

FOLDER = 'Party/GUI/layouts'

TEXT = "0.925 0.929 0.933 1"
TEXT2 = "0.788 0.804 0.82 1"
MUTED = "0.608 0.631 0.651 1"
DIM = "0.502 0.525 0.549 1"
ACCENT = "0.851 0.62 0.161 1"
INK = "0.067 0.078 0.09 1"
PARTY = "0.239 0.725 0.651 1"
PARTY_HI = "0.361 0.827 0.753 1"
DANGER_HI = "0.949 0.467 0.482 1"
WINDOW = "0.067 0.078 0.09 0.96"
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


def frame(name, geom, children, extra=None):
    props = ["ignorepointer 1"] + geom + (extra or [])
    return lambda d: node("FrameWidgetClass", name, props, children, d)


def rel_panel(name, x, y, w, h, color, hexact, vexact, halign="left_ref", valign="top_ref"):
    """A panel sized relative to its parent on the axes where `*exact` is 0."""
    return lambda d: node("PanelWidgetClass", name, [
        "ignorepointer 1", "inheritalpha 0", f"position {x} {y}", f"size {w} {h}", f"halign {halign}",
        f"valign {valign}", "hexactpos 1", "vexactpos 1", f"hexactsize {hexact}", f"vexactsize {vexact}",
        f"color {color}", "style rover_sim_colorable"], None, d)


def rel_text(name, color, font, size, value='""', halign="center"):
    """A label filling its parent, so script only ever resizes the parent."""
    return lambda d: node("TextWidgetClass", name, [
        "ignorepointer 1", "inheritalpha 0", "position 0 0", "size 1 1", "hexactpos 1", "vexactpos 1",
        "hexactsize 0", "vexactsize 0", f"color {color}", f"text {value}", f'font "gui/fonts/{font}"',
        '"exact text" 1', f'"exact text size" {size}', f'"text halign" {halign}', '"text valign" center'], None, d)


def rel_outline(prefix, t, color):
    """Four edges that follow the parent's size."""
    return [
        rel_panel(prefix + "Top", 0, 0, 1, t, color, 0, 1),
        rel_panel(prefix + "Bottom", 0, 0, 1, t, color, 0, 1, valign="bottom_ref"),
        rel_panel(prefix + "Left", 0, 0, t, 1, color, 1, 0),
        rel_panel(prefix + "Right", 0, 0, t, 1, color, 1, 0, halign="right_ref"),
    ]


def keycap(name, x, y, w, h, value='""'):
    return panel(name, exact(x, y, w, h), "1 1 1 0.07", children=[
        rel_panel(name + "Base", 0, 0, 1, 3, "1 1 1 0.18", 0, 1, valign="bottom_ref"),
        rel_text(name + "Text", TEXT2, "metron-bold16", 16, value)])


def button(name, geom, children, extra=None):
    props = geom + ["color 0 0 0 0", "style Empty", '"no focus" 0'] + (extra or [])
    return lambda d: node("ButtonWidgetClass", name, props, children, d)


def outline_button(name, x, y, w, h, edge, label_color, value, fill=None, extra=None):
    """A button whose edges and label follow its size, so script can fit it to its text."""
    kids = []
    if fill:
        kids.append(rel_panel(name + "Fill", 0, 0, 1, 1, fill, 0, 0))
    kids += rel_outline(name + "Edge", 2, edge)
    kids.append(rel_text(name + "Label", label_color, "metron-bold16", 16, value))
    return button(name, exact(x, y, w, h), kids, extra)


def scroll(name, rows_name, geom, row_h):
    spacer = lambda d: node("WrapSpacerWidgetClass", rows_name, [
        "ignorepointer 1", "position 0 0", f"size 1 {row_h}", "hexactpos 1", "vexactpos 1", "hexactsize 0",
        "vexactsize 1", '"Size To Content V" 1'], None, d)
    return lambda d: node("ScrollWidgetClass", name, geom + ['"Scrollbar V" 1'], [spacer], d)


DW, DH = 1440, 888
IW, IH = DW - 2, DH - 2
HEAD_H = 114
BODY_Y = HEAD_H + 2
FOOT_Y = IH - 102
BODY_H = FOOT_Y - 2 - BODY_Y
LEFT_W = 570
RIGHT_X = LEFT_W + 2
RW = IW - RIGHT_X
CARD_W = LEFT_W - 42 - 36
CARD_H, CARD_PITCH = 84, 93
ONLINE_X = 36
ONLINE_W = RW - 36 - 42
ROW_H = 72

header = [
    panel("TitleMark", exact(42, 33, 6, 48), PARTY),
    text("Title", exact(66, 24, 900, 39), TEXT, "metron-bold28", 28, value='"#STR_PARTY_MENU_TITLE"'),
    text("Subtitle", exact(66, 63, 1000, 27), MUTED, "metron16", 16),
    button("CloseButton", exact(IW - 30 - 210, 24, 210, 66), rel_outline("CloseEdge", 2, "1 1 1 0.1") + [
        keycap("CloseKey", 21, 18, 54, 30),
        text("CloseLabel", exact(90, 0, 105, 66), TEXT2, "metron-bold16", 16, value='"#STR_PARTY_CLOSE"')]),
    panel("HeaderLine", exact(0, HEAD_H, IW, 2), LINE),
]

BANNER_W, BANNER_H = IW - 84, 84
banner = panel("InviteBanner", exact(42, 140, BANNER_W, BANNER_H), "0.851 0.62 0.161 0.4", ["visible 0"], [
    panel("InviteBannerFill", exact(2, 2, BANNER_W - 4, BANNER_H - 4), "0.161 0.145 0.098 1"),
    text("InviteText", exact(24, 0, 860, BANNER_H), TEXT, "metron22", 22),
    text("InviteTimer", exact(BANNER_W - 345 - 120, 0, 105, BANNER_H), MUTED, "metron16", 16, "right"),
    outline_button("InviteDeclineButton", BANNER_W - 345, 15, 150, 54, "1 1 1 0.16", TEXT2, '"#STR_PARTY_DECLINE"'),
    outline_button("InviteAcceptButton", BANNER_W - 180, 15, 150, 54, ACCENT, INK, '"#STR_PARTY_ACCEPT"', fill=ACCENT),
])

search = panel("SearchBox", exact(RW - 42 - 360, 15, 360, 54), "1 1 1 0.1", children=[
    panel("SearchFill", exact(2, 2, 356, 50), "0.047 0.055 0.063 1"),
    text("SearchPlaceholder", exact(18, 0, 324, 54), DIM, "metron22", 22, value='"#STR_PARTY_SEARCH"'),
    lambda d: node("EditBoxWidgetClass", "SearchEdit", [
        "inheritalpha 0", "position 18 10", "size 324 34", "hexactpos 1", "vexactpos 1", "hexactsize 1",
        "vexactsize 1", f"color {TEXT}", '"limit visible" 1', '"exact text" 0', 'font "gui/fonts/metron22"'], None, d),
])

body = frame("Body", exact(0, BODY_Y, IW, BODY_H), [
    text("YourPartyLabel", exact(42, 30, CARD_W, 24), DIM, "metron-bold16", 16, value='"#STR_PARTY_YOUR_PARTY"'),
    scroll("PartyScroll", "MemberRows", exact(42, 66, CARD_W + 18, BODY_H - 66 - 24), CARD_PITCH),
    panel("ColumnLine", exact(LEFT_W, 0, 2, BODY_H), LINE),
    frame("OnlineColumn", exact(RIGHT_X, 0, RW, BODY_H), [
        text("OnlineLabel", exact(ONLINE_X, 30, 420, 24), DIM, "metron-bold16", 16),
        search,
        panel("OnlineTopLine", exact(ONLINE_X, 84, ONLINE_W, 2), LINE),
        scroll("OnlineScroll", "OnlineRows", exact(ONLINE_X, 86, ONLINE_W + 18, BODY_H - 86 - 24), ROW_H),
        text("OnlineEmpty", exact(ONLINE_X + 12, 96, ONLINE_W - 24, 60), DIM, "metron22", 22, extra=["visible 0"]),
    ]),
])

footer = frame("Footer", exact(0, FOOT_Y, IW, 102), [
    outline_button("CreateButton", 42, 21, 240, 60, "0.239 0.725 0.651 0.5", PARTY_HI, '"#STR_PARTY_CREATE"'),
    outline_button("LeaveButton", 42, 21, 150, 60, "1 1 1 0.16", TEXT2, '"#STR_PARTY_LEAVE"'),
    outline_button("DisbandButton", 204, 21, 180, 60, "0.898 0.282 0.302 0.45", DANGER_HI, '"#STR_PARTY_DISBAND"'),
    keycap("PingKey", 0, 36, 36, 30),
    text("PingHint", exact(0, 36, 200, 30), MUTED, "metron16", 16, value='"#STR_PARTY_HINT_PING"'),
    keycap("PingClearKey", 0, 36, 36, 30),
    text("PingClearHint", exact(0, 36, 200, 30), MUTED, "metron16", 16, value='"#STR_PARTY_HINT_PINGCLEAR"'),
])

dialog = panel("DialogEdge", exact(0, 0, DW, DH, ["halign center_ref", "valign center_ref"]), BORDER, children=[
    panel("Dialog", exact(1, 1, IW, IH), WINDOW, children=header + [
        banner, body, panel("FooterLine", exact(0, FOOT_Y - 2, IW, 2), LINE), footer])])

_layoutgen.emit("party_menu.layout", [
    "// The party menu (P). Generated by Tools/layouts/gen_party_menu.py - edit that, not this.",
    "// Lengths in 1920-wide reference units: declared geometry is scaled by viewport/1920.",
] + node("PanelWidgetClass", "PartyMenuRoot", [
    "inheritalpha 0", "position 0 0", "size 1 1", "hexactpos 0", "vexactpos 0", "hexactsize 0", "vexactsize 0",
    "color 0.024 0.031 0.039 0.62", "style rover_sim_colorable"], [dialog], 0), FOLDER)

# --- one card in "Your party": a member, an invite on its way, or an open slot
_layoutgen.emit("party_menu_member_row.layout", [
    "// One card in the party column: a member, a pending invite or an open slot. Mode, colours and",
    "// the badge and button positions are set by VigridPartyMenu. Generated - see Tools/layouts.",
] + node("PanelWidgetClass", "MemberRowRoot", ["visible 1", "ignorepointer 1", "inheritalpha 0"] +
        exact(0, 0, CARD_W, CARD_PITCH) + [f"color {CLEAR}", "style rover_sim_colorable"], [
    panel("MemberRowEdge", exact(0, 0, CARD_W, CARD_H), BORDER),
    panel("MemberRowFill", exact(2, 2, CARD_W - 4, CARD_H - 4), "0.09 0.106 0.118 1"),
    panel("MemberRowAvatar", exact(15, 15, 54, 54), "0.239 0.725 0.651 0.18", children=[
        rel_text("MemberRowInitial", PARTY_HI, "metron-bold22", 22)]),
    text("MemberRowName", exact(84, 12, CARD_W - 99, 36), TEXT, "metron-bold22", 22),
    panel("MemberRowBadge", exact(0, 18, 75, 27), "0.239 0.725 0.651 0.16", ["visible 0"], [
        rel_text("MemberRowBadgeText", PARTY_HI, "metron-bold16", 16, '"#STR_PARTY_LEADER_TAG"')]),
    text("MemberRowStatus", exact(84, 45, CARD_W - 99, 27), MUTED, "metron16", 16),
    text("MemberRowOpen", exact(84, 0, CARD_W - 99, CARD_H), DIM, "metron16", 16, value='"#STR_PARTY_OPEN_SLOT"',
         extra=["visible 0"]),
    outline_button("MemberRowPromoteButton", CARD_W - 15 - 120 - 9 - 120, 21, 120, 42, "1 1 1 0.16", TEXT2,
                   '"#STR_PARTY_PROMOTE"', extra=["visible 0"]),
    outline_button("MemberRowKickButton", CARD_W - 15 - 120, 21, 120, 42, "0.898 0.282 0.302 0.45", DANGER_HI,
                   '"#STR_PARTY_KICK"', extra=["visible 0"]),
], 0), FOLDER)

# --- one row in the online list
BTN_W = 150
_layoutgen.emit("party_menu_player_row.layout", [
    "// One connected player. Status, button and colours are set by VigridPartyMenu. Generated - see",
    "// Tools/layouts.",
] + node("PanelWidgetClass", "PlayerRowRoot", ["visible 1", "ignorepointer 1", "inheritalpha 0"] +
        exact(0, 0, ONLINE_W, ROW_H) + [f"color {CLEAR}", "style rover_sim_colorable"], [
    panel("PlayerRowLine", exact(0, ROW_H - 2, ONLINE_W, 2), "1 1 1 0.04"),
    panel("PlayerRowAvatar", exact(12, 15, 42, 42), "0.133 0.153 0.173 1", children=[
        rel_text("PlayerRowInitial", TEXT2, "metron-bold16", 16)]),
    text("PlayerRowName", exact(72, 0, ONLINE_W - 72 - BTN_W - 18 - 210, ROW_H), TEXT, "metron22", 22),
    text("PlayerRowStatus", exact(ONLINE_W - 6 - BTN_W - 18 - 210, 0, 210, ROW_H), MUTED, "metron16", 16, "right"),
    outline_button("PlayerRowInviteButton", ONLINE_W - 6 - BTN_W, 9, BTN_W, 54, "0.239 0.725 0.651 0.5", PARTY_HI,
                   '"#STR_PARTY_INVITE"'),
    text("PlayerRowSent", exact(ONLINE_W - 6 - BTN_W, 0, BTN_W, ROW_H), MUTED, "metron16", 16, "center",
         value='"#STR_PARTY_INVITE_SENT_STATUS"', extra=["visible 0"]),
], 0), FOLDER)
