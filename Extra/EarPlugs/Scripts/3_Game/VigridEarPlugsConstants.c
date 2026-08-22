/**
 *  EarPlugs - shared constants. No guard: this file compiles on both client and server.
 *
 *  Nothing here runs on a server - the whole feature is client-local, and every 5_Mission file is
 *  #ifndef SERVER - but the constants themselves are side-agnostic and the logger beside them
 *  follows the house shape, so neither is guarded.
 */

//--- Asset prefix. The ONLY hard-coded path in the addon - build every layout path from it.
//--- CI1.bat derives each PBO's prefix as <PrefixLinkRoot>\<folder>, so this tracks the folder.
static const string VIGRID_EARPLUGS_PREFIX = "Vigrid-BattleRoyale/Extra/EarPlugs/";

//--- Logging. Diag builds are noisy by default, release builds are silent but for errors.
#ifdef DIAG
#define VIGRID_EARPLUGS_TRACE_ENABLED
#endif

#ifdef VIGRID_EARPLUGS_TRACE_ENABLED
    static const int VIGRID_EARPLUGS_LOG_LEVEL = 4; // Trace
#else
    static const int VIGRID_EARPLUGS_LOG_LEVEL = 0; // Error only
#endif

//--- Preferences. The addon's own profile folder, deliberately not the host mod's, so that
//--- extraction does not strand the file behind. $profile: on a client resolves to the CLIENT
//--- profile directory, which is the only place a per-player choice can live.
static const string VIGRID_EARPLUGS_SETTINGS_FOLDER = "$profile:Vigrid-EarPlugs\\";
static const string VIGRID_EARPLUGS_PREFS_FILE = "$profile:Vigrid-EarPlugs\\earplugs_client.json";

//--- Inputs. Resolved by name with GetUApi().GetInputByName rather than through the generated
//--- constant, which comes from this PBO's own Inputs.xml and may not exist at compile time.
static const string VIGRID_EARPLUGS_INPUT_TOGGLE = "UAVigridEarPlugsToggle";

//--- Levels. Off is always 0 so an unwritable prefs file degrades to "no earplugs", never to
//--- "silently deaf". Adding a fourth level (a full mute, say) means one more branch in each of
//--- VigridEarPlugsLevels' three switches and one more entry in the stringtable - nothing else,
//--- because every consumer asks that class rather than indexing a table.
static const int VIGRID_EARPLUGS_LEVEL_OFF = 0;
static const int VIGRID_EARPLUGS_LEVEL_LIGHT = 1;
static const int VIGRID_EARPLUGS_LEVEL_HEAVY = 2;
static const int VIGRID_EARPLUGS_LEVEL_COUNT = 3;

//--- Volume multipliers, applied to the player's OWN volume rather than used as absolute values.
//--- That distinction is the whole point: a player running at 40% effects volume must not be
//--- blasted to 100% when they take the plugs out, and "45%" must not end up LOUDER than their
//--- normal setting on a quiet setup.
static const float VIGRID_EARPLUGS_FACTOR_LIGHT = 0.40;
static const float VIGRID_EARPLUGS_FACTOR_HEAVY = 0.12;

//--- Fade for a player-initiated change, in seconds - the second argument to every AbstractSoundScene
//--- setter. Reconciler corrections deliberately use 0 instead; see VigridEarPlugsController.
//---
//--- The _MS twin is THE SAME FIGURE and must be kept in step: it is how long the reconciler holds
//--- off after a toggle, and a fade outlasting that hold would be read as "above target" and
//--- corrected mid-fade. Two constants rather than one arithmetic expression because GetTime() is
//--- int milliseconds and the setters take float seconds.
static const float VIGRID_EARPLUGS_FADE_S = 0.35;
static const int VIGRID_EARPLUGS_FADE_MS = 350;

//--- How often the reconciler runs. Fast enough that a vanilla volume restore is corrected before
//--- the player registers it, slow enough to be free.
static const int VIGRID_EARPLUGS_TICK_MS = 250;

//--- Volume comparison tolerance. The engine's stored value is not bit-identical to what was written.
static const float VIGRID_EARPLUGS_EPSILON = 0.01;

//--- Which buses are scaled.
//---
//--- Effects is the whole point. Radio follows it because a radio is diegetic world audio - the same
//--- kind of thing as a gunshot - and plugged ears do not distinguish.
//---
//--- Music, VOIP and SpeechEx are deliberately NOT scaled, and VOIP is the one that matters: this
//--- mod ships parties, and a player who plugs their ears must still hear their squad. Muffling
//--- comms would make the feature a competitive liability rather than a comfort setting. Music is
//--- non-diegetic and already has its own Options slider.
//---
//--- ⚠️ SetMasterAttenuation is a MASTER bus filter, so the muffle half may reach VOIP whatever this
//--- says. If that turns out to be audible and unwanted, the two halves are independent by
//--- construction: drop the attenuation and keep the volume scaling.
static const bool VIGRID_EARPLUGS_SCALE_RADIO = true;

//--- HUD badge. Its POSITION is authored against a 1920-wide screen and scaled by
//--- parent_w / REFERENCE_W, because SetPos and SetSize take REAL screen pixels while a layout's
//--- declared position and size are scaled by viewport/1920. Mixing the two shifts a whole group
//--- while its spacing stays correct, which reads as anything but a coordinate bug.
static const float VIGRID_EARPLUGS_HUD_REFERENCE_W = 1920.0;
static const float VIGRID_EARPLUGS_HUD_X = 26.0;
static const float VIGRID_EARPLUGS_HUD_Y = 64.0;

//--- ONLY THE POSITION IS VIEWPORT-SCALED. The badge's SIZE is derived from the measured
//--- text, and must be: glyph size is fixed by the declared font face, so a box that scaled
//--- with the viewport shrank under its own contents. At 1280 wide it came out 127x23 while
//--- the text stayed at its authored pixel size, and "BOUCHONS" overran "FORT" - correct at
//--- 1440p, clipped at 720p. Sizing from the text also means no localization can overflow it,
//--- which the old fixed 55/45 split could only ever approximate.

//--- Viewport width at or above which the large font tier is used. Below it, one step down on
//--- each face - see earplugs.layout, which carries a widget per tier because there is no
//--- SetFont and SetTextExactSize was measured to do nothing.
static const float VIGRID_EARPLUGS_HUD_TIER_W = 1920.0;

//--- Padding around and between the two words, per tier. NOT viewport-scaled, for the same
//--- reason the box is not: these sit alongside fixed-pixel glyphs and must stay in proportion
//--- to them, not to the screen.
static const float VIGRID_EARPLUGS_HUD_PAD_LARGE = 14.0;
static const float VIGRID_EARPLUGS_HUD_GAP_LARGE = 26.0;
static const float VIGRID_EARPLUGS_HUD_VPAD_LARGE = 8.0;
static const float VIGRID_EARPLUGS_HUD_PAD_SMALL = 10.0;
static const float VIGRID_EARPLUGS_HUD_GAP_SMALL = 18.0;
static const float VIGRID_EARPLUGS_HUD_VPAD_SMALL = 6.0;

//--- Alpha while the plugs are simply in. Low enough to be furniture, high enough to be noticed -
//--- this is the fix for a player forgetting they are deaf, so it must never fade to nothing.
static const float VIGRID_EARPLUGS_HUD_IDLE_ALPHA = 0.55;
static const float VIGRID_EARPLUGS_HUD_FLASH_ALPHA = 1.0;

//--- How long the badge stays at full alpha after a change, and how long it then takes to ease back
//--- to idle. The flash is also what announces a change TO Off, which is the one state with no
//--- persistent badge at all.
static const int VIGRID_EARPLUGS_HUD_FLASH_MS = 2000;
static const int VIGRID_EARPLUGS_HUD_FADE_MS = 600;

/**
 *  Per-level lookup.
 *
 *  Deliberately three switches rather than three parallel arrays. Global const arrays are awkward in
 *  EnfusionScript, an out-of-range index on one of them is a silent wrong answer rather than an
 *  error, and this repo has already been bitten by an array read that returned entries from a
 *  DIFFERENT array when it shared an expression with a call. A switch cannot do any of that: an
 *  unknown level falls through to the Off answer, which is the safe one.
 */
class VigridEarPlugsLevels
{
    //! Multiplier applied to the player's own volume. 1.0 for Off, so Off needs no special case.
    static float Factor(int level)
    {
        if (level == VIGRID_EARPLUGS_LEVEL_LIGHT)
            return VIGRID_EARPLUGS_FACTOR_LIGHT;
        if (level == VIGRID_EARPLUGS_LEVEL_HEAVY)
            return VIGRID_EARPLUGS_FACTOR_HEAVY;

        return 1.0;
    }

    //! CfgSoundEffects >> AttenuationsEffects classname, or "" for no muffle. See config.cpp.
    static string Attenuation(int level)
    {
        if (level == VIGRID_EARPLUGS_LEVEL_LIGHT)
            return "VigridEarPlugsLightAttenuation";
        if (level == VIGRID_EARPLUGS_LEVEL_HEAVY)
            return "VigridEarPlugsHeavyAttenuation";

        return "";
    }

    //! Stringtable key for the badge. Bare "#KEY" - SetText resolves it.
    static string LabelKey(int level)
    {
        if (level == VIGRID_EARPLUGS_LEVEL_LIGHT)
            return "#STR_EARPLUGS_LEVEL_LIGHT";
        if (level == VIGRID_EARPLUGS_LEVEL_HEAVY)
            return "#STR_EARPLUGS_LEVEL_HEAVY";

        return "#STR_EARPLUGS_LEVEL_OFF";
    }

    //! Untranslated, for the log only - a localized log line is a log line nobody can grep.
    static string DebugName(int level)
    {
        if (level == VIGRID_EARPLUGS_LEVEL_LIGHT)
            return "Light";
        if (level == VIGRID_EARPLUGS_LEVEL_HEAVY)
            return "Heavy";

        return "Off";
    }

    //! Wraps back to Off past the last level.
    static int Next(int level)
    {
        int next = level + 1;
        if (next >= VIGRID_EARPLUGS_LEVEL_COUNT)
            return VIGRID_EARPLUGS_LEVEL_OFF;

        return next;
    }

    //! The prefs file is hand-editable, so a stale value from a build with more levels must not
    //! reach Factor() and mean something unintended.
    static int Clamp(int level)
    {
        if (level < VIGRID_EARPLUGS_LEVEL_OFF)
            return VIGRID_EARPLUGS_LEVEL_OFF;
        if (level >= VIGRID_EARPLUGS_LEVEL_COUNT)
            return VIGRID_EARPLUGS_LEVEL_OFF;

        return level;
    }
}
