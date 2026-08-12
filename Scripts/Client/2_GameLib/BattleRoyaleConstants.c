/**
 *  This is the master constants file for DayZBR
 *  Any constants for the client are set here
 *  Most, if not all, server constants, will be stored in settings file @ $profile:BattleRoyale\\
 * TODO: move these constants into something a bit easier for modders to work with, that way any 3rd party can launch their own BR off my framework
 */

static const string BATTLEROYALE_VERSION = "0.1.0-Vigrid";

#ifdef DIAG
#define BR_TRACE_ENABLED
#define CF_TRACE_ENABLED
#endif

//--- debug settings
#ifdef BR_TRACE_ENABLED
	static const int BATTLEROYALE_LOG_LEVEL = 4; // Trace
#else
	static const int BATTLEROYALE_LOG_LEVEL = 0; // Error
#endif


//--- GitHub repository
static const string GITHUB_URL = "https://github.com/LeMyst/Vigrid-BattleRoyale";


//--- API endpoint
static const string BATTLEROYALE_API_ENDPOINT = "https://api.vigrid.ovh/";


//--- settings files
static const string BATTLEROYALE_SETTINGS_FOLDER = "$profile:Vigrid-BattleRoyale\\";
static const string BATTLEROYALE_SETTINGS_MISSION_FOLDER = "$mission:Vigrid-BattleRoyale\\";


//--- RPC namespaces
static const string RPC_DAYZBR_NAMESPACE = "RPC-DayZBR"; //BattleRoyaleClient.c RPC calls
static const string RPC_DAYZBRSERVER_NAMESPACE = "RPC-DayZBR-Server"; //BattleRoyaleServer.c RPC calls


//--- constant strings
static const string BATTLEROYALE_FADE_MESSAGE = "DayZ Battle Royale";
static const string BATTLEROYALE_LOADING_MODDED_MESSAGE = "Remember! This is not normal DayZ.";


// Textures
static const string BATTLEROYALE_LOGO_IMAGE = "set:battleroyale_gui image:DayZBRLogo_White";


//--- game values
static const float BATTLEROYALE_HEALTH_REGEN_MODIFIER = 10; //multiplier from base game values on HP regen speed
static const float BATTLEROYALE_BLOOD_REGEN_MODIFIER = 10; //multiplier from base game values on blood regen speed


//--- notification messages
static const float DAYZBR_MSG_TIME = 7;
static const string DAYZBR_MSG_IMAGE = "set:expansion_iconset image:icon_info";
static const string DAYZBR_MSG_TITLE = "DayZ Battle Royale";


//--- broken debug zone values
static const int DAYZBR_DEBUG_HEAL_TICK = 5;


//--- spawn selection menu
static const float HEATMAP_GRID_SIZE_MULTIPLIER = 4.0; //multiplier for the heatmap grid size, e.g. 4.0 = 4x the spawn size
static const int HEATMAP_MAX_DENSITY = 5; //max density for color scaling in the heatmap

//--- spawn selection menu rendering
//heatmap grid cells are packed into a single int key instead of an "x,z" string,
//so the density map needs no string building and no Split()/ToInt() round trip.
//The bias keeps the key positive for negative cell indices and the stride must
//exceed twice the bias. Together they cover cell indices -16384..16383, which at
//the default 200 m cell size is +/- 3276 km -- far beyond any DayZ world.
static const int HEATMAP_KEY_BIAS = 16384;
static const int HEATMAP_KEY_STRIDE = 32768;
//distance, in metres, between the two world points probed through MapToScreen
//once per frame to recover the map's affine world->screen transform. Any pair of
//distinct points works; shrink it if precision ever suffers at extreme zoom.
static const float HEATMAP_PROBE_DISTANCE = 1000.0;
//max stroke width, in pixels, used to fill one heatmap cell. A cell is filled by
//stacking horizontal lines of this thickness. Raise it to draw a cell in fewer
//calls; set it to 0 to fill each cell with a single full-height stroke, which is
//fastest but assumes CanvasWidget.DrawLine centres its stroke on the line -- if
//the heatmap ends up offset by half a cell vertically, that assumption is wrong.
static const float HEATMAP_FILL_MAX_STROKE = 4.0;
//force a full repaint at least this often (ms), even when nothing changed. Bounds
//the damage if a canvas ever stops retaining its draw list between frames: the
//map visibly strobes instead of silently going blank.
static const int HEATMAP_REPAINT_WATCHDOG_MS = 500;
//segment count bounds for map ovals. n = PI*sqrt(radius_px) keeps the chord
//deviation at half a pixel for any radius; the bounds just cap the extremes.
static const int BR_OVAL_MIN_SEGMENTS = 16;
static const int BR_OVAL_MAX_SEGMENTS = 180;


//--- zoning subsystem
static const float DAYZBR_ZS_MIN_DISTANCE_PERCENT = 0.25; //min next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MAX_DISTANCE_PERCENT = 0.75; //max next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MIN_ANGLE = 0; //degrees
static const float DAYZBR_ZS_MAX_ANGLE = 360; //non-inclusive


//---- DayZ Expansion Loading Screens
static const string DAYZBR_LOADING_SCREENS_PATH     = "Vigrid-BattleRoyale/Data/LoadingScreens.json";
static const string DAYZBR_LOADING_MESSAGES_PATH    = "Vigrid-BattleRoyale/Data/LoadingMessages.json";
static const int DAYZBR_LOADING_BAR_COLOR           = ARGB( 255, 0, 0, 0 );     //! A = Alpha (opacity) / R = Red / G = Green / B = Blue
