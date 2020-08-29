/**
 *  This is the master constants file for DayZBR
 *  Any constants for the client are set here
 *  Most, if not all, server constants, will be stored in settings file @ $profile:BattleRoyale\\
 * TODO: move these constants into something a bit easier for modders to work with, that way any 3rd party can launch their own BR off my framework
 */

static const string BATTLEROYALE_WEB_CLIENT_ENDPOINT = "https://dayzbr.dev";
static const string BATTLEROYALE_SETTINGS_FOLDER = "$profile:BattleRoyale\\";
static const string BATTLEROYALE_VEHICLES_FOLDER = BATTLEROYALE_SETTINGS_FOLDER + "\\Vehicles\\";
static const string BATTLEROYALE_LOOT_ENTRIES_FOLDER = BATTLEROYALE_SETTINGS_FOLDER + "\\Loot\\Entries\\";
static const string BATTLEROYALE_LOOT_CATEGORIES_FOLDER = BATTLEROYALE_SETTINGS_FOLDER + "\\Loot\\Categories\\";
static const string RPC_DAYZBR_NAMESPACE = "RPC-DayZBR";
static const string RPC_DAYZBRBASE_NAMESPACE = "RPC-DayZBR-Base";

//TODO: this will get removed
static const string BATTLERYALE_FADE_MESSAGE = "DayZ Battle Royale";


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
static const string BATTLEROYALE_TSHIRT_SKINS_PATH = "BattleRoyale\\GUI\\textures\\tshirt\\";