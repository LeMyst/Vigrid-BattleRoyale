#define BR_BETA_LOGGING

class BattleRoyalePrepare extends BattleRoyaleState
{
    protected int i_TimeToUnlock;
    protected bool b_HasStarted;

    void BattleRoyalePrepare()
    {
        #ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyalePrepare::Constructor()");
		#endif
        //seconds until unlock
        i_TimeToUnlock = 10;
        b_HasStarted = false;
    }

	override void Activate()
	{
		super.Activate();

        for(int i = (i_TimeToUnlock-1); i > 0;i--)
        {
            m_CallQueue.CallLater(this.MessageUnlock, i*1000, false, i_TimeToUnlock - i); //30 seconds until zone locks
        }
        m_CallQueue.CallLater(this.UnlockPlayers, i_TimeToUnlock*1000, false);
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
        //TODO: RPC to unlock players
        MessagePlayers("The match has started!");
        b_HasStarted = true;
    }
}