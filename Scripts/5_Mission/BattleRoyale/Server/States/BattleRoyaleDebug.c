class BattleRoyaleDebug extends BattleRoyaleDebugState {
	protected int i_MinPlayers;
	protected int i_TimeBetweenMessages;
	
	void BattleRoyaleDebug()
	{	
		BattleRoyaleDebugData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();
		
		if(m_DebugSettings)
		{
			i_MinPlayers = m_DebugSettings.minimum_players; 
			i_TimeBetweenMessages = 120;
		}
		else
		{
			Error("DEBUG SETTINGS IS NULL!");
			i_MinPlayers = 10;
			i_TimeBetweenMessages = 120;
		}
	}

	override string GetName()
	{
		return DAYZBR_SM_DEBUG_ZONE_NAME;
	}

	//returns true when this state is complete
	override bool IsComplete()
	{
		if(GetPlayers().Count() >= i_MinPlayers && IsActive())
		{
			Deactivate();
		}
		return super.IsComplete();
	}
	
	override void Activate()
	{
		m_CallQueue.CallLater(this.MessageWaiting, i_TimeBetweenMessages*1000, true);
		super.Activate();


		

	}
	override void Deactivate()
	{
		m_CallQueue.Remove(this.MessageWaiting);
		super.Deactivate();
	}
	

	void MessageWaiting()
	{
		MessagePlayers(GetWaitingMessage());
	}
	string GetWaitingMessage()
	{
		int waiting_on_count = i_MinPlayers - GetPlayers().Count();

		//TODO: add this to battleroyaleconstants & use string replace for count & plural
		string message = "Waiting for " + waiting_on_count.ToString() + " more ";
		if(waiting_on_count > 1)
			message += "players";
		else
			message += "player";
		
		message += " to connect";

		return message;
	}
}