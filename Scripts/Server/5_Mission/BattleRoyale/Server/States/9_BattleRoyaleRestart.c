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

        BattleRoyaleUtils.Trace("[Restart State] Restarting!");

        m_ShutdownTimer = AddTimer(10.0, this, "Shutdown", NULL, false);
    }

    override string GetName()
    {
        return "Restart State";
    }

    override void Deactivate()
    {
        Error("RESTART STATE WAS DEACTIVATED!");
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
