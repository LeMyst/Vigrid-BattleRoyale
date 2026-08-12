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

//--- Menu id. Vanilla stops at 46 and the Battle Royale spawn selection uses 75.
static const int MENU_VIGRID_PARTY = 176;

//--- Keybind action name. Read with GetUApi().GetInputByName() rather than the generated
//--- UAVigridPartyMenu constant, so nothing here depends on a symbol produced by another PBO.
static const string VIGRID_PARTY_INPUT_MENU = "UAVigridPartyMenu";

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
