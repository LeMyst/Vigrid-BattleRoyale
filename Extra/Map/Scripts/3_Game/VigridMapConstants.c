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
static const string VIGRID_MAP_INPUT_COMPASS = "UAVigridMapCompassToggle";

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
//--- Unlike the minimap's, this default is permissive: the compass is meant to be there unless an
//--- admin takes it away, so a client that has not yet had VM_Settings shows it rather than
//--- flickering it in a second later.
static const bool VIGRID_MAP_DEF_COMPASS_ALLOWED = true;
static const int VIGRID_MAP_PLACE_COOLDOWN_MS = 250;

//--- How long an optimistically-drawn marker survives without the server echoing it back. Two
//--- seconds is long enough for a round trip on a bad connection and short enough that a rejected
//--- placement does not linger looking accepted.
//---
//--- It is a BACKSTOP, not the normal exit. Every refusal the server can reach now answers with a
//--- VM_Rejected (see MapMissionServer.RejectRequest), so reaching this timeout means the request
//--- went unanswered entirely.
static const int VIGRID_MAP_PENDING_TTL_MS = 2000;

//--- What the client has asked the server for and is drawing ahead of the answer.
//---
//--- Plain ints rather than an enum to match the rest of this file, and because the values are only
//--- ever compared - nothing switches on them.
//---
//--- PLACE covers both placing and MOVING: the store keys one marker per owner, so a second click is
//--- a move, and the optimistic draw has to override the confirmed position rather than sit beside
//--- it. REMOVE is the mirror - it suppresses a confirmed marker that the server has not dropped
//--- yet. Without those two the only interaction that felt instant was the very first click of a
//--- session, which is the one a player makes least often.
static const int VIGRID_MAP_PENDING_NONE = 0;
static const int VIGRID_MAP_PENDING_PLACE = 1;
static const int VIGRID_MAP_PENDING_REMOVE = 2;

//--- How close the server's echo must be to the position we asked for before a pending PLACE counts
//--- as confirmed, in metres.
//---
//--- A tolerance rather than equality only because the position makes a float round trip; the server
//--- does not adjust it. VigridMapMarkerStore.Place REJECTS a position outside the world rather than
//--- clamping it, so the echo is arithmetically the same value that was sent. A metre is far below
//--- the distance between two distinguishable clicks at any zoom.
static const float VIGRID_MAP_PENDING_MATCH_EPSILON_M = 1.0;

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
static const int VIGRID_MAP_COLOR_ZONE_LINE = 0xDC3C82FF;     // ARGB(220, 60, 130, 255)
//--- Hot zones. Red, because it is the one hue the play-area blue and white cannot be confused with,
//--- and drawn UNDER them so a hot zone never obscures the circle a player has to run to. Slightly
//--- translucent for the same reason.
static const int VIGRID_MAP_COLOR_HOT_ZONE = 0xC8FF3232;       // ARGB(200, 255, 50, 50)
//--- Its fill. Low alpha on purpose: this is a tint over terrain the player is still reading, not a
//--- block of colour. Matches BR_HOT_ZONE_FILL_COLOR on the spawn-selection screen so a circle looks
//--- the same in both places.
static const int VIGRID_MAP_COLOR_HOT_ZONE_FILL = 0x3CFF3232;  // ARGB(60, 255, 50, 50)
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

//--- Dashed line from the player to the near edge of the circle they have to reach - the next zone
//--- when there is one, the current zone otherwise - and drawn only while they are outside it. It
//--- stops at the ring rather than running on to the centre, so its length IS the distance still to
//--- cover. The period is in screen pixels so the dashes stay the same size at every zoom; the line
//--- is clipped to the canvas before being dashed, because at maximum zoom-in an unclipped 5 km
//--- line is ~50 000 px.
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

//--- Deliberately LONGER than VIGRID_MAP_PLACE_COOLDOWN_MS, so the client is the stricter of the two
//--- gates and a click that survives it is one the server will accept.
//---
//--- They used to be equal, which made the ordering depend on latency: the server measures the gap
//--- between ARRIVALS, so two clicks 250 ms apart arrive 250 + (rtt2 - rtt1) apart and any jitter
//--- towards zero put the second one inside the cooldown. It was then dropped silently, and the only
//--- exit was the pending TTL. The margin does not make that impossible - a 50 ms swing still
//--- inverts it - so it is a reduction in frequency, not a fix. The fix is that a refused place now
//--- answers with a VM_Rejected the client can act on.
static const int VIGRID_MAP_CLICK_DEBOUNCE_MS = 300;

//--- How long a refusal stays on screen. Long enough to read a sentence without hunting for it,
//--- short enough that it is gone before the player wonders whether it is stuck.
static const int VIGRID_MAP_TOAST_MS = 4000;

//--- The MapWidget ignores SetMapPos until it has been laid out, so the initial centring is
//--- re-issued from the call queue one frame batch later. Copied from the spawn-selection menu,
//--- which needed exactly this.
//---
//--- This is now the BACKSTOP rather than the mechanism - VigridMapMenu.SettleView re-issues the
//--- view every frame until a readback proves it took, which lands far sooner. 100 ms is ~6 frames
//--- at 60 fps, and those 6 frames are exactly the "map opens off-centre, then jumps" artefact.
static const int VIGRID_MAP_CENTER_DELAY_MS = 100;

//--- Settling the initial view. The retry is capped so a widget that never reports a size cannot
//--- hold the map hostage; past the cap the view is accepted and the delayed backstop above still
//--- runs, which is precisely the behaviour that shipped before SettleView existed.
static const int VIGRID_MAP_SETTLE_MAX_FRAMES = 30;

//--- How close the read-back centre must be to count as "the SetMapPos took". Generous on purpose:
//--- this is distinguishing "landed" from "was dropped and left at the engine default", which is a
//--- whole-map distance apart, not a rounding difference.
static const float VIGRID_MAP_SETTLE_EPSILON_M = 5.0;

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
//--- The local player, drawn as the same heading dart the minimap uses (was an axis-aligned plus at
//--- 10 px until 2026-08-11 - it could not answer "which way am I facing", which is the question a
//--- map gets opened for). Raised to 16 for two reasons: a dart needs more pixels than a plus before
//--- its direction is readable, and "you" should stay the easiest glyph to find on a big map, which
//--- was the plus's one real virtue. 16 also keeps it clear of VIGRID_MAP_TEAM_PX (14), now the
//--- closest silhouette on the map.
//---
//--- The dart is PURE yellow, and that matters: it was white until 2026-08-15, chosen then because
//--- VigridPartyPalette contains no white and so colour separated you from a teammate on its own.
//--- Yellow gives that up - the nearest palette entry is slot 0's amber (242,199,68), which is only
//--- distinguishable because it is desaturated and darker. Separation is now carried mainly by
//--- silhouette (a notched dart against a triangle) and size (16 against 14). Moving a palette entry
//--- towards pure yellow would erode what is left of it.
static const float VIGRID_MAP_SELF_PX = 16.0;
static const float VIGRID_MAP_SELF_LINE_WIDTH = 2.0;
static const int VIGRID_MAP_COLOR_SELF = 0xFFFFFF00;

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

//--- ADMIN PLAYER LAYER. An arbitrary set of named, coloured people the host mod asked to be plotted
//--- (VigridMapAPI.SetAdminPlayers) - in practice an admin spectator's overview of a whole match.
//---
//--- 12 px, deliberately smaller than the teammate triangle's 14 and the self dart's 16. There can be
//--- sixty of these against at most a handful of teammates, so the layer has to stay quiet enough
//--- that the map underneath it is still readable.
static const float VIGRID_MAP_ADMIN_PX = 12.0;
static const float VIGRID_MAP_ADMIN_LINE_WIDTH = 2.0;

//--- Bound on pooled name labels, matching the host's own 64-row admin list cap. Reached in the
//--- ordinary course of a full match, not just in pathological cases.
static const int VIGRID_MAP_ADMIN_NAME_MAX = 64;

//--- Pixels between the top of a glyph and the bottom of its name label.
static const float VIGRID_MAP_ADMIN_NAME_GAP_PX = 3.0;

//--- Fallback label size in real pixels, used only on the first frame before the pooled widget has
//--- been shown and can report its own. MUST track the root size in map_admin_tag.layout.
static const float VIGRID_MAP_ADMIN_NAME_W = 120.0;
static const float VIGRID_MAP_ADMIN_NAME_H = 16.0;

//--- Names are hidden below this zoom. At a wide zoom sixty labels overlap into an unreadable smear
//--- and the glyphs alone carry the picture; the names come back as soon as the admin zooms into the
//--- fight they care about. The glyphs themselves are never hidden.
static const float VIGRID_MAP_ADMIN_NAME_MIN_SCALE = 0.14;

//--- Throttle on the one funnel diagnostic this layer emits. It runs at the team layer's 10 Hz, so
//--- without a throttle it would be six hundred lines a minute for as long as a map is open.
static const int VIGRID_MAP_ADMIN_FUNNEL_MS = 2000;

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

//--- The same heading dart as the fullscreen map's, and the same drawn-not-rotated reasoning - see
//--- WorldRenderHeadingArrow. SMALLER than VIGRID_MAP_SELF_PX only because the minimap is 200 px
//--- across: 16 px of dart on it would crowd out the zone rings it is meant to be read against.
static const float VIGRID_MAP_MINIMAP_ARROW_PX = 14.0;

//--- Markers on the minimap are plain dots, not the fullscreen map's ring-and-cross: at this size a
//--- ring closes into a blob and the cross reads as noise.
static const float VIGRID_MAP_MINIMAP_MARKER_PX = 7.0;

//--- Compass strip. Every length here is in SCREEN PIXELS - the strip annotates the camera, not the
//--- world, so nothing about it scales with zoom or distance.
//---
//--- The window is what makes the strip readable: 90 degrees across 620 px is ~6.9 px per degree, so
//--- a 15-degree tick spacing lands them ~103 px apart and the eight named directions are never all
//--- on screen at once. Widening the window (or narrowing the strip) crowds them; narrowing it makes
//--- the strip swing alarmingly fast for a small turn.
//---
//--- The height is three stacked lanes and they are packed tight: ticks 0-10, labels 10-38, carets
//--- 38-42. There is deliberately NO slack at the bottom - an earlier 40 px strip left 11 px of empty
//--- backdrop under the labels, which reads as a misaligned box whenever no caret happens to be up.
//--- The label lane is the one that sets the height, so raising the cardinal text size grows it.
//---
//--- TOP is 0 so the strip sits flush against the top of the screen. Nothing may overhang above it
//--- any more, which is why the cursor's overhang is gone.
//---
//--- These are the AUTHORITY, not the layout. Every widget in compass.layout is positioned and sized
//--- from script against the measured root size, so the numbers declared there are placeholders that
//--- only have to look right in Workbench. See compass.layout's header for why it works that way.
//--- The screen width every constant in this block is authored against. VigridMapCompass divides the
//--- measured viewport by it and scales everything through, so the strip is the same fraction of the
//--- display at 1280 as at 2560. 1920 is chosen because it is also the reference the ENGINE uses when
//--- it scales a widget's declared geometry, so the two agree.
static const float VIGRID_MAP_COMPASS_REFERENCE_W = 1920.0;

static const float VIGRID_MAP_COMPASS_WIDTH = 620.0;
static const float VIGRID_MAP_COMPASS_HEIGHT = 42.0;
static const float VIGRID_MAP_COMPASS_TOP = 0.0;
static const float VIGRID_MAP_COMPASS_WINDOW_DEG = 90.0;

//--- The reading edge, and the furniture under it.
//---
//--- The cursor deliberately stops at the LABEL LANE rather than spanning the strip. A full-height
//--- bar is more striking, and it draws straight through the label of whichever mark is currently
//--- under it - so the one label the player is actually reading is the one the cursor obscures. It
//--- only ever has to mark a position among the TICKS, which is where the precision is. It is a
//--- pixel wider and two taller than a major tick, which is what separates it from one.
static const float VIGRID_MAP_COMPASS_CURSOR_W = 3.0;
static const float VIGRID_MAP_COMPASS_CURSOR_H = 12.0;
static const float VIGRID_MAP_COMPASS_READOUT_W = 120.0;
static const float VIGRID_MAP_COMPASS_READOUT_H = 22.0;
static const float VIGRID_MAP_COMPASS_READOUT_GAP = 4.0;

//--- The entry pool is indexed BY BEARING, not by visible slot: entry i is permanently the
//--- i*15-degree mark. That is why its label is set once at creation and never again - only SetPos,
//--- SetAlpha and Show run per frame. A slot pool would have to re-localise a stringtable key for
//--- every visible tick, every frame, for no gain.
static const int VIGRID_MAP_COMPASS_STEP_DEG = 15;
static const int VIGRID_MAP_COMPASS_ENTRY_COUNT = 24;  // 360 / STEP_DEG
static const float VIGRID_MAP_COMPASS_ENTRY_W = 48.0;
static const float VIGRID_MAP_COMPASS_ENTRY_H = 42.0;

//--- Three tick weights for the three kinds of mark: a named direction every 45 degrees, a numeric
//--- degree label every 30, and an unlabelled tick every 15.
static const float VIGRID_MAP_COMPASS_TICK_MAJOR_H = 10.0;
static const float VIGRID_MAP_COMPASS_TICK_MEDIUM_H = 7.0;
static const float VIGRID_MAP_COMPASS_TICK_MINOR_H = 4.0;
static const float VIGRID_MAP_COMPASS_TICK_W = 2.0;

//--- The label lane inside an entry, below the tallest tick. It is sized to the LARGEST text tier
//--- below, since the label is vertically centred in it - undersize it and the cardinals clip.
static const float VIGRID_MAP_COMPASS_LABEL_Y = 10.0;
static const float VIGRID_MAP_COMPASS_LABEL_H = 28.0;

//--- THE THREE TEXT SIZES ARE NOT HERE, AND CANNOT BE. A widget's glyph size is fixed by the FONT
//--- FACE it declares, and there is no SetFont to change one from script - so the tiers live in
//--- compass_entry.layout as three separate label widgets (metron-bold28 / -bold22 / -bold14), of
//--- which VigridMapCompass.PickLabel shows exactly one per entry.
//---
//--- MEASURED 2026-08-11, DO NOT RETRY: TextWidget.SetTextExactSize does nothing here. Asking for
//--- 28 / 18 / 13 on a single widget rendered 28 / 28 / 28 under GetTextSize, so all three tiers came
//--- out identical in game. The tell was there beforehand and was missed - SetTextExactSize has ONE
//--- call site in all of P:\scripts (tutorialsmenu.c:291), the same shape as OverrideAimChangeX/Y,
//--- which also compiled, ran and silently did nothing.

//--- An unlabelled tick is scenery, not information, so it sits back from the ones that are read.
static const float VIGRID_MAP_COMPASS_MINOR_ALPHA = 0.55;

//--- Everything fades out over the last few degrees of the window rather than relying on the strip
//--- to clip it. `clipchildren 1` IS set on the container, but clipping of absolutely-positioned
//--- children is unproven on this codebase - the fade makes it cosmetic, so a clip that turns out
//--- not to work costs nothing visible instead of leaving labels hanging past the backdrop.
static const float VIGRID_MAP_COMPASS_FADE_DEG = 8.0;

//--- Carets: where the next zone, a teammate and a party ping sit on the strip. Unlike the entries
//--- these are a slot pool - the set changes as people move, ping and die.
//---
//--- Teammate and ping necessarily share a colour, since both are the owner's party slot, so they
//--- are separated the same two ways the map's triangle and diamond are: height and opacity. Do not
//--- collapse those without giving one of them a different silhouette.
static const int VIGRID_MAP_COMPASS_MAX_CARETS = 12;
static const float VIGRID_MAP_COMPASS_CARET_W = 12.0;
static const float VIGRID_MAP_COMPASS_CARET_H = 5.0;
static const float VIGRID_MAP_COMPASS_CARET_ZONE_W = 3.0;
static const float VIGRID_MAP_COMPASS_CARET_TEAM_W = 2.0;
static const float VIGRID_MAP_COMPASS_CARET_FULL_H = 5.0;
static const float VIGRID_MAP_COMPASS_CARET_PING_H = 3.0;

//--- The caret lane is the bottom of the strip, below the labels. Both caret heights are measured
//--- DOWN from this line, so a full and a half caret start together and only their length differs -
//--- which is what makes the difference readable without a baseline to compare against.
static const float VIGRID_MAP_COMPASS_CARET_LANE_Y = 38.0;

static const int VIGRID_MAP_COLOR_COMPASS_TICK = 0xFFFFFFFF;

//--- Stringtable keys. Held with the leading '#' so a widget localises them itself.
static const string STR_VIGRID_MAP_MARKERS_OFF = "#STR_MAP_MARKERS_OFF";

//--- The eight named directions, in strip order (0, 45, 90 ... 315 degrees). Localised because the
//--- letters genuinely differ - German uses O for east, French O for west, Russian a Cyrillic set -
//--- and resolved once per entry at pool creation, never per frame.
static const string STR_VIGRID_MAP_COMPASS_N = "#STR_MAP_COMPASS_N";
static const string STR_VIGRID_MAP_COMPASS_NE = "#STR_MAP_COMPASS_NE";
static const string STR_VIGRID_MAP_COMPASS_E = "#STR_MAP_COMPASS_E";
static const string STR_VIGRID_MAP_COMPASS_SE = "#STR_MAP_COMPASS_SE";
static const string STR_VIGRID_MAP_COMPASS_S = "#STR_MAP_COMPASS_S";
static const string STR_VIGRID_MAP_COMPASS_SW = "#STR_MAP_COMPASS_SW";
static const string STR_VIGRID_MAP_COMPASS_W = "#STR_MAP_COMPASS_W";
static const string STR_VIGRID_MAP_COMPASS_NW = "#STR_MAP_COMPASS_NW";
