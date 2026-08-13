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
//...and the third tab, which is not a ladder at all: the previous match, read back from disk.
//It shares the menu and nothing else - its own request RPC, its own cache and its own sequence
//counter. See the warning on BattleRoyaleClient.RequestLeaderboard about never passing this value.
static const int BR_LEADERBOARD_BOARD_LASTMATCH = 2;


//--- last match summary persistence
//One match, written once, read back by the NEXT server process. The whole feature exists because
//this server restarts between matches, so the lobby a player reconnects into is the only place a
//summary can reach the winner as well as the dead.
static const string BATTLEROYALE_LASTMATCH_FILE = "$profile:Vigrid-BattleRoyale\\last_match.json";
//Same non-atomic SaveFile problem as the leaderboard. Here the copy-aside doubles as a free
//one-deep history: at the instant of the single write the primary still holds the PREVIOUS match.
static const string BATTLEROYALE_LASTMATCH_BACKUP = "$profile:Vigrid-BattleRoyale\\last_match.json.bak";

//Hard cap on rows, applied on write and again on read. Sits above any realistic field on purpose -
//see the TRUNCATED flag below for why a cap that actually bites is a correctness problem and not
//just a display one.
static const int BR_LASTMATCH_MAX_ROWS = 64;
//Per-player floor between last-match requests. Its OWN map, not the leaderboard's: sharing one
//budget means opening F4 and switching tabs quickly gets silently refused, and the menu's 1000 ms
//retry would then thrash against it.
static const int BR_LASTMATCH_REQUEST_COOLDOWN_MS = 500;
//On-disk format version. Nothing to migrate yet; present from day one because this file is a
//readable, uid-keyed, write-once artefact that an external tool could reasonably consume.
static const int BR_LASTMATCH_FILE_VERSION = 1;

//Flags on SetLastMatchTable.
//GROUPED: field_size counts GROUPS, and that figure only means something when parties are actually
//in play. VigridPartyAPI.GetGroupCount degrades to one group per player when the party manager is
//disabled, which is numerically identical to the player count - so without this the card would
//render "#4 of 12 squads" on a server with no squads. Same trap as BR_HUD_GROUPS_NONE.
static const int BR_LASTMATCH_FLAG_GROUPED = 1;
//TRUNCATED: the row cap bit. The squad block is summed CLIENT-side from the table, so a missing
//squadmate silently produces a smaller, wrong total. With this set the client hides the squad block
//outright - no figure beats a wrong figure.
static const int BR_LASTMATCH_FLAG_TRUNCATED = 2;

//How long a zone-damage hint stays good. The play area is the one environmental cause worth naming,
//and scripted damage reaches EEKilled with the victim as their own killer, so it is only knowable
//from a hint dropped at the damage site. Consumed on read, so a stale hint cannot mislabel the next
//environmental death.
static const int BR_KILL_HINT_TTL_MS = 5000;

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

//floor applied to general_settings.json `late_join_kick_seconds`. A player who connects mid-match is
//scheduled for a disconnect rather than dropped on the spot, and this is how short that grace period
//is allowed to get. The reason is on record in BattleRoyaleServer.OnPlayerConnected: calling
//GetGame().DisconnectPlayer on an identity whose client has not finished establishing its connection
//crashes the server. Do not lower this to "make the kick snappier" - the seconds are load-bearing.
static const int BR_LATE_JOIN_KICK_MIN_SECONDS = 5;
//ceiling, in seconds from the connect event, after which a late joiner is kicked even though their
//client never reported itself loaded in. The grace period normally starts at that report rather than
//at connect - see BattleRoyaleServer.PlayerLoadedIn - because vanilla calls InvokeOnConnect from the
//ClientNew path, which fires while the client is still loading the world. Measured 2026-08-10: for a
//new character ClientReadyEventTypeID never fires at all, and ClientPrepare -> ClientNew alone took
//20 s, so the whole grace period could be spent on a loading screen the player cannot read.
//This ceiling only exists so a client that never reports in is still removed eventually.
static const int BR_LATE_JOIN_READY_TIMEOUT_SECONDS = 90;
//seconds remaining at which the "you are about to be disconnected" notification is repeated once.
//The first one fires as soon as the kick is scheduled, which is while the player is still staring at
//a loading screen on a slow client, so a single notification is easy to miss entirely.
static const int BR_LATE_JOIN_FINAL_WARN_SECONDS = 5;

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


//--- how long -br-autoconnect waits after the main menu finishes building before it connects on its
//own. Deferred rather than immediate because ConnectFromServerBrowser tears the menu down, and
//calling it from inside Init() would do that to a widget tree Init() has not finished assembling.
//Half a second is not a settling delay the connect needs - it is only there to land the call on a
//later frame - so it is short enough to feel instant. DIAG-only, like the flag it serves.
static const int BR_AUTOCONNECT_DELAY_MS = 500;


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

//--- hot zones on the spawn selection map. The same red as VIGRID_MAP_COLOR_HOT_ZONE on the
//--- in-game map, so a circle a player picked their drop against looks the same once they land.
//--- Filled here and outline-only there on purpose: this screen is a one-off decision made from a
//--- zoomed-out view, where a wash of colour reads at a glance, while the in-game map is something
//--- a player pans around and a filled disc would hide the terrain they are reading.
static const int BR_HOT_ZONE_OUTLINE_COLOR = 0xC8FF3232;  // ARGB(200, 255, 50, 50)
static const int BR_HOT_ZONE_FILL_COLOR = 0x3CFF3232;     // ARGB(60, 255, 50, 50)

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


//--- spectating
//the script class SelectSpectator instantiates on the client. It resolves a SCRIPT class name, not
//a config class - vanilla's own DayZSpectator has no CfgVehicles entry anywhere in P:\dz - so this
//name must match the class in BattleRoyaleSpectatorCamera.c exactly, and no config.cpp entry exists
//or is needed for it.
static const string BR_SPECTATE_CAM_CLASS = "BattleRoyaleSpectatorCamera";
//how long the death screen stays up before the camera takes over ON ITS OWN. The player can skip
//this by pressing Spectate; this is only the "did nothing" path, so it is generous rather than
//snappy - it has to be long enough to read the screen and decide. Compared as a plain deadline in
//the 10 Hz tick rather than armed as a Timer, so a stalled server delays entry instead of firing a
//callback at a victim who has since been freed.
static const int BR_SPECTATE_ENTRY_DELAY_MS = 20000;
//how long the death screen waits before quitting to the main menu ON ITS OWN, whenever no spectate
//offer is standing - because spectate_enabled is off, because the player died in a state that does
//not allow spectating, or because the match ended and the server withdrew the offer. Without it the
//screen is a dead end on those paths: the only way out is the Quit button, and an AFK player holds
//their slot forever. 15 s is what the pre-spectate death path used for its unconditional
//CallLater(LeaveServer), so a server that never turns spectating on keeps the timing it had.
//Never runs while an offer stands - that case counts down to BR_SPECTATE_ENTRY_DELAY_MS instead.
static const int BR_DEAD_AUTO_QUIT_MS = 15000;
//floor between two "running post-process" lines. The set can flip stopped/running every frame while
//a corpse re-asserts an effect, and logging each flip put fifteen lines in one second.
static const int BR_SPECTATE_PPE_LOG_MS = 2000;
//how often the server re-sends the current target. This is a keepalive, not just an edge: a client
//that missed the first push, or could not resolve the target entity because it was outside their
//network bubble, simply latches on the next one.
static const int BR_SPECTATE_PUSH_MS = 1000;
//hard cap on killer-chain hops, independent of the visited-set cycle guard. A cycle is unreachable
//in a real match because death is permanent, so both guards are purely defensive - against a
//corrupted ledger, and against any future revive mechanic.
static const int BR_SPECTATE_CHAIN_MAX_HOPS = 64;
static const int BR_SPECTATE_MODE_FOLLOW = 0;
static const int BR_SPECTATE_MODE_ORBIT = 1;
//--- Admin free camera. Only ever set for an admin entry; an ordinary spectator can never reach it,
//--- because nothing client-side chooses the mode - the server pushes it in SetSpectateTarget and
//--- only ever pushes FREE in response to an admin-gated request.
static const int BR_SPECTATE_MODE_FREE = 2;

//--- ADMIN SPECTATE. Gated on admins_steamid64 plus admin_spectate_enabled (general_settings.json).
//---
//--- The governing rule is that admin spectate requires being a NON-PARTICIPANT: alive, holding a
//--- body, and absent from m_Players. A competing admin is refused, because a competitor who can
//--- freecam the map is indistinguishable from a cheat. There are two ways to be a non-participant -
//--- connect mid-match (OnPlayerConnected already places those at the live circle), or die and take
//--- the admin respawn, which is the bridge between the two halves of the lifecycle.
//---
//--- Character the admin respawn creates. scope=2 in dz/characters/data/config.cpp, so it is
//--- spawnable; deliberately a fixed type rather than a random one, since this body is carried,
//--- invisible and never seen by anyone.
static const string BR_ADMIN_RESPAWN_CHARACTER = "SurvivorM_Mirek";
//--- ADMIN ANCHOR PLACEMENT. The admin's body is not simply parked on the camera: it is placed to
//--- put as many players as possible inside the replication bubble, so the admin can see AND HEAR
//--- the most of the match, while staying off anyone's lap.
//---
//--- Radius the body is assumed to cover. Inside DayZ's default 1000 m networkRangePlayers with
//--- enough margin that a player jogging near the edge does not flicker in and out.
static const float BR_ADMIN_ANCHOR_COVER_M = 900.0;
//--- Never place the body closer than this to a living player. It is invisible, but it is still a
//--- simulated entity that can be bumped into, and an admin materialising in someone's kitchen is
//--- the thing to avoid. Relaxed automatically when no candidate satisfies it - see ChooseAnchorPosition.
static const float BR_ADMIN_ANCHOR_MIN_PLAYER_M = 150.0;
//--- How far the body may sit from the camera. The camera's own surroundings must stay well inside
//--- the bubble, or the admin optimises coverage of players they cannot see.
static const float BR_ADMIN_ANCHOR_MAX_OFFSET_M = 400.0;
//--- Do not bother moving for less than this. Stops the body jittering between two near-equal spots.
static const float BR_ADMIN_ANCHOR_STEP_M = 150.0;
//--- Ordinary re-placement cadence. Deliberately lazy: each move is a sync juncture on a live
//--- entity, and the coverage answer changes slowly while players run around.
static const int BR_ADMIN_ANCHOR_INTERVAL_MS = 5000;
//--- ...but a fast free camera outruns that. At the top speed step the camera covers over 200 m/s,
//--- so past this distance the body is moved immediately regardless of the interval, or the admin
//--- flies clean out of their own bubble and everything derenders.
static const float BR_ADMIN_ANCHOR_URGENT_M = 500.0;
//--- How often the client reports its camera position so the server can carry the body. Twice the
//--- rate of the keepalive: this one is what keeps the replication bubble under the camera, so it is
//--- the one that must not lag. Cheap - one vector, and only ever for an admin who is spectating.
static const int BR_ADMIN_CAMPOS_PUSH_MS = 500;
//--- Free camera base speed, m/s, before the gear-up/down multiplier and before Shift doubles it.
//--- Vanilla DayZSpectator uses 5.0; this is faster because the thing an admin flies a camera across
//--- is a battle royale map, not a room.
static const float BR_SPECTATE_FREE_SPEED = 12.0;
//--- Ceiling on the speed multiplier. Vanilla's equivalent is unclamped and goes negative, which
//--- flies the camera backwards with nothing on screen to say why.
static const float BR_SPECTATE_FREE_SPEED_MAX_MULT = 16.0;
//--- Floor. Well below 1, so the wheel can slow the camera right down for precise framing rather
//--- than only speeding it up.
static const float BR_SPECTATE_FREE_SPEED_MIN_MULT = 0.15;
//--- Ratio per wheel notch. Multiplicative rather than vanilla's flat +/-2: a fixed step is a huge
//--- jump when crawling and imperceptible when flying, so it gives even control across the range.
//--- 1.3 spans the full 0.15-16 range in about seventeen notches.
static const float BR_SPECTATE_FREE_SPEED_STEP = 1.3;
//--- Gap between teleporting the admin's body on exit and handing control of it back. Handing it
//--- back in the SAME frame crashed the client in vanilla's own first-person camera - the body has
//--- never been simulated, so the camera initialises against a player that is still mid-juncture.
//--- Generous rather than minimal: nothing is waiting on it, and one frame would be cutting it fine.
static const int BR_ADMIN_EXIT_SELECT_DELAY_MS = 400;
//--- Hard cap on rows in the admin player list. This mod has no RPC chunking anywhere (see the
//--- leaderboard's own 50-row cap for the same reason), so the payload has to be bounded by
//--- construction rather than by hoping a match stays small.
static const int BR_ADMIN_LIST_MAX = 64;
//--- ADMIN OVERLAY TAGS. Fallback size in pixels, used only on the first frame before the widget
//--- has been shown and can report a real one. MUST track the root size in spectator_tag.layout.
static const float BR_SPECTATE_TAG_SIZE_W = 190.0;
static const float BR_SPECTATE_TAG_SIZE_H = 54.0;
//--- Gap in pixels between the bottom of the tag and the head it sits above.
static const float BR_SPECTATE_TAG_GAP_PX = 8.0;
//--- Metres above the head bone, and above the feet when there is no entity to read a bone from.
static const float BR_SPECTATE_TAG_HEAD_OFFSET = 0.25;
static const float BR_SPECTATE_TAG_HEIGHT_OFFSET = 1.9;
//--- Inset in pixels for a tag clamped to the screen edge.
static const float BR_SPECTATE_TAG_EDGE_MARGIN = 60.0;
//--- Off-screen tags are dimmed rather than hidden: an admin wants to know somebody is behind them.
static const float BR_SPECTATE_TAG_OFFSCREEN_ALPHA = 0.45;
//--- Name colour for the player the camera is currently following, and for anyone with no party.
//--- The target colour is deliberately not any VigridPartyPalette slot, so it cannot be mistaken
//--- for a team colour.
static const int BR_SPECTATE_TAG_TARGET_COLOUR = 0xFFFFDD44;
static const int BR_SPECTATE_TAG_SOLO_COLOUR = 0xFFFFFFFF;

//--- LOBBY NAME TAGS. A name over every non-teammate while the players are still gathered in the
//--- lobby, replacing the "point at somebody to read their name" tag this mod used to re-enable
//--- there (vanilla's own, which ships #ifdef PLATFORM_PS4 and is dead on PC otherwise).
//---
//--- Set false to compile the whole feature out. Client cosmetic, so compile-time: the settings
//--- files are server-side only and there is nothing here an operator needs to tune per match.
static const bool BR_LOBBY_TAGS_ENABLED = true;
//--- Fallback size in pixels, used on the first frame before the widget has been shown and can
//--- report a real one. MUST track the root size in lobby_tag.layout.
static const float BR_LOBBY_TAG_SIZE_W = 190.0;
static const float BR_LOBBY_TAG_SIZE_H = 22.0;
//--- Gap in pixels between the bottom of the tag and the head it sits above.
static const float BR_LOBBY_TAG_GAP_PX = 6.0;
//--- Metres above the head bone. Lower than the spectator tag's, which has a health bar and a
//--- second line under it and so needs the clearance.
static const float BR_LOBBY_TAG_HEAD_OFFSET = 0.22;
//--- Fallback anchor when the head bone will not resolve, measured from the feet.
static const float BR_LOBBY_TAG_HEIGHT_OFFSET = 1.9;
//--- Past this there is no tag at all. The lobby is one clearing and everybody in it is within a
//--- few dozen metres; the cap is what stops the far side of the map filling the screen with names
//--- during the pre-match countdown, when players have started to spread out.
static const float BR_LOBBY_TAG_MAX_DISTANCE_M = 80.0;
//--- Fade over the last stretch of that range, so a tag thins out rather than blinking off.
static const float BR_LOBBY_TAG_FADE_START_M = 55.0;
//--- Bound on rows, matching the pooling everywhere else in this mod. A full lobby is 60 players
//--- and every one of them is in the same clearing.
static const int BR_LOBBY_TAG_MAX_ROWS = 64;
//--- How often the server pushes each player their lobby name list. 1 Hz: the contents only change
//--- when somebody joins, leaves or dies, and the client resolves live entities for the POSITIONS,
//--- so this carries no motion and does not need to keep up with any.
static const int BR_LOBBY_NAMES_PUSH_MS = 1000;
//--- Throttle on the one diagnostic line the tag renderer emits while it is active.
static const int BR_LOBBY_TAG_DIAG_MS = 2000;
//--- Plain white. Party members are excluded from this overlay entirely - they already have the
//--- party's own coloured name tags - so there is no palette slot to honour and a second colour
//--- here would only invite the two to be confused.
static const int BR_LOBBY_TAG_COLOUR = 0xFFFFFFFF;

//--- SKELETON OVERLAY. How far from the CAMERA a player is still drawn, in metres of view depth.
//--- Ours, not COT's: JMESPModule culls at its own ESPRadius (200 m by default and an admin's
//--- personal COT setting), which is well inside the range a spectating admin watches from and is
//--- why driving COT's own loop was never going to be enough.
static const float BR_SPECTATE_SKELETON_RANGE_M = 500.0;
//--- Line width passed to JMESPSkeleton.Draw. 1 px is COT's own default and reads cleanly at range.
static const float BR_SPECTATE_SKELETON_THICKNESS = 1.0;
//--- Corpses too, so a fight can be found afterwards. Set false for living players only.
static const bool BR_SPECTATE_SKELETON_CORPSES = true;
//--- ⚠️ CORPSES ARE DRAWN BY US, NOT BY COT, AND THE COLOUR IS THE WHOLE REASON.
//--- JMESPSkeleton.Draw takes no colour - it derives one from GetHealthLevel(), and a dead body is
//--- STATE_RUINED, which COT paints 0xFF232323. That is near-black: the worst possible colour for
//--- the one job corpse skeletons have. JMESPCanvas.DrawLine does take a colour, so the corpse pass
//--- goes straight to the canvas.
static const int BR_SPECTATE_SKELETON_CORPSE_COLOUR = 0xFFFF3030;
//--- Thicker than a living skeleton: a body lies flat and foreshortens to almost nothing at range.
static const int BR_SPECTATE_SKELETON_CORPSE_THICKNESS = 2;

//--- AdminEligibility verdicts. Every admin RPC consults it, and the whole lifecycle is these four
//--- values - there is deliberately no fifth "maybe" state to reason about.
//--- NOT_ADMIN is answered SILENTLY - telling a non-admin that an admin feature exists, and that
//--- they are not on the list, is information they have no use for. Every other verdict is a real
//--- admin who deserves to know why their key did nothing.
static const int BR_ADMIN_REFUSED_NOT_ADMIN = 0;
static const int BR_ADMIN_REFUSED_COMPETING = 1;
//--- An admin, in a phase that does not allow spectating (lobby, spawn selection, prepare, warm-up,
//--- post-match). Split out from NOT_ADMIN so it can be explained: without its own verdict this was
//--- indistinguishable from a dead key, since the refusal was a server-side Warn and nothing else.
static const int BR_ADMIN_REFUSED_PHASE = 2;
static const int BR_ADMIN_OFFER_RESPAWN = 3;
static const int BR_ADMIN_ALLOW_SPECTATE = 4;

//--- CORPSE CARRY. The replication bubble is centred on the spectator's CORPSE, not on the camera -
//--- UpdateSpectatorPosition does not move it (measured both directions 2026-08-10). So a target
//--- beyond DayZ's default 1000 m networkRangePlayers simply is not replicated, and the spectator
//--- gets a nametag with no character. Moving the corpse moves the bubble, and needs no entity
//--- creation, which is what killed the carrier body. See CLAUDE.md -> Spectating.
//---
//--- Compile-time on purpose: these are engine-behaviour tuning, not server-operator policy, and a
//--- wrong value here degrades quietly rather than visibly. Nothing reads them unless spectate_enabled
//--- is on, so a server that never spectates is unaffected either way.
//---
//--- Set false to disable carrying entirely and keep the ~1 km limit. The corpse then never moves.
static const bool BR_SPECTATE_CARRY_CORPSE = true;
//--- Carry once the target is this far from the corpse AND nobody is standing near the body. Well
//--- inside the ~1 km boundary, so the move happens long before anything breaks.
static const float BR_SPECTATE_CARRY_TRIGGER_M = 250.0;
//--- Carry regardless of bystanders at this distance. The point of waiting is to let somebody
//--- actually loot the body; the point of this bound is that a spectator seeing nothing is worse.
static const float BR_SPECTATE_CARRY_FORCED_M = 750.0;
//--- After the first carry the corpse is empty and invisible, so it only needs to keep up, not to
//--- track. Re-carry when the target has drifted this far from it.
static const float BR_SPECTATE_CARRY_STEP_M = 250.0;
//--- "Somebody is at the body." Generous compared with looting reach, because the case being avoided
//--- is a player walking up to a corpse that vanishes as they arrive.
static const float BR_SPECTATE_CARRY_BYSTANDER_M = 50.0;
//--- How often the carry pass runs. It resolves bodies by walking every Man in the world, so it is
//--- deliberately not on the 10 Hz tick.
static const int BR_SPECTATE_CARRY_INTERVAL_MS = 1000;
//seconds the dead screen takes to clear once spectating begins.
static const float BR_SPECTATE_FADE_SECONDS = 1.0;
//how close a local entity must be to the server-pushed position to be accepted as the target, when
//the RPC's Object reference arrived NULL. Metres.
//Was 3.0, which made this fallback dead code for any target who was MOVING: the pushed position is
//refreshed at 1 Hz, and a sprinting player covers about 6 m in that second, so the entity was
//essentially never within 3 m of the position being compared against. 15 m is a second of sprint
//plus margin, and still far tighter than the spacing at which mistaking one player for another
//becomes plausible - this only runs when CF handed back no entity at all.
static const float BR_SPECTATE_LATCH_RADIUS = 15.0;
//camera boom, in metres, measured from the target's HEAD BONE - not their feet. The WIP branch used
//1.0 back / 1.6 up from the feet, which on a ~1.8 m character sits inside the head and shoulders.
static const float BR_SPECTATE_BOOM_BACK = 3.5;
static const float BR_SPECTATE_BOOM_UP = 0.4;
//sphere radius for the boom's collision trace, and how far to pull back off a contact point.
static const float BR_SPECTATE_CAM_RADIUS = 0.25;
static const float BR_SPECTATE_CAM_SKIN = 0.30;
//minimum clearance above the surface, so the camera never ends up underground behind a prone target.
static const float BR_SPECTATE_FLOOR_CLEARANCE = 0.35;
//fixed downward tilt. Deliberately a constant and never the target's own aim angle:
//GetCommandModifier_Weapons() can be NULL for a remote entity on a client, in which case
//GetBaseAimingAngleUD() returns 0.0 and the camera silently goes flat with nothing in the log.
static const float BR_SPECTATE_PITCH = -10.0;
//damping rates. Higher is snappier; these are multiplied by the frame time and clamped to 1.
static const float BR_SPECTATE_YAW_DAMP = 6.0;
static const float BR_SPECTATE_POS_DAMP = 8.0;
//beyond this distance the camera teleports instead of interpolating, so a retarget across the map
//does not fly the camera over the terrain. Metres.
static const float BR_SPECTATE_SNAP_DISTANCE = 30.0;
//the no-target fallback: a slow orbit of the current play area centre.
static const float BR_SPECTATE_ORBIT_RADIUS = 120.0;
static const float BR_SPECTATE_ORBIT_HEIGHT = 60.0;
static const float BR_SPECTATE_ORBIT_PITCH = -25.0;
static const float BR_SPECTATE_ORBIT_DEG_PER_SEC = 4.0;


//--- VANILLA HUD NOTIFIERS. Extra/PreventPlayerModifiers already neuters ThirstMdfr, HungerMdfr and
//--- the heat-comfort/disease modifiers - their OnTick bodies return immediately - so the thirst,
//--- hunger and temperature notifiers never move for the whole match. They are pinned decoration,
//--- so IngameHud hides them along with the divider that would be left dangling beside Blood.
//---
//--- Compile-time on purpose: this is a client-side cosmetic choice, and the settings files are
//--- server-side only, so an admin toggle would need a new field, an RPC and a BattleRoyaleRPC
//--- member for something a rebuild flips in one line. Set false to get vanilla's HUD back.
static const bool BR_HIDE_SURVIVAL_NOTIFIERS = true;
//--- Where the badge group goes once the three notifiers are gone. Vanilla puts BadgesSpacer at 213
//--- and BadgesPanel at 252, measured from the RIGHT edge of HudPanel (everything there is
//--- halign right_ref), which leaves a ~174 px hole between Blood and the first badge. Both are
//--- shifted right by 143 px, which lands BadgeNotifierDivider exactly on the x of 86 that the
//--- hidden NotifierDivider used to occupy and preserves vanilla's 4 px spacer-to-panel gap.
//--- Absolute rather than a delta so re-applying them after a respawn is idempotent.
static const float BR_HUD_BADGES_SPACER_X = 70.0;
static const float BR_HUD_BADGES_PANEL_X = 109.0;


//--- THE PLAYERS/GROUPS COUNTER. The SetPlayerCount RPC carries two plain integers, and the group
//--- one doubles as a two-value enum for the cases where there is no number to show. The contract
//--- lived as a pair of bare literals in the sender (0_BattleRoyaleState.OnPlayerCountChanged) and
//--- the reader (BattleRoyaleHud.SetCount) with a comment in each, which is how -2 came to be
//--- unreachable without anyone noticing - see the note on BR_HUD_GROUPS_NONE.
//
//there is no group figure to show, so the client hides the group panel outright. Reached whenever
//the party system is not in play: the addon compiled out, or its manager disabled in
//party_settings.json. **That second case used to be missed**, and it is the reason this sentinel
//needs a name: the sender only ever produced -2 under #ifdef !VIGRID_PARTY, and since Party ships
//in this repo that branch is always compiled out. A server that turned parties off at runtime got
//VigridPartyAPI.GetGroupCount()'s solo-groups fallback instead - one group per player, i.e. a
//figure always identical to the player count - presented under a group icon as if it meant
//something. Ask VigridPartyAPI.IsReady() before believing a group count.
static const int BR_HUD_GROUPS_NONE = -2;
//the figure exists but is deliberately withheld: the client shows "???" in its place. Driven by
//hide_players_endgame (general_settings.json) once the match is down to BR_HUD_ENDGAME_PLAYERS.
static const int BR_HUD_GROUPS_CONCEALED = -1;
//what counts as "the endgame" for that concealment. Deliberately a player count rather than a state
//or a round index: the point is to stop the last few survivors reading the exact team composition
//off the HUD, and that becomes possible at a population, not at a phase.
static const int BR_HUD_ENDGAME_PLAYERS = 10;


//--- zoning subsystem
//
//Circles are generated SMALLEST FIRST: index 0 is the tight final circle and each later one must
//CONTAIN it, so a step may move the centre by at most (r_i - r_{i-1}) and still stay contained.
//
//The whole search rests on one geometric fact: the world-fit boxes [r_i, W - r_i]^2 are nested and
//all share the map centre, so stepping the maximum allowed straight toward that centre is provably
//the optimal chain, not a heuristic. That makes "can this position still be extended to a full set
//of circles" answerable in pure arithmetic (BattleRoyaleZone.CanChainComplete), and it means every
//accepted circle has a guaranteed next step available - the witness step. Placement therefore
//cannot dead-end, and the old RequestExit-the-server failure path is gone rather than made rarer.
//
//Reach the oracle and the witness step plan on. Containment only needs d <= r_i - r_{i-1}; this is
//how much of that legal reach the chain is allowed to count on. It is the single biggest lever on
//how many POIs are usable as a final zone: raising it 0.75 -> 0.95 grows the usable-POI disc on an
//8192 m map from 3559 m to 3895 m of radius.
static const float BR_ZONE_REACH_PERCENT = 0.95;

//Tier 1 is the normal match, and a healthy generation never leaves it. MAX raised 0.75 -> 0.85:
//a quarter of the legal reach was being discarded for free, and circles sitting further off-centre
//inside their parent is better battle-royale design anyway - it creates rotation pressure. Kept
//below BR_ZONE_REACH_PERCENT so an ordinary step never hugs the containment boundary.
static const float DAYZBR_ZS_MIN_DISTANCE_PERCENT = 0.25; //min next zone distance as a percent of maximum distance (1 => 100%)
static const float DAYZBR_ZS_MAX_DISTANCE_PERCENT = 0.85; //max next zone distance as a percent of maximum distance (1 => 100%)
static const float BR_ZONE_T1_ARC_DEG  = 45.0;  //half-width of the cone pointed at the map centre
static const float BR_ZONE_T1_LAND_MIN = 0.60;  //overridden per-map by zone_settings.zone_min_land_fraction
static const int   BR_ZONE_T1_ROLLS    = 24;

//Tier 2 unlocks only when tier 1 found nothing at all, so a normal match never sees it.
static const float BR_ZONE_T2_MIN_PCT  = 0.35;
static const float BR_ZONE_T2_MAX_PCT  = 0.95;
static const float BR_ZONE_T2_ARC_DEG  = 90.0;
static const float BR_ZONE_T2_LAND_MIN = 0.35;
static const int   BR_ZONE_T2_ROLLS    = 24;

//Tier 3 is "anywhere legal at all" - any direction, almost any distance, barely any land.
static const float BR_ZONE_T3_MIN_PCT  = 0.05;
static const float BR_ZONE_T3_MAX_PCT  = 0.99;
static const float BR_ZONE_T3_ARC_DEG  = 180.0;
static const float BR_ZONE_T3_LAND_MIN = 0.10;
static const int   BR_ZONE_T3_ROLLS    = 24;
static const int   BR_ZONE_TIER_COUNT  = 3;

//One deterministic sweep after the random tiers. This is NOT the primary mechanism and must not
//become it: accept-first pays ~2 rolls in the common case where a sweep pays all 96 every time,
//determinism would remove the match-to-match variety, and a deterministic best-pick would break
//backtracking outright - re-rolling a parent only makes progress if it can return something else.
//It is a completeness oracle, so that backtracking is triggered by evidence that the parent really
//is a dead end rather than by another few hundred random rolls proving the same thing slowly.
static const int   BR_ZONE_SWEEP_ANGLES    = 24;
static const int   BR_ZONE_SWEEP_DISTANCES = 4;

//Land sampling. Rings sit at equal-AREA radii, so the outer band - which is most of a big circle -
//is not under-sampled. 1 + 2*6 = 13 SurfaceIsSea calls per large candidate.
static const int   BR_ZONE_LAND_RINGS    = 2;
static const int   BR_ZONE_LAND_PER_RING = 6;
//At or below this radius the strict single-point test is kept instead: a 35 m circle centred 20 m
//offshore really is unplayable, where a 3375 m one that is 90% dry is completely fine.
static const float BR_ZONE_SMALL_CIRCLE_R = 200.0;

//How far a final circle may sit from the village it was seeded on, in metres. end_in_villages means
//"centred within this of the town's CfgWorlds point", not "somewhere in the town".
static const float BR_ZONE_POI_JITTER_M = 10.0;

//Search budgets. Every one of these bounds WORK only - none of them can cause a failure, because
//the witness step at the end of TryPlaceLevel cannot be rejected.
static const int   BR_ZONE_LEVEL_RETRIES = 3;   //attempts at a level before it takes the witness step
static const int   BR_ZONE_SEED_WORK     = 40;  //placements before abandoning a POI for a different one
static const int   BR_ZONE_MAX_SEEDS     = 8;
static const int   BR_ZONE_SEED_ROLLS    = 64;

//Adaptive draw. pressure = (centre-ward travel this step MUST make) / (reach available to it).
//0 keeps the original spread; 1 pins the draw to the top of the tier's window and narrows the cone.
//Reach is spent only when the chain still owes ground, so the common case is unchanged.
static const float BR_ZONE_PRESSURE_BIAS        = 1.0;
static const float BR_ZONE_PRESSURE_ARC_TIGHTEN = 0.7;

//Extra round seconds granted when a circle lands far from its parent. NOTE the old threshold of
//1500 m was DEAD at the shipped sizes: the longest possible step is 0.85 * 1175 = 999 m, so
//s_PlayAreaDurationOffsets has always been all zeros and the feature never once fired. 600 m makes
//it real; the cap stops a single long step adding minutes to a round.
static const float BR_ZONE_OFFSET_MIN_DISTANCE = 600.0;
static const float BR_ZONE_OFFSET_SPEED_MPS    = 6.0;
static const float BR_ZONE_OFFSET_MAX_SECONDS  = 120.0;

//Self test. Generates full chains headlessly and reports the failure/backtrack/tier distribution,
//which is what turns "relaunch the server twenty times and hope" into a number.
static const int   BR_ZONE_SELFTEST_DEFAULT_RUNS = 50;
static const int   BR_ZONE_SELFTEST_WORK_CAP     = 20000;


//---- DayZ Expansion Loading Screens
static const string DAYZBR_LOADING_SCREENS_PATH     = "Vigrid-BattleRoyale/Data/LoadingScreens.json";
static const string DAYZBR_LOADING_MESSAGES_PATH    = "Vigrid-BattleRoyale/Data/LoadingMessages.json";
static const int DAYZBR_LOADING_BAR_COLOR           = ARGB( 255, 0, 0, 0 );     //! A = Alpha (opacity) / R = Red / G = Green / B = Blue
