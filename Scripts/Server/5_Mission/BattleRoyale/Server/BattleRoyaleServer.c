#ifdef SERVER
class BattleRoyaleServer: BattleRoyaleBase
{
	protected static BattleRoyaleServer m_Instance;
#ifdef SPECTATOR
    protected ref BattleRoyaleSpectators m_SpectatorSystem;
#endif
    ref array<ref BattleRoyaleState> m_States;
    int i_CurrentStateIndex;

    int i_NumRounds;

    bool b_ShowSpawnSelectionMenu;

    string match_uuid;

    protected ref Timer m_Timer;

    void BattleRoyaleServer()
    {
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerUnstuck", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestEntityHealthUpdate", this);
#ifdef VPPADMINTOOLS
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "NextState", this, SingleplayerExecutionType.Server);
#ifdef SPECTATOR
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "StartSpectate", this, SingleplayerExecutionType.Server);
#endif
#endif

        Init();
    }

    void ~BattleRoyaleServer()
    {
        if ( m_Timer && m_Timer.IsRunning() )
        {
            m_Timer.Stop();
        }

        delete m_Timer;
    }

    void Init()
    {
		m_Instance = this;

        BattleRoyaleUtils.Trace("BattleRoyaleServer() Init()");
#ifdef VPPADMINTOOLS
        GetPermissionManager().AddPermissionType({ "MenuBattleRoyaleManager" });
#endif

        BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();
        BattleRoyaleServerData m_ServerData = config_data.GetServerData();

		if ( m_ServerData.enable_vigrid_api )
		{
			LockServerWebhook serverWebhook = new LockServerWebhook( m_ServerData.webhook_jwt_token );
			serverWebhook.UnlockServer();
		}

		if ( m_ServerData.enable_vigrid_api )
		{
			BattleRoyaleUtils.Trace("Server password: " + m_ServerData.server_password);
			CreateMatchWebhook createMatchWebhook = new CreateMatchWebhook( m_ServerData.webhook_jwt_token, m_ServerData.server_password );
			match_uuid = createMatchWebhook.getMatchUUID();

			if ( match_uuid.Length() != 36 )
			{
				if( m_ServerData.force_match_uuid )
				{
					BattleRoyaleUtils.LogMessage("Erreur getting match uuid. Restarting. Got: " + match_uuid);
					GetGame().RequestExit(0);
				}
				else
				{
					match_uuid = "";
					BattleRoyaleUtils.LogMessage("Erreur getting match uuid. Got: " + match_uuid);
				}
			}
			BattleRoyaleUtils.Trace("Match UUID: " + match_uuid);
        } else {
        	match_uuid = "disabled";  // Set a string to disable the ingame error message if no uuid is set
        }

        m_Timer = new Timer;

#ifdef SPECTATOR
        m_SpectatorSystem = new BattleRoyaleSpectators;
#endif

        //load config (this may error because GetBattleRoyale would return false)
        BattleRoyaleGameData m_GameData = config_data.GetGameData();
        i_NumRounds = m_GameData.num_zones;
        b_ShowSpawnSelectionMenu = m_GameData.show_spawn_selection_menu;

        //--- initialize all states (in order from start to finish)
        m_States = new array<ref BattleRoyaleState>;

        // (1) DEBUG ZONE
        BattleRoyaleDebug debug_state = new BattleRoyaleDebug;
        m_States.Insert(debug_state); //insert debug state

        // (2) PLAYER COUNT REACHED COUNTDOWN
        BattleRoyaleCountReached count_reached = new BattleRoyaleCountReached;
        m_States.Insert(count_reached);

        // (3) SPAWN SELECTION MENU
        if (b_ShowSpawnSelectionMenu)
		{
			BattleRoyaleSpawnSelection spawn_selection = new BattleRoyaleSpawnSelection;
			m_States.Insert(spawn_selection);
		}

        // (4) PREPARE CLIENTS & TELEPORT
        BattleRoyalePrepare prepare_clients = new BattleRoyalePrepare;
        m_States.Insert(prepare_clients);

        // (5) UNLOCK CLIENTS AND START MATCH WOO
        BattleRoyaleStartMatch start_match = new BattleRoyaleStartMatch;
        m_States.Insert(start_match);

		// (6) ROUNDS
        int num_states = m_States.Count();
        for(int i = 0; i < i_NumRounds; i++)
        {
            BattleRoyaleUtils.Trace("Add Round " + i);
            int prev_state_ind = i + num_states - 1;
            BattleRoyaleState previous_state = m_States[prev_state_ind];
            BattleRoyaleRound round = new BattleRoyaleRound(previous_state);
            m_States.Insert(round);
        }

        // (7) LAST ROUND
        BattleRoyaleLastRound last_round = new BattleRoyaleLastRound(m_States[m_States.Count() - 1]);
        m_States.Insert(last_round);

        // (8) WINNING PLAYER/TEAM
        m_States.Insert(new BattleRoyaleWin);

        // (9) RESTART SERVER
        m_States.Insert(new BattleRoyaleRestart);

        i_CurrentStateIndex = 0;  // start at the first state
        GetCurrentState().Activate();  // activate the first state

        RandomizeServerEnvironment();

#ifdef BLUE_ZONE
        BattleRoyaleUtils.Trace("Instance BlueZone Server");
        vector blue_zone_pos = "14829.2 73 14572.3";
        blue_zone_pos[1] = GetGame().SurfaceY(blue_zone_pos[0], blue_zone_pos[2]) + 10;

        BattleRoyaleUtils.Trace(blue_zone_pos);

        GetGame().CreateObjectEx( "BlueZone", blue_zone_pos, ECE_NOLIFETIME );
#endif
    }

	static BattleRoyaleServer GetInstance()
	{
		return m_Instance;
	}

    override bool IsDebug()
    {
        BattleRoyaleState m_CurrentState = GetCurrentState();
        BattleRoyaleDebug m_Debug;

        if(Class.CastTo(m_Debug, m_CurrentState))
        {
            return true;
        }

        //not debug state, check if match is actually running!
        BattleRoyalePrepare m_Prep;

        if(Class.CastTo(m_Prep, m_CurrentState))
        {
            //we are in prep state! - consider this a debug state!
            return true;
        }
        return false;
    }

	const float CHECK_IS_COMPLETE = 0.1;  //seconds
	float m_TimeSinceLastTick = CHECK_IS_COMPLETE + 1;

    override void Update(float delta)
    {
        float timeslice = delta; //Legacy

#ifdef SPECTATOR
        m_SpectatorSystem.Update(timeslice);
#endif

        foreach(BattleRoyaleState state: m_States)
        {
            if(state)
                state.Update(timeslice);
            else
                Error("BAD STATE IN m_States!");
        }

		m_TimeSinceLastTick += delta;

        //--- transition states
        if (m_TimeSinceLastTick > CHECK_IS_COMPLETE)
        {
        	m_TimeSinceLastTick = 0;

			if (GetCurrentState().IsComplete()) //current state is complete
			{
				int next_index = GetNextStateIndex();
				if(next_index > 0)
				{
					BattleRoyaleState next_state = GetState(next_index);

					BattleRoyaleUtils.Trace("[State Machine] Leaving State `" + GetCurrentState().GetName() + "`");
					if(GetCurrentState().IsActive())
						GetCurrentState().Deactivate(); //deactivate old state

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
					BattleRoyaleUtils.Trace("[State Machine] Entering State `" + GetCurrentState().GetName() + "`");
					GetCurrentState().Activate(); //activate new state
				}
				else
				{
					Error("NEXT STATE IS NULL!");
				}
			}
        }
    }

    void OnPlayerConnected(PlayerBase player)
    {
        //Teleport player into debug zone
        BattleRoyaleUtils.Trace("Player " + player.GetIdentity().GetName() + " connected!"); //lets find out if respawning players end up here

        //Copy PlainID (steamid) to PlayerBase to avoid the disparition of PlayerIdentity (OnPlayerDisconnected)
        player.player_steamid = player.GetIdentity().GetPlainId();

        //Dirty way to sync server settings with the client | this should be converted into a generic "sync settings" function
        BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();

        BattleRoyaleDebug m_Debug = BattleRoyaleDebug.Cast( GetState(0) );
        vector debug_pos = m_Debug.GetCenter();

        vector spawn_pos = "0 0 0";
        spawn_pos[0] = Math.RandomFloatInclusive((debug_pos[0] - 5), (debug_pos[0] + 5));
        spawn_pos[2] = Math.RandomFloatInclusive((debug_pos[2] - 5), (debug_pos[2] + 5));
        spawn_pos[1] = GetGame().SurfaceY(spawn_pos[0], spawn_pos[2]);

        player.SetPosition( spawn_pos );

		float dir = Math.RandomFloat(0, 360);
		vector playerDir = vector.YawToVector(dir);
		player.SetDirection( Vector(playerDir[0], 0, playerDir[1]) );

        bool b_AutoSpectateMode = BattleRoyaleConfig.GetConfig().GetGameData().auto_spectate_mode;

        BattleRoyaleDebugState m_DebugStateObj;

        // if we are not in the debug state, then we need to check if the player can spectate
        if(!Class.CastTo(m_DebugStateObj, GetCurrentState()))
        {
#ifdef SPECTATOR
            if(m_SpectatorSystem.CanSpectate( player ))
            {
                if (b_AutoSpectateMode)
                {
                    BattleRoyaleUtils.Trace("Spectator connected, inserting into spectator system");

                    float time_until_spectate = 20.0;

                    //it seems that AddPlayer's client init may be causing some crashes, so we'll wait 15 seconds and then initialize the player as a spectator
                    //note that 20 seconds is still too short. increased for now, but a more effective way of knowing when the player is "ready for interaction" is necessary
                    m_Timer.Run( time_until_spectate, m_SpectatorSystem, "AddPlayer", new Param1<PlayerBase>( player ), false);

                    string message = string.Format("You will be given spectator in ~%1 seconds...", time_until_spectate);

                    //m_SpectatorSystem.AddPlayer( player );
                    if(player)
                    {
						MessagePlayer( player, message, DAYZBR_MSG_TIME, time_until_spectate );
                    }
                }
            }
            else
            {
#endif
                //BAD VERY BAD!
                //This gives the player 15 seconds to finish his setup before we boot him. There may still be a chance it crashes.
                //Ideally the player should notify us when he is "ready" to be disconnected (I have no idea when that would be)

                //NOTE: calling this will immediately crash the server (as the player hasn't fully established his connection yet) GetGame().DisconnectPlayer(player.GetIdentity());

                Error("PLAYER CONNECTED DURING NON-DEBUG ZONE STATE!");
                m_Timer.Run( 30.0, this, "Disconnect", new Param1<PlayerIdentity>( player.GetIdentity() ), false);
#ifdef SPECTATOR
            }
#endif

            //TODO: Create a *spectator* system that handles players connecting during non-debug zone states
            //Note: the spectator system will also handle client respawn events too.
            //We need to create a list of *allowed* spectators. This should be in the server config (for private servers)

			// If the player is not a spectator, we don't want to add him to the state
            return;
        }

        // only add player if they connect during debug (otherwise they're a spectator)
        if( player.GetIdentity() )
            player.owner_id = player.GetIdentity().GetPlainId(); //cache their id (for connection loss)

        GetCurrentState().AddPlayer(player);

        if( match_uuid == "" )
        	GetCurrentState().MessagePlayerUntranslated( player, "STR_BR_MM_ERROR_REGISTERING_MATCH");
    }

    void Disconnect(PlayerIdentity identity)
    {
        GetGame().DisconnectPlayer( identity ); //can't directly call disconnectplayer with timer, so we use this method
    }

    void OnPlayerDisconnect(PlayerBase player, PlayerIdentity identity)
    {
    	// If the game is currently running, and the player is not a spectator, we kill the player
#ifdef SPECTATOR
		if ( GetCurrentState().ContainsPlayer(player) && !m_SpectatorSystem.ContainsPlayer(player) )
#else
		if ( GetCurrentState().ContainsPlayer(player) )
#endif
		{
		    if ( player.IsUnconscious() )
            {
                // We add a kill to the last damage source
                if ( player.last_unconscious_source )
                {
                    if ( player.last_unconscious_source.IsInherited( PlayerBase ) )
                    {
                        BattleRoyaleUtils.Info("Player " + player.GetIdentity().GetName() + " disconnected while unconscious, adding kill to last damage source.");

                        PlayerBase killer = PlayerBase.Cast( player.last_unconscious_source );
                        GetCurrentState().OnPlayerKilled( player, killer );
                    } else {
                        BattleRoyaleUtils.Error("Player " + player.GetIdentity().GetName() + " disconnected while unconscious, but the last damage source is not a player.");
                    }
                } else {
                    BattleRoyaleUtils.Error("Player " + player.GetIdentity().GetName() + " disconnected while unconscious, but there is no last damage source.");
                }

                // If the player is alive, we kill him
                BattleRoyaleUtils.Info("Player " + player.GetIdentity().GetName() + " disconnected while unconscious, killing him.");
                player.SetHealth("GlobalHealth", "Health", 0);
                player.SetHealth("GlobalHealth", "Blood", 0);
            }
		}
    }

    void OnPlayerDisconnected(PlayerBase player, PlayerIdentity identity)
    {
        if(GetCurrentState().ContainsPlayer(player))
        {
            //if we are in a round, then we need to call OnPlayerDisconnected (since it's not a state based function we must cast)
            if(i_CurrentStateIndex > 2 && i_CurrentStateIndex < m_States.Count() - 2 )
                GetCurrentState().OnPlayerDisconnected(player);

            GetCurrentState().RemovePlayer(player);
        }
    }

    override void OnPlayerKilled(PlayerBase killed, Object killer)
    {
        if(GetCurrentState().ContainsPlayer(killed))
        {
            //if we are in a round, then we need to call onplayerkilled (since it's not a state based function we must cast)
            if(i_CurrentStateIndex > 2 && i_CurrentStateIndex < m_States.Count() - 2 )
                GetCurrentState().OnPlayerKilled(killed, killer);

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
            GetCurrentState().OnPlayerTick(player, timeslice);

            //--- check if they have entered an invalid position
            vector pos = player.GetPosition();
            float bigNum = 1000000;
            //when invalid, height gets fucked, lets check that (others are NaN & may cause errors)
            if(pos[1] > bigNum || pos[1] < (-1 * bigNum))
            {
                if(temp_disconnecting.Find(player) == -1)
                {
                    temp_disconnecting.Insert(player);
                    //GetGame().DisconnectPlayer( player.GetIdentity() );
                    GetGame().SendLogoutTime(player, 0);
                    //BattleRoyaleUtils.Trace(pos);
                    //Error("PLAYER FOUND IN INVALID POSITION!");
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
#ifdef SPECTATOR
                    if(m_SpectatorSystem.ContainsPlayer( player ))
                    {
                        m_SpectatorSystem.OnPlayerTick( player, timeslice );
                    }
                    else
                    {
#endif
                        if ( temp_disconnecting.Find(player) == -1 )
                        {
#ifdef SPECTATOR
							if ( m_SpectatorSystem.CanSpectate( player ) ) //TODO: find a better way to do this, if someone is bugged,but they can be a spectator, they aren't kicked :/
							{
#endif
								//this ensures we only call disconnect on this player once
								temp_disconnecting.Insert(player);

								GetGame().SendLogoutTime(player, 0);
								//GetGame().DisconnectPlayer( player.GetIdentity() );
								//Error("GetCurrentState() DOES NOT CONTAIN PLAYER TICKING!");
#ifdef SPECTATOR
							}
#endif
                        }
#ifdef SPECTATOR
                    }
#endif
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

        for(int i = i_CurrentStateIndex + 1; i < m_States.Count(); i++)
        {
            BattleRoyaleState state = m_States[i];
            if( !state.SkipState(GetState(i_CurrentStateIndex)) )
            {
                return i;
            } else {
                BattleRoyaleUtils.Trace("[State Machine] Skipping State `" + state.GetName() + "`");
            }
        }

        return -1;
    }

#ifdef SPECTATOR
    ref BattleRoyaleSpectators GetSpectatorSystem()
    {
        return m_SpectatorSystem;
    }

    void RequestEntityHealthUpdate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {

        PlayerBase pbTarget;
        //--- client is requesting stats on the existing player (ensure their stats are updated and send a result back only to that specific client)
        BattleRoyaleUtils.Trace("Spectator client requested status update for target");
        if(Class.CastTo( pbTarget, target ))
        {
            pbTarget.UpdateHealthStats( pbTarget.GetHealth01("", "Health"), pbTarget.GetHealth01("", "Blood") );
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateEntityHealth", new Param2<float,float>( pbTarget.health_percent, pbTarget.blood_percent ), true, sender, pbTarget);
        }
    }
#endif

    void PlayerReadyUp(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
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

    void PlayerUnstuck(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
		if(!target) return;

		PlayerBase targetBase = PlayerBase.Cast(target);

		if(!targetBase) return;

		if(type == CallType.Server)
		{
			BattleRoyaleStartMatch m_StartMatchStateObj;
			if(!Class.CastTo(m_StartMatchStateObj, GetCurrentState()))
				return;

            if(!m_StartMatchStateObj.ContainsPlayer(targetBase))
            {
                Error("StartMatch state does not contain player requesting unstuck!");
                return;
            }

			m_StartMatchStateObj.DeferredUnstuck( targetBase );
			targetBase.SetSynchDirty();
		}
    }

    //TODO: This will need a rework!
    void RandomizeServerEnvironment()
    {
        BattleRoyaleUtils.Trace("BattleRoyale: Randomizing Environment!");
        //NOTE: this is all legacy, we should find a better way to do this
        int year = 2018;
        int month = Math.RandomIntInclusive(3, 9); //march to september
        int day = 24;
        int hour = Math.RandomIntInclusive(8,16); //7am to 4pm
        int minute = 0;
        GetGame().GetWorld().SetDate(year, month, day, hour, minute);

		string world_name = "";
		GetGame().GetWorldName(world_name);
		world_name.ToLower();

		if (world_name != "takistanplus")
		{
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

#ifdef SPECTATOR
    //--- Admin tool functions only
    void TestSpectator(PlayerBase player)
    {
        OnPlayerKilled(player, NULL); //1. simulate player killed ()
        GetSpectatorSystem().AddPlayer( player ); //2. insert them into spectator system (like simulating joining during a non-debug state)
    }
#endif

    void NextState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        BattleRoyaleUtils.Trace("BattleRoyaleManager NextState");
#ifdef SERVER
        BattleRoyaleServer m_BrServer;
        if(Class.CastTo( m_BrServer, GetBR()))
        {
            BattleRoyaleUtils.Trace("[DayZBR COT] State Machine Skipping!");
            m_BrServer.GetCurrentState().Deactivate();// super.IsComplete() will return TRUE when this is run
        }
        else
        {
            Error("Failed to cast GetBR() to BattleRoyaleServer");
        }
#endif
    }

#ifdef SPECTATOR
    void StartSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        BattleRoyaleUtils.Trace("BattleRoyaleManager StartSpectate");
        PlayerBase pbTarget;
        if(Class.CastTo( pbTarget, target ))
        {
            BattleRoyaleServer m_BrServer;
            if(Class.CastTo( m_BrServer, GetBR()))
            {
                BattleRoyaleUtils.Trace("[DayZBR COT] Testing Spectating!");
                m_BrServer.TestSpectator(pbTarget);
            }
            else
            {
                Error("Failed to cast GetBR() to BattleRoyaleServer");
            }
        }
        else
        {
            Error("Failed to cast TestSpectator target object to playerbase");
        }
    }
#endif
}
