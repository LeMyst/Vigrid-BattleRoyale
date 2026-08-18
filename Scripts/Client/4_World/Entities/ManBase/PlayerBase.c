modded class PlayerBase
{
#ifndef SERVER
    //credit to wardog for the quick fix for client localplayers grabbing
    private static autoptr array<PlayerBase> s_LocalPlayers = new array<PlayerBase>();

    void PlayerBase()
    {
        if (s_LocalPlayers)
        {
            s_LocalPlayers.Insert(this);
        }
    }

    void ~PlayerBase()
    {
        if (s_LocalPlayers)
        {
            int localIndex = s_LocalPlayers.Find(this);
            if (localIndex >= 0)
            {
                s_LocalPlayers.Remove(localIndex);
            }
        }
    }

    static void GetLocalPlayers(out array<PlayerBase> players)
    {
        players = new array<PlayerBase>();
        players.Copy(s_LocalPlayers);
    }

    //--- The client half of the teleport juncture. The server half (which is what actually moves the
    //--- player) lives in Scripts/Server/4_World/.../PlayerBase.c, and being #ifdef SERVER it does
    //--- not exist here - so before this, a teleported client kept locally predicting whatever
    //--- movement command it was in and went on playing the climb animation at the new position.
    //---
    //--- Position is deliberately NOT read or set here: it already arrives through ordinary network
    //--- sync, and re-applying it from a juncture that may land on a different simulation step would
    //--- only add a pop. All this side owes is to stop predicting the old command.
    override void OnSyncJuncture(int pJunctureID, ParamsReadContext pCtx)
    {
        super.OnSyncJuncture(pJunctureID, pCtx);

        //--- The client does receive this - verified by instrumenting both sides, which was worth
        //--- doing precisely because the server half lives in an #ifdef SERVER file and nothing had
        //--- ever proven the client half fired.
        if ( pJunctureID == BR_SYNC_JUNCTURE_TELEPORT )
            BR_NotifyTeleported();
    }
#endif

    //--- True while a movement command is running that vanilla pairs with an inventory lock:
    //--- OnCommandFallStart / ClimbStart / LadderStart / SwimStart each take LOCK_FROM_SCRIPT and
    //--- only release it in the matching ...Finish (playerbase.c:3964-4085).
    //---
    //--- Unguarded on purpose. The server needs it to decide whether a player can be dressed at all,
    //--- and the client needs the same answer to stop predicting a climb it is no longer doing - and
    //--- the two must never disagree about what counts.
    bool BR_IsInLockingMovementCommand()
    {
        if ( GetCommand_Fall() )
            return true;
        if ( GetCommand_Climb() )
            return true;
        if ( GetCommand_Ladder() )
            return true;
        if ( GetCommand_Swim() )
            return true;

        return false;
    }

#ifdef DIAG_DEVELOPER
    //--- Ticks of CommandHandler still to log after a teleport juncture. See BR_LogTeleportState.
    protected int m_BR_TraceTicksLeft = 0;

    /**
     *  One line describing this character's command state, for the teleport trace.
     *
     *  THIS IS THE MEASUREMENT, not a convenience. Three explanations of the "F2 unstuck on a ladder
     *  leaves the player pinned in the ladder animation" bug have been wrong, two of them refuted by
     *  a build each, and CLAUDE.md's standing instruction is to instrument before writing a fourth.
     *  What it distinguishes is exactly one thing: whether the LADDER COMMAND is still running after
     *  the juncture (command stuck - HumanCommandLadder.Exit is then the lead) or whether the command
     *  is already correct and only the animation graph is wedged (in which case every command-level
     *  lead is dead, which is also why a jump cures it).
     *
     *  Lives in the UNGUARDED part of this file on purpose. Only the s_LocalPlayers / OnSyncJuncture
     *  block above is #ifndef SERVER; everything from here down compiles and runs on both sides,
     *  because Scripts/Server lists this addon in requiredAddons and mods over it. So this one method
     *  and the one call in CommandHandler instrument the client AND the server.
     *
     *  GetGame().GetTime() is included so the two logs can be aligned afterwards - the two processes
     *  write separate files and nothing else in the line is comparable between them.
     */
    void BR_LogTeleportState(string tag)
    {
        if ( !BattleRoyaleDiag.trace_teleport )
            return;

        string side = "client";
        if ( GetGame().IsServer() )
            side = "server";

        string commands = "";
        if ( GetCommand_Ladder() )
            commands = commands + "Ladder ";
        if ( GetCommand_Climb() )
            commands = commands + "Climb ";
        if ( GetCommand_Fall() )
            commands = commands + "Fall ";
        if ( GetCommand_Move() )
            commands = commands + "Move ";
        if ( GetCommand_Script() )
            commands = commands + "Script ";
        if ( GetCommand_Vehicle() )
            commands = commands + "Vehicle ";
        if ( commands == "" )
            commands = "none";

        //--- Built one statement at a time: EnfusionScript does not accept an expression continued
        //--- across lines, the same restriction that forbids a multi-line `if` condition.
        bool on_ladder = false;
        if ( GetCommand_Ladder() )
            on_ladder = true;

        string line = "[TPTrace][" + side + "][" + tag + "] t=" + GetGame().GetTime();
        line = line + " type=" + Type().ToString();
        line = line + " cmdID=" + GetCurrentCommandID();
        line = line + " ladder=" + on_ladder;
        line = line + " running={" + commands + "}";
        line = line + " falling=" + PhysicsIsFalling(true);
        line = line + " pos=" + GetPosition();
        line = line + " forceMove=" + m_BR_ForceMoveRequested;
        line = line + " settle=" + m_BR_PostTeleportSettle;

        BattleRoyaleUtils.Debug(line);
    }

    //! Start (or restart) the post-teleport tick window. Called from BR_NotifyTeleported, so both
    //! halves of the juncture open the window without either having to remember to.
    void BR_BeginTeleportTrace()
    {
        if ( !BattleRoyaleDiag.trace_teleport )
            return;

        m_BR_TraceTicksLeft = BattleRoyaleDiag.trace_teleport_ticks;
    }

    //--- Aim trace state: the earliest GetTime() ms at which the next heartbeat line may go out,
    //--- and the last values reported, so a real change logs immediately regardless of the clock.
    protected int m_BR_AimTraceNextMs = 0;
    protected float m_BR_AimTraceLastUD = 0;
    protected bool m_BR_AimTraceHadHCW = false;

    /**
     *  One line describing this instance's copy of the base aiming angles.
     *
     *  THE INSTRUMENT THAT FOUND THE REMOTE-ADS-PITCH DESYNC. Aim pitch is never replicated as an
     *  absolute value - HumanInputController.GetAimChange (human.c:31) is a per-tick delta, so the
     *  owner and the server each integrate their OWN copy, and a copy that freezes while the other
     *  keeps moving stays offset until the real aim saturates the pitch clamp (the players' "sweep
     *  full up then full down" workaround). Diffing the two copies of one player - matched on net=
     *  across machines - is what convicted hic.SetDisabled in DisableInput below.
     *
     *  TWO copies, not three, and that is itself a measurement (2026-08-18): a REMOTE instance
     *  never runs this override - zero trace lines from proxies on a client with the toggle on -
     *  so proxies hold no integrated aim copy at all and what an observer renders follows the
     *  SERVER's copy. The server's log carries every player; each client's log carries only its
     *  own. Do not build anything on "the proxy's copy of the aim"; there is none reachable here.
     *
     *  Lives in the UNGUARDED part of this file for the same reason BR_LogTeleportState does: one
     *  method instruments both sides.
     *
     *  Throttled to a 500 ms heartbeat per instance, but a change of more than a degree - or the
     *  weapons modifier appearing/disappearing - logs at once, so the interesting edges (a command
     *  restart zeroing the angle) land on the tick they happen.
     */
    void BR_LogAimState()
    {
        if ( !BattleRoyaleDiag.trace_aim )
            return;

        HumanCommandWeapons hcw = GetCommandModifier_Weapons();

        float ud = 0;
        bool has_hcw = false;
        if ( hcw )
        {
            has_hcw = true;
            ud = hcw.GetBaseAimingAngleUD();
        }

        int now = GetGame().GetTime();

        bool due = false;
        if ( now >= m_BR_AimTraceNextMs )
            due = true;
        if ( has_hcw != m_BR_AimTraceHadHCW )
            due = true;
        if ( Math.AbsFloat( ud - m_BR_AimTraceLastUD ) > 1.0 )
            due = true;

        if ( !due )
            return;

        m_BR_AimTraceNextMs = now + 500;
        m_BR_AimTraceLastUD = ud;
        m_BR_AimTraceHadHCW = has_hcw;

        string side = "client";
        if ( GetGame().IsServer() )
            side = "server";

        //--- The network id is the one identifier every machine agrees on for every instance - a
        //--- proxy's identity name would work too, but the id needs no identity to be resolvable.
        int net_low = 0;
        int net_high = 0;
        GetNetworkID( net_low, net_high );

        //--- Built in steps: the funnel shape trips the expression-complexity ceiling.
        string line = "[AimTrace][" + side + "] t=" + now;
        line = line + " inst=" + typename.EnumToString( DayZPlayerInstanceType, GetInstanceType() );
        line = line + " net=" + net_low;
        line = line + " cmd=" + GetCurrentCommandID();
        line = line + " hcw=" + has_hcw;
        line = line + " ud=" + ud;
        if ( hcw )
            line = line + " lr=" + hcw.GetBaseAimingAngleLR();

        BattleRoyaleUtils.Debug( line );
    }
#endif

    //--- Pending deferred reset, consumed by CommandHandler below.
    protected bool m_BR_ForceMoveRequested = false;

    //--- Seconds left of the post-teleport window in which CommandHandler keeps asking the
    //--- controller whether it is airborne. See BR_TELEPORT_SETTLE_SECONDS.
    protected float m_BR_PostTeleportSettle = 0;

    /**
     *  Tell this character it has just been teleported. **This is the one to call.**
     *
     *  Called from both halves of the BR_SYNC_JUNCTURE_TELEPORT handler, and the only thing that
     *  makes a teleport end in an actual command transition.
     *
     *  It only records the request; CommandHandler does the work. Vanilla never starts a command
     *  anywhere but inside CommandHandler, and always returns immediately afterwards
     *  (dayzplayerimplement.c:2366-2400 does it four times in a row). Starting one from outside that
     *  tick - from a juncture handler, say - produces a player who *looks* right, standing in a
     *  normal pose, but whose movement input is not driving anything: they are pinned in place until
     *  some real command transition happens and resyncs them. Jumping once was the observed cure,
     *  which is exactly that.
     *
     *  This used to be gated on BR_IsInLockingMovementCommand, so that only a player stuck in a
     *  fall/climb/ladder/swim was ever reset. A player teleported out of an ordinary standing Move -
     *  which is every player at match start - failed that gate, the request was dropped, and neither
     *  side ever told the engine anything had changed. That is what left characters hovering above
     *  the ground until their first input.
     */
    void BR_NotifyTeleported()
    {
#ifdef DIAG_DEVELOPER
        //--- Logged BEFORE the two early returns, so a teleport that this method declines to act on
        //--- is still visible in the trace. Both halves of the juncture funnel through here, so one
        //--- call covers client and server and the two lines are directly comparable.
        BR_BeginTeleportTrace();
        BR_LogTeleportState("juncture");
#endif

        //--- A teleport must not yank somebody out of a vehicle seat or a scripted command; that
        //--- narrowness used to come free from the locking-command gate, so it is stated here
        //--- instead now that the gate is gone.
        if ( GetCommand_Vehicle() )
            return;
        if ( GetCommand_Script() )
            return;

        m_BR_ForceMoveRequested = true;
        m_BR_PostTeleportSettle = BR_TELEPORT_SETTLE_SECONDS;
    }

    //--- Immediate variant, for the one caller that needs the side effect *now* rather than next
    //--- tick: BattleRoyalePrepare has to get vanilla to drop its inventory lock before it can
    //--- create the starting clothes, and it cannot yield. Safe there specifically because player
    //--- input is disabled for the whole of that state and a teleport follows immediately, so the
    //--- pinned-input problem described above has no chance to be observed.
    //---
    //--- Still gated on the locking commands, unlike BR_NotifyTeleported: this one exists to undo an
    //--- inventory lock, and a player who does not hold one has nothing here to fix.
    bool BR_ForceMoveCommandImmediate()
    {
        if ( !BR_IsInLockingMovementCommand() )
            return false;

        StartCommand_Move();
        return true;
    }

    //--- Hand the character to the Fall command from wherever it is standing right now. Vanilla
    //--- pairs these two calls everywhere it leaves a command into a fall
    //--- (dayzplayerimplement.c:2383-2387, 2540-2545) and the YDiff is what the landing measures its
    //--- drop against, so they must not be separated.
    protected void BR_StartFallFromHere()
    {
        StartCommand_Fall(0);
        SetFallYDiff( GetPosition()[1] );
    }

    //--- Consume the request at the only point the engine sanctions for it. Returning right after
    //--- StartCommand_Move mirrors vanilla, which never falls through to the rest of the handler on
    //--- a frame where it changed command.
    override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
    {
#ifdef DIAG_DEVELOPER
        //--- The "few ticks after" half of the measurement. This override is outside the
        //--- #ifndef SERVER block above, so this single call instruments both sides.
        if ( m_BR_TraceTicksLeft > 0 )
        {
            m_BR_TraceTicksLeft = m_BR_TraceTicksLeft - 1;
            BR_LogTeleportState("tick");
        }

        //--- Sampled BEFORE the force-move consume below, so the last pre-restart angle is on
        //--- record; the post-restart value logs next tick through the on-change trigger.
        BR_LogAimState();
#endif

        if ( m_BR_ForceMoveRequested )
        {
            m_BR_ForceMoveRequested = false;

            //--- Re-checked rather than trusted from BR_NotifyTeleported: a tick has passed since
            //--- the juncture, and ejecting somebody from a vehicle seat is not a teleport's job.
            if ( !GetCommand_Vehicle() && !GetCommand_Script() )
            {
#ifdef DIAG_DEVELOPER
                //--- The event the aim trace exists to bracket: mark it so the log needs no
                //--- inference about which tick the restart landed on, on which instance.
                if ( BattleRoyaleDiag.trace_aim )
                    BattleRoyaleUtils.Debug( "[AimTrace] command restart on " + typename.EnumToString( DayZPlayerInstanceType, GetInstanceType() ) );
#endif
                //--- Mirror vanilla's own sequence for leaving a command
                //--- (dayzplayerimplement.c:2381-2400): if physics says we are airborne, hand over
                //--- to Fall so the engine runs its normal landing rather than dropping the player
                //--- straight into a move they are not standing up for; otherwise start the move.
                //---
                //--- The move is unconditional, and that is the point of it. It is a genuine command
                //--- transition, which is exactly what a single jump used to supply by hand for a
                //--- player left pinned by a teleport - and it is cheap for everybody else, since
                //--- restarting Move from Move is what vanilla itself does every time a command
                //--- finishes.
                if ( PhysicsIsFalling(true) )
                {
                    BR_StartFallFromHere();
                    return;
                }

                StartCommand_Move();
                return;
            }
        }

        //--- The controller does not re-evaluate its ground contact when a script moves the
        //--- character, so the one-shot above can honestly answer "not airborne" while standing a
        //--- metre up - and on the client it can run before the corrected position has even arrived.
        //--- Keep asking for a moment. Only a fall is started here: the move has already happened,
        //--- and re-issuing one every frame would fight whatever the player has started doing since.
        if ( m_BR_PostTeleportSettle > 0 )
        {
            m_BR_PostTeleportSettle = m_BR_PostTeleportSettle - pDt;

            if ( !GetCommand_Fall() && PhysicsIsFalling(true) )
            {
                m_BR_PostTeleportSettle = 0;
                BR_StartFallFromHere();
                return;
            }
        }

        super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);
    }

    void DisableInput(bool disabled)
    {
    	if ( disabled )
        	BattleRoyaleUtils.Trace( "Call To Disable Player Input" );
		else
			BattleRoyaleUtils.Trace( "Call To Enable Player Input" );

		//--- Voice is deliberately NOT touched here. This method runs on the client for states 2 and
		//--- 3 (driven by the SetInput RPC) where MuteAllPlayers/EnableVoN are no-ops, because the
		//--- VON router only answers to the server - so the gag this used to attempt never happened.
		//--- On the server, in state 4, it did work, but it gagged everyone globally and would defeat
		//--- party voice during prepare. BattleRoyaleVoice is now the single owner of voice policy.

		HumanInputControllerOverrideType override_type = HumanInputControllerOverrideType.DISABLED;
		if ( disabled )
		{
			// disabled means we want to enable the override
			override_type = HumanInputControllerOverrideType.ENABLED;
		}

        SetSynchDirty();

        HumanInputController hic = GetInputController();
        if ( hic )
		{
			hic.OverrideMovementSpeed( override_type, 0 );
			hic.OverrideMovementAngle( override_type, 0 );
			hic.OverrideMeleeEvade( override_type, false );
			hic.OverrideRaise( override_type, false );
			hic.OverrideFreeLook( override_type, false );

			//--- hic.SetDisabled() used to be called here, and IT WAS THE REMOTE-ADS-PITCH DESYNC.
			//--- Measured 2026-08-17 (Run A of the aim trace): through the lobby, owner and server
			//--- integrate the aim angles in perfect lockstep - then the moment this method ran with
			//--- disabled=true, the SERVER's copy froze at its last value while the OWNER's kept
			//--- moving (a constant-rate ramp to the +85 clamp), and the teleport restart later
			//--- rode the server's copy to the OPPOSITE clamp. Post-unlock the two copies mirrored
			//--- every delta again (lr identical to three decimals) but the ud baselines stayed
			//--- ~80 degrees apart - B aiming level read as aiming into the ground on every other
			//--- screen, until the player saturated the pitch clamp by sweeping full up then down.
			//---
			//--- Remote clients render the SERVER's copy (remote instances run no script
			//--- CommandHandler - measured, zero trace lines), so a frozen server copy is what every
			//--- other player sees. The freeze itself is carried by the overrides above, every one
			//--- of which has vanilla call sites; SetDisabled has ZERO in all of P:\scripts, the
			//--- same unvetted-symbol class as OverrideAimChangeX/Y. Do not put it back.
		}
    }
}
