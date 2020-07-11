class BattleRoyaleState {
	
	protected ref array<PlayerBase> m_Players;
	protected bool b_IsActive;
	protected ref ScriptCallQueue m_CallQueue;
	
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
	bool SkipState(BattleRoyaleState m_PreviousState)  //if true, we will skip activating/deactivating this state entirely
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


	//CreateNotification( ref StringLocaliser title, ref StringLocaliser text, string icon, int color, float time, PlayerIdentity identity ) ()
	void MessagePlayers(string message, string title = "DayZ Battle Royale", string icon = "set:expansion_iconset image:icon_info", int color = COLOR_EXPANSION_NOTIFICATION_INFO, float time = 7)
	{
		StringLocaliser title_sl = new StringLocaliser( title ); //This comes form CommunityFramework (if Translatestring fails, we get default text value here)
		StringLocaliser text = new StringLocaliser( message );
		GetNotificationSystem().CreateNotification(title_sl,text,icon,color,time);
	}
	void MessagePlayer(PlayerBase player, string message, string title = "DayZ Battle Royale", string icon = "set:expansion_iconset image:icon_info", int color = COLOR_EXPANSION_NOTIFICATION_INFO, float time = 7)
	{
		if(player)
		{
			PlayerIdentity identity = player.GetIdentity();
			if(identity)
			{
				StringLocaliser title_sl = new StringLocaliser( title ); //This comes form CommunityFramework (if Translatestring fails, we get default text value here)
				StringLocaliser text = new StringLocaliser( message );
				GetNotificationSystem().CreateNotification(title_sl,text,icon,color,time, identity);
			}
		}
		ExpansionNotificationSystem m_notif_sys = GetNotificationSystem();
	}
}