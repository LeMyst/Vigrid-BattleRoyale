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
 *    - Camera.LookAt is not used and the target's aim angle is never read.
 *      GetCommandModifier_Weapons() can be NULL for a remote entity, in which case
 *      GetBaseAimingAngleUD() returns 0.0 and the camera silently goes flat with nothing in the log.
 *      Orientation is a damped yaw/pitch pair instead, and roll is never taken from the target.
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

        //--- Shortest-arc yaw damping.
        float delta = Math.NormalizeAngle(desired_yaw - m_Yaw);
        if (delta > 180)
            delta = delta - 360;

        m_Yaw = Math.NormalizeAngle(m_Yaw + delta * Math.Clamp(timeSlice * BR_SPECTATE_YAW_DAMP, 0, 1));

        if (m_Mode == BR_SPECTATE_MODE_ORBIT)
            m_Pitch = BR_SPECTATE_ORBIT_PITCH;
        else
            m_Pitch = BR_SPECTATE_PITCH;

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
