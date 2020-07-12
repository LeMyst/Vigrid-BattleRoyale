#define BR_BETA_LOGGING

class BattleRoyaleCountReached extends BattleRoyaleDebugState
{
    protected int i_TimeToStart;
    protected bool b_ReadyToStart;
    
    void BattleRoyaleCountReached()
    {
        #ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleCountReached::Constructor()");
		#endif

        i_TimeToStart = 30; //TODO: pull from config
        
        b_ReadyToStart = false;
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
		return b_ReadyToStart || super.IsComplete();
	}
    void DoStart()
    {
        b_ReadyToStart = true;
    }
}