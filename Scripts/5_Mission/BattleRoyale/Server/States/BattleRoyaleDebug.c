class BattleRoyaleDebug extends BattleRoyaleDebugState {

	protected ref array<PlayerBase> m_ReadyList;

	protected bool b_UseVoteSystem;
	protected float f_VoteThreshold;

	protected int i_MinPlayers;
	protected int i_TimeBetweenMessages;
	
	void BattleRoyaleDebug()
	{	
		m_ReadyList = new array<PlayerBase>();

		BattleRoyaleDebugData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();
		
		if(m_DebugSettings)
		{
			i_MinPlayers = m_DebugSettings.minimum_players; 
			i_TimeBetweenMessages = 120;
			b_UseVoteSystem = (m_DebugSettings.use_ready_up == 1);
			f_VoteThreshold = m_DebugSettings.ready_up_percent;
		}
		else
		{
			Error("DEBUG SETTINGS IS NULL!");
			i_MinPlayers = 10;
			i_TimeBetweenMessages = 120;
			b_UseVoteSystem = 1;
			f_VoteThreshold = 0.8;
		}
	}

	override string GetName()
	{
		return DAYZBR_SM_DEBUG_ZONE_NAME;
	}

	//returns true when this state is complete
	override bool IsComplete()
	{
		if( (GetPlayers().Count() >= i_MinPlayers && IsActive()) || IsVoteReady() )
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

		if(b_UseVoteSystem)
		{
			int ready_count = m_ReadyList.Count();
			message += ". " + ready_count.ToString() + " player";
			if(ready_count > 1)
			{
				message += "s";
			}
			message += " are ready! (Press F1 to ready up)";
		}
		return message;
	}

	bool IsVoteReady()
	{
		if(!b_UseVoteSystem)
			return false;

		int ready_count = m_ReadyList.Count();
		int player_count = GetPlayers().Count();

		if(player_count == 1) //need more than 1 player to start
			return false;

		float percent = (ready_count / player_count);
		return (percent >= f_VoteThreshold);
	}

	void ReadyUp(PlayerBase player)
	{
		if(m_ReadyList.Find(player) >= 0)
			return;

		MessagePlayer(player, "You have readied up!");

		m_ReadyList.Insert( player );
	}
	override void RemovePlayer(PlayerBase player)
	{
		super.RemovePlayer( player );
		m_ReadyList.RemoveItem( player );
	}
	override array<PlayerBase> RemoveAllPlayers()
	{
		m_ReadyList.Clear();
		return super.RemoveAllPlayers();
	}
}