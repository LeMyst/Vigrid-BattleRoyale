#define BR_BETA_LOGGING

class BattleRoyaleState {
	
	protected ref array<PlayerBase> m_Players;
	protected bool b_IsActive;
	protected ref ScriptCallQueue m_CallQueue;
	protected bool b_IsPaused;

	string GetName()
	{
		return "Unknown State";
	}

	void BattleRoyaleState()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleState::Constructor()");
		#endif
		
		m_Players = new array<PlayerBase>();
		m_CallQueue = new ScriptCallQueue;
		b_IsActive = false;
		b_IsPaused = false;
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
	bool IsPaused()
	{
		return b_IsPaused;
	}
	bool Pause()
	{
		b_IsPaused = true;
	}
	bool Resume()
	{
		b_IsPaused = false;
	}
	bool IsComplete()
	{
		//state cannot complete if paused
		if(b_IsPaused)
			return false;

		return !IsActive();
	}
	bool SkipState(BattleRoyaleState m_PreviousState)  //if true, we will skip activating/deactivating this state entirely
	{
		return false;
	}
	
	array<PlayerBase> GetPlayers()
	{	
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


//base state for the Debug Zone.
//This handles healing / godmode / and teleporting
class BattleRoyaleDebugState extends BattleRoyaleState {
	protected vector v_Center;
	protected float f_Radius;
	protected int i_HealTickTime;

	void BattleRoyaleDebugState()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebugState::Constructor()");
		#endif

        BattleRoyaleDebugData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();
        if(m_DebugSettings)
		{
			v_Center = m_DebugSettings.spawn_point; 
			f_Radius = m_DebugSettings.radius; 
		}
		else
		{
			Error("DEBUG SETTINGS IS NULL!");
			v_Center = "14829.2 72.3148 14572.3";
			f_Radius = 50;
		}
		BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData(); 
		if(m_GameSettings)
		{
			i_HealTickTime = m_GameSettings.debug_heal_tick_seconds
		}
		else
		{
			Error("GAME SETTINGS IS NULL");
			i_HealTickTime = 5;
		}
		
	}
	override string GetName()
	{
		return "Unknown Debug State";
	}
	override void AddPlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebugState::AddPlayer()");
		#endif
		
		if(player)
		{
			player.SetAllowDamage(false); //all players in this state are god mode
			player.Heal();
		}
		super.AddPlayer(player);
	}
    override array<PlayerBase> RemoveAllPlayers()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebugState::RemoveAllPlayers()");
		#endif
		
		array<PlayerBase> players = super.RemoveAllPlayers();
		foreach(PlayerBase player : players)
		{
			player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
		}
		return players;
	}
	override void RemovePlayer(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebugState::RemovePlayer()");
		#endif
		
		if(player)
        {
			player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }
		super.RemovePlayer(player);
	}

	//--- debug states must lock players into the debug zone & heal them
    override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		super.OnPlayerTick(player, timeslice);

		vector playerPos = player.GetPosition();
		float distance = vector.Distance(playerPos, v_Center);
		if(distance > f_Radius)
		{
			#ifdef BR_BETA_LOGGING
			BRPrint("BattleRoyaleDebugState::OnPlayerTick() => Snapping player back to debug zone");
			#endif
			player.SetPosition(v_Center);
		}


		if(player.time_until_heal <= 0)
		{
			player.time_until_heal = i_HealTickTime;
			player.Heal();
		}
		player.time_until_heal -= timeslice;


	}


	//DEPRECATED: should pull value from config
	//GOAL: make this protected
	vector GetCenter()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::GetCenter()");
		#endif
		
		return v_Center;
	}
	//DEPRECATED: should pull value from config
	//GOAL: make this protected
	float GetRadius()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleDebug::GetRadius()");
		#endif
		
		return f_Radius;
	}
}