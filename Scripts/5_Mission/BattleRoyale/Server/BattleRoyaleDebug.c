#define BR_BETA_LOGGING


class BattleRoyaleDebug extends BattleRoyaleState {
	
	protected vector v_Center;
	protected float f_Radius;
	protected int i_MinPlayers;
	protected int i_TimeBetweenMessages;
	
	void BattleRoyaleDebug()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::Constructor()");
		#endif
		
		BattleRoyaleDebugData m_DebugSettings = GetBRConfig().GetDebugData();
		
		if(m_DebugSettings)
		{
			v_Center = m_DebugSettings.spawn_point; 
			f_Radius = m_DebugSettings.radius; 
			i_MinPlayers = m_DebugSettings.minimum_players; 
			i_TimeBetweenMessages = 120;
		}
		else
		{
			Error("DEBUG SETTINGS IS NULL!");
			v_Center = "14829.2 72.3148 14572.3";
			f_Radius = 50;
			i_MinPlayers = 10;
			i_TimeBetweenMessages = 120;
		}
	}

	
	//returns true when this state is complete
	override bool IsComplete()
	{
		if(!IsActive())
			return true;
		
		if(GetPlayers().Count() >= i_MinPlayers)
			return true;
		
		return false;
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
	
	vector GetCenter()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::GetCenter()");
		#endif
		
		return v_Center;
	}
	float GetRadius()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::GetRadius()");
		#endif
		
		return f_Radius;
	}
	
	override array<PlayerBase> RemoveAllPlayers()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::RemoveAllPlayers()");
		#endif
		
		array<PlayerBase> players = super.RemoveAllPlayers();
		foreach(PlayerBase player : players)
		{
			player.SetAllowDamage(true); //leaving debug state = disable god mode
		}
		return players;
	}
	override void RemovePlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::RemovePlayer()");
		#endif
		
		if(player)
			player.SetAllowDamage(true); //leaving debug state = disable god mode
		super.RemovePlayer(player);
	}
	override void AddPlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::AddPlayer()");
		#endif
		
		if(player)
			player.SetAllowDamage(false); //all players in debug are god mode TODO: ensure that dayz expansion doesn't override this
		super.AddPlayer(player);
	}
	
	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
	
		vector playerPos = player.GetPosition();
		float distance = vector.Distance(playerPos, v_Center);
		if(distance > f_Radius)
		{
			#ifdef BR_BETA_LOGGING
			BRPrint("BattleRoyaleDebug::OnPlayerTick() => Snapping player back to debug zone");
			#endif
			player.SetPosition(v_Center);
		}

	}


	void MessageWaiting()
	{
		int waiting_on_count = i_MinPlayers - GetPlayers().Count();

		string message = "Waiting for " + waiting_on_count.ToString() + " more ";
		if(waiting_on_count > 1)
			message += "players";
		else
			message += "player";
		
		message += " to connect";

		MessagePlayers(message);
	}
}