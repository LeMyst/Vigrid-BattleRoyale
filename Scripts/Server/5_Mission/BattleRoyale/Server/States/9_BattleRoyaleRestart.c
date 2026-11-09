#ifdef SERVER
class BattleRoyaleRestart: BattleRoyaleState
{
    protected ref Timer m_ShutdownTimer;

    override void Activate()
    {
        super.Activate();

        //--- THE guaranteed leaderboard flush. There is no reliable shutdown hook in DayZ -
        //--- Mission.OnMissionFinish() is an empty stub MissionServer never overrides - but this
        //--- state is reached by every match end, including "everyone rage-quit", and it sits a
        //--- full 10 seconds ahead of RequestExit. Every other flush in the match is an optimisation
        //--- over this one.
        BattleRoyaleLeaderboard.GetInstance().EndMatch();

        //--- THE match summary write, and the only one. Here rather than in 8_BattleRoyaleWin for a
        //--- reason that is easy to miss: the winner's RecordExit does not run until KickWinner,
        //--- fifteen seconds AFTER that state activates, so a write there produces a file with no
        //--- placement and no survival time for the winner - the one row everybody looks at first.
        //--- Restart activates only once Win has deactivated, so every row exists by now.
        //---
        //--- Deliberately not paired with a belt in 8_BattleRoyaleWin: that write would be the bad
        //--- one, and any latch protecting it would then refuse this good one.
        BattleRoyaleMatchStats.GetInstance().EndMatch();

        //--- Same reasoning for spectators: 8_BattleRoyaleWin normally gets here first, but this
        //--- state is reached by every match end. EndAll() latches, so the second call is a no-op.
        //--- Nothing here can delay the shutdown - IsComplete() is unconditionally false and the
        //--- 10 s timer below holds no spectator state.
        BattleRoyaleSpectators.GetInstance().EndAll();

        //--- The in-place reset (server_settings.json in_place_reset), when it can run: the next
        //--- match starts in this process and this state is simply dropped with the old server.
        //--- Begin() only arms it - the work runs from MissionServer.OnUpdate, outside this frame.
        //--- Any reason it cannot run is logged there, and the restart below stays the fallback.
        if ( BattleRoyaleInPlaceReset.Begin() )
            return;

        BattleRoyaleUtils.Trace("[Restart State] Restarting!");

        m_ShutdownTimer = AddTimer(10.0, this, "Shutdown", NULL, false);
    }

    override string GetName()
    {
        return "Restart State";
    }

    //--- Deactivate() stops the shutdown timer and IsComplete() is always false: the server would never exit.
    override bool AllowsAdminSkip()
    {
        return false;
    }

    override bool IsMatchOver()
    {
        return true;
    }

    override void Deactivate()
    {
        //--- Warn, not Error(): Error() unwinds the stack (memory: BattleRoyaleUtils.Error() is fatal),
        //--- which would take the rest of the caller down with it for a message.
        BattleRoyaleUtils.Warn("RESTART STATE WAS DEACTIVATED!");
        if ( m_ShutdownTimer && m_ShutdownTimer.IsRunning() )
        {
            m_ShutdownTimer.Stop();
        }
        
        super.Deactivate();
    }

    override bool IsComplete()
    {
        return false;
    }

    void Shutdown()
    {
        GetGame().RequestExit(0);
    }
}
#endif
