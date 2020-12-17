/**
 *  This is the master constants file for DayZBR
 *  Any constants for the client are set here
 *  Most, if not all, server constants, will be stored in settings file @ $profile:BattleRoyale\\
 * TODO: move these constants into something a bit easier for modders to work with, that way any 3rd party can launch their own BR off my framework
 */

static const string BATTLEROYALE_VERSION = "0.1.t_namalsk";

//--- default web endpoint
static const string BATTLEROYALE_WEB_CLIENT_ENDPOINT = "https://dayzbr.dev";
static const string BATTLEROYALE_WEBSITE = "https://dayzbr.dev";

//--- settings files
static const string BATTLEROYALE_SETTINGS_FOLDER = "$profile:BattleRoyale\\";
static const string BATTLEROYALE_VEHICLES_FOLDER = BATTLEROYALE_SETTINGS_FOLDER + "\\Vehicles\\";
static const string BATTLEROYALE_LOOT_ENTRIES_FOLDER = BATTLEROYALE_SETTINGS_FOLDER + "\\Loot\\Entries\\";
static const string BATTLEROYALE_LOOT_CATEGORIES_FOLDER = BATTLEROYALE_SETTINGS_FOLDER + "\\Loot\\Categories\\";

//--- RPC namespaces
static const string RPC_DAYZBR_NAMESPACE = "RPC-DayZBR"; //BattleRoyaleClient.c RPC calls
static const string RPC_DAYZBRBASE_NAMESPACE = "RPC-DayZBR-Base"; //BattleRoyaleBase.c RPC calls
static const string RPC_DAYZBRSERVER_NAMESPACE = "RPC-DayZBR-Server"; //BattleRoyaleServer.c RPC calls

//--- constant strings
static const string BATTLEROYALE_FADE_MESSAGE = "DayZ Battle Royale";
static const string BATTLEROYALE_LOADING_MODDED_MESSAGE = "Remember! This is not normal DayZ.";
static const string BATTLEROYALE_VISIT_WEBSITE_MESSAGE = "Visit DayZBR.Dev";


//--- dummy mission
//--- perhaps this could be pulled from the mod config? (or dynamically generated? randomized?)
static const string BATTLEROYALE_DUMMY_MISSION_WORLD = "Namalsk"; //ChernarusPlus


//--- web API response error codes
static const int DAYZBR_NETWORK_ERRORCODE_NULL_RESULT = 1500; //web returned 200, but response body was blank
static const int DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL_RESULT = 1600; //json response from the webserver cannot be parsed, remember dayz can't parse Booleans
static const int DAYZBR_NETWORK_ERRORCODE_CONTEXT_NULL_RESULT = 1700; //somehow the REST context is returning NULL
static const int DAYZBR_NETWORK_ERRORCODE_WEBPLAYER_NULL_RESULT = 1800; //same as 1900 but internal to the API class structure


//--- error codes
static const int DAYZBR_NETWORK_ERRORCODE_NULL_PLAYER_DATA = 1900; //player json object in API is NULL


//--- GUI
static const int DAYZBR_SKIN_SELECTION_MENU = 5000;


// Textures
static const string BATTLEROYALE_LOGO_IMAGE = "set:battleroyale_gui image:DayZBRLogo_White";

static const string BATTLEROYALE_LOADING_SCREENS_PATH = "BattleRoyale/GUI/textures/loading_screens/";

//--- game values
static const float BATTLEROYALE_HEALTH_REGEN_MODIFIER = 10; //multiplier from base game values on HP regen speed


//TODO: figure out which mission is loaded in realtime (or use a server-specific setting)
//static const string BATTLEROYALE_LOOT_XML_PATH = "$CurrentDir:mpmissions\\BattleRoyale.ChernarusPlusGloom\\mapgroupproto.xml";

//TODO: move these into loot settings config file
static const string BATTLEROYALE_LOOT_MAGAZINES_CATEGORY = "magazines";
static const string BATTLEROYALE_LOOT_AMMO_CATEGORY = "ammo";
static const string BATTLEROYALE_LOOT_ATTACHMENTS_CATEGORY = "attachments";
static const string BATTLEROYALE_LOOT_BROKEN_STYLE_ITEM_CLASSNAME = "Zucchini"; //This item spawns whenever an entry has no styles defined



//--- state machine | state names
static const string DAYZBR_SM_COUNT_REACHED_NAME = "Player Count Reached State";
static const string DAYZBR_SM_DEBUG_ZONE_NAME = "Debug Zone State";
static const string DAYZBR_SM_LAST_ROUND_NAME = "Last Gameplay State";
static const string DAYZBR_SM_PREPARE_NAME = "Prepare State";
static const string DAYZBR_SM_RESTART_NAME = "Restart State";
static const string DAYZBR_SM_GAMEPLAY_NAME = "Gameplay State";
static const string DAYZBR_SM_START_MATCH_NAME = "Start Match State";
static const string DAYZBR_SM_UNKNOWN_NAME = "Unknown State";
static const string DAYZBR_SM_WIN_NAME = "Win State";
static const string DAYZBR_SM_UNKNOWN_DEBUG_NAME = "Unknown Debug State";


//--- messages
static const float DAYZBR_MSG_TIME = 7;
static const string DAYZBR_MSG_IMAGE = "set:expansion_iconset image:icon_info";
static const string DAYZBR_MSG_TITLE = "DayZ Battle Royale";

static const string DAYZBR_MSG_NEW_ZONE_OUTSIDE = "A new zone has appeared! YOU ARE OUTSIDE THE PLAY AREA!";
static const string DAYZBR_MSG_NEW_ZONE_INSIDE = "A new zone has appeared! You are in the play area!";
static const string DAYZBR_MSG_TAKING_DAMAGE = "You are taking zone damage!";
static const string DAYZBR_MSG_MATCH_STARTED = "The match has started!";

//--- broken debug zone values
static const vector DAYZBR_DEBUG_CENTER = "3954.45 5.67608 10243.8";
static const float DAYZBR_DEBUG_RADIUS = 100;
static const int DAYZBR_DEBUG_HEAL_TICK = 5;


//--- vehicle subsystem
static const float DAYZBR_VS_MIN_SURFACE_FRICTION = 0.94; //todo: config this as a setting

//--- zoning subsystem
static const float DAYZBR_ZS_MIN_DISTANCE_PERCENT = 0; //min next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MAX_DISTANCE_PERCENT = 1; //max next zone distance as a percent of maxmimum distance (1 => 100%)
static const float DAYZBR_ZS_MIN_ANGLE = 0; //degrees
static const float DAYZBR_ZS_MAX_ANGLE = 360; //non-inclusive


//--- popup text
static const string DAYZBR_CONNECTING_TO_NETWORK_MSG = "Connecting to the Battle Royale Network...";
static const string DAYZBR_MATCHMAKING_MSG = "Matchmaking...";
static const string DAYZBR_CONNECTING_TO_SERVER_MSG = "Connecting to match. Please be patient, this could take a while...";
static const string DAYZBR_FAILED_TO_CONNECT_MSG = "Error! Failed to connect to the provided server!";
static const string DAYZBR_TIMEOUT_MSG = "Failed to connect! Timed out!";
static const string DAYZBR_NULL_RESPONSE_MSG = "Error! NULL Response!";

//TODO: move this to the web API
static const string BATTLEROYALE_SERVER_PASSWORD = "DayZBR_Beta";