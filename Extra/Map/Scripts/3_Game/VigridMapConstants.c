/**
 *  Vigrid Map - constants. No guard: compiles on both client and server.
 *
 *  Several values here are deliberate copies of Party's, not references to them. The discipline
 *  rule keeps this addon free of Party symbols outside VigridMapTeam, so the crosshair-fade shape
 *  is duplicated rather than shared. Where a value is copied, the source is named so the two can be
 *  compared by eye when either is tuned.
 */

//--- Asset prefix. The ONLY hard-coded path in the addon - build every layout path from it.
//--- CI1.bat derives each PBO's prefix as <PrefixLinkRoot>\<folder>, so this tracks the folder.
static const string VIGRID_MAP_PREFIX = "Vigrid-BattleRoyale/Extra/Map/";

//--- Menu id. Vanilla stops at 46; the Battle Royale spawn selection uses 75, the party menu 176
//--- and the leaderboard 177.
static const int MENU_VIGRID_MAP = 178;

//--- Logging. Diag builds are noisy by default, release builds are silent but for errors.
#ifdef DIAG
#define VIGRID_MAP_TRACE_ENABLED
#endif

#ifdef VIGRID_MAP_TRACE_ENABLED
    static const int VIGRID_MAP_LOG_LEVEL = 4; // Trace
#else
    static const int VIGRID_MAP_LOG_LEVEL = 0; // Error only
#endif

//--- Settings. The addon's own profile folders, deliberately not the host mod's, so that
//--- extraction does not strand the files behind. The client prefs file lives in the CLIENT
//--- profile directory - which is exactly why it cannot live in the server settings file.
static const string VIGRID_MAP_SETTINGS_FOLDER = "$profile:Vigrid-Map\\";
static const string VIGRID_MAP_SETTINGS_FILE = "$profile:Vigrid-Map\\map_settings.json";
static const string VIGRID_MAP_PREFS_FILE = "$profile:Vigrid-Map\\map_client.json";

//--- Inputs. Resolved by name with GetUApi().GetInputByName rather than through the generated
//--- constants, which come from another PBO's Inputs.xml and may not exist at compile time.
static const string VIGRID_MAP_INPUT_TOGGLE = "UAVigridMapToggle";
static const string VIGRID_MAP_INPUT_MINIMAP = "UAVigridMapMinimapToggle";

//--- There is deliberately NO input exclude group here, and adding one is a trap - see
//--- VigridMapMenu.SuppressGameplayInputs for the whole story. The map suppresses individual UApi
//--- inputs per frame instead.

//--- RPC. CF dispatches by METHOD NAME, so every handler method must be named exactly its string.
static const string RPC_VIGRIDMAP_NAMESPACE = "RPC-VigridMap";               // server -> client
static const string RPC_VIGRIDMAP_SERVER_NAMESPACE = "RPC-VigridMap-Server"; // client -> server

static const string VM_RPC_SETTINGS = "VM_Settings";
static const string VM_RPC_MARKERS = "VM_Markers";
static const string VM_RPC_REJECTED = "VM_Rejected";
static const string VM_RPC_PLACE = "VM_Place";
static const string VM_RPC_REMOVE = "VM_Remove";
static const string VM_RPC_REQUEST_SYNC = "VM_RequestSync";

//--- Server settings defaults, mirrored on the client until the first VM_Settings push arrives.
static const bool VIGRID_MAP_DEF_MARKERS_ENABLED = true;
static const bool VIGRID_MAP_DEF_MINIMAP_ALLOWED = false;
static const int VIGRID_MAP_DEF_LABEL_MAX = 32;
static const int VIGRID_MAP_PLACE_COOLDOWN_MS = 250;

//--- How long an optimistically-drawn marker survives without the server echoing it back. Two
//--- seconds is long enough for a round trip on a bad connection and short enough that a rejected
//--- placement does not linger looking accepted.
static const int VIGRID_MAP_PENDING_TTL_MS = 2000;

//--- How often the server recomputes every player's visible marker set. This is what makes a
//--- player who joined a party mid-match start seeing their teammates' markers without Party
//--- having to notify this addon.
static const int VIGRID_MAP_RESYNC_INTERVAL_MS = 5000;

//--- Oval rendering. n = PI*sqrt(r) holds the sagitta at half a pixel for any radius, so the
//--- segment count scales with on-screen size instead of being fixed. Copied from the Battle
//--- Royale spawn-selection renderer, which arrived at the same bounds.
static const int VIGRID_MAP_OVAL_MIN_SEGMENTS = 16;
static const int VIGRID_MAP_OVAL_MAX_SEGMENTS = 180;

//--- Distance, in metres, between the two points used to probe the map's world->screen transform.
//--- Large enough that float error in the difference is negligible, small enough to stay inside a
//--- sane world.
static const float VIGRID_MAP_PROBE_DISTANCE = 1000.0;

//--- Repaint even when the transform has not moved, so a canvas that lost its draw list for any
//--- reason recovers within a second rather than staying blank.
static const int VIGRID_MAP_REPAINT_WATCHDOG_MS = 1000;

//--- Zone colours. The same blue and white the Expansion map integration used, so the change is
//--- invisible to players who were already reading those circles.
static const int VIGRID_MAP_COLOR_CURRENT_ZONE = 0xFF3C82FF;  // ARGB(255, 60, 130, 255)
static const int VIGRID_MAP_COLOR_NEXT_ZONE = 0xFFFFFFFF;     // ARGB(255, 255, 255, 255)
static const int VIGRID_MAP_COLOR_NEXT_LINE = 0xDC3C82FF;     // ARGB(220, 60, 130, 255)
static const int VIGRID_MAP_COLOR_OWN_MARKER = 0xFFFFFFFF;
//--- A teammate's marker normally takes their party slot colour. This is what VigridMapTeam answers
//--- with when there is no palette to ask at all - Party not installed - so it is the colour every
//--- non-own marker takes in that build. With Party present, a slot of -1 reads as the palette's own
//--- off-white instead, matching how a party ping from an unknown owner is drawn.
static const int VIGRID_MAP_COLOR_TEAM_MARKER = 0xFF7FD4FF;

//--- Circle line width and the side of the square drawn at a zone centre. A 6 px "circle" is
//--- indistinguishable from a square, and CanvasWidget has no fill primitive.
static const float VIGRID_MAP_ZONE_LINE_WIDTH = 2.0;
static const float VIGRID_MAP_CENTER_DOT_PX = 6.0;

//--- Dashed line from the player to the next zone centre. The period is in screen pixels so the
//--- dashes stay the same size at every zoom; the line is clipped to the canvas before being
//--- dashed, because at maximum zoom-in an unclipped 5 km line is ~50 000 px.
static const float VIGRID_MAP_DASH_ON_PX = 12.0;
static const float VIGRID_MAP_DASH_OFF_PX = 8.0;
static const float VIGRID_MAP_DASH_WIDTH = 2.0;
static const int VIGRID_MAP_DASH_MAX_SEGMENTS = 200;

//--- Map screen. Scale runs 0 (fully zoomed in) to 1 (fully out); vanilla opens its map at 0.33.
static const float VIGRID_MAP_DEF_SCALE = 0.20;

//--- Zoom limits. The engine's own wheel zoom bottoms out far closer than is useful - past this
//--- the map is a handful of contour lines with no landmark in frame, which is disorienting rather
//--- than detailed. There is no zoom event to hook, so the menu clamps GetScale every frame.
//--- Raise the floor to allow less zoom-in, lower it to allow more.
static const float VIGRID_MAP_MIN_SCALE = 0.06;
static const float VIGRID_MAP_MAX_SCALE = 1.0;
static const int VIGRID_MAP_CLICK_DEBOUNCE_MS = 250;

//--- The MapWidget ignores SetMapPos until it has been laid out, so the initial centring is
//--- re-issued from the call queue one frame batch later. Copied from the spawn-selection menu,
//--- which needed exactly this.
static const int VIGRID_MAP_CENTER_DELAY_MS = 100;

//--- 2D marker, drawn on the canvas. In screen pixels, not metres: a marker annotates a point, it
//--- has no extent in the world, so it should not grow when the map is zoomed in.
static const float VIGRID_MAP_MARKER_PX = 18.0;
static const float VIGRID_MAP_MARKER_LINE_WIDTH = 2.0;

//--- Teammates, party pings and the local player, all drawn on their own canvas.
//---
//--- Three glyphs that must never be confused with each other or with a placed marker, drawn with
//--- nothing but DrawLine. The map already spends the circle twice - zone rings and the
//--- ring-and-cross marker - so both new shapes are straight-edged, and the pair that could still
//--- collide at small size, triangle and diamond, is separated on three axes at once: three
//--- vertices against four, a heavy stroke against a light one, and full opacity against 0.75. That
//--- weight difference carries the meaning too - a teammate is a fact, a ping is a callout.
//---
//--- If triangle and diamond ever do read alike when fully zoomed out, the tested alternative is a
//--- six-pointed asterisk for pings: three lines crossing at a common centre, no enclosed area at
//--- all, so it cannot be mistaken for any polygon on the map.
//---
//--- Sizes are screen pixels, not metres, for the same reason as the marker above.
static const float VIGRID_MAP_TEAM_PX = 14.0;
static const float VIGRID_MAP_TEAM_LINE_WIDTH = 2.0;
static const float VIGRID_MAP_PING_PX = 12.0;
//--- Was 1.0, as one of three axes separating a ping from a teammate (vertex count, stroke weight,
//--- opacity). Raised to match the triangle after 2026-08-08 testing: at 12 px a 1 px stroke is not
//--- reliably visible over satellite imagery, and an invisible glyph separates from nothing. The
//--- other two axes still carry the distinction - 4 vertices vs 3, and 0.75 alpha vs 1.0 - so do
//--- NOT also raise VIGRID_MAP_PING_ALPHA, which is now the only non-shape separator left.
static const float VIGRID_MAP_PING_LINE_WIDTH = 2.0;
static const float VIGRID_MAP_SELF_PX = 10.0;
static const float VIGRID_MAP_SELF_LINE_WIDTH = 2.0;
static const int VIGRID_MAP_COLOR_SELF = 0xFFFFFFFF;

//--- Copy of VIGRID_PARTY_PING_BASE_ALPHA, so a ping reads at the same weight on the map as it does
//--- in the world.
static const float VIGRID_MAP_PING_ALPHA = 0.75;

//--- Copy of the x0.5 the party name tags apply to a stale teammate.
static const float VIGRID_MAP_TEAM_STALE_ALPHA = 0.5;

//--- The team layer repaints on a timer instead of on an edge, because there is no edge to catch:
//--- Party's roster sequence moves when the party changes shape, never when somebody walks, and
//--- positions are interpolated continuously between pushes. 10 Hz is the same rate, and the same
//--- argument, as VIGRID_MAP_MINIMAP_TICK_MS below - member state arrives at 2 Hz, a sprinter covers
//--- about 0.6 m in 100 ms, and at the default scale that is roughly one pixel.
static const int VIGRID_MAP_TEAM_TICK_MS = 100;

//--- 3D marker. Height offset lifts the tag off the ground so it is not buried in terrain.
static const float VIGRID_MAP_MARKER_HEIGHT_OFFSET = 1.5;
static const float VIGRID_MAP_MARKER_EDGE_MARGIN = 40.0;
static const float VIGRID_MAP_MARKER_SIZE_W = 90.0;  // fallback if GetScreenSize returns 0
static const float VIGRID_MAP_MARKER_SIZE_H = 36.0;  // must track map_marker_3d.layout

//--- Distance fade. A marker never fades out completely with distance: unlike a ping it is a
//--- standing plan, and a plan you cannot see is not a plan.
static const float VIGRID_MAP_MARKER_FADE_DISTANCE = 1500.0;
static const float VIGRID_MAP_MARKER_MIN_ALPHA = 0.55;

//--- Crosshair fade, so a marker cannot hide someone standing between you and it. Copied from
//--- VigridPartyScreen.CrosshairFade's call site in VigridPartyPings - same shape, so markers and
//--- pings fade alike. Fractions of screen HEIGHT, worked in pixels, so the dead zone stays
//--- circular on 16:9 instead of the ellipse a normalised radius would give.
static const float VIGRID_MAP_MARKER_CENTER_HIDE = 0.05;
static const float VIGRID_MAP_MARKER_CENTER_FADE = 0.13;
static const float VIGRID_MAP_MARKER_CENTER_MIN_ALPHA = 0.15;

//--- Above this many metres the on-screen distance reads in kilometres.
static const float VIGRID_MAP_KM_THRESHOLD = 1000.0;

//--- Minimap. Ticked on a timer rather than every frame - it follows the camera, but at 200 px
//--- across nobody can see the difference between 10 Hz and 60 Hz.
static const int VIGRID_MAP_MINIMAP_TICK_MS = 100;
static const float VIGRID_MAP_MINIMAP_SCALE = 0.10;

//--- The heading dart marking the local player. Drawn on the canvas rather than being a rotated
//--- ImageWidget, so there is no icon rest-angle constant to get wrong - see WorldRenderHeadingArrow.
//--- Larger than VIGRID_MAP_SELF_PX because the minimap is only 200 px across and this is the one
//--- glyph the player looks for.
static const float VIGRID_MAP_MINIMAP_ARROW_PX = 14.0;

//--- Markers on the minimap are plain dots, not the fullscreen map's ring-and-cross: at this size a
//--- ring closes into a blob and the cross reads as noise.
static const float VIGRID_MAP_MINIMAP_MARKER_PX = 7.0;

//--- Stringtable keys. Held with the leading '#' so a widget localises them itself.
static const string STR_VIGRID_MAP_MARKERS_OFF = "#STR_MAP_MARKERS_OFF";
