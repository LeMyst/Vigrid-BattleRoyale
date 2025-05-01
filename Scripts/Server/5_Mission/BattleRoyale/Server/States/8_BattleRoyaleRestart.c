#ifdef SERVER
class BattleRoyaleRestart: BattleRoyaleState
{
    protected ref Timer m_ShutdownTimer;

    override void Activate()
    {
        super.Activate();

        BattleRoyaleUtils.Trace("[Restart State] Restarting!");

        m_ShutdownTimer = AddTimer(10.0, this, "Shutdown", NULL, false);
    }

    override string GetName()
    {
        return DAYZBR_SM_RESTART_NAME;
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
