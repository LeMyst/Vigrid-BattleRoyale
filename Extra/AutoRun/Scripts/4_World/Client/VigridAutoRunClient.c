#ifndef SERVER
/**
 *  Auto-Run - the whole client feature: state, the per-frame apply, and every cancel condition.
 *
 *  It lives in 4_World rather than beside the MissionGameplay glue in 5_Mission for one structural
 *  reason: VigridAutoRunAPI is 4_World (a host mod's server states have to reach its server half),
 *  and a 4_World class cannot name a 5_Mission type. Party solves the same problem the same way -
 *  state low, renderer high. Nothing here needs 5_Mission anyway: GetGame().GetPlayer(), GetUApi()
 *  and GetRPCManager() are all reachable from here.
 *
 *  THE MECHANISM is HumanInputController.OverrideMovementSpeed / OverrideMovementAngle
 *  (P:\scripts\3_game\human.c:234-237), held at ENABLED - "permanently active until DISABLED is
 *  passed" - and released with DISABLED. It is not a new API for this repo: PlayerBase.DisableInput
 *  already uses exactly this pair with a speed of 0 to freeze players, and that call site is the
 *  reference for the shape.
 *
 *  IT IS ALSO WHY VigridAutoRunAPI.SetAllowed EXISTS. A host mod that freezes players is writing to
 *  the very same override, and this class re-asserts its own every frame - so the host has to be
 *  able to say "not now" or the two fight and the freeze loses.
 */
class VigridAutoRunClient
{
    private static ref VigridAutoRunClient s_Instance;

    //! Whether a speed is being held right now.
    private bool m_Active;

    //! What the player asked for: 1 walk, 2 run, 3 sprint. Adopted from what they were already doing.
    private int m_RequestedSpeed;

    //! Last value the server was told. VIGRID_AUTORUN_SPEED_OFF means "the server holds nothing".
    private int m_SentSpeed;

    /**
     *  False until every movement key has been seen released at least once since the toggle.
     *
     *  Without it the feature is unusable: the natural way to start auto-run is to press the bind
     *  WHILE running, so W is still held on that very frame and the movement-cancel test below would
     *  fire immediately. Auto-run would appear to do nothing at all.
     */
    private bool m_CancelArmed;

    //! Set false by a host mod that is about to drive the same override itself.
    private bool m_Allowed;

    void VigridAutoRunClient()
    {
        m_Allowed = true;
    }

    static VigridAutoRunClient GetInstance()
    {
        if (!s_Instance)
            s_Instance = new VigridAutoRunClient();

        return s_Instance;
    }

    bool IsActive()
    {
        return m_Active;
    }

    /**
     *  Drop everything, without touching the controller.
     *
     *  For a mission restart only: the player entity of the previous session is gone, so there is
     *  nothing to release, and reaching for one would be reaching through a dangling reference.
     */
    void Reset()
    {
        m_Active = false;
        m_RequestedSpeed = VIGRID_AUTORUN_SPEED_OFF;
        m_SentSpeed = VIGRID_AUTORUN_SPEED_OFF;
        m_CancelArmed = false;
        m_Allowed = true;
    }

    //! The keybind. Stops if running, otherwise adopts the speed the player is already moving at.
    void Toggle()
    {
        if (m_Active)
        {
            Stop("toggle");
            return;
        }

        if (!m_Allowed)
        {
            VigridAutoRunLog.Debug("Toggle refused: not allowed right now");
            return;
        }

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        HumanInputController hic = player.GetInputController();
        if (!hic)
            return;

        if (!CanHold(player, hic))
        {
            VigridAutoRunLog.Debug("Toggle refused: the player cannot hold a speed right now");
            return;
        }

        m_RequestedSpeed = ResolveStartSpeed(hic);
        m_Active = true;
        m_CancelArmed = false;

        VigridAutoRunLog.Debug("Started, holding speed " + m_RequestedSpeed);
    }

    /**
     *  Refuse to start and cancel anything running. Called by a host mod that is about to write the
     *  movement override itself - see VigridAutoRunAPI.
     */
    void SetAllowed(bool allowed)
    {
        if (m_Allowed == allowed)
            return;

        m_Allowed = allowed;

        if (!allowed)
            Stop("host mod");
    }

    /**
     *  Every frame, from MissionGameplay.OnUpdate, and ABOVE its menu guards - opening a menu that
     *  leaves the player moving (the Vigrid map does exactly that) must not silently drop the hold.
     */
    void Update(float timeslice)
    {
        if (!m_Active)
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
        {
            //--- No entity to release the override on. Drop the state and tell the server, which
            //--- still has a player object of its own to clear.
            m_Active = false;
            m_CancelArmed = false;
            SendSpeed(VIGRID_AUTORUN_SPEED_OFF);
            return;
        }

        HumanInputController hic = player.GetInputController();
        if (!hic)
            return;

        if (!CanHold(player, hic))
        {
            Stop("state");
            return;
        }

        if (ShouldCancelOnInput(hic))
        {
            Stop("input");
            return;
        }

        //--- Stamina. LimitsIsSprintDisabled un-latches by itself once stamina recovers, so asking
        //--- it every frame is the whole of "drops to a run when winded, sprints again when rested".
        int effective = m_RequestedSpeed;
        if (effective == VIGRID_AUTORUN_SPEED_SPRINT)
        {
            if (hic.LimitsIsSprintDisabled())
                effective = VIGRID_AUTORUN_SPEED_RUN;
        }

        //--- Re-asserted every frame rather than only on change. ENABLED is documented as permanent,
        //--- so this is belt and braces against anything else in the load order clearing it - and it
        //--- costs two proto calls.
        hic.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, effective);
        hic.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, VIGRID_AUTORUN_MOVEMENT_ANGLE);

        //--- The wire, by contrast, only moves on an edge.
        SendSpeed(effective);
    }

    //! Release the override locally and tell the server to do the same.
    void Stop(string reason)
    {
        if (!m_Active)
            return;

        m_Active = false;
        m_CancelArmed = false;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
        {
            HumanInputController hic = player.GetInputController();
            if (hic)
            {
                hic.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
                hic.OverrideMovementAngle(HumanInputControllerOverrideType.DISABLED, 0);
            }
        }

        SendSpeed(VIGRID_AUTORUN_SPEED_OFF);

        VigridAutoRunLog.Debug("Stopped (" + reason + ")");
    }

    /**
     *  What speed a press adopts.
     *
     *  Banded rather than rounded: EnfusionScript has no implicit float -> int narrowing to lean on.
     *
     *  A player who is already moving has an unambiguous answer, because the sprint modifier is
     *  ALREADY folded into what GetMovement reports - Shift + W is a 3, so the moving cases need no
     *  modifier check at all. Standing still is the only case with nothing to adopt, and it is the
     *  only place the modifier has to be read directly.
     */
    private int ResolveStartSpeed(HumanInputController hic)
    {
        float current_speed;
        vector local_dir;
        hic.GetMovement(current_speed, local_dir);

        if (current_speed >= 2.5)
            return VIGRID_AUTORUN_SPEED_SPRINT;
        if (current_speed >= 1.5)
            return VIGRID_AUTORUN_SPEED_RUN;
        if (current_speed >= 0.5)
            return VIGRID_AUTORUN_SPEED_WALK;

        //--- Standing still. Sprint + the bind means "set off at a sprint" - without this the only
        //--- way to start a sprint from a standstill is to run first and press the bind again.
        //---
        //--- The arming latch below only watches the four movement keys, never UATurbo, so holding
        //--- Shift across the toggle cannot cancel what it just started.
        if (IsSprintModifierHeld())
            return VIGRID_AUTORUN_SPEED_SPRINT;

        return VIGRID_AUTORUN_DEFAULT_SPEED;
    }

    //! Whether the sprint modifier (Shift by default) is down right now.
    private bool IsSprintModifierHeld()
    {
        if (!GetUApi())
            return false;

        return GetUApi().GetInputByID(UATurbo).LocalValue() > 0;
    }

    /**
     *  Whether the player is in a state where holding a movement speed makes any sense.
     *
     *  Each test on its own line - EnfusionScript rejects a multi-line condition, and a single
     *  ten-term one would be unreadable anyway.
     *
     *  Swimming is refused for now. It would most likely work, being the same movement input, but it
     *  is one more axis nobody has tested; relaxing it later is deleting one line.
     */
    private bool CanHold(PlayerBase player, HumanInputController hic)
    {
        if (!m_Allowed)
            return false;
        if (!player.IsAlive())
            return false;
        if (player.IsUnconscious())
            return false;
        if (player.GetCommand_Vehicle())
            return false;
        if (player.GetCommand_Ladder())
            return false;
        if (player.GetCommand_Climb())
            return false;
        if (player.GetCommand_Swim())
            return false;

        return true;
    }

    /**
     *  "Touch a movement key and it stops."
     *
     *  Held state rather than a press event, so it also catches a key that was already down; the
     *  m_CancelArmed latch is what stops that firing on the frame auto-run started. Stance change is
     *  a press event and needs no latch. Jump is deliberately NOT a cancel - vaulting a fence
     *  mid-route is exactly the sort of thing auto-run should survive.
     */
    private bool ShouldCancelOnInput(HumanInputController hic)
    {
        if (!GetUApi())
            return false;

        if (hic.IsStanceChange())
            return true;

        bool moving = false;
        if (GetUApi().GetInputByID(UAMoveForward).LocalValue() > 0)
            moving = true;
        if (GetUApi().GetInputByID(UAMoveBack).LocalValue() > 0)
            moving = true;
        if (GetUApi().GetInputByID(UAMoveLeft).LocalValue() > 0)
            moving = true;
        if (GetUApi().GetInputByID(UAMoveRight).LocalValue() > 0)
            moving = true;

        if (!m_CancelArmed)
        {
            //--- Arm on the first frame with nothing held, and never cancel before then.
            if (!moving)
                m_CancelArmed = true;

            return false;
        }

        return moving;
    }

    //! Edge-triggered: the server is only ever told when the held value actually changes.
    private void SendSpeed(int speed)
    {
        if (m_SentSpeed == speed)
            return;

        m_SentSpeed = speed;

        if (!GetRPCManager())
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDAUTORUN_SERVER_NAMESPACE, VA_RPC_SET_SPEED, new Param1<int>(speed), true);

        VigridAutoRunLog.Debug("Sent speed " + speed + " to the server");
    }
}
#endif
