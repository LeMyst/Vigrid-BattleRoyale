#define BR_BETA_LOGGING


class BattleRoyaleServer extends BattleRoyaleBase
{
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


		//load config (this may error because GetBattleRoyale would return false)
		BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();
		BattleRoyaleGameData m_GameData = config_data.GetGameData();
		i_NumRounds = m_GameData.num_zones;

		//--- initialize all states (in order from start to finish)
		m_States = new array<ref BattleRoyaleState>;

		BattleRoyaleDebug debug_state = new BattleRoyaleDebug;
		if(debug_state)
			m_States.Insert(debug_state); //insert debug state
		else
			Error("DEBUG STATE CONSTRUCTOR RETURNED NULL");
		
		BattleRoyaleCountReached count_reached = new BattleRoyaleCountReached;
		if(count_reached)
			m_States.Insert(count_reached); //insert debug state
		else
			Error("COUNT REACHED CONSTRUCTOR RETURNED NULL");


		BattleRoyalePrepare prepare_clients = new BattleRoyalePrepare;
		if(prepare_clients)
			m_States.Insert(prepare_clients); //insert debug state
		else
			Error("PREPARE CONSTRUCTOR RETURNED NULL");

		//TODO: transition states from debug zone to gameplay
		//https://gitlab.desolationredux.com/DayZ/BattleRoyale/-/blob/master/dayzbr/Addons/scripts/scripts/5_Mission/battleroyale/BattleRoyaleRound.c
			
		int num_states = m_States.Count();
		for(int i = 0; i < i_NumRounds;i++)
		{
			int prev_state_ind = i + num_states - 1;
			BattleRoyaleState previous_state = m_States[prev_state_ind];
			BattleRoyaleRound round = new BattleRoyaleRound(previous_state);
			if(round)
				m_States.Insert(round);
			else
				Error("round is NULL! Constructor must have failed!");
			
		}
		m_States.Insert(new BattleRoyaleWin);
		
		i_CurrentStateIndex = 0;
		GetCurrentState().Activate();

		RandomizeServerEnvironment();


		BattleRoyaleAPI.GetAPI().RequestServerStart(); //request server start
	}
	void Update(float timeslice)
	{
		#ifdef BR_BETA_LOGGING
		if(update_once)
		{
			update_once = false;
			BRPrint("BattleRoyaleServer::Update() => Running!");
		}
		#endif
		
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

				GetCurrentState().Deactivate(); //deactivate old state
				array<PlayerBase> players = GetCurrentState().RemoveAllPlayers(); //remove players from old state
				foreach(PlayerBase player : players)
				{
					next_state.AddPlayer(player); //add players to new state
				}
				i_CurrentStateIndex = next_index;//move us to the next state
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
		//TODO: warning if player joins after game start
		GetCurrentState().AddPlayer(player);
	}
	void OnPlayerDisconnected(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::OnPlayerDisconnected()");
		#endif
		
		if(GetCurrentState().ContainsPlayer(player))
			GetCurrentState().RemovePlayer(player);
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
			GetCurrentState().OnPlayerTick(player,timeslice);
		else
		{
			//current state does not contain player, wtf is going on
			BRPrint("ERROR! GetCurrentState() DOES NOT CONTAIN PLAYER TICKING!");
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
	
	//--- TODO: this is all legacy code, find a new way to implement
	
	void RandomizeServerEnvironment()
	{
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