
class BattleRoyaleServer extends BattleRoyaleBase
{
	protected ref BattleRoyaleSpectators m_SpectatorSystem;
	protected ref BattleRoyaleVehicles m_VehicleSystem; 
	protected ref BattleRoyaleLoot m_LootSystem;
	ref array<ref BattleRoyaleState> m_States;
	int i_CurrentStateIndex;
	
	int i_NumRounds;
	
	protected ref MatchData match_data;
	protected ref Timer m_Timer;

	
	void BattleRoyaleServer() 
	{
		GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", this);
		GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestEntityHealthUpdate", this);

		Init();
	}

	void Init()
	{
		//update our banlist
		BattleRoyaleBans.GetBans().WriteBanlist();

		m_Timer = new Timer;

		match_data = new MatchData;

		match_data.CreateWorld("Namalsk", "Solos"); //todo figure out world dynamically & get game mode from config

		m_LootSystem = new BattleRoyaleLoot; //--- construct loot system
		m_VehicleSystem = new BattleRoyaleVehicles;
		m_SpectatorSystem = new BattleRoyaleSpectators;

		m_VehicleSystem.Preinit();

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

		BattleRoyaleLastRound last_round = new BattleRoyaleLastRound(m_States[m_States.Count() - 1]);
		m_States.Insert(last_round);
			
		m_States.Insert(new BattleRoyaleWin);
		m_States.Insert(new BattleRoyaleRestart);
		
		i_CurrentStateIndex = 0;
		GetCurrentState().Activate();

		string worldName = "empty";
		GetGame().GetWorldName( worldName );
		worldName.ToLower();
		if ( worldName != "namalsk" )
			RandomizeServerEnvironment();

		//--- this will halt the server until we successfully register is as unlocked in the DB
		if(BattleRoyaleAPI.GetAPI().ShouldUseApi())
		{
			Print("Requesting Server Startup...")
			ServerData m_ServerData = BattleRoyaleAPI.GetAPI().RequestServerStart(); //request server start
			while(!m_ServerData || m_ServerData.locked != 0)
			{
				Print("Invalid Server State on Startup...");
				BattleRoyaleAPI.GetAPI().ServerSetLock(false); //report this server as ready to go!
				m_ServerData = BattleRoyaleAPI.GetAPI().GetServer(m_ServerData._id);
			}
		}
	}
	override void Update(float delta)
	{
		float timeslice = delta; //Legacy
		
		
		m_LootSystem.Update(timeslice);
		m_VehicleSystem.Update(timeslice);
		m_SpectatorSystem.Update(timeslice);

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


				Print("[State Machine] Leaving State `" + GetCurrentState().GetName() + "`");
				if(GetCurrentState().IsActive())
				{
					GetCurrentState().Deactivate(); //deactivate old state
				}
				ref array<PlayerBase> players = GetCurrentState().RemoveAllPlayers(); //remove players from old state
				for(int i = 0; i < players.Count(); i++) //can't use foreach because it doesn't play nice with null entries
				{
					if(players[i])
					{					
						next_state.AddPlayer(players[i]); //add players to new state
					}
					else
					{
						Error("null player in RemoveAllPlayers result!");
					}
					
				}
				i_CurrentStateIndex = next_index;//move us to the next state
				Print("[State Machine] Entering State `" + GetCurrentState().GetName() + "`");
				GetCurrentState().Activate(); //activate new state
			}
			else
			{
				Error("NEXT STATE IS NULL!");
			}
		}
	}
	void OnPlayerConnected(PlayerBase player)
	{
		//Teleport player into debug zone
		Print("Player connected!" + player.GetIdentity().GetName()); //lets find out if respawning players end up here


		//Dirty way to sync server settings with the client | this should be converted into a generic "sync settings" function
		BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();
		BattleRoyaleAPIData m_APIData = config_data.GetApiData();
		bool is_unlock_skins_enabled = m_APIData.unlock_all_purchasables;
		if(is_unlock_skins_enabled)
		{
			Print("Notifying of unlocked skins flag")
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetServerSkinUnlockValue", new Param1<bool>(true), true, player.GetIdentity());
		}

		
		BattleRoyaleDebug m_Debug = BattleRoyaleDebug.Cast( GetState(0) );
		vector debug_pos = m_Debug.GetCenter();
		player.SetPosition(debug_pos);		

		BattleRoyaleDebugState m_DebugStateObj;
		if(!Class.CastTo(m_DebugStateObj, GetCurrentState()))
		{
			

			
			if(m_SpectatorSystem.CanSpectate( player ))
			{
				Print("Spectator connected, inserting into spectator system");

				//it seems that AddPlayer's client init may be causing some crashes, so we'll wait 15 seconds and then initialize the player as a spectator
				//note that 15 seconds is still too short. increased for now, but a more effective way of knowing when the player is "ready for interaction" is necessary
				m_Timer.Run( 15.0, m_SpectatorSystem, "AddPlayer", new Param1<PlayerBase>( player ), false);
				
				string message = "You will be given spectator in ~15 seconds...";
				string title = DAYZBR_MSG_TITLE;
				string icon = DAYZBR_MSG_IMAGE;
				int color = COLOR_EXPANSION_NOTIFICATION_INFO;
				float time = DAYZBR_MSG_TIME;

				//m_SpectatorSystem.AddPlayer( player );
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
			else
			{
				//BAD VERY BAD!
				//This gives the player 15 seconds to finish his setup before we boot him. There may still be a chance it crashes.
				//Ideally the player should notify us when he is "ready" to be disconnected (I have no idea when that would be)
				
				//NOTE: calling this will immediately crash the server (as the player hasn't fully established his connection yet) GetGame().DisconnectPlayer(player.GetIdentity());
				
				Error("PLAYER CONNECTED DURING NON-DEBUG ZONE STATE!");
				m_Timer.Run( 15.0, this, "Disconnect", new Param1<PlayerIdentity>( player.GetIdentity() ), false);
			}
			
			
			//TODO: Create a *spectator* system that handles players conencting during non-debug zone states
			//Note: the spectator system will also handle client respawn events too.
			//We need to create a list of *allowed* spectators. This should be in the server config (for private servers)
			
			return;
		}


		// only add player if they connect during debug (otherwise they're a spectator)
		if(player.GetIdentity())
			player.owner_id = player.GetIdentity().GetPlainId(); //cache their id (for connection loss)

		GetCurrentState().AddPlayer(player);
		m_LootSystem.AddPlayer( player );
	}
	
	void Disconnect(PlayerIdentity identity)
	{
		GetGame().DisconnectPlayer( identity ); //can't directly call disconnectplayer with timer, so we use this method
	}

	void OnPlayerDisconnected(PlayerBase player, PlayerIdentity identity)
	{		
		if(GetCurrentState().ContainsPlayer(player))
		{
			GetCurrentState().RemovePlayer(player);
			
			BattleRoyaleDebug m_DebugStateObj;
			if(!Class.CastTo( m_DebugStateObj, GetCurrentState() ))
			{
				if(player)
				{
					string player_steamid = player.owner_id; //on connect, the player object gets a reference to the steamid64
					if(identity) //this is null if the player crashes out (so we use the cached version)
					{
						player_steamid = identity.GetPlainId();
					}
					if(player_steamid != "")
					{
						//--- this ensures the leaderboard logs this player's death as zone damage
						if(!GetMatchData().ContainsDeath(player_steamid))
						{
							vector player_position = player.GetPosition();
							int time = GetGame().GetTime();
							ref array<string> killed_with = new array<string>();
							killed_with.Insert( "Disconnected" );
							GetMatchData().CreateDeath( player_steamid, player_position, time, "", killed_with, Vector(0,0,0) );
						}
					}
					else
					{
						Error("Player disconnected with unknown owner id!");
					}
					
				}
			}
		}

		//if player is in the loot system, remove them
		m_LootSystem.RemovePlayer( player );
	}
	
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
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

	ref array<PlayerBase> temp_disconnecting;
	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		if(!temp_disconnecting)
		{
			temp_disconnecting = new array<PlayerBase>();
		}

		if(GetCurrentState().ContainsPlayer(player))
		{
			GetCurrentState().OnPlayerTick(player,timeslice);


			//--- check if they have entered an invalid position
			vector pos = player.GetPosition();
			float bigNum = 1000000;
			//when invalid, height gets fucked, lets check that (others are NaN & may cause errors)
			if(pos[1] > bigNum || pos[1] < (-1 * bigNum))
			{
				if(temp_disconnecting.Find(player) == -1)
				{
					temp_disconnecting.Insert(player);
					GetGame().DisconnectPlayer( player.GetIdentity() );
					Print(pos);
					Error("PLAYER FOUND IN INVALID POSITION!");
				}
			}
		}
		else
		{
			//current state does not contain player, wtf is going on
			int life_state = player.GetPlayerState();
			if(life_state == EPlayerStates.ALIVE)
			{
				if(player && player.GetIdentity())
				{
					if(m_SpectatorSystem.ContainsPlayer( player ))
					{
						m_SpectatorSystem.OnPlayerTick( player, timeslice );
					}	
					else
					{
						if( !m_SpectatorSystem.CanSpectate( player ) ) //TODO: find a better way to do this, if someone is bugged,but they can be a spectator, they aren't kicked :/
						{
							//this ensures we only call disconnect on this player once
							if(temp_disconnecting.Find(player) == -1)
							{
								temp_disconnecting.Insert(player);
								GetGame().DisconnectPlayer( player.GetIdentity() );
								Error("GetCurrentState() DOES NOT CONTAIN PLAYER TICKING!");
							}
						}
					}
				}
				
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
	ref BattleRoyaleSpectators GetSpectatorSystem()
	{
		return m_SpectatorSystem;
	}

	ref MatchData GetMatchData()
	{
		return match_data;
	}
	
	void RequestEntityHealthUpdate(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		
		PlayerBase pbTarget;
		//--- client is requesting stats on the existing player (ensure their stats are updated and send a result back only to that specific client)
		Print("Spectator client requested status update for target");
		if(Class.CastTo( pbTarget, target ))
		{
			pbTarget.UpdateHealthStats( pbTarget.GetHealth01("", "Health"), pbTarget.GetHealth01("", "Blood") )
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateEntityHealth", new Param2<float,float>( pbTarget.health_percent, pbTarget.blood_percent ), true, sender, pbTarget);
		}
		
	}

	void PlayerReadyUp(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1< bool > data;
		if( !ctx.Read( data ) ) return;
		
		if(!target) return;

		PlayerBase targetBase = PlayerBase.Cast(target);

		if(!targetBase) return;

		if(type == CallType.Server)
		{
			BattleRoyaleDebug m_DebugStateObj;
			if(!Class.CastTo(m_DebugStateObj, GetCurrentState())) //this ensures we can only ready up during the debug state
				return;

			if(!m_DebugStateObj.ContainsPlayer(targetBase))
			{
				Error("Debug state does not contain player requesting ready up!");
				return;
			}

			if(data.param1)
			{
				m_DebugStateObj.ReadyUp(targetBase);
			}
			else
			{
				//perhaps allow readyup to be toggled?
			}
			
		}
	}

	//TODO: This will need a rework!
	void RandomizeServerEnvironment()
	{
		Print("BattleRoyale: Randomizing Environment!");
		//NOTE: this is all legacy, we should find a better way to do this
		int year = 2018;
		int month = 12;
		int day = 24;
		int hour = Math.RandomIntInclusive(8,17); //7am to 5pm
		int minute = 0;
		GetGame().GetWorld().SetDate(year, month, day, hour, minute);
		
		GetMatchData().SetWorldStartTime( hour, minute );

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





	//--- Admin tool functions only
	void TestSpectator(PlayerBase player)
	{
		OnPlayerKilled(player, NULL); //1. simulate player killed ()
		GetSpectatorSystem().AddPlayer( player ); //2. insert them into spectator system (like simulating joining during a non-debug state)
	}
}