#define BR_BETA_LOGGING


class BattleRoyaleServer extends BattleRoyaleBase
{
	protected ref BattleRoyaleVehicles m_VehicleSystem; 
	protected ref BattleRoyaleLoot m_LootSystem;
	ref array<ref BattleRoyaleState> m_States;
	int i_CurrentStateIndex;
	
	int i_NumRounds;
	
	
	#ifdef BR_BETA_LOGGING
	bool update_once = true;
	bool tick_once = true;
	#endif
	
	void BattleRoyaleServer() 
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::Constructor()");
		#endif

		Init();
	}
	void Init()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::Init()");
		#endif

		m_LootSystem = new BattleRoyaleLoot; //--- construct loot system
		m_VehicleSystem = new BattleRoyaleVehicles;

		m_VehicleSystem.Start();  //TODO: change this to Preinit()

		//load config (this may error because GetBattleRoyale would return false)
		BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();
		BattleRoyaleGameData m_GameData = config_data.GetGameData();
		i_NumRounds = m_GameData.num_zones;

		//--- initialize all states (in order from start to finish)
		m_States = new array<ref BattleRoyaleState>;

		//DEBUG ZONE
		BattleRoyaleDebug debug_state = new BattleRoyaleDebug;
		m_States.Insert(debug_state); //insert debug state
		
		//PLAYER COUNT REACHED COUNTDOWN
		BattleRoyaleCountReached count_reached = new BattleRoyaleCountReached;
		m_States.Insert(count_reached);

		//PREPARE CLIENTS & TELEPORT
		BattleRoyalePrepare prepare_clients = new BattleRoyalePrepare;
		m_States.Insert(prepare_clients);

		//UNLOCK CLIENTS AND START MATCH WOO
		BattleRoyaleStartMatch start_match = new BattleRoyaleStartMatch;
		m_States.Insert(start_match);

		
		int num_states = m_States.Count();
		for(int i = 0; i < i_NumRounds;i++)
		{
			int prev_state_ind = i + num_states - 1;
			BattleRoyaleState previous_state = m_States[prev_state_ind];
			BattleRoyaleRound round = new BattleRoyaleRound(previous_state);
			m_States.Insert(round);
			
		}

		//TODO: add a custom "round" state that represents 'no zone', constantly damages all players, and waits for only one player to remain alive
		BattleRoyaleLastRound last_round = new BattleRoyaleLastRound(m_States[m_States.Count() - 1]);
		m_States.Insert(last_round);
			
		m_States.Insert(new BattleRoyaleWin);
		m_States.Insert(new BattleRoyaleRestart);
		
		i_CurrentStateIndex = 0;
		GetCurrentState().Activate();

		RandomizeServerEnvironment();

		//--- this will halt the server until we successfully register is as unlocked in the DB
		Print("Requesting Server Startup...")
		ServerData m_ServerData = BattleRoyaleAPI.GetAPI().RequestServerStart(); //request server start
		while(m_ServerData.locked != 0)
		{
			Print("Server state is locked! Unlocking...");
			BattleRoyaleAPI.GetAPI().ServerSetLock(false); //report this server as ready to go!
			m_ServerData = BattleRoyaleAPI.GetAPI().GetServer(m_ServerData._id);
		}
	}
	override void Update(float delta)
	{
		float timeslice = delta; //Legacy
		
		#ifdef BR_BETA_LOGGING
		if(update_once)
		{
			update_once = false;
			BRPrint("BattleRoyaleServer::Update() => Running!");
		}
		#endif
		
		m_LootSystem.Update(timeslice);
		m_VehicleSystem.Update(timeslice);

		foreach(BattleRoyaleState state : m_States)
		{
			if(state)
				state.Update(timeslice);
			else
				Error("BAD STATE IN m_States!");
		}
		
		
		
		//--- transition states
		if(GetCurrentState().IsComplete()) //current state is complete
		{
			int next_index = GetNextStateIndex();
			if(next_index > 0)
			{
				BattleRoyaleState next_state = GetState(next_index);


				BRPrint("Leaving State `" + GetCurrentState().GetName() + "`");
				if(GetCurrentState().IsActive())
				{
					GetCurrentState().Deactivate(); //deactivate old state
				}
				array<PlayerBase> players = GetCurrentState().RemoveAllPlayers(); //remove players from old state
				foreach(PlayerBase player : players)
				{
					next_state.AddPlayer(player); //add players to new state
				}
				i_CurrentStateIndex = next_index;//move us to the next state
				BRPrint("Entering State `" + GetCurrentState().GetName() + "`");
				GetCurrentState().Activate(); //activate new state
			}
			else
			{
				BRPrint("ERROR! NEXT STATE IS NULL!");
			}
		}
	}
	void OnPlayerConnected(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::OnPlayerConnected()");
		#endif

		//Teleport player into debug zone
		BattleRoyaleDebug m_Debug = BattleRoyaleDebug.Cast( GetState(0) );
		vector debug_pos = m_Debug.GetCenter(); //TODO: we need debug zone position
		player.SetPosition(debug_pos);		

		//TODO: this is not working at all
		BattleRoyaleDebugState m_DebugStateObj;
		if(!Class.CastTo(m_DebugStateObj, GetCurrentState())
			Error("PLAYER CONNECTED DURING NON-DEBUG ZONE STATE!");

		GetCurrentState().AddPlayer(player);
		m_LootSystem.AddPlayer( player );
	}
	void OnPlayerDisconnected(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::OnPlayerDisconnected()");
		#endif
		
		if(GetCurrentState().ContainsPlayer(player))
			GetCurrentState().RemovePlayer(player);

		m_LootSystem.RemovePlayer( player );
	}
	
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::OnPlayerKilled()");
		#endif
		
		if(GetCurrentState().ContainsPlayer(killed))
		{
			//if we are in a round, then we need to call the onkilled event 
			BattleRoyaleRound p_Round;
			if(Class.CastTo(p_Round, GetCurrentState()))
			{
				p_Round.OnPlayerKilled(killed, killer); //if we are in a round, then we need to call onplayerkilled (since it's not a state based function we must cast)
			}
			BattleRoyaleLastRound p_LastRound;
			if(Class.CastTo(p_LastRound, GetCurrentState()))
			{
				p_LastRound.OnPlayerKilled(killed, killer); //if we are in a round, then we need to call onplayerkilled (since it's not a state based function we must cast)
			}

			//remove player from the state (this would take place in on-disconnect, but some players would choose not to disconnect)
			GetCurrentState().RemovePlayer(killed);
		}
	}
	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		#ifdef BR_BETA_LOGGING
		if(tick_once)
		{
			tick_once = false;
			BRPrint("BattleRoyaleServer::OnPlayerTick() => Running!");
		}
		#endif
		
		if(GetCurrentState().ContainsPlayer(player))
		{
			GetCurrentState().OnPlayerTick(player,timeslice);
		}
		else
		{
			//current state does not contain player, wtf is going on
			//TODO: i ran into a bug where this was the case, no idea how this happened!
			int life_state = player.GetPlayerState();
			if(life_state == EPlayerStates.ALIVE)
			{
				BRPrint("ERROR! GetCurrentState() DOES NOT CONTAIN PLAYER TICKING!");
			}
			//any other case here, the player is dead & therefore shouldn't count towards any state
		}
		
	}
	
	
	
	BattleRoyaleState GetState(int index)
	{
		if(index < 0 || index >= m_States.Count())
			return NULL;

		return m_States[index];
	}
	BattleRoyaleState GetCurrentState()
	{
		return GetState(i_CurrentStateIndex);
	}
	int GetNextStateIndex()
	{
		
		if(m_States.Count() <= (i_CurrentStateIndex + 1))
			return -1;

		for(int i = i_CurrentStateIndex + 1; i < m_States.Count();i++)
		{
			BattleRoyaleState state = m_States[i];
			if(!state.SkipState(GetState(i_CurrentStateIndex)))
			{
				return i;
			}
		}

		return -1;
	}
	
	ref BattleRoyaleVehicles GetVehicleSystem()
	{
		return m_VehicleSystem;
	}
	ref BattleRoyaleLoot GetLootSystem()
	{
		return m_LootSystem;
	}
	
	void RandomizeServerEnvironment()
	{
		//NOTE: this is all legacy, we should find a better way to do this
		int year = 2018;
		int month = 12;
		int day = 24;
		int hour = Math.RandomIntInclusive(8,17); //7am to 5pm
		int minute = 0;
		GetGame().GetWorld().SetDate(year, month, day, hour, minute);
		
		//Set Random Weather
		Weather weather = GetGame().GetWeather();

		weather.GetOvercast().SetLimits( 0.0 , 1.0 );
		weather.GetRain().SetLimits( 0.0 , 1.0 );
		weather.GetFog().SetLimits( 0.0 , 0.25 );

		weather.GetOvercast().SetForecastChangeLimits( 0.5, 0.8 );
		weather.GetRain().SetForecastChangeLimits( 0.1, 0.3 );
		weather.GetFog().SetForecastChangeLimits( 0.05, 0.10 );

		weather.GetOvercast().SetForecastTimeLimits( 3600 , 3600 );
		weather.GetRain().SetForecastTimeLimits( 300 , 300 );
		weather.GetFog().SetForecastTimeLimits( 3600 , 3600 );

		weather.GetOvercast().Set( Math.RandomFloatInclusive(0.0, 0.3), 0, 0);
		weather.GetRain().Set( Math.RandomFloatInclusive(0.0, 0.2), 0, 0);
		weather.GetFog().Set( Math.RandomFloatInclusive(0.0, 0.1), 0, 0);
	}
}