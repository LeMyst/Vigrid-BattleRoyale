/**
 *  This is the master constants file for DayZBR
 *  Any constants for the client are set here
 *  Most, if not all, server constants, will be stored in settings file @ $profile:BattleRoyale\\
 * TODO: move these constants into something a bit easier for modders to work with, that way any 3rd party can launch their own BR off my framework
 */

static const string BATTLEROYALE_VERSION = "0.0.14-Vigrid";

//--- debug settings
static const int BATTLEROYALE_SOLO_GAME = 0;
static const int BATTLEROYALE_LOG_LEVEL = 4; // Default TRACE


//--- default web endpoint
//static const string BATTLEROYALE_WEB_CLIENT_ENDPOINT = "https://dayzbr.dev";
//static const string BATTLEROYALE_WEBSITE = "https://dayzbr.dev";


//--- settings files
static const string BATTLEROYALE_SETTINGS_FOLDER = "$profile:Vigrid-BattleRoyale\\";


//--- RPC namespaces
static const string RPC_DAYZBR_NAMESPACE = "RPC-DayZBR"; //BattleRoyaleClient.c RPC calls
static const string RPC_DAYZBRBASE_NAMESPACE = "RPC-DayZBR-Base"; //BattleRoyaleBase.c RPC calls
static const string RPC_DAYZBRSERVER_NAMESPACE = "RPC-DayZBR-Server"; //BattleRoyaleServer.c RPC calls


//--- constant strings
static const string BATTLEROYALE_FADE_MESSAGE = "DayZ Battle Royale";
static const string BATTLEROYALE_LOADING_MODDED_MESSAGE = "Remember! This is not normal DayZ.";
//static const string BATTLEROYALE_VISIT_WEBSITE_MESSAGE = "Visit DayZBR.Dev";


//--- dummy mission
//--- perhaps this could be pulled from the mod config? (or dynamically generated? randomized?)
static const string BATTLEROYALE_DUMMY_MISSION_WORLD = "ChernarusPlus"; //ChernarusPlus


//--- net manager constants
static const string DAYZBR_NETWORK_ERRORCODE_TIMEOUT = "request timed out";
static const string DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL = "failed to parse json";
static const string DAYZBR_NETWORK_ERRORCODE_WEBPLAYER_NULL = "webplayer is null";
static const string DAYZBR_NETWORK_ERRORCODE_FILE = "file transfered? wierd error";


//--- error codes
static const int DAYZBR_NETWORK_ERRORCODE_NULL_PLAYER_DATA = 1900; //player json object in API is NULL


// Textures
static const string BATTLEROYALE_LOGO_IMAGE = "set:battleroyale_gui image:DayZBRLogo_White";
static const string BATTLEROYALE_LOADING_SCREENS_PATH = "Vigrid-BattleRoyale/GUI/textures/loading_screens/";


//--- game values
static const float BATTLEROYALE_HEALTH_REGEN_MODIFIER = 10; //multiplier from base game values on HP regen speed
static const float BATTLEROYALE_BLOOD_REGEN_MODIFIER = 10; //multiplier from base game values on blood regen speed
static const float BATTLEROYALE_UNCONSCIOUS_MODIFIER = 0.8; //multiplier from base game values on unconscious and conscious threshold


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


//--- zoning subsystem
static const float DAYZBR_ZS_MIN_DISTANCE_PERCENT = 0.25; //min next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MAX_DISTANCE_PERCENT = 0.75; //max next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MIN_ANGLE = 0; //degrees
static const float DAYZBR_ZS_MAX_ANGLE = 360; //non-inclusive


//TODO: move this to the web API
static const string BATTLEROYALE_SERVER_PASSWORD = "DayZBR_Beta";


//---- DayZ Expansion Loading Screens
static const string DAYZBR_LOADING_SCREENS_PATH     = "Vigrid-BattleRoyale/Data/LoadingScreens.json";
static const string DAYZBR_LOADING_MESSAGES_PATH    = "Vigrid-BattleRoyale/Data/LoadingMessages.json";
static const int DAYZBR_LOADING_BAR_COLOR           = ARGB( 255, 0, 0, 0 );     //! A = Alpha (opacity) / R = Red / G = Green / B = Blue
