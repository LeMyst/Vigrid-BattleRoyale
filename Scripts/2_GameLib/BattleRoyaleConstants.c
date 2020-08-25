/**
 *  This is the master constants file for DayZBR
 *  Any constants for the client are set here
 *  Most, if not all, server constants, will be stored in settings file @ $profile:BattleRoyale\\
 * TODO: move these constants into something a bit easier for modders to work with, that way any 3rd party can launch their own BR off my framework
 */

static const string BATTLEROYALE_WEB_CLIENT_ENDPOINT = "https://dayzbr.dev";
static const string BATTLEROYALE_SETTINGS_FOLDER = "$profile:BattleRoyale\\";
static const string BATTLEROYALE_VEHICLES_FOLDER = BATTLEROYALE_SETTINGS_FOLDER + "\\Vehicles\\";
static const string RPC_DAYZBR_NAMESPACE = "RPC-DayZBR";

//TODO: this will get removed
static const string BATTLERYALE_FADE_MESSAGE = "DayZ Battle Royale";


//--- web API response error codes
static const int DAYZBR_NETWORK_ERRORCODE_NULL_RESULT = 1500;
static const int DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL_RESULT = 1600;
static const int DAYZBR_NETWORK_ERRORCODE_CONTEXT_NULL_RESULT = 1700;
static const int DAYZBR_NETWORK_ERRORCODE_WEBPLAYER_NULL_RESULT = 1800;

