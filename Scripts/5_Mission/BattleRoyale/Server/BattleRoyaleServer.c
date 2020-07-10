
class BattleRoyaleServer extends BattleRoyaleBase
{
	//TODO: restructure these states into a single array of states (so we can easily insert more states for debug->gameplay transition
	ref BattleRoyaleDebug m_Debug;
	ref BattleRoyaleWin m_WinState;
	ref array<ref BattleRoyaleRound> m_Rounds;
	
	ref BattleRoyaleState m_CurrentState;
	
	
	int num_rounds = 7;
	
	
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
		//--- initialize all states
		m_Debug = new BattleRoyaleDebug;
		m_WinState = new BattleRoyaleWin;
		m_Rounds = new array<ref BattleRoyaleRound>(); 
		for(int i = 0; i < num_rounds;i++)
		{
			BattleRoyaleState previous_state = m_Debug;
			if(i > 0)
				previous_state = m_Rounds[i-1];
			BattleRoyaleRound round = new BattleRoyaleRound(previous_state);
			m_Rounds.Insert(round);
		}
		
		m_Debug.Activate();
		m_CurrentState = m_Debug;
		RandomizeServerEnvironment();
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
		
		m_Debug.Update(timeslice);
		foreach(BattleRoyaleRound round : m_Rounds)
		{
			round.Update(timeslice);
		}
		m_WinState.Update(timeslice);
		
		
		
		//--- transition states
		if(m_CurrentState.IsComplete()) //current state is complete
		{
			BattleRoyaleState state = GetNextState(); //get next state
			if(state)
			{
				m_CurrentState.Deactivate(); //deactivate old state
				array<PlayerBase> players = m_CurrentState.RemoveAllPlayers(); //remove players from old state
				foreach(PlayerBase player : players)
				{
					state.AddPlayer(player); //add players to new state
				}
				m_CurrentState = state; //change current state
				m_CurrentState.Activate(); //activate new state
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
		if(m_CurrentState != m_Debug)
		{
			BRPrint("ERROR! PLAYER CONNECTED DURING NON-DEBUG STATE");
		}
		m_CurrentState.AddPlayer(player);
	}
	void OnPlayerDisconnected(PlayerBase player)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::OnPlayerDisconnected()");
		#endif
		
		if(m_CurrentState.ContainsPlayer(player))
			m_CurrentState.RemovePlayer(player);
	}
	
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::OnPlayerKilled()");
		#endif
		
		if(m_CurrentState.ContainsPlayer(killed))
		{
			//if we are in a round, then we need to call the onkilled event 
			if(m_CurrentState != m_Debug && m_CurrentState != m_WinState)
			{
				BattleRoyaleRound.Cast( m_CurrentState ).OnPlayerKilled(killed,killer);
			}
			//remove player from the state (this would take place in on-disconnect, but some players would choose not to disconnect)
			m_CurrentState.RemovePlayer(killed);
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
		
		if(m_CurrentState.ContainsPlayer(player))
			m_CurrentState.OnPlayerTick(player,timeslice);
		else
		{
			//current state does not contain player, wtf is going on
			BRPrint("ERROR! m_CurrentState DOES NOT CONTAIN PLAYER TICKING!");
		}
		
	}
	
	
	
	BattleRoyaleState GetNextState()
	{
		if(m_CurrentState == m_Debug)
		{
			return m_Rounds[0];
		}
		else if(m_CurrentState == m_WinState)
		{
			return NULL; //winstate has no subsequent state
		}
		else
		{
			int index = m_Rounds.Find(m_CurrentState);
			if(index == -1)
			{
				BRPrint("ERROR! m_Rounds DOES NOT CONTAIN CURRENT STATE & IS NOT DEBUG!");
				return m_Debug;
			}
			else
			{
				if(m_Rounds.Count() == (index+1))
				{
					return m_WinState;
				}
				else
				{
					//TODO: if player count is 1 or 0, we need to flip into the win state, not the next round
					return m_Rounds[index+1];
					
				}
			}
		}
	}
	BattleRoyaleDebug GetDebug()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleServer::GetDebug()");
		#endif
		
		return m_Debug;
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