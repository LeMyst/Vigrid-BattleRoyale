/**
 *  This is the master constants file for DayZBR
 *  Any constants for the client are set here
 *  Most, if not all, server constants, will be stored in settings file @ $profile:BattleRoyale\\
 * TODO: move these constants into something a bit easier for modders to work with, that way any 3rd party can launch their own BR off my framework
 */

static const string BATTLEROYALE_WEB_CLIENT_ENDPOINT = "https://dayzbr.dev";
static const string BATTLEROYALE_SETTINGS_FOLDER = "$profile:BattleRoyale\\";
static const int BATTLEROYALE_DEBUG_HEAL_TICK_TIME = 5; //TODO: config this