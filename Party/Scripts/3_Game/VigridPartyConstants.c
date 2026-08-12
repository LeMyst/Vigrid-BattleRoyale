/**
 *  Vigrid Party - shared constants. No guard: this file compiles on both client and server.
 *
 *  Everything Party needs to know about itself lives here. In particular VIGRID_PARTY_PREFIX is
 *  the single place the asset path appears; every layout path is built from it, so extracting
 *  this addon into a standalone mod is a one-line change here plus config.cpp.
 */

static const string VIGRID_PARTY_VERSION = "0.1.0";

//--- Asset prefix. The ONLY hard-coded path in the addon - build every layout path from it.
static const string VIGRID_PARTY_PREFIX = "Vigrid-BattleRoyale/Party/";

#ifdef DIAG
#define VIGRID_PARTY_TRACE_ENABLED
#endif

//--- Log verbosity. Overridden at runtime by -party-trace/-party-debug/-party-info/-party-warn/
//--- -party-none on the command line, or by the PartyLogLevel key in serverDZ.cfg.
#ifdef VIGRID_PARTY_TRACE_ENABLED
    static const int VIGRID_PARTY_LOG_LEVEL = 4; // Trace
#else
    static const int VIGRID_PARTY_LOG_LEVEL = 0; // Error only
#endif

//--- Settings + persistence. Party's own profile folder, deliberately NOT the Battle Royale one,
//--- so that extraction does not strand the data behind.
static const string VIGRID_PARTY_SETTINGS_FOLDER = "$profile:Vigrid-Party\\";
static const string VIGRID_PARTY_SETTINGS_FILE = "$profile:Vigrid-Party\\party_settings.json";
static const string VIGRID_PARTY_STORE_FILE = "$profile:Vigrid-Party\\parties.json";

//--- RPC namespaces (CF RPCManager, string-named).
static const string RPC_VIGRIDPARTY_NAMESPACE = "RPC-VigridParty";               // server -> client
static const string RPC_VIGRIDPARTY_SERVER_NAMESPACE = "RPC-VigridParty-Server"; // client -> server

//--- Server -> client message names.
static const string VP_RPC_SETTINGS = "VP_Settings";
static const string VP_RPC_LOCKED = "VP_Locked";
static const string VP_RPC_ROSTER = "VP_Roster";
static const string VP_RPC_TEAMSTATE = "VP_TeamState";
static const string VP_RPC_INVITE_RECEIVED = "VP_InviteReceived";
static const string VP_RPC_INVITE_CANCELLED = "VP_InviteCancelled";
static const string VP_RPC_PLAYERLIST = "VP_PlayerList";
static const string VP_RPC_NOTIFY = "VP_Notify";
static const string VP_RPC_PING_SETTINGS = "VP_PingSettings";
static const string VP_RPC_PING_SET = "VP_PingSet";

//--- Client -> server message names.
static const string VP_RPC_CREATE = "VP_Create";
static const string VP_RPC_INVITE = "VP_Invite";
static const string VP_RPC_INVITE_RESPOND = "VP_InviteRespond";
static const string VP_RPC_KICK = "VP_Kick";
static const string VP_RPC_LEAVE = "VP_Leave";
static const string VP_RPC_DISBAND = "VP_Disband";
static const string VP_RPC_TRANSFER_LEADER = "VP_TransferLeader";
static const string VP_RPC_REQUEST_PLAYERLIST = "VP_RequestPlayerList";
static const string VP_RPC_REQUEST_SYNC = "VP_RequestSync";
static const string VP_RPC_PING_ADD = "VP_PingAdd";
static const string VP_RPC_PING_CLEAR = "VP_PingClear";

//--- Menu id. Vanilla stops at 46 and the Battle Royale spawn selection uses 75.
static const int MENU_VIGRID_PARTY = 176;

//--- Keybind action name. Read with GetUApi().GetInputByName() rather than the generated
//--- UAVigridPartyMenu constant, so nothing here depends on a symbol produced by another PBO.
static const string VIGRID_PARTY_INPUT_MENU = "UAVigridPartyMenu";
static const string VIGRID_PARTY_INPUT_PING = "UAVigridPartyPing";
static const string VIGRID_PARTY_INPUT_PING_CLEAR = "UAVigridPartyPingClear";

//--- Sent in a roster or a notification in place of a name when a member is offline AND no name
//--- was ever recorded for them - an existing parties.json written before names were persisted.
//--- It travels as a stringtable key because the server has no client locale; every client-side
//--- consumer resolves it, which is centralised in VigridPartyAPI.GetMemberName.
static const string VIGRID_PARTY_UNKNOWN_NAME_KEY = "#STR_PARTY_UNKNOWN_MEMBER";

//--- Member state flags carried by VP_TeamState.
static const int VIGRID_PARTY_FLAG_ONLINE = 1;      // bit0
static const int VIGRID_PARTY_FLAG_ALIVE = 2;       // bit1
static const int VIGRID_PARTY_FLAG_UNCONSCIOUS = 4; // bit2 - populated but unstyled in v1

//--- Settings defaults. Mirrored by VigridPartyData; kept here so the client has sane values
//--- before the first VP_Settings lands.
static const bool VIGRID_PARTY_DEF_ENABLED = true;
static const int VIGRID_PARTY_DEF_MAX_SIZE = 4;
static const int VIGRID_PARTY_DEF_INVITE_TTL = 60;
static const int VIGRID_PARTY_DEF_PARTY_TTL_HOURS = 24;
static const int VIGRID_PARTY_DEF_STATE_INTERVAL_MS = 500;
static const float VIGRID_PARTY_DEF_NAMETAG_MAX_DIST = 0.0; // 0 = unlimited
static const float VIGRID_PARTY_DEF_NAMETAG_MIN_ALPHA = 0.35;

//--- Invite anti-spam.
static const int VIGRID_PARTY_MAX_PENDING_INVITES = 5;
static const int VIGRID_PARTY_INVITE_COOLDOWN_MS = 1000;
static const int VIGRID_PARTY_PLAYERLIST_COOLDOWN_MS = 1000;

//--- Persistence write debounce.
static const int VIGRID_PARTY_FLUSH_DEBOUNCE_MS = 5000;

//--- Nametag rendering.
static const float VIGRID_PARTY_TAG_EDGE_MARGIN = 48.0;    // px inset when clamped to the screen edge
static const float VIGRID_PARTY_TAG_FADE_DISTANCE = 400.0; // m, used when max distance is unlimited
static const float VIGRID_PARTY_TAG_HEIGHT_OFFSET = 1.8;   // m above a pushed position
static const int VIGRID_PARTY_STALE_HIDE_MS = 10000;

//--- Tag geometry. SetPos anchors a widget by its top-left corner, so the tag has to be shifted by
//--- its own size to end up centred above the head rather than hanging off to the right.
static const float VIGRID_PARTY_TAG_SIZE_W = 170.0;        // fallback if GetScreenSize returns 0
static const float VIGRID_PARTY_TAG_SIZE_H = 40.0;         // must track party_nametag.layout
static const float VIGRID_PARTY_TAG_GAP_PX = 6.0;          // gap between the head and the tag bottom
static const float VIGRID_PARTY_TAG_HEAD_OFFSET = 0.25;    // m above the Head bone

//--- Crosshair fade: a tag must not hide an enemy standing between the player and a teammate. The
//--- floor is deliberately non-zero so a distant teammate is not lost the instant you look at them.
static const float VIGRID_PARTY_TAG_CENTER_HIDE = 0.05;      // fraction of screen height, floor alpha
static const float VIGRID_PARTY_TAG_CENTER_FADE = 0.13;      // fraction of screen height, full alpha
static const float VIGRID_PARTY_TAG_CENTER_MIN_ALPHA = 0.15; // opacity right under the crosshair

//--- Ping defaults. Mirrored by VigridPartyData; only ping_enabled and ping_cooldown_ms are sent to
//--- the client. The cap is learned by watching a marker get evicted and the lifetime arrives per
//--- ping as milliseconds remaining, so neither needs to travel as a setting.
static const bool VIGRID_PARTY_DEF_PING_ENABLED = true;
static const int VIGRID_PARTY_DEF_PING_MAX = 3;            // per owner, FIFO - Carim's maxPings
static const int VIGRID_PARTY_DEF_PING_TTL_SECONDS = 30;   // 0 = permanent, which is what Carim did
static const int VIGRID_PARTY_DEF_PING_COOLDOWN_MS = 700;

//--- Placement. The ray matches Carim's reach; the bound is what the server checks a reported
//--- position against, and is deliberately looser than the ray so a legitimate long shot survives
//--- the round trip.
static const float VIGRID_PARTY_PING_RAY_LENGTH = 8000.0;
static const float VIGRID_PARTY_PING_MAX_PLACE_DIST = 9000.0;

//--- Ping rendering.
static const float VIGRID_PARTY_PING_HEIGHT_OFFSET = 0.2;  // m above the contact point
static const float VIGRID_PARTY_PING_EDGE_MARGIN = 40.0;   // px inset when clamped to the edge
static const float VIGRID_PARTY_PING_SIZE_W = 90.0;        // fallback if GetScreenSize returns 0
static const float VIGRID_PARTY_PING_SIZE_H = 36.0;        // must track party_ping.layout

//--- A marker is a hint, not a HUD element: it never draws at full opacity, and it carries no owner
//--- name - who placed it is read from the colour instead.
static const float VIGRID_PARTY_PING_BASE_ALPHA = 0.75;
static const float VIGRID_PARTY_PING_FADE_DISTANCE = 1000.0;
static const float VIGRID_PARTY_PING_MIN_ALPHA = 0.45;
static const float VIGRID_PARTY_PING_KM_THRESHOLD = 1000.0; // m, above which distance reads in km

//--- Crosshair fade, matching the name tags: a marker must not sit on top of whatever the player is
//--- aiming at. The floor is higher than the tags' because you place a ping by looking straight at
//--- it, so this is the alpha it settles to for the second or two right after placement.
//---
//--- The floor was 0.25 and that was too aggressive: it MULTIPLIES with BASE_ALPHA, so looking
//--- straight at a nearby ping gave 0.75 x 0.25 = 0.19 - nearly invisible at exactly the moment you
//--- most want to see it, which is the one reported in testing on 2026-08-08. At 0.55 the same case
//--- lands at 0.41, still clearly dimmed out of the way of the crosshair but actually readable.
//--- Keep this ABOVE the name tags' floor; a tag is a label you can ignore, a ping is a callout.
static const float VIGRID_PARTY_PING_CENTER_HIDE = 0.05;      // fraction of screen height
static const float VIGRID_PARTY_PING_CENTER_FADE = 0.13;      // fraction of screen height
static const float VIGRID_PARTY_PING_CENTER_MIN_ALPHA = 0.55;

//--- Ceiling on pooled marker widgets: the largest party times the largest legal cap.
static const int VIGRID_PARTY_PING_MAX_RENDERED = 160;

//--- Marker colours, one per party slot rather than Carim's own-vs-teammate split: with a single
//--- shade for every teammate you can see that somebody called something out but not who, which is
//--- the thing worth knowing when two people are marking at once. member_uids is join-ordered and
//--- never reshuffled (see VigridPartyAPI), so a member's slot - and therefore their colour - is
//--- stable for the life of the party.
//---
//--- Nothing is special-cased for the local player: your own markers use your own slot colour, so
//--- what you see is what your team sees.
//---
//--- The colours themselves live in VigridPartyPalette.ColourForSlot, in this same stage, built
//--- with ARGB() so the marker's opacity can be baked into them - an ImageWidget ignores its
//--- parent's alpha, and a CanvasWidget has no alpha to inherit at all.
static const int VIGRID_PARTY_PING_PALETTE_SIZE = 8;

//--- HUD panel geometry, in pixels. These MUST track party_hud_row.layout: VigridPartyHud.LayoutRow
//--- measures the text and repositions every cell from these numbers, so a size changed in the
//--- layout alone silently mis-places the row rather than failing.
//---
//--- A row is two lines - the name over the condition icons and the distance - and hugs its content
//--- horizontally, so the width below is only the fixed part. The name line no longer scales its
//--- glyphs to the box (`scaled 1`), which is what used to render a short name enormous.
static const int VIGRID_PARTY_HUD_ROW_HEIGHT = 38;   // one row, both lines
static const int VIGRID_PARTY_HUD_ROW_GAP = 3;       // vertical gap; pitch = height + gap
static const int VIGRID_PARTY_HUD_NAME_TOP = 2;      // y of the first line
static const int VIGRID_PARTY_HUD_NAME_HEIGHT = 18;  // first line
static const int VIGRID_PARTY_HUD_STAT_HEIGHT = 16;  // second line
static const int VIGRID_PARTY_HUD_STAT_TOP = 20;     // y of the second line
static const int VIGRID_PARTY_HUD_ACCENT_W = 3;      // slot-colour bar down the left edge
static const int VIGRID_PARTY_HUD_PAD_L = 8;         // x of the first cell, clear of the accent bar
static const int VIGRID_PARTY_HUD_PAD_R = 6;
static const int VIGRID_PARTY_HUD_ICON = 15;         // one condition badge, square
static const int VIGRID_PARTY_HUD_ICON_GAP = 4;      // between the two badges
static const int VIGRID_PARTY_HUD_STAT_GAP = 8;      // between the badges and the distance

//--- Backdrop opacity is NOT here: it lives on RowBackdrop in party_hud_row.layout and nothing in
//--- script touches it. Setting it from here would mean SetColor or SetAlpha on that widget, and
//--- either one overwrites the declared alpha - see the header comment on the layout.
