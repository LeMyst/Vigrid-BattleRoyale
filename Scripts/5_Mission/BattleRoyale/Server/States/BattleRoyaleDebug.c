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
			i_TimeBetweenMessages = 45;
			b_UseVoteSystem = (m_DebugSettings.use_ready_up == 1);
			f_VoteThreshold = m_DebugSettings.ready_up_percent;
		}
		else
		{
			Error("DEBUG SETTINGS IS NULL!");
			i_MinPlayers = 10;
			i_TimeBetweenMessages = 120;
			b_UseVoteSystem = true;
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
		if(GetPlayers().Count() >= i_MinPlayers && IsActive())
		{
			Deactivate();
		}
		return super.IsComplete();
	}

	override void Activate()
	{
		//these loop & will be automatically cleaned up on Deactivation
		AddTimer(i_TimeBetweenMessages, this, "MessageWaiting", NULL, true);
		AddTimer(2.0, this, "CheckReadyState", NULL, true);
		super.Activate();
	}

	override void Deactivate()
	{
		super.Deactivate();
	}

	void CheckReadyState()
	{
		if(IsVoteReady())
		{
			Deactivate();
		}
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
			int ready_count = GetReadyCount();
			message += ". " + ready_count.ToString() + " player";
			if(ready_count > 1)
			{
				message += "s";
			}
			message += " are ready! (Press F1 to ready up)";
		}
		return message;
	}

	int GetReadyCount()
	{
		int ready_count = 0;
		for(int a = 0; a < m_ReadyList.Count(); a++)
		{
			if(m_ReadyList[a])
				ready_count++;
		}
		return ready_count;
	}

	bool IsVoteReady()
	{
		if(!b_UseVoteSystem)
			return false;

		int ready_count = GetReadyCount();
		int player_count = GetPlayers().Count();

		if(player_count <= 1 ) //need more than 1 player to start
			return false;

		float percent = (ready_count / player_count);
		return (percent >= f_VoteThreshold);
	}

	void ReadyUp(PlayerBase player)
	{
		if(m_ReadyList.Find(player) != -1) {
			MessagePlayer(player, "You have already readied up...");
			return;
		}

		MessagePlayer(player, "You have readied up!");
		m_ReadyList.Insert( player );

		//this is here because we don't want someone mass spamming all players by spamming F1
		int count = GetReadyCount();
		int max = GetPlayers().Count();
		MessagePlayers("Player readied up. (" + count.ToString() + "/" + max.ToString() + " players)");

	}

	override void RemovePlayer(PlayerBase player)
	{
		super.RemovePlayer( player );
		m_ReadyList.RemoveItem( player );
	}

	override ref array<PlayerBase> RemoveAllPlayers()
	{
		m_ReadyList.Clear();
		ref array<PlayerBase> players = super.RemoveAllPlayers();
		return players;
	}
}
