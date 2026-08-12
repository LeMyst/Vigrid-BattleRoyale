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


//--- leaderboard persistence
//the leaderboard is player data, not settings, so it lives outside the BattleRoyaleConfig registry
//(that registry re-Save()s every entry at boot and applies $mission: overrides - both wrong here).
static const string BATTLEROYALE_LEADERBOARD_FILE = "$profile:Vigrid-BattleRoyale\\leaderboard.json";
//JsonFileLoader.SaveFile is not atomic, so the previous file is copied aside before every write.
//A truncated write would otherwise lose all history; the backup bounds the loss to one match.
static const string BATTLEROYALE_LEADERBOARD_BACKUP = "$profile:Vigrid-BattleRoyale\\leaderboard.json.bak";
//%1 is the season number being retired.
static const string BATTLEROYALE_LEADERBOARD_ARCHIVE_FMT = "$profile:Vigrid-BattleRoyale\\leaderboard_s%1.json";

//write debounce while a match is running. Serialising the whole table on every death is pure waste,
//and a hard process kill can lose at most this much. Match end always forces a flush regardless.
static const int BR_LEADERBOARD_FLUSH_DEBOUNCE_MS = 15000;
//per-player floor between leaderboard requests. The client caches each ladder after its first
//fetch, so in practice this is hit at most twice per session - once per tab. Kept short anyway
//because a refused request is invisible to the player: at 2000 ms, opening the menu and switching
//tabs quickly got the second ladder silently dropped and the list rendered empty.
static const int BR_LEADERBOARD_REQUEST_COOLDOWN_MS = 500;
//hard cap on rows per RPC, applied regardless of what the JSON asks for. There is no RPC chunking
//anywhere in this mod, so the payload has to stay small by construction.
static const int BR_LEADERBOARD_MAX_ROWS = 50;

//which ladder a result lands on, decided by the player's own group size at match start.
static const int BR_LEADERBOARD_BOARD_SOLO = 0;
static const int BR_LEADERBOARD_BOARD_GROUP = 1;

//id of the mod's own DayZPlayer sync juncture, used by every teleport this mod performs (match
//start and F2 unstuck). Both halves of the handler key off it - the server one repositions, the
//client one stops predicting the movement command it was in - so it needs a name rather than a
//literal 88 in four places.
static const int BR_SYNC_JUNCTURE_TELEPORT = 88;

//metres above the resolved ground position a teleport actually places the player, rather than
//setting them exactly on it. A seating epsilon and nothing more: enough that the capsule is not
//started inside the surface it is standing on, far too small to see.
//
//**It used to be 1.0, and that was a mistake worth recording.** The theory was that dropping the
//player in would leave them briefly airborne, the engine would run its own fall -> land
//transition, and that landing would reset the animation graph. The landing never happens: the
//character controller does not re-evaluate its ground contact after a scripted SetPosition, so a
//player placed a metre up simply believes they are standing there. The instrumented run said so at
//the time - PhysicsIsFalling read "not airborne" on BOTH sides with the metre applied - and the
//conclusion drawn from it was too narrow. It was not "the Fall branch is not what fixes the
//unstuck", it was "nothing converts this drop into a fall at all".
//
//What that shipped was a character hovering a metre off the ground after every teleport, on both
//paths, until the player's first input forced a command transition and dropped them. Reported
//against the match-start teleport 2026-08-09 and confirmed on the F2 unstuck.
//
//Two things replace it, because the metre was masking two different faults:
//  - PlayerBase.CommandHandler now ends every teleport with a real command transition (see
//    BR_NotifyTeleported), which is the "jump once" cure without the jump.
//  - BattleRoyaleDebugState.FindUnstuckPosition validates its lobby-centre fallback, so the
//    unstuck stops landing on an unvetted position that could seat the capsule in the scenery.
//That second one was always the principled fix, and this comment used to say so.
static const float BR_TELEPORT_DROP_HEIGHT = 0.05;

//how long after a teleport PlayerBase.CommandHandler keeps asking the controller whether it is
//airborne. One check on the next command tick is not enough on the client: the juncture can arrive
//a frame or two before the corrected position does, so the first check legitimately reads the old
//position and answers "no". Only a fall is ever started from inside this window - the one-shot move
//has already run by then - so a wrong answer costs nothing but the check.
static const float BR_TELEPORT_SETTLE_SECONDS = 0.75;

//per-player floor between granted F2 unstuck teleports. Only the pending-request flag guarded this
//while unstuck existed solely during the warm-up, where the state lasts under a minute; the lobby
//can now answer it too and players sit there for many minutes, so without a cooldown F2 is free
//fast-travel around the lobby. Measured against GetTickTime(), which is in seconds.
static const int BR_UNSTUCK_COOLDOWN_SECONDS = 30;

//field-size weighting curves for leaderboard_settings.json `field_weight_mode`.
static const int BR_LEADERBOARD_WEIGHT_FLAT = 0;
static const int BR_LEADERBOARD_WEIGHT_LOG2 = 1;
static const int BR_LEADERBOARD_WEIGHT_SQRT = 2;
static const int BR_LEADERBOARD_WEIGHT_LINEAR = 3;


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

//--- spawn selection snapping
//a click on water or outside the first zone is snapped to the nearest valid point
//by walking from the click towards the zone centre. Distance, in metres, between
//two samples along that line: fine enough not to step over a beach, coarse enough
//that a 1500 m radius costs ~60 iterations of two native surface calls -- all
//inside a single mouse-up.
static const float BR_SPAWN_SNAP_STEP = 25.0;
//runaway guard on that walk. At the step above this covers 12.8 km, more than any
//zone radius, so it should never be the reason a search stops.
static const int BR_SPAWN_SNAP_MAX_STEPS = 512;
//how far inside the circle a click from outside it lands, in metres. Keeps the
//snapped point clear of the boundary rather than sitting exactly on it.
static const float BR_SPAWN_SNAP_INSET = 25.0;


//--- "who is speaking" HUD list
//
//This list is built on the SERVER and pushed to each client, which is not the obvious design.
//It is that way because DayZPlayer.IsPlayerSpeaking() behaves differently on the two sides, as
//measured on 2026-08-04 with two clients:
//  - server: correctly per-entity. The speaker reads non-zero, everyone else reads exactly 0.
//  - client: returns the LOCAL microphone level no matter which entity it is called on. While
//    Client_B spoke, B's own client reported the same amplitude for Client_B and Client_A alike,
//    and A's client reported 0 for both. It is a global mic meter, not a per-player query.
//So the client physically cannot work out who is talking, and the server can.
//
//how often the server samples every player's amplitude. Voice arrives in packets, so sampling much
//slower than this drops short words; much faster buys nothing a listener can perceive.
static const int BR_SPEAKING_POLL_MS = 200;
//how long a row stays up after a speaker goes quiet. Without it the list strobes between syllables,
//because normal speech dips below the threshold several times a second.
static const int BR_SPEAKING_LINGER_MS = 700;
//amplitude above which a player counts as speaking.
//NOT vanilla's 0.1 - that is its talking-ANIMATION threshold and is far too high here. Measured
//speech peaks at 0.05-0.14 and normally sits at 0.01-0.09, so 0.1 misses almost every word. The
//silence floor is 3.05176e-05, exactly 1/32768 - one LSB of 16-bit audio - so anything comfortably
//above that separates speech from silence.
static const float BR_SPEAKING_AMPLITUDE_THRESHOLD = 0.005;
//hard cap on rows. In the frozen states you only ever hear your own party, so this is only reached
//during the match in a crowd; past it, the newest speaker is simply not shown.
static const int BR_SPEAKING_MAX_ROWS = 6;
//row pitch in pixels, and where the panel sits. Chosen to clear the party HUD, which occupies
//(24, 180) to (24, 660) - see Party/GUI/layouts/party_hud.layout.
static const int BR_SPEAKING_ROW_HEIGHT = 26;


//--- party gathering at spawn selection
//how far from their leader a gathered member is placed, in metres. Small on purpose: voice is
//clamped to Whisper until the match starts, and CGame.MutePlayer is subtractive - it can only
//remove hearing from inside the engine's proximity set, never add it. So a teammate who is out of
//whisper range stays inaudible no matter what the mute matrix says, and the only way party voice
//works during spawn selection is if the party is physically together.
static const float BR_PARTY_GATHER_RADIUS = 3.0;


//--- zoning subsystem
static const float DAYZBR_ZS_MIN_DISTANCE_PERCENT = 0.25; //min next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MAX_DISTANCE_PERCENT = 0.75; //max next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MIN_ANGLE = 0; //degrees
static const float DAYZBR_ZS_MAX_ANGLE = 360; //non-inclusive


//---- DayZ Expansion Loading Screens
static const string DAYZBR_LOADING_SCREENS_PATH     = "Vigrid-BattleRoyale/Data/LoadingScreens.json";
static const string DAYZBR_LOADING_MESSAGES_PATH    = "Vigrid-BattleRoyale/Data/LoadingMessages.json";
static const int DAYZBR_LOADING_BAR_COLOR           = ARGB( 255, 0, 0, 0 );     //! A = Alpha (opacity) / R = Red / G = Green / B = Blue
