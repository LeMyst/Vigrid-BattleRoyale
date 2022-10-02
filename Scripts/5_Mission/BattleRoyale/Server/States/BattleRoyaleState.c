class BattleRoyaleState extends Timeable {
	
	protected ref array<PlayerBase> m_Players;
	protected bool b_IsActive;
	protected bool b_IsPaused;

	string GetName()
	{
		return DAYZBR_SM_UNKNOWN_NAME;
	}

	void BattleRoyaleState()
	{		
		m_Players = new array<PlayerBase>();
		
		
		b_IsActive = false;
		b_IsPaused = false;
	}

	void Update(float timeslice)
	{
		
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

		//--- stop all repeating timers
		StopTimers();

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

	void Pause()
	{
		b_IsPaused = true;
	}

	void Resume()
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

	ref array<PlayerBase> GetPlayers()
	{	
		return m_Players;
	}

	void AddPlayer(PlayerBase player)
	{
		m_Players.Insert( player );
		OnPlayerCountChanged();
	}

	void RemovePlayer(PlayerBase player)
	{
		m_Players.RemoveItem(player);
		OnPlayerCountChanged();
	}

	ref array<PlayerBase> RemoveAllPlayers()
	{
		ref array<PlayerBase> result_array = new array<PlayerBase>();
		result_array.InsertAll(m_Players);
		m_Players.Clear();
		OnPlayerCountChanged();
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
	{
		if(player)
		{
			if(player.UpdateHealthStatsServer( player.GetHealth01("", "Health"), player.GetHealth01("", "Blood"), timeslice ))
			{
				//Print("Player Health Changed! Syncing Network...");
				//the player's stats changed (sync it over the network)
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateEntityHealth", new Param2<float, float>( player.health_percent, player.blood_percent ), true, NULL, player);
			}

			//UpdateHealthStatsServer will ensure that time_since_last_net_sync == 0 when a potential health update gets triggered. We can use this same window to also send map entity data updates
			//TODO: look for a more performant way of doing this maybe?
			if(player.time_since_last_net_sync == 0)
			{
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateMapEntityData", new Param4<string, string, vector, vector>( player.GetIdentity().GetId(), player.GetIdentityName(), player.GetPosition(), player.GetDirection() ), true);
			}
		}
	}

	//player count changed event handler
	protected void OnPlayerCountChanged()
	{
		if(IsActive())
		{
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", new Param1<int>( GetPlayers().Count() ), true);
		}
	}

	//CreateNotification( ref StringLocaliser title, ref StringLocaliser text, string icon, int color, float time, PlayerIdentity identity ) ()
	void MessagePlayers(string message, string title = DAYZBR_MSG_TITLE, string icon = DAYZBR_MSG_IMAGE, int color = COLOR_EXPANSION_NOTIFICATION_INFO, float time = DAYZBR_MSG_TIME)
	{
		StringLocaliser title_sl = new StringLocaliser( title ); //This comes form CommunityFramework (if Translatestring fails, we get default text value here)
		StringLocaliser text = new StringLocaliser( message );
		GetNotificationSystem().CreateNotification(title_sl,text,icon,color,time);
	}

	void MessagePlayer(PlayerBase player, string message, string title = DAYZBR_MSG_TITLE, string icon = DAYZBR_MSG_IMAGE, int color = COLOR_EXPANSION_NOTIFICATION_INFO, float time = DAYZBR_MSG_TIME)
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
        BattleRoyaleDebugData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();
        if(m_DebugSettings)
		{
			v_Center = m_DebugSettings.spawn_point; 
			f_Radius = m_DebugSettings.radius; 
		}
		else
		{
			Error("DEBUG SETTINGS IS NULL!");
			v_Center = DAYZBR_DEBUG_CENTER;
			f_Radius = DAYZBR_DEBUG_RADIUS;
		}
		BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData(); 
		if(m_GameSettings)
		{
			i_HealTickTime = m_GameSettings.debug_heal_tick_seconds;
		}
		else
		{
			Error("GAME SETTINGS IS NULL");
			i_HealTickTime = DAYZBR_DEBUG_HEAL_TICK;
		}
		
	}

	override string GetName()
	{
		return DAYZBR_SM_UNKNOWN_DEBUG_NAME;
	}

	override void AddPlayer(PlayerBase player)
	{
		if(player)
		{
			player.SetAllowDamage(false); //all players in this state are god mode
			player.Heal();
		}
		super.AddPlayer(player);
	}

    /*override ref array<PlayerBase> RemoveAllPlayers()
	{
		ref array<PlayerBase> players = super.RemoveAllPlayers();
		foreach(PlayerBase player : players)
		{
			player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
		}
		return players;
	}
	override void RemovePlayer(PlayerBase player)
	{
		if(player)
        {
			player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }
		super.RemovePlayer(player);
	}*/

	//--- debug states must lock players into the debug zone & heal them
    override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		super.OnPlayerTick(player, timeslice);

		vector playerPos = player.GetPosition();
		float distance = vector.Distance(playerPos, v_Center);
		if(distance > f_Radius)
		{
			player.SetPosition(v_Center);
		}

		if(player.time_until_heal <= 0)
		{
			player.time_until_heal = i_HealTickTime;
			player.Heal();
		}
		player.time_until_heal -= timeslice;
	}

	//TODO:
	//DEPRECATED: should pull value from config
	//GOAL: make this protected
	vector GetCenter()
	{
		return v_Center;
	}

	//DEPRECATED: should pull value from config
	//GOAL: make this protected
	float GetRadius()
	{
		return f_Radius;
	}
}
