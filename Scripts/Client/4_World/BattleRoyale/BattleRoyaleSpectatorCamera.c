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

        vector anchor = ResolveAnchor(timeSlice);
        float desired_yaw = ResolveYaw();

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
