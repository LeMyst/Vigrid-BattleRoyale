class BattleRoyaleState {
	
	protected ref array<PlayerBase> m_Players;
	protected bool b_IsActive;
	protected ScriptCallQueue m_CallQueue;
	
	void BattleRoyaleState()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleState::Constructor()");
		#endif
		
		m_Players = new array<PlayerBase>();
		m_CallQueue = new ScriptCallQueue;
		b_IsActive = false;
	}
	
	void Update(float timeslice)
	{
		m_CallQueue.Tick(timeslice);
	}
	
	//state controls
	void Activate()
	{
		//Note: this is called AFTER players are added
		b_IsActive = true;
	}
	void Deactivate()
	{
		//Note: this is called BEFORE players are removed
		b_IsActive = false; 
	}
	bool IsActive()
	{
		return b_IsActive;
	}
	bool IsComplete()
	{
		return false;
	}
	
	array<PlayerBase> GetPlayers()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleState::GetPlayers()");
		#endif
		
		return m_Players;
	}
	void AddPlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleState::AddPlayer()");
		#endif
		
		m_Players.Insert(player);
	}
	void RemovePlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleState::RemovePlayer()");
		#endif
		
		m_Players.RemoveItem(player);
	}
	array<PlayerBase> RemoveAllPlayers()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleState::RemoveAllPlayers()");
		#endif
		
		ref array<PlayerBase> result_array = new array<PlayerBase>();
		result_array.InsertAll(m_Players);
		m_Players.Clear();
		return result_array;
	}
	bool ContainsPlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleState::ContainsPlayer()");
		#endif
		
		if(m_Players.Find(player) >= 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	void OnPlayerTick(PlayerBase player, float timeslice)
	{}
}