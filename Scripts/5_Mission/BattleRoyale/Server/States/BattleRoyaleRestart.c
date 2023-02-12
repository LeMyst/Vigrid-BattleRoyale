class BattleRoyaleRestart extends BattleRoyaleState
{
    protected ref Timer m_ShutdownTimer;
    override void Activate()
    {
        super.Activate();
        Print("[Restart State] Restarting!");

        m_ShutdownTimer = AddTimer(10.0, this, "Shutdown", NULL, false);
    }

    override string GetName()
    {
        return DAYZBR_SM_RESTART_NAME;
    }

    override void Deactivate()
    {
        Error("RESTART STATE WAS DEACTIVATED!");
        m_ShutdownTimer.Stop();
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
