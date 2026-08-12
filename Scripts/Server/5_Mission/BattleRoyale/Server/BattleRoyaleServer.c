#ifdef SERVER
class BattleRoyaleServer: BattleRoyaleBase
{
	protected static BattleRoyaleServer m_Instance;
    ref array<ref BattleRoyaleState> m_States;
    int i_CurrentStateIndex;

    int i_NumRounds;

    bool b_EnableSpawnSelectionMenu;

    string match_uuid;

    protected ref Timer m_Timer;

    void BattleRoyaleServer()
    {
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerUnstuck", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestEntityHealthUpdate", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestLeaderboard", this);
#ifdef VPPADMINTOOLS
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "NextState", this, SingleplayerExecutionType.Server);
#endif
#ifdef DIAG_DEVELOPER
        //--- Guarded on the same define as its handler, unlike NextState above, whose registration
        //--- is VPPADMINTOOLS while its only caller is JM_COT - a COT-without-VPP build has a live
        //--- button and nothing listening.
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "BRDiagAction", this );
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

		//--- Reads steam_names.json back in. The process restarts between matches, so without this
		//--- every match would re-query every returning player.
		BattleRoyaleNameService.Init();

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
        	match_uuid = "";  // No API, no match to register
        }

        m_Timer = new Timer;

        //load config (this may error because GetBattleRoyale would return false)
        BattleRoyaleZoneData m_ZoneData = config_data.GetZoneData();
        BattleRoyaleLobbyData m_LobbyData = config_data.GetLobbyData();
        i_NumRounds = m_ZoneData.num_zones;
        b_EnableSpawnSelectionMenu = m_LobbyData.enable_spawn_selection_menu;

        //--- initialize all states (in order from start to finish)
        m_States = new array<ref BattleRoyaleState>;

        // (1) DEBUG ZONE
        BattleRoyaleDebug debug_state = new BattleRoyaleDebug;
        m_States.Insert(debug_state); //insert debug state

        // (2) PLAYER COUNT REACHED COUNTDOWN
        BattleRoyaleCountReached count_reached = new BattleRoyaleCountReached;
        m_States.Insert(count_reached);

        // (3) SPAWN SELECTION MENU
        if (b_EnableSpawnSelectionMenu)
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

#ifdef DIAG_DEVELOPER
	//! Diag "Jump To State" target, or -1. See the fast-forward block in Update().
	int m_DiagTargetState = -1;
#endif

    override void Update(float delta)
    {
        float timeslice = delta; //Legacy

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

			//--- Debounced leaderboard write. Cheap when nothing changed, and match end forces its
			//--- own flush, so this only ever catches mid-match progress.
			BattleRoyaleLeaderboard.GetInstance().Tick();

			//--- Push each player the list of speakers they can actually hear. Self-throttled to
			//--- BR_SPEAKING_POLL_MS and only sends on change, so this is cheap at 10 Hz.
			BattleRoyaleVoice.UpdateSpeakers();

			//--- Flush queued Steam name lookups once the batch window closes, so a lobby filling up
			//--- in a burst costs one request rather than one per player. Two comparisons when idle.
			BattleRoyaleNameService.Tick();

#ifdef VIGRID_PARTY
			//--- A resolved name is not a party composition change, so Party has no reason to re-send
			//--- its rosters and its HUD row would keep rendering "Survivor". Consume-once flag, so
			//--- this is a single bool test per tick.
			if ( BattleRoyaleNameService.ConsumePartyRefresh() )
				VigridPartyAPI.RefreshRosterNames();
#endif

#ifdef DIAG_DEVELOPER
			//--- Diag fast-forward. Deliberately a fast-forward and not a seek: it deactivates the
			//--- current state once per tick and lets the transition below run normally, so every
			//--- state on the way still gets its Activate() and nothing is left half-initialised.
			//--- Ten states cost one second. Clearing the target on arrival is what stops it from
			//--- driving the machine off the end.
			if ( m_DiagTargetState >= 0 )
			{
				if ( i_CurrentStateIndex >= m_DiagTargetState )
					m_DiagTargetState = -1;
				else if ( GetCurrentState().IsActive() )
					GetCurrentState().Deactivate();
			}
#endif

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
        //Same reason: the leaderboard has to render a name for someone who already left.
        player.player_name = BattleRoyaleNameService.ResolveIdentity( player.GetIdentity() );

        //--- A player who never set a name in the launcher connects as "Survivor", and the engine
        //--- turns a second one into "Survivor (2)". Queue a Steam lookup; the answer lands a few
        //--- seconds later and rewrites player_name plus vanilla's own cached name. No-op unless
        //--- enable_steam_name_lookup is on.
        BattleRoyaleNameService.RequestForPlayer( player );

        //--- And hand them everyone else's resolved names, so a late joiner does not see "Survivor"
        //--- on players the server corrected before they arrived.
        BattleRoyaleNameService.SendAllResolvedNames( player.GetIdentity() );

        //Dirty way to sync server settings with the client | this should be converted into a generic "sync settings" function
        BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();
        BattleRoyaleServerData m_ServerData = config_data.GetServerData();

        //--- The speaking-players panel is a client HUD element gated by two server settings, so the
        //--- joining player needs them before their first frame of gameplay.
        BattleRoyaleVoiceData voice_settings = config_data.GetVoiceData();
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetVoiceSettings", new Param2<bool, bool>( voice_settings.show_speaking_players, voice_settings.speaking_list_during_match ), true, player.GetIdentity() );

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

        BattleRoyaleDebugState m_DebugStateObj;

        if(!Class.CastTo(m_DebugStateObj, GetCurrentState()))
        {
			//BAD VERY BAD!
			//This gives the player 15 seconds to finish his setup before we boot him. There may still be a chance it crashes.
			//Ideally the player should notify us when he is "ready" to be disconnected (I have no idea when that would be)

			//NOTE: calling this will immediately crash the server (as the player hasn't fully established his connection yet) GetGame().DisconnectPlayer(player.GetIdentity());

			BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
			ref array<string> a_AdminsList = m_GameSettings.admins_steamid64;

			if ( a_AdminsList.Find( player.GetIdentity().GetPlainId() ) != -1 )
			{
				BattleRoyaleUtils.Info("Admin " + player.GetIdentity().GetName() + " has connected during non-debug state, allowing connection.");
				return; //allow admins to connect during non-debug state
			}

			Error("PLAYER CONNECTED DURING NON-DEBUG ZONE STATE!");
			m_Timer.Run( 30.0, this, "Disconnect", new Param1<PlayerIdentity>( player.GetIdentity() ), false);

            return;
        }

        // only add player if they connect during debug
        if( player.GetIdentity() )
            player.owner_id = player.GetIdentity().GetPlainId(); //cache their id (for connection loss)

        GetCurrentState().AddPlayer(player);

        if( m_ServerData.enable_vigrid_api && m_ServerData.warning_no_uuid && match_uuid == "" )
        	GetCurrentState().MessagePlayerUntranslated( player, "STR_BR_MM_ERROR_REGISTERING_MATCH");
    }

    void Disconnect(PlayerIdentity identity)
    {
        GetGame().DisconnectPlayer( identity ); //can't directly call disconnectplayer with timer, so we use this method
    }

    void OnPlayerDisconnect(PlayerBase player, PlayerIdentity identity)
    {
		if ( GetCurrentState().ContainsPlayer(player) )
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
                        BattleRoyaleUtils.Info("Player " + player.GetIdentity().GetName() + " disconnected while unconscious, but the last damage source is not a player.");
                    }
                } else {
                    BattleRoyaleUtils.Info("Player " + player.GetIdentity().GetName() + " disconnected while unconscious, but there is no last damage source.");
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
					if ( temp_disconnecting.Find(player) == -1 )
					{
						//this ensures we only call disconnect on this player once
						temp_disconnecting.Insert(player);

						GetGame().SendLogoutTime(player, 0);
						//GetGame().DisconnectPlayer( player.GetIdentity() );
						//Error("GetCurrentState() DOES NOT CONTAIN PLAYER TICKING!");
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

        for(int i = i_CurrentStateIndex + 1; i < m_States.Count(); i++)
        {
            BattleRoyaleState state = m_States[i];
            if( !state.SkipState(GetState(i_CurrentStateIndex)) )
            {
                return i;
            } else {
                BattleRoyaleUtils.Trace("[State Machine] Skipping State `" + state.GetName() + "`");
                //--- Record it: a skipped round still holds a fully constructed zone, and without
                //--- this the next round would treat that never-played circle as the live boundary.
                state.SetSkipped(true);
            }
        }

        return -1;
    }

    //--- Null-safe identity description for rejection logs. `sender` may legitimately be NULL,
    //--- so it must never be dereferenced directly inside a log string.
    static string GetIdentityLogName(PlayerIdentity identity)
    {
        if(!identity)
            return "<null identity>";

        return identity.GetName() + " (" + identity.GetPlainId() + ")";
    }

    //--- Authorization gate for admin-only RPCs.
    //--- `sender` is supplied by the engine and cannot be forged by the client, unlike the
    //--- `Object target` these handlers used to trust. Uses the same admins_steamid64 list that
    //--- OnPlayerConnected() already consults to let admins join outside the lobby state.
    //--- Static so the COT module (BRMasterControlsModule) can share it.
    static bool IsAdminIdentity(PlayerIdentity identity)
    {
        if(!identity)
            return false;

        string steamid = identity.GetPlainId();
        if(steamid == "")
            return false;

        BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();
        if(!config_data)
            return false;

        BattleRoyaleGameData game_settings = config_data.GetGameData();
        if(!game_settings)
            return false;

        if(!game_settings.admins_steamid64)
            return false;

        return (game_settings.admins_steamid64.Find(steamid) != -1);
    }

    //--- NOTE: `target` is deliberately ignored here - it is client-chosen and could name any
    //--- other player. The subject is always resolved from `sender` instead.
    void PlayerReadyUp(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1< bool > data;
        if( !ctx.Read( data ) ) return;

        if(type == CallType.Server)
        {
            BattleRoyaleDebug m_DebugStateObj;
            if(!Class.CastTo(m_DebugStateObj, GetCurrentState())) //this ensures we can only ready up during the debug state
                return;

            PlayerBase senderBase = m_DebugStateObj.GetPlayerFromIdentity(sender);
            if(!senderBase)
            {
                Error("Debug state does not contain player requesting ready up!");
                return;
            }

            if(data.param1)
            {
                m_DebugStateObj.ReadyUp(senderBase);
            }
            else
            {
                //perhaps allow readyup to be toggled?
            }

        }
    }

    //--- NOTE: `target` is deliberately ignored here - see PlayerReadyUp above.
    void PlayerUnstuck(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
		if(type == CallType.Server)
		{
			//--- Asked of the state rather than by casting to one. This used to require
			//--- BattleRoyaleStartMatch specifically, which meant F2 was dead in the lobby - where a
			//--- wedged player has no other way out, and where staying wedged until Prepare costs
			//--- them their whole loadout. Every state that has not opted in still refuses.
			BattleRoyaleState state = GetCurrentState();
			if(!state || !state.AllowsUnstuck())
				return;

            PlayerBase senderBase = state.GetPlayerFromIdentity(sender);
            if(!senderBase)
            {
                //--- Warn, not Error: Error() raises a VM exception, and now that the lobby is in
                //--- scope an RPC arriving from someone the state has not registered yet - or has
                //--- just dropped - is an ordinary race rather than a fatal condition.
                BattleRoyaleUtils.Warn("Current state does not contain the player requesting unstuck!");
                return;
            }

			state.DeferredUnstuck( senderBase );
			senderBase.SetSynchDirty();
		}
    }

    //--- NOTE: `target` is deliberately ignored here - see PlayerReadyUp above.
    //--- Unlike the other handlers this one is answerable in ANY state: the leaderboard is mostly a
    //--- lobby feature, and there is nothing state-sensitive about reading it.
    void RequestLeaderboard(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1< int > data;
        if( !ctx.Read( data ) ) return;

        if(type == CallType.Server)
        {
            //--- Rate limiting and board validation both live in ServeRequest.
            BattleRoyaleLeaderboard.GetInstance().ServeRequest( sender, data.param1 );
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

		string world_name = GetGame().GetWorldName();
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

#ifdef DIAG_DEVELOPER
    /**
     *  Every server-side debug action, behind one RPC.
     *
     *  One RPC carrying an action id rather than one named RPC per action: adding an action is one
     *  enum value plus one case here, with no new registration and no wire change to keep in sync
     *  between the two sides.
     *
     *  The refusal path logs. A silently-refused button and a broken button look identical from the
     *  diag menu, and the usual cause is simply that the tester is not in admins_steamid64 - which
     *  is read from the PROFILE general_settings.json, since that field is exempt from the mission
     *  override.
     */
    void BRDiagAction(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        Param3<int, int, float> data;
        if(!ctx.Read(data))
        {
            BattleRoyaleUtils.Warn("Failed to read BRDiagAction RPC");
            return;
        }

        if(!IsAdminIdentity(sender))
        {
            BattleRoyaleUtils.Warn("Rejected unauthorized BRDiagAction " + data.param1 + " from " + GetIdentityLogName(sender));
            return;
        }

        BattleRoyaleUtils.Info("[Diag] action " + data.param1 + " (" + data.param2 + ", " + data.param3 + ") from " + GetIdentityLogName(sender));

        BattleRoyaleState state = GetCurrentState();

        //--- Asked of the state rather than of the game: a player the current state does not hold
        //--- is one OnPlayerTick would be force-logging-out anyway, so "not in this state" is the
        //--- right answer to "who sent this".
        PlayerBase sender_player;
        if(state)
            sender_player = state.GetPlayerFromIdentity(sender);

        switch(data.param1)
        {
            case BattleRoyaleDiagAction.SKIP_STATE:
            {
                //--- Same one-liner the COT and VPP paths use: a state signals "done" by
                //--- deactivating itself, and Update() picks the transition up at 10 Hz.
                if(state)
                    state.Deactivate();
                break;
            }

            case BattleRoyaleDiagAction.SET_PAUSED:
            {
                if(!state)
                    break;

                if(data.param2 != 0)
                    state.Pause();
                else
                    state.Resume();
                break;
            }

            case BattleRoyaleDiagAction.GOTO_STATE:
            {
                //--- Clamped, and this is not a nicety: the last state is BattleRoyaleRestart, whose
                //--- Activate() calls RequestExit. A fat-fingered target would kill the server.
                //--- Not named `target`: this handler already has an Object parameter by that name,
                //--- and EnfusionScript allows one declaration per name per method scope.
                int max_target = m_States.Count() - 2;
                int goto_target = data.param2;
                if(goto_target > max_target)
                    goto_target = max_target;
                if(goto_target < 0)
                    goto_target = 0;

                if(goto_target <= i_CurrentStateIndex)
                {
                    BattleRoyaleUtils.Warn("[Diag] cannot jump back to state " + goto_target + ", already at " + i_CurrentStateIndex);
                    break;
                }

                BattleRoyaleUtils.Info("[Diag] fast-forwarding from state " + i_CurrentStateIndex + " to " + goto_target);
                m_DiagTargetState = goto_target;
                break;
            }

            case BattleRoyaleDiagAction.FORCE_READY_ALL:
            {
                //--- BattleRoyaleDebug specifically, not BattleRoyaleDebugState: ReadyUp and the
                //--- ready list live on the lobby state, while CountReached shares the base class.
                BattleRoyaleDebug lobby;
                if(!Class.CastTo(lobby, state))
                {
                    BattleRoyaleUtils.Warn("[Diag] Force Ready All only works in the lobby state");
                    break;
                }

                //--- Readies everyone through the real ReadyUp path, so the countdown, the messages
                //--- and the vote threshold all behave exactly as they would with real players.
                ref array<PlayerBase> lobby_players = lobby.GetPlayers();
                for(int r = 0; r < lobby_players.Count(); r++)
                {
                    if(lobby_players[r])
                        lobby.ReadyUp(lobby_players[r]);
                }
                break;
            }

            case BattleRoyaleDiagAction.LOG_STATE:
            {
                if(!state)
                    break;

                //--- Info rather than Debug: on a DIAG server this mirrors into in-game chat, so the
                //--- answer lands where the tester is looking instead of only in the log.
                BattleRoyaleUtils.Info("[Diag] state " + i_CurrentStateIndex + "/" + (m_States.Count() - 1) + " `" + state.GetName() + "` players=" + state.GetPlayers().Count() + " active=" + state.IsActive());
                break;
            }

            case BattleRoyaleDiagAction.TP_ZONE_CENTER:
            {
                BattleRoyaleRound round_current;
                if(!Class.CastTo(round_current, state))
                {
                    BattleRoyaleUtils.Warn("[Diag] TP: Zone Centre needs a round state, not `" + state.GetName() + "`");
                    break;
                }

                //--- GetActiveZone is the circle that is actually damaging right now, which before
                //--- the 80% lock is the PREVIOUS one - that is the circle a tester means.
                BattleRoyaleZone active_zone = round_current.GetActiveZone();
                if(!active_zone)
                    active_zone = round_current.GetZone();

                if(active_zone && active_zone.GetArea())
                    state.BR_DiagTeleport(sender_player, active_zone.GetArea().GetCenter());
                break;
            }

            case BattleRoyaleDiagAction.TP_NEXT_ZONE:
            {
                BattleRoyaleRound round_next;
                if(!Class.CastTo(round_next, state))
                {
                    BattleRoyaleUtils.Warn("[Diag] TP: Next Zone needs a round state, not `" + state.GetName() + "`");
                    break;
                }

                if(round_next.GetZone() && round_next.GetZone().GetArea())
                    state.BR_DiagTeleport(sender_player, round_next.GetZone().GetArea().GetCenter());
                break;
            }

            case BattleRoyaleDiagAction.TP_LOBBY:
            {
                //--- State 0 is always the lobby, whatever the rest of the machine looks like.
                BattleRoyaleDebugState lobby_state;
                if(Class.CastTo(lobby_state, GetState(0)) && state)
                    state.BR_DiagTeleport(sender_player, lobby_state.GetCenter());
                break;
            }

            case BattleRoyaleDiagAction.FORCE_UNSTUCK:
            {
                if(!state || !sender_player)
                    break;

                //--- Clear the gates rather than going around DeferredUnstuck: the 1-3 s defer and
                //--- the position search are the parts worth exercising, and the 30 s cooldown is
                //--- the only thing that makes the ladder repro slow to iterate on.
                sender_player.wait_unstuck = false;
                sender_player.next_unstuck_time = 0;
                state.DeferredUnstuck(sender_player);
                break;
            }

            case BattleRoyaleDiagAction.LOG_ZONE_TABLE:
            {
                //--- Generation runs smallest-first, so index 0 is the FINAL circle and each later
                //--- index contains the one before it. This dump is the cheapest way to see that.
                if(!BattleRoyaleZone.m_PlayAreas)
                {
                    BattleRoyaleUtils.Warn("[Diag] no play areas generated");
                    break;
                }

                BattleRoyaleUtils.Info("[Diag] play areas, generation order (index 0 is the FINAL circle):");
                for(int z = 0; z < BattleRoyaleZone.m_PlayAreas.Count(); z++)
                {
                    BattleRoyalePlayArea area = BattleRoyaleZone.m_PlayAreas[z];
                    if(!area)
                        continue;

                    float offset = 0;
                    if(BattleRoyaleZone.s_PlayAreaDurationOffsets && z < BattleRoyaleZone.s_PlayAreaDurationOffsets.Count())
                        offset = BattleRoyaleZone.s_PlayAreaDurationOffsets[z];

                    BattleRoyaleUtils.Info("[Diag]   [" + z + "] center=" + area.GetCenter() + " radius=" + area.GetRadius() + " duration_offset=" + offset);
                }
                break;
            }

            case BattleRoyaleDiagAction.CLEAR_MAP_MARKERS:
            {
#ifdef VIGRID_MAP
                VigridMapAPI.ClearAllMarkers();
                BattleRoyaleUtils.Info("[Diag] cleared every map marker");
#endif
                break;
            }

            case BattleRoyaleDiagAction.SET_LOG_LEVEL:
            {
                BattleRoyaleDiag.log_level_override = data.param2;
                //--- Printed unconditionally through Print, because the new level may well be one
                //--- that swallows this very line.
                Print("[DayZ-BattleRoyale][Diag] server log level override -> " + data.param2);
                break;
            }

            case BattleRoyaleDiagAction.SET_CHAT_MIRROR:
            {
                BattleRoyaleDiag.chat_mirror = (data.param2 != 0);
                Print("[DayZ-BattleRoyale][Diag] chat mirror -> " + BattleRoyaleDiag.chat_mirror);
                break;
            }

            case BattleRoyaleDiagAction.SET_TRACE_TP:
            {
                BattleRoyaleDiag.trace_teleport = (data.param2 != 0);
                BattleRoyaleDiag.trace_teleport_ticks = (int)data.param3;
                BattleRoyaleUtils.Info("[Diag] teleport trace -> " + BattleRoyaleDiag.trace_teleport + " for " + BattleRoyaleDiag.trace_teleport_ticks + " ticks");
                break;
            }

            default:
            {
                BattleRoyaleUtils.Warn("[Diag] unknown action " + data.param1);
                break;
            }
        }
    }
#endif

    void NextState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        BattleRoyaleUtils.Trace("BattleRoyaleManager NextState");
#ifdef SERVER
        if(type != CallType.Server)
            return;

        //--- Skipping a match phase is an admin action. The VPP "MenuBattleRoyaleManager" permission
        //--- only decides whether the client renders the button, so it is no protection at all here.
        if(!IsAdminIdentity(sender))
        {
            BattleRoyaleUtils.Warn("Rejected unauthorized NextState request from " + GetIdentityLogName(sender));
            return;
        }

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
}
