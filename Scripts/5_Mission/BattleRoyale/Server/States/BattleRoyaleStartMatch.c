#define BR_BETA_LOGGING

class BattleRoyaleStartMatch extends BattleRoyaleState
{
    protected int i_TimeToUnlock;
    protected bool b_HasStarted;
    protected int i_FirstRoundDelay;

    void BattleRoyaleStartMatch()
    {
        #ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleStartMatch::Constructor()");
		#endif

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
		BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
		i_FirstRoundDelay = (60 * m_GameSettings.round_duration_minutes) / 2;

        //seconds until unlock
        i_TimeToUnlock = m_GameSettings.time_until_teleport_unlock;

        b_HasStarted = false;
    }

	override void Activate()
	{
		super.Activate();

        int max_time = i_TimeToUnlock - 1;
        for(int i = max_time;i > 0;i--)
        {
            m_CallQueue.CallLater(this.MessageUnlock, i*1000, false, i_TimeToUnlock - i); //30 seconds until zone locks
        }
        m_CallQueue.CallLater(this.UnlockPlayers, i_TimeToUnlock*1000, false); //delay before we unlock player input
        
        m_CallQueue.CallLater(this.StartZoning, i_FirstRoundDelay*1000, false); //delay before first zone appears
	}
	override void Deactivate()
	{
		super.Deactivate();
	}
    
	override bool IsComplete()
	{
		return b_HasStarted || super.IsComplete();
	}

    void MessageUnlock(int seconds_till)
    {
        string second = "second";
        if(seconds_till != 1)
            second = "seconds";

        MessagePlayers("Starting in " + seconds_till.ToString() + " " + second + "!");
    }
    void UnlockPlayers()
    {
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(false), true); //enable user input
        MessagePlayers("The match has started!");
    }
    void StartZoning()
    {
        b_HasStarted = true;
    }
}