class BattleRoyaleCountReached extends BattleRoyaleDebugState
{
    protected int i_TimeToStart;
    protected ref Timer m_StartTimer;

    void BattleRoyaleCountReached()
    {

        BattleRoyaleDebugData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();
        if(m_DebugSettings)
        {
            i_TimeToStart = m_DebugSettings.time_to_start_match_seconds;
        }
        else
        {
            Error("FAILED TO READ DEBUG SETTINGS");
            i_TimeToStart = 30;
        }
    }
    override string GetName()
	{
		return DAYZBR_SM_COUNT_REACHED_NAME;
	}

	override void Activate()
	{
		super.Activate();

        string second = "second";
        if(i_TimeToStart != 1)
            second = "seconds";

        //TODO: use string replace and make this string a constant in BattleRoyaleConstants (perhaps a setting eventually)

        MessagePlayers("Player count reached! Match is starting in " + i_TimeToStart.ToString() + " " + second + "!");
        m_StartTimer = AddTimer(i_TimeToStart, this, "DoStart", NULL, false);
	}
	override void Deactivate()
	{
        m_StartTimer.Stop();
		super.Deactivate();
	}

	override bool IsComplete()
	{
		return super.IsComplete();
	}

    void DoStart()
    {
        Deactivate();
    }
}
