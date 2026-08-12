/**
 *  KillFeed - shared constants. No guard: this file compiles on both client and server.
 *
 *  Everything the addon needs to know about itself lives here. In particular KILLFEED_PREFIX is
 *  the single place the asset path appears; every layout path is built from it, so extracting this
 *  addon into a standalone mod is a one-line change here plus config.cpp.
 */

static const string KILLFEED_VERSION = "0.1.0";

//--- Asset prefix. The ONLY hard-coded path in the addon - build every layout path from it.
static const string KILLFEED_PREFIX = "Vigrid-BattleRoyale/Extra/KillFeed/";

#ifdef DIAG
#define KILLFEED_TRACE_ENABLED
#endif

//--- Log verbosity. Overridden at runtime by -killfeed-trace/-killfeed-debug/-killfeed-info/
//--- -killfeed-warn/-killfeed-none on the command line, or by KillFeedLogLevel in serverDZ.cfg.
#ifdef KILLFEED_TRACE_ENABLED
    static const int KILLFEED_LOG_LEVEL = 4; // Trace
#else
    static const int KILLFEED_LOG_LEVEL = 0; // Error only
#endif

//--- Settings. The addon's own profile folder, deliberately not the host mod's, so that
//--- extraction does not strand the file behind.
static const string KILLFEED_SETTINGS_FOLDER = "$profile:KillFeed\\";
static const string KILLFEED_SETTINGS_FILE = "$profile:KillFeed\\killfeed_settings.json";

//--- RPC namespace (CF RPCManager, string-named). Server -> client only; the client never talks
//--- back, so there is no second namespace to declare.
static const string RPC_KILLFEED_NAMESPACE = "RPC-KillFeed";

//--- Server -> client message name. CF's AddRPC dispatches by method name, so the handler on
//--- KillFeedRPC must be named exactly this.
static const string KF_RPC_ENTRY = "KF_Entry";

//--- Attachment types travel as one string so the payload stays a flat Param. ';' cannot appear
//--- in a DayZ classname, which makes it a safe separator.
static const string KILLFEED_ATTACHMENT_SEPARATOR = ";";

//--- Display. These are constants rather than settings because the settings file is #ifdef SERVER
//--- and the client cannot read it; pushing them per-connect is a possible follow-up.
static const int KILLFEED_MAX_ROWS = 4;         //!< rows on screen; also caps live preview entities
static const int KILLFEED_ROW_SECONDS = 8;      //!< how long a row stays up
static const int KILLFEED_ROW_HEIGHT = 60;      //!< px between row origins, must clear the row layout

//--- Icon shown in place of the weapon preview when there is nothing to render. A vanilla imageset
//--- on purpose: depending on another mod's texture would break the standalone promise.
static const string KILLFEED_ICON_DEFAULT = "set:dayz_gui image:iconSkull";

//--- Stringtable keys for the weaponless causes. The weapon causes render a model instead and so
//--- need no phrase of their own.
static const string STR_KILLFEED_ZONE = "#STR_KF_ZONE";
static const string STR_KILLFEED_ENVIRONMENT = "#STR_KF_ENVIRONMENT";
static const string STR_KILLFEED_INFECTED = "#STR_KF_INFECTED";
static const string STR_KILLFEED_ANIMAL = "#STR_KF_ANIMAL";
static const string STR_KILLFEED_BAREHANDS = "#STR_KF_BAREHANDS";
static const string STR_KILLFEED_EXPLOSIVE = "#STR_KF_EXPLOSIVE";

//--- How long a cause hint pushed through KillFeedAPI stays valid. A hint is a statement about why
//--- a player is losing health right now, so it has to expire - otherwise a player who walked out
//--- of the zone and starved to death an hour later would still be reported as a zone kill.
static const int KILLFEED_HINT_TTL_MS = 10000;
