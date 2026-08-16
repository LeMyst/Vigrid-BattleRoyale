/**
 *  Auto-Run - shared constants. No guard: this file compiles on both client and server.
 */

static const string VIGRID_AUTORUN_VERSION = "0.1.0";

//--- The one place this addon's asset path appears. Nothing loads an asset today; it is here so a
//--- future layout or icon has a single place to come from.
static const string VIGRID_AUTORUN_PREFIX = "Vigrid-BattleRoyale/Extra/AutoRun/";

#ifdef DIAG
#define VIGRID_AUTORUN_TRACE_ENABLED
#endif

//--- Log verbosity. Overridden at runtime by -autorun-trace/-autorun-debug/-autorun-info/
//--- -autorun-warn/-autorun-none on the command line, or by AutoRunLogLevel in serverDZ.cfg.
#ifdef VIGRID_AUTORUN_TRACE_ENABLED
    static const int VIGRID_AUTORUN_LOG_LEVEL = 4; // Trace
#else
    static const int VIGRID_AUTORUN_LOG_LEVEL = 0; // Error only
#endif

//--- Inputs. Resolved by name with GetUApi().GetInputByName rather than through the generated
//--- constant, which comes from this PBO's own Inputs.xml and may not exist at compile time.
static const string VIGRID_AUTORUN_INPUT_TOGGLE = "UAVigridAutoRunToggle";

//--- RPC. CF dispatches by METHOD NAME, so the handler method must be named exactly its string.
//--- There is no server -> client direction: the server is told what to hold and never answers.
static const string RPC_VIGRIDAUTORUN_SERVER_NAMESPACE = "RPC-VigridAutoRun-Server"; // client -> server

static const string VA_RPC_SET_SPEED = "VA_SetSpeed";

//--- Movement speeds, as HumanInputController.GetMovement reports and OverrideMovementSpeed takes
//--- them (P:\scripts\3_game\human.c:25). 0 doubles as "auto-run is off" on the wire.
static const int VIGRID_AUTORUN_SPEED_OFF = 0;
static const int VIGRID_AUTORUN_SPEED_WALK = 1;
static const int VIGRID_AUTORUN_SPEED_RUN = 2;
static const int VIGRID_AUTORUN_SPEED_SPRINT = 3;

//--- What a press from a standing start holds. Auto-run otherwise adopts whatever the player was
//--- already doing, so this only decides the one case that has nothing to adopt.
static const int VIGRID_AUTORUN_DEFAULT_SPEED = VIGRID_AUTORUN_SPEED_RUN;

/**
 *  The value handed to OverrideMovementAngle while auto-run holds a speed.
 *
 *  ⚠️ THIS IS THE ONE NUMBER TO FLIP FIRST if the character veers off course or refuses to move at
 *  all, and it is a named constant for exactly that reason.
 *
 *  0 is what this repo's own PlayerBase.DisableInput passes and is the semantically obvious "no
 *  deviation from where the camera points". Vanilla's own call sites pass 1 instead - the AI bot
 *  (P:\scripts\4_world\systems\bot\bot_hunt.c:145) and the camera tools
 *  (P:\scripts\5_mission\gui\cameratools\ctevent.c:50) - with no documented unit anywhere, and the
 *  proto declaration (human.c:237) says nothing but "float value". Neither reading has been
 *  measured, so this ships at 0 and stays a single-variable experiment.
 */
static const float VIGRID_AUTORUN_MOVEMENT_ANGLE = 0;
