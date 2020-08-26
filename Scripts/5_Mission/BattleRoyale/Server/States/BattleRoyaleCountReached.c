#define BR_BETA_LOGGING

class BattleRoyaleCountReached extends BattleRoyaleDebugState
{
    protected int i_TimeToStart;
    
    void BattleRoyaleCountReached()
    {
        #ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleCountReached::Constructor()");
		#endif

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
		return "Player Count Reached State";
	}

	override void Activate()
	{
		super.Activate();
		
        string second = "second";
        if(i_TimeToStart != 1)
            second = "seconds";

        MessagePlayers("Player count reached! Match is starting in " + i_TimeToStart.ToString() + " " + second + "!");
        m_CallQueue.CallLater(this.DoStart, i_TimeToStart*1000, false);
	}
	override void Deactivate()
	{
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