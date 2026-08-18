/**
 *  The spectator camera.
 *
 *  NO GUARD ON THIS FILE, deliberately. It compiles on both sides, exactly like vanilla's own
 *  P:\scripts\4_world\entities\dayzspectator.c and like this mod's unguarded BattleRoyalePlayArea.c.
 *  The server calls GetGame().SelectSpectator(identity, BR_SPECTATE_CAM_CLASS, pos), which resolves
 *  a SCRIPT class name - not a config class - so this class must exist by name and needs no
 *  CfgVehicles entry. (Contrast GetGame().CreateObject, which DOES resolve through config; that is
 *  what the abandoned VPP port used, against a config class that never existed.)
 *
 *  Every per-frame path opens with an IsClient() check, since the class is visible server-side too.
 *
 *  Built from vanilla DayZSpectator's chassis - the modern GetUApi input API, the SurfaceY floor
 *  clamp and the 0.5 s UpdateSpectatorPosition cadence are all lifted from it, because it is the one
 *  camera of the three candidates that is known to compile and that the engine itself instantiates.
 *  The follow behaviour is new. Two deliberate departures from the obvious implementation:
 *
 *    - The anchor is the target's HEAD BONE, not their feet plus a stance offset. One call collapses
 *      prone, crouched, in-vehicle, unconscious and mid-ragdoll into a single case, with no stance
 *      table and no GetMovementState() (whose behaviour for a REMOTE entity on a CLIENT is not
 *      something the vanilla source establishes).
 *
 *    - Camera.LookAt is not used and GetCommandModifier_Weapons() is never read. That modifier is
 *      MEASURED NULL for a remote entity on a client (2026-08-18), which is why the third-person boom
 *      holds a constant pitch rather than the target's aim. Orientation is a damped yaw/pitch pair,
 *      and roll is never taken from the target.
 *
 *  FIRST PERSON (#288) is a third construction again, and none of the above applies to it: the eye
 *  sits at the Head bone smoothed in the target's MODEL SPACE, and the view points down the WEAPON
 *  BARREL whenever the weapon is up. See ResolveEyePosition and ResolveFirstPersonLook - both carry
 *  the measurements that produced them, and both were wrong twice first.
 */
class BattleRoyaleSpectatorCamera extends Camera
{
    //! Set in the ctor so BattleRoyaleClient can find the camera the engine created for it.
    protected static BattleRoyaleSpectatorCamera s_Instance;

    //--- Weak. Re-latched from the 1 Hz push and by proximity every frame, so it is allowed to go
    //--- NULL when the target streams out of the bubble - m_TargetPos carries us until it returns.
    protected Object m_Target;
    protected string m_TargetUid;
    protected vector m_TargetPos;
    protected int m_Mode;

    //--- FIRST PERSON (#288). Purely local - the server is never told, because nothing it owns
    //--- changes. Only honoured in FOLLOW mode; see IsFirstPersonActive.
    protected bool m_FirstPerson;

    //--- Last FOV handed to SetFOV, so ApplyFieldOfView can notice the user moving the slider
    //--- mid-match without calling into the engine every frame. Zero means "never applied".
    protected float m_AppliedFov;

    //--- Near plane currently applied, same edge-tracking reason as m_AppliedFov. First person needs
    //--- vanilla's tight 0.04 so the watched character's own skull does not clip the view.
    protected float m_AppliedNearPlane;

    //--- Which source the first-person look came from this frame: "weapon", "bone" or "none". Purely
    //--- diagnostic, but the one line that says whether a spectator's crosshair is the player's - see
    //--- ResolveFirstPersonLook.
    protected string m_LookSource;

    //--- First-person eye, held in the TARGET'S MODEL SPACE so it can be smoothed there. See
    //--- ResolveEyePosition for why the frame matters; m_FpSeeded is cleared whenever there is no
    //--- bone to smooth against, so the next good frame snaps instead of easing from a stale value.
    protected vector m_FpEyeMS;
    protected bool m_FpSeeded;

    //--- The player whose head we asked COT to hide, so it can be put back on a retarget or on the
    //--- way out. Weak, like m_Target: if they are freed there is nothing left to restore anyway.
    protected PlayerBase m_HeadHiddenPlayer;

    protected vector m_SmoothPos;
    protected float m_Yaw;
    protected float m_Pitch;
    protected float m_BubbleAcc;
    protected float m_OrbitAngle;
    protected bool m_HasSnapped;

    //--- FREE mode (admin only). m_FreeSeeded is what makes entering the free camera continuous
    //--- rather than a jump: on the first free frame the flying position and angles are taken from
    //--- wherever the follow camera had got to, so the view does not snap when the mode flips.
    protected bool m_FreeSeeded;
    protected float m_FreeSpeedMult;
    protected float m_CamPosAcc;

    //--- Diagnostics for the network-bubble question. m_BubblePushes counts UpdateSpectatorPosition
    //--- calls, so "is it even being called" stops being a matter of opinion.
    protected float m_TraceAcc;
    protected int m_BubblePushes;

    void BattleRoyaleSpectatorCamera()
    {
        SetEventMask(EntityEvent.FRAME);

        m_Target = NULL;
        m_TargetUid = "";
        m_TargetPos = "0 0 0";
        m_Mode = BR_SPECTATE_MODE_ORBIT;
        m_FirstPerson = false;
        m_AppliedFov = 0;
        m_AppliedNearPlane = 0;
        m_LookSource = "none";
        m_FpEyeMS = "0 0 0";
        m_FpSeeded = false;
        m_HeadHiddenPlayer = NULL;
        m_SmoothPos = "0 0 0";
        m_Yaw = 0;
        m_Pitch = BR_SPECTATE_PITCH;
        m_BubbleAcc = 0;
        m_OrbitAngle = 0;
        m_HasSnapped = false;
        m_FreeSeeded = false;
        m_FreeSpeedMult = 1.0;
        m_CamPosAcc = 0;
        m_TraceAcc = 0;
        m_BubblePushes = 0;

        s_Instance = this;
    }

    void ~BattleRoyaleSpectatorCamera()
    {
        //--- Never leave a headless character behind. The camera is destroyed when the session ends
        //--- but the player it was watching is not, and the hide is a local render flag nothing else
        //--- would ever clear.
        ApplyHeadHide(false);

        if (s_Instance == this)
            s_Instance = NULL;
    }

    static BattleRoyaleSpectatorCamera GetInstance()
    {
        return s_Instance;
    }

    /**
     *  Tolerant of being called every frame with unchanged values - BattleRoyaleClient does exactly
     *  that, so the camera keeps following even if a push is lost.
     */
    void SetTarget(Object target, string uid, vector position, int mode)
    {
        m_Target = target;
        m_TargetPos = position;

        //--- Leaving FREE re-arms the seed, so the next entry picks up from wherever the camera has
        //--- since ended up rather than from a stale free position. Checked against the OLD mode, so
        //--- it has to happen before the assignment below.
        if (m_Mode == BR_SPECTATE_MODE_FREE && mode != BR_SPECTATE_MODE_FREE)
        {
            m_FreeSeeded = false;

            //--- Coming back to a follow camera from somewhere across the map: snap rather than fly.
            m_HasSnapped = false;
        }

        m_Mode = mode;

        if (uid == m_TargetUid)
            return;

        //--- A genuine retarget: snap rather than fly the camera across the map, and clear any
        //--- depth-of-field the dying player left behind.
        m_TargetUid = uid;
        m_HasSnapped = false;
        PPEffects.ResetDOFOverride();

        //--- Push the bubble on the NEXT frame instead of waiting out the rest of the 0.5 s window.
        //--- A retarget is exactly when the camera is about to move furthest - potentially across the
        //--- map - so it is the worst moment to tell the server about it half a second late. Setting
        //--- the accumulator past the threshold is enough; EOnFrame does the rest.
        m_BubbleAcc = 1.0;
    }

    string GetTargetUid()
    {
        return m_TargetUid;
    }

    /**
     *  Ask for the eye to sit in the target's head rather than over their shoulder.
     *
     *  Called every frame with an unchanged value, exactly like SetTarget, so it must stay cheap and
     *  idempotent - which it is: the whole state is one bool, and the mode test that decides whether
     *  it means anything is taken fresh each frame in IsFirstPersonActive.
     */
    void SetFirstPerson(bool first_person)
    {
        m_FirstPerson = first_person;
    }

    bool IsFirstPerson()
    {
        return m_FirstPerson;
    }

    /**
     *  Is the first-person eye actually in force this frame?
     *
     *  The REQUEST is mode-independent and survives a mode change, so an admin who flips out to the
     *  free camera and back lands in the view they left. What it means is not: ORBIT has no target to
     *  sit inside, and FREE is a camera being flown by hand.
     */
    protected bool IsFirstPersonActive()
    {
        if (!m_FirstPerson)
            return false;

        return m_Mode == BR_SPECTATE_MODE_FOLLOW;
    }

    override void EOnFrame(IEntity other, float timeSlice)
    {
        if (!GetGame())
            return;
        if (!GetGame().IsClient())
            return;

        //--- Move the client's network bubble to the camera. Vanilla's exact cadence
        //--- (dayzspectator.c:52-58). This is the ONLY network traffic the camera generates: there
        //--- is no position RPC, and no server-side teleport of anything.
        m_BubbleAcc = m_BubbleAcc + timeSlice;
        if (m_BubbleAcc > 0.5)
        {
            GetGame().UpdateSpectatorPosition(GetPosition());
            m_BubbleAcc = 0;
            m_BubblePushes = m_BubblePushes + 1;
        }

        //--- WHAT THIS LINE IS FOR, and what it has already settled. It reports whether
        //--- UpdateSpectatorPosition is being called at all (m_BubblePushes climbing) and whether the
        //--- entity goes NULL while the camera sits right next to the pushed position. Camera ~3 m
        //--- from the target with entity=0 would mean the push runs and has no effect - an
        //--- engine-level answer rather than a scripting mistake.
        //---
        //--- IT DOES. THE PUSH RUNS AND HAS NO EFFECT. Established 2026-08-10 by putting the target
        //--- at an exact radius with the diag TP Target entry, which gives the measurement in both
        //--- directions instead of waiting for a foot race: at 1200 m from the spectator's corpse
        //--- entity went 0 and stayed 0 for 100+ samples over 90 s, did NOT recover when the target
        //--- walked back to 1122 m, and came straight back at 700 m. 507 pushes across the whole
        //--- run, camera never more than ~4 m from the pushed position. networkRangePlayers is unset
        //--- in serverDZ.cfg, so the default 1000 m applies. The bubble stays on the corpse.
        //---
        //--- THERE IS HYSTERESIS, and it is why this took four attempts to pin down. Approaching
        //--- from inside, a target stays replicated to ~1068-1200 m; once dropped it does not return
        //--- until well inside. So a session that never crosses the boundary - 39 clean samples out
        //--- to 929 m, also measured 2026-08-10 - looks exactly like proof that nothing is wrong.
        //--- A run that stops short of ~1100 m proves NOTHING.
        //---
        //--- TWO OTHER THINGS PRODUCE entity=0. A retarget to somebody far away, which lands as one
        //--- huge single-sample step; and the match ending, since EndSpectate deliberately leaves
        //--- this camera running - after which every field freezes to identical values while
        //--- m_BubblePushes keeps climbing. Neither is the range effect, and 4005d62 mistook the
        //--- first for a refutation of it. Do not repeat that: a teleporting target and a range
        //--- cutoff produce the same entity=0, and finding one is not evidence against the other.
        //---
        //--- The carrier body is still NOT the fix - it crashed the server outright. See CLAUDE.md.
        float trace_interval = 5.0;
#ifdef DIAG_DEVELOPER
        //--- Turned down for a deliberate range test, where 5 s gives too few samples to separate a
        //--- sustained entity=0 from the transient one a teleport always produces.
        trace_interval = BattleRoyaleDiag.spectate_trace_interval;
#endif

        m_TraceAcc = m_TraceAcc + timeSlice;
        if (m_TraceAcc > trace_interval)
        {
            m_TraceAcc = 0;

            bool has_entity = m_Target != NULL;
            float target_distance = vector.Distance(GetPosition(), m_TargetPos);

            BattleRoyaleUtils.Trace(string.Format("[Spectate] cam=%1 target=%2 dist=%3 entity=%4 pushes=%5",
                GetPosition().ToString(), m_TargetPos.ToString(), target_distance, has_entity, m_BubblePushes));

            //--- Only while somebody is actually sitting in the target's head. The probe this
            //--- replaced dumped the whole bone basis to identify the forward axis; that question is
            //--- answered (see ResolveHeadLook), so what is left is the far cheaper regression check:
            //--- the derived look against the body yaw it used to be pinned to. A first-person yaw
            //--- that never diverges from body_yaw means the bone read has silently stopped working.
            if (m_FirstPerson && m_Target)
            {
                vector target_orientation = m_Target.GetOrientation();
                float body_yaw = target_orientation[0];

                string look_line = "[Spectate] first person source=" + m_LookSource;
                look_line = look_line + " yaw=" + m_Yaw;
                look_line = look_line + " pitch=" + m_Pitch;
                look_line = look_line + " body_yaw=" + body_yaw;
                BattleRoyaleUtils.Trace(look_line);
            }
        }

        //--- FREE is a completely different camera: it is flown, not aimed at anything, so none of
        //--- the anchor / boom / damping machinery below applies to it.
        if (m_Mode == BR_SPECTATE_MODE_FREE)
        {
            UpdateFreeCamera(timeSlice);
            PushCamPos(timeSlice);
            return;
        }

        vector anchor = ResolveAnchor(timeSlice);
        float desired_yaw = ResolveYaw();

        PushCamPos(timeSlice);

        bool first_person = IsFirstPersonActive();

        //--- The player's own FOV, in every mode. A bare Camera otherwise runs at the engine's
        //--- default scene FOV, which is far tighter than what the watched player sees.
        ApplyFieldOfView();
        ApplyNearPlane(first_person);

        float yaw_damp = BR_SPECTATE_YAW_DAMP;

        //--- Third person holds a constant tilt, so it has nothing to damp; only first person, which
        //--- tracks a live head, needs a damped pitch. Zero means "assign it outright".
        float pitch_damp = 0;

        float desired_pitch = BR_SPECTATE_PITCH;
        if (m_Mode == BR_SPECTATE_MODE_ORBIT)
            desired_pitch = BR_SPECTATE_ORBIT_PITCH;

        if (first_person)
        {
            yaw_damp = BR_SPECTATE_FP_SMOOTH;
            pitch_damp = BR_SPECTATE_FP_SMOOTH;

            //--- Level, not the boom's -10 downward tilt, when there is nothing to read. This is the
            //--- streamed-out case; the eye is on the last pushed position and simply looks straight.
            desired_pitch = 0;

            float look_yaw;
            float look_pitch;

            if (ResolveFirstPersonLook(look_yaw, look_pitch))
            {
                desired_yaw = look_yaw;
                desired_pitch = look_pitch;
            }
        }

        //--- Shortest-arc yaw damping.
        float delta = Math.NormalizeAngle(desired_yaw - m_Yaw);
        if (delta > 180)
            delta = delta - 360;

        m_Yaw = Math.NormalizeAngle(m_Yaw + delta * Math.Clamp(timeSlice * yaw_damp, 0, 1));

        //--- Pitch is clamped to +/-85 rather than wrapped, so a plain lerp is correct here where the
        //--- yaw above needs the shortest-arc treatment.
        if (pitch_damp > 0)
            m_Pitch = m_Pitch + (desired_pitch - m_Pitch) * Math.Clamp(timeSlice * pitch_damp, 0, 1);
        else
            m_Pitch = desired_pitch;

        //--- FIRST PERSON IS RIGID, AND SKIPS BOTH OF THE CORRECTIONS BELOW. Positional damping would
        //--- make the view swim inside the character's own head every time they moved, and the floor
        //--- clamp would shove the camera up out of a PRONE target's skull the moment their eyes sat
        //--- lower than FLOOR_CLEARANCE above the surface. Neither correction has anything to fix
        //--- here: the eye is anchored to a bone that is already exactly where it should be.
        //---
        //--- m_SmoothPos and m_HasSnapped are still maintained, so switching back to third person
        //--- eases out from the head rather than starting from a stale position half a match old.
        if (first_person)
        {
            vector eye_pos = ResolveEyePosition(anchor, timeSlice);

            //--- Hide the watched player's head while the eye is inside it, or the spectator is
            //--- looking at the back of their own target's face. Vanilla gets away with a 4 cm offset
            //--- because in first person it renders the local player's HEADLESS model; a remote
            //--- character is drawn in full, which is what "I see the inside of the head" was.
            ApplyHeadHide(true);

            m_SmoothPos = eye_pos;
            m_HasSnapped = true;

            SetPosition(eye_pos);
            SetOrientation(Vector(m_Yaw, m_Pitch, 0));
            return;
        }

        //--- Third person, or first person that could not anchor: give the head back and drop the
        //--- model-space seed so the next entry snaps rather than easing from a stale offset.
        ApplyHeadHide(false);
        m_FpSeeded = false;

        vector desired_pos = ResolveBoom(anchor);

        //--- Snap on the first frame and on every retarget; interpolate otherwise. This is the
        //--- correct version of the check the VPP port got wrong: its m_vPreviousPosition started at
        //--- "0 0 0", so its >50 m branch fired on frame one and fired an RPC to teleport the player.
        if (!m_HasSnapped || vector.Distance(m_SmoothPos, desired_pos) > BR_SPECTATE_SNAP_DISTANCE)
        {
            m_SmoothPos = desired_pos;
            m_HasSnapped = true;
        }
        else
        {
            m_SmoothPos = m_SmoothPos + (desired_pos - m_SmoothPos) * Math.Clamp(timeSlice * BR_SPECTATE_POS_DAMP, 0, 1);
        }

        vector final_pos = m_SmoothPos;

        //--- Never underground. SurfaceY rather than vanilla's SurfaceRoadY, matching the rest of
        //--- this mod; the clearance is what keeps a prone target's camera above the dirt.
        float floor_y = GetGame().SurfaceY(final_pos[0], final_pos[2]) + BR_SPECTATE_FLOOR_CLEARANCE;
        if (final_pos[1] < floor_y)
            final_pos[1] = floor_y;

        SetPosition(final_pos);
        SetOrientation(Vector(m_Yaw, m_Pitch, 0));
    }

    /**
     *  Report where the camera is, so the server can keep the admin's body - and therefore the
     *  replication bubble - underneath it.
     *
     *  RUNS IN EVERY MODE, not just FREE, and that is the whole point. It lived inside
     *  UpdateFreeCamera at first, which meant a FOLLOW-mode admin never reported anything:
     *  entry.cam_pos stayed at the value BeginAdminSpectate seeded it with (the respawn position),
     *  CarryAnchorBody measured zero drift from there and never carried, the bubble sat in the map
     *  corner where the admin respawned, and a target on the far side of the map was simply not
     *  replicated. The symptom is the one that matters: the admin opens the camera and sees NO
     *  PLAYERS AT ALL. Observed 2026-08-11.
     *
     *  Only sent for an admin - an ordinary spectator's corpse is carried by CarryCorpse, which
     *  chases the target and needs nothing from the client. The server drops the message for any
     *  entry that is not an admin anyway, so this gate is bandwidth, not safety.
     */
    protected void PushCamPos(float timeSlice)
    {
        //--- GUARDED, because THIS FILE HAS NO TOP-LEVEL GUARD - it compiles on the server too, and
        //--- BattleRoyaleRPC is #ifndef SERVER. Naming it unguarded here took the whole World module
        //--- down with "Unknown type 'BattleRoyaleRPC'" on the dedicated server, which is the exact
        //--- hazard the header comment on this class warns about.
#ifndef SERVER
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if (!br_rpc)
            return;
        if (!br_rpc.is_admin)
            return;

        m_CamPosAcc = m_CamPosAcc + timeSlice;
        if (m_CamPosAcc * 1000 < BR_ADMIN_CAMPOS_PUSH_MS)
            return;

        m_CamPosAcc = 0;
        GetRPCManager().SendRPC(RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateCamPos", new Param1<vector>(GetPosition()), true);
#endif
    }

    /**
     *  The admin free camera. WASD to fly, mouse to look, Shift to boost, gear up/down to change
     *  the base speed.
     *
     *  Movement, the speed stepping and the +/-89 pitch clamp are vanilla DayZSpectator's
     *  (P:\scripts\4_world\entities\dayzspectator.c:12-59), including the `vector.Up * direction`
     *  cross product for the strafe axis. Two deliberate departures:
     *
     *    - SurfaceY rather than vanilla's SurfaceRoadY, matching every other height clamp in this
     *      mod, and with the same FLOOR_CLEARANCE the follow camera uses so the free camera cannot
     *      be flown into the dirt either.
     *
     *    - The position is reported to the server every BR_ADMIN_CAMPOS_PUSH_MS. That is what keeps
     *      the admin's own body - and therefore the replication bubble - underneath the camera.
     *      Without it the free camera flies fine and everything past ~1 km of where the admin
     *      started is simply not replicated: terrain, no players. UpdateSpectatorPosition above does
     *      NOT do this; it is measured not to move the bubble at all.
     *
     *  Reading input here rather than in MissionGameplay.OnUpdate is deliberate: these are
     *  continuous per-frame axes, not edge-triggered actions, and they need the frame's timeSlice.
     *  The discrete keys (toggle, mode, cycle) are edges and do live in the mission update.
     */
    protected void UpdateFreeCamera(float timeSlice)
    {
        if (!GetUApi())
            return;

        //--- Continuous from wherever the follow camera was, so flipping into free flight does not
        //--- teleport the view. GetOrientation rather than m_Yaw/m_Pitch: in FOLLOW those are the
        //--- damped values that were on their way somewhere, and the rendered orientation is what
        //--- the admin is actually looking at.
        if (!m_FreeSeeded)
        {
            m_FreeSeeded = true;
            m_SmoothPos = GetPosition();

            vector current = GetOrientation();
            m_Yaw = current[0];
            m_Pitch = current[1];
        }

        //--- MOUSE WHEEL first, gear keys as a fallback. The wheel is what COT's cinematic camera
        //--- uses (JMCinematicCamera.c:57) and what an admin coming from COT will reach for.
        //---
        //--- Resolved BY NAME and null-checked, deliberately: UACameraToolSpeedIncrease/Decrease
        //--- appear NOWHERE in P:\scripts - only in COT - so they are engine-registered inputs this
        //--- repo cannot verify from the vanilla source. A name the engine does not know returns
        //--- NULL from GetInputByName and would otherwise be a silent no-op, which is exactly the
        //--- failure mode this codebase has been bitten by before. The gear keys stay wired so the
        //--- feature degrades to what it did before rather than to nothing.
        float speed_step = 0;

        UAInput wheel_up = GetUApi().GetInputByName("UACameraToolSpeedIncrease");
        UAInput wheel_down = GetUApi().GetInputByName("UACameraToolSpeedDecrease");
        if (wheel_up && wheel_down)
            speed_step = wheel_up.LocalValue() - wheel_down.LocalValue();

        if (GetUApi().GetInputByID(UACarShiftGearUp).LocalPress())
            speed_step = 1;
        if (GetUApi().GetInputByID(UACarShiftGearDown).LocalPress())
            speed_step = -1;

        //--- MULTIPLICATIVE, not the flat +/-2 vanilla DayZSpectator uses. A fixed step is wrong at
        //--- both ends of the range: it is a huge jump when crawling and imperceptible when flying.
        //--- A ratio gives even control across the whole span, which is the point of putting it on
        //--- the wheel at all.
        if (speed_step > 0)
            m_FreeSpeedMult = m_FreeSpeedMult * BR_SPECTATE_FREE_SPEED_STEP;
        else if (speed_step < 0)
            m_FreeSpeedMult = m_FreeSpeedMult / BR_SPECTATE_FREE_SPEED_STEP;

        //--- Clamped, unlike vanilla's, which happily goes negative and flies the camera backwards
        //--- with no way to tell why. The floor is well below 1 so the camera can be slowed for
        //--- precise framing, which is the other half of what the wheel is for.
        m_FreeSpeedMult = Math.Clamp(m_FreeSpeedMult, BR_SPECTATE_FREE_SPEED_MIN_MULT, BR_SPECTATE_FREE_SPEED_MAX_MULT);

        float speed = BR_SPECTATE_FREE_SPEED * m_FreeSpeedMult;
        if (GetUApi().GetInputByID(UATurbo).LocalValue())
            speed = speed * 2;

        float forward = GetUApi().GetInputByID(UAMoveForward).LocalValue() - GetUApi().GetInputByID(UAMoveBack).LocalValue();
        float strafe = GetUApi().GetInputByID(UAMoveRight).LocalValue() - GetUApi().GetInputByID(UAMoveLeft).LocalValue();

        //--- Look first, then move, so a frame's movement uses that frame's heading.
        float yaw_diff = GetUApi().GetInputByID(UAAimLeft).LocalValue() - GetUApi().GetInputByID(UAAimRight).LocalValue();
        float pitch_diff = GetUApi().GetInputByID(UAAimDown).LocalValue() - GetUApi().GetInputByID(UAAimUp).LocalValue();

        m_Yaw = Math.NormalizeAngle(m_Yaw - Math.RAD2DEG * yaw_diff * timeSlice);
        m_Pitch = Math.Clamp(m_Pitch - Math.RAD2DEG * pitch_diff * timeSlice, -89, 89);

        SetOrientation(Vector(m_Yaw, m_Pitch, 0));

        //--- GetDirection() after SetOrientation, so it reflects the heading just applied.
        vector direction = GetDirection();
        vector direction_aside = vector.Up * direction;

        vector new_pos = m_SmoothPos + (forward * timeSlice * direction * speed) + (strafe * timeSlice * direction_aside * speed);

        float floor_y = GetGame().SurfaceY(new_pos[0], new_pos[2]) + BR_SPECTATE_FLOOR_CLEARANCE;
        if (new_pos[1] < floor_y)
            new_pos[1] = floor_y;

        m_SmoothPos = new_pos;
        SetPosition(new_pos);

    }

    /**
     *  The point the camera looks at.
     *
     *  FOLLOW with a live entity  -> the target's head bone.
     *  FOLLOW without one         -> the last server-pushed position, raised to head height.
     *  ORBIT                      -> the pushed position (the final circle's centre).
     */
    protected vector ResolveAnchor(float timeSlice)
    {
        if (m_Mode == BR_SPECTATE_MODE_ORBIT)
        {
            m_OrbitAngle = Math.NormalizeAngle(m_OrbitAngle + timeSlice * BR_SPECTATE_ORBIT_DEG_PER_SEC);
            return m_TargetPos;
        }

        if (m_Target)
        {
            Human human_target = Human.Cast(m_Target);
            if (human_target)
            {
                int bone = human_target.GetBoneIndexByName("Head");
                if (bone != -1)
                    return m_Target.GetBonePositionWS(bone);
            }

            //--- Bone lookup failed: wrong by about a metre for a prone target, but the floor clamp
            //--- still keeps the camera above ground.
            return m_Target.GetPosition() + "0 1.5 0";
        }

        return m_TargetPos + "0 1.5 0";
    }

    protected float ResolveYaw()
    {
        //--- Orbit: look inward, so the camera trails the orbit angle by half a turn.
        if (m_Mode == BR_SPECTATE_MODE_ORBIT)
            return Math.NormalizeAngle(m_OrbitAngle + 180);

        //--- Follow: sit behind the target, facing the way they face. Their yaw only - never their
        //--- aim pitch, and never their roll.
        if (m_Target)
            return m_Target.GetOrientation()[0];

        //--- No entity to read: hold the last heading rather than snapping to zero.
        return m_Yaw;
    }

    /**
     *  The first-person eye: the head bone, offset in the BONE'S OWN FRAME exactly as vanilla does.
     *
     *  WHY THE FRAME MATTERS, and what the first build got wrong. Vanilla's DayZPlayerCamera1stPerson
     *  puts the camera at m_iDirectBone = Head with m_OffsetLS = "0.04 0.04 0" as the translation of
     *  the camera transform - 4 cm up and 4 cm forward *of the bone*. The first build pushed 22 cm
     *  along the camera's WORLD yaw from the bone origin instead, which is a different place by an
     *  order of magnitude and does not rotate with the head. That is why the view read as "not the
     *  player's camera" even once the look direction was right: the head bone is not where the
     *  player's eye is, and the correction has to be applied in the head's frame.
     *
     *  Falls back to the world-up/forward pair when there is no bone basis to use, which is the
     *  streamed-out case where the anchor is a server-pushed position rather than a bone at all.
     *
     *  NO COLLISION TRACE, unlike the boom. The boom traces because it swings a 3.5 m arm through
     *  whatever is behind the target; 4 cm off a head bone has nothing to hit that the character's
     *  own body is not already standing in, and a trace here would collapse the eye onto the target
     *  every frame - the exact failure the boom's own m_Target exclusion exists to prevent.
     */
    protected vector ResolveEyePosition(vector anchor, float timeSlice)
    {
        vector eye_pos;
        vector forward;

        if (!ResolveHeadFrame(eye_pos, forward))
        {
            //--- Streamed out: the anchor is a server-pushed position, not a bone. Nothing to smooth
            //--- against either, so drop the model-space seed and take the anchor as-is.
            m_FpSeeded = false;

            float yaw_rad = m_Yaw * Math.DEG2RAD;

            vector fallback = anchor;
            fallback[0] = anchor[0] + Math.Sin(yaw_rad) * BR_SPECTATE_FP_OFFSET_FORWARD;
            fallback[1] = anchor[1] + BR_SPECTATE_FP_OFFSET_UP;
            fallback[2] = anchor[2] + Math.Cos(yaw_rad) * BR_SPECTATE_FP_OFFSET_FORWARD;

            return fallback;
        }

        //--- SMOOTHED IN THE CHARACTER'S MODEL SPACE, NOT IN WORLD SPACE, and that distinction is the
        //--- entire bob fix. Head bob is motion of the head RELATIVE TO THE BODY; walking and turning
        //--- are motion of the body through the world. Convert the eye into the character's own frame
        //--- and smooth it there, and the first is damped away while the second is followed exactly,
        //--- with no lag - which is not something world-space damping can do, since it cannot tell the
        //--- two apart and trades bob against the camera dragging behind a sprinting player.
        //---
        //--- The technique is Community-Online-Tools', whose observer camera carries the comment
        //--- "Interpolate in model space so camera sticks to character". Their rate is 5.0 and this
        //--- matches it; the first build's rigid world-space eye and the second's damping of 20 both
        //--- reproduced every stride faithfully, which is exactly what was reported.
        vector object_tm[4];
        m_Target.GetTransform(object_tm);

        vector eye_ms = eye_pos.InvMultiply4(object_tm);

        if (!m_FpSeeded)
        {
            m_FpSeeded = true;
            m_FpEyeMS = eye_ms;
        }
        else
        {
            m_FpEyeMS = vector.Lerp(m_FpEyeMS, eye_ms, Math.Clamp(timeSlice * BR_SPECTATE_FP_SMOOTH, 0, 1));
        }

        return m_FpEyeMS.Multiply4(object_tm);
    }

    /**
     *  Where the target is actually looking, from their HEAD BONE. Returns false when there is no
     *  bone to read, and the caller then falls back to the body yaw.
     *
     *  THE AXIS IS MEASURED, NOT ASSUMED, and that is the whole story of this method. The bone
     *  transform is the rendered answer - the animation result, not a command modifier that
     *  GetCommandModifier_Weapons() may hand back as NULL for a remote entity, which is the trap this
     *  class's header warns about and the reason BR_SPECTATE_PITCH is still a constant for the
     *  third-person boom. But the bone's axis convention is per-bone RIGGING: vanilla's own consumer
     *  of GetBoneRotationWS (P:\scripts\5_mission\gui\cameratools\cameratoolsmenu.c:614) swizzles the
     *  components and adds 325 / 245 / 290 degrees for LeftHand_Dummy, so nothing about the Head bone
     *  is derivable from the source.
     *
     *  So the first build shipped a probe instead of a guess, and the run on 2026-08-18 answered it
     *  outright. With the target standing still at body_yaw 17.647:
     *
     *      head axes x=<-0.0072, 0.9997, 0.0239>  y=<0.3307, -0.0202, 0.9435>  z=<0.9437, 0.0147, -0.3305>
     *
     *  Axis 0 is world UP (its Y component pins to 1.0 in every sample). Axis 1 is the head's FORWARD
     *  vector: atan2(0.3307, 0.9435) = 19.3 degrees against a body yaw of 17.6, i.e. the body's
     *  heading plus the small amount the head was turned. It tracked correctly across nine further
     *  samples spanning the full yaw circle. Axis 2 is the head's right.
     *
     *  Math3D.QuatToAngles is deliberately NOT used, and the same run is why: its first component ran
     *  a consistent ~90 degrees off the body yaw (109.4 against 17.6, 151.7 against 57.2, -86.1
     *  against -172.9), which is exactly the rigging offset above. A direction vector needs no such
     *  correction, so reading the matrix is both simpler and the thing that cannot silently rot.
     */
    /**
     *  The target's head bone as a frame: where the eye goes, and which way it looks.
     *
     *  GetBoneTransformWS rather than GetBoneRotationWS + GetBonePositionWS - one call, and the
     *  translation comes back in the same matrix as the basis. Index 3 is the position, index 0 is
     *  up, index 1 is FORWARD.
     *
     *  THAT AXIS HAS TWO INDEPENDENT CONFIRMATIONS, which is worth recording because it was the open
     *  question for two builds. Measured here on 2026-08-18 (with the target still at body_yaw 17.647,
     *  axis 1 read <0.3307, -0.0202, 0.9435>, i.e. atan2 -> 19.3 degrees), and Community-Online-Tools
     *  independently uses `dir = headTransform[1]` off the same bone for its own first-person
     *  observer camera. Math3D.QuatToAngles is still NOT usable - its first component runs ~90 degrees
     *  off the body yaw, per-bone rigging exactly like the 325/245/290 offsets vanilla's camera tools
     *  apply to LeftHand_Dummy.
     *
     *  The eye offset is applied in the BONE'S frame - vanilla's own m_OffsetLS "0.04 0.04 0", up then
     *  forward (dayzplayercamera1stperson.c:20).
     */
    protected bool ResolveHeadFrame(out vector eye_pos, out vector forward)
    {
        eye_pos = vector.Zero;
        forward = vector.Zero;

        if (!m_Target)
            return false;

        Human human_target = Human.Cast(m_Target);
        if (!human_target)
            return false;

        int bone = human_target.GetBoneIndexByName("Head");
        if (bone == -1)
            return false;

        vector head_tm[4];
        m_Target.GetBoneTransformWS(bone, head_tm);

        vector axis_up = head_tm[0];
        vector axis_forward = head_tm[1];
        vector bone_pos = head_tm[3];

        //--- A bone that has not been posed yet reports a zero or near-zero basis; normalising that
        //--- would divide by ~0 and point the camera at nothing in particular.
        if (axis_forward.Length() < 0.001)
            return false;
        if (axis_up.Length() < 0.001)
            return false;

        axis_up = axis_up.Normalized();
        forward = axis_forward.Normalized();

        vector up_offset = axis_up * BR_SPECTATE_FP_OFFSET_UP;
        vector forward_offset = forward * BR_SPECTATE_FP_OFFSET_FORWARD;

        eye_pos = bone_pos + up_offset + forward_offset;

        return true;
    }

    /**
     *  Where the target's WEAPON is pointing, in world space, or false when they are not holding one.
     *
     *  THIS IS THE ANSWER TO THE PvP CORRECTNESS PROBLEM. The head bone says where a character is
     *  anatomically looking, which is close to but not the same as where their shot goes - so a
     *  spectator watching the head sees the player put rounds into a wall and kill somebody standing
     *  next to it. In a PvP game the centre of the spectator's screen has to be the centre of the
     *  player's, and the only thing authoritative about that is the gun.
     *
     *  It is derived from the WEAPON'S OWN RENDER TRANSFORM plus its barrel memory points, so unlike
     *  every aim accessor it needs nothing that is local-player-only: the weapon is attached to the
     *  character and drawn in the correct orientation on every client, which is exactly what makes it
     *  readable for a REMOTE target. Community-Online-Tools' observer camera derives its aim the same
     *  way ("konec hlavne" to "usti hlavne"), which is where the memory point names come from.
     *
     *  ⚠️ THE ROUTE THIS REPLACES IS NOW MEASURED DEAD, not merely suspect.
     *  GetCommandModifier_Weapons().GetBaseAimingAngleLR/UD is what vanilla composes a world aim from
     *  (weapon_base.c:1707-1710) and the previous build preferred it, with the head bone as fallback
     *  and a trace naming the winner. The run on 2026-08-18 logged `source=bone` on every sample: the
     *  modifier is NULL for a remote entity on a client, exactly as this class's header always
     *  claimed. That claim is no longer an assertion - do not re-add the aim-angle path.
     */
    protected bool ResolveWeaponAim(out vector aim_dir)
    {
        aim_dir = vector.Zero;

        if (!BR_SPECTATE_FP_USE_WEAPON_AIM)
            return false;

        DayZPlayer dayz_target = DayZPlayer.Cast(m_Target);
        if (!dayz_target)
            return false;

        HumanItemAccessor accessor = dayz_target.GetItemAccessor();
        if (!accessor)
            return false;

        //--- Hidden in hands during some animations, and its transform is not to be trusted then.
        if (accessor.IsItemInHandsHidden())
            return false;

        HumanInventory inventory = dayz_target.GetHumanInventory();
        if (!inventory)
            return false;

        Weapon_Base weapon = Weapon_Base.Cast(inventory.GetEntityInHands());
        if (!weapon)
            return false;

        vector weapon_tm[4];
        weapon.GetTransform(weapon_tm);

        vector start_ls = weapon.GetSelectionPositionLS("konec hlavne");
        vector end_ls = weapon.GetSelectionPositionLS("usti hlavne");

        vector start_ws = start_ls.Multiply4(weapon_tm);
        vector end_ws = end_ls.Multiply4(weapon_tm);

        vector barrel = end_ws - start_ws;

        //--- A weapon whose model carries neither memory point hands back two zero vectors, so this
        //--- is also the "not supported by this model" branch rather than only a degenerate one.
        if (barrel.Length() < 0.001)
            return false;

        aim_dir = barrel.Normalized();

        return true;
    }

    /**
     *  The first-person look direction: the weapon when it is being aimed, the head otherwise.
     *
     *  WHY IT IS A CHOICE RATHER THAN JUST THE GUN. A lowered weapon points at the floor while the
     *  player looks at the horizon, so following the barrel unconditionally would be far worse than
     *  the head. What matters is that the two agree precisely at the moment that decides a fight, and
     *  they only agree when the weapon is actually up.
     *
     *  The raised test is therefore THE AGREEMENT ITSELF - the dot product of the barrel against the
     *  head's forward vector - rather than a stance query. Deliberate on two counts: it is
     *  self-correcting, since the barrel is only ever adopted when it is already nearly where the
     *  spectator was looking, so it can never throw the view somewhere surprising; and it avoids
     *  IsRaised() / GetMovementState(), whose behaviour for a remote entity on a client this repo has
     *  no measurement for. COT avoids it in the same place for the same reason and says so.
     */
    protected bool ResolveFirstPersonLook(out float yaw, out float pitch)
    {
        yaw = 0;
        pitch = 0;

        m_LookSource = "none";

        vector eye_pos;
        vector head_forward;

        if (!ResolveHeadFrame(eye_pos, head_forward))
            return false;

        vector chosen = head_forward;
        m_LookSource = "bone";

        vector aim_dir;
        if (ResolveWeaponAim(aim_dir))
        {
            float alignment = vector.Dot(aim_dir, head_forward);
            if (alignment >= BR_SPECTATE_FP_AIM_MIN_DOT)
            {
                chosen = aim_dir;
                m_LookSource = "weapon";
            }
        }

        float dir_x = chosen[0];
        float dir_y = chosen[1];
        float dir_z = chosen[2];

        yaw = Math.Atan2(dir_x, dir_z) * Math.RAD2DEG;

        pitch = Math.Asin(Math.Clamp(dir_y, -1.0, 1.0)) * Math.RAD2DEG;
        pitch = Math.Clamp(pitch, BR_SPECTATE_FP_PITCH_MIN, BR_SPECTATE_FP_PITCH_MAX);

        return true;
    }

    /**
     *  Hide the watched player's head while the eye is inside it, and put it back afterwards.
     *
     *  WHY THIS IS NEEDED AT ALL, and why vanilla's offsets alone are not enough. Vanilla's first
     *  person camera sits 4 cm from the Head bone and gets away with it because it renders the LOCAL
     *  player, whose first-person model has no head. A spectator is watching a REMOTE character drawn
     *  in full, so the same 4 cm puts the eye inside their skull - reported as "I see the inside of
     *  the spectated player head". Pushing the camera out in front of the face instead was the first
     *  build's approach and is what made the perspective wrong; the head has to go, not the camera.
     *
     *  COT'S API, CALLED, NOT COPIED - the same licence position as JMESPSkeleton, and for the same
     *  reason: Community-Online-Tools is CC BY-SA 4.0 and this repo is DSPL-SA, so adapting their
     *  code is blocked, but calling a published method on a class they mod is interoperation and
     *  carries no obligation. SetHeadInvisible is theirs, and it covers the head placeholder plus the
     *  Headgear, Mask and Eyewear attachments - all four of which would otherwise still be floating
     *  in front of the camera. Their own observer camera calls it on the spectated player the same
     *  way, which is also the evidence that it is safe on a remote entity client-side.
     *
     *  No distance test, unlike COT's 0.25 m one: their camera can be anywhere, ours is pinned to the
     *  bone by construction, so "first person is active on a live target" is the same condition.
     *
     *  ⚠️ RESTORING IT IS THE HALF THAT BITES. A head left hidden is a headless character for the
     *  rest of the session, so every exit routes here: the third-person branch each frame, a retarget
     *  (the equality test below catches it, and restores the OLD player rather than the current one),
     *  and the destructor.
     */
    protected void ApplyHeadHide(bool wanted)
    {
#ifdef JM_COT
        if (!BR_SPECTATE_FP_HIDE_HEAD)
            return;

        PlayerBase target_player = NULL;
        if (wanted)
            target_player = PlayerBase.Cast(m_Target);

        //--- Runs every frame, so the common case has to be a pointer compare and nothing else.
        if (m_HeadHiddenPlayer == target_player)
            return;

        if (m_HeadHiddenPlayer)
            m_HeadHiddenPlayer.SetHeadInvisible(false);

        m_HeadHiddenPlayer = target_player;

        if (m_HeadHiddenPlayer)
            m_HeadHiddenPlayer.SetHeadInvisible(true);
#endif
    }

    /**
     *  Match the player's own field of view.
     *
     *  A bare Camera runs at the engine's default scene FOV - vanilla names it as 0.5236 rad / 30
     *  degrees in plugincharplacement.c:15 - while both of vanilla's PLAYER cameras run at
     *  GetUserFOV() via StdFovUpdate -> GetFOVByZoomType(ECameraZoomType.NONE)
     *  (dayzplayercamera_base.c:323). Never calling SetFOV therefore gave the spectator a permanently
     *  different, much tighter view than the player being watched, which is what #288 came back on.
     *
     *  APPLIED IN BOTH MODES, not just first person. The Camera is one object with one FOV, so third
     *  person was equally wrong - it just had nothing to be compared against. Vanilla's own third
     *  person camera uses the same GetUserFOV(), so this matches the game in both.
     *
     *  SetFOV takes RADIANS (camera.c:63-67) and GetUserFOV() is already in radians - vanilla passes
     *  one straight to the other at plugincharplacement.c:66, so there is no conversion here. The
     *  change guard is not for cost but so a user who moves the FOV slider mid-match is picked up.
     */
    protected void ApplyFieldOfView()
    {
        float wanted = GetDayZGame().GetUserFOV();
        if (wanted <= 0)
            return;

        if (Math.AbsFloat(wanted - m_AppliedFov) < 0.0001)
            return;

        m_AppliedFov = wanted;
        SetFOV(wanted);

        BattleRoyaleUtils.Debug("[Spectate] camera FOV set to " + wanted + " rad");
    }

    /**
     *  Tighten the near plane while the eye is 4 cm from a character's skull, and put it back for the
     *  third-person boom.
     *
     *  Vanilla's first person camera sets m_fNearPlane = 0.04 and notes "0.07 default"
     *  (dayzplayercamera1stperson.c:58). Without it the watched character's own head geometry clips
     *  through the view - which reads as the picture being wrong rather than as clipping, and is a
     *  plausible part of what "the FOV is still not good" was describing.
     */
    protected void ApplyNearPlane(bool first_person)
    {
        float wanted = BR_SPECTATE_NEAR_PLANE_DEFAULT;
        if (first_person)
            wanted = BR_SPECTATE_FP_NEAR_PLANE;

        if (Math.AbsFloat(wanted - m_AppliedNearPlane) < 0.0001)
            return;

        m_AppliedNearPlane = wanted;
        SetNearPlane(wanted);
    }

    /**
     *  Push the camera back off the anchor, then pull it in if geometry is in the way.
     *
     *  In ORBIT the boom is the wide zone-overview arm instead.
     */
    protected vector ResolveBoom(vector anchor)
    {
        float yaw_rad = m_Yaw * Math.DEG2RAD;
        float back = BR_SPECTATE_BOOM_BACK;
        float up = BR_SPECTATE_BOOM_UP;

        if (m_Mode == BR_SPECTATE_MODE_ORBIT)
        {
            back = BR_SPECTATE_ORBIT_RADIUS;
            up = BR_SPECTATE_ORBIT_HEIGHT;
        }

        vector desired = anchor;
        desired[0] = anchor[0] - Math.Sin(yaw_rad) * back;
        desired[1] = anchor[1] + up;
        desired[2] = anchor[2] - Math.Cos(yaw_rad) * back;

        //--- No collision trace in orbit: the arm is 120 m and would snap to every treetop.
        if (m_Mode == BR_SPECTATE_MODE_ORBIT)
            return desired;

        vector contact_pos;
        vector contact_dir;
        int contact_component;

        //--- Ignore the target itself, or the boom collapses onto their own body every frame.
        if (DayZPhysics.RaycastRV(anchor, desired, contact_pos, contact_dir, contact_component, NULL, NULL, m_Target, false, false, ObjIntersectView, BR_SPECTATE_CAM_RADIUS))
        {
            //--- Normalized(), not a manual divide: EnfusionScript has no vector/float operator.
            vector pull_dir = (anchor - desired).Normalized();
            desired = contact_pos + (pull_dir * BR_SPECTATE_CAM_SKIN);
        }

        return desired;
    }
}
