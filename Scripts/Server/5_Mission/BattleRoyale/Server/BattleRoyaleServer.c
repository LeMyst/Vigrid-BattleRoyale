#ifdef SERVER
//--- One player who connected after the match left the lobby, waiting to be disconnected.
//
//--- `player` is deliberately a weak reference and is null-checked on every sweep: the player can
//--- quit on their own before the deadline, and PlayerIdentity is destroyed with them. `plain_id` is
//--- kept alongside so the entry can still be matched and logged after that happens - the same
//--- reason OnPlayerConnected caches player_steamid onto PlayerBase.
class BattleRoyaleLateJoiner
{
    PlayerBase player;
    string plain_id;
    int connect_ms;      //when OnPlayerConnected saw them (GetGame().GetTime() basis)
    bool armed;          //their client reported itself loaded in, so the grace period is running
    int deadline_ms;     //only meaningful once armed
    bool warned_final;
}

class BattleRoyaleServer: BattleRoyaleBase
{
	protected static BattleRoyaleServer m_Instance;
    ref array<ref BattleRoyaleState> m_States;
    int i_CurrentStateIndex;

    int i_NumRounds;

    bool b_EnableSpawnSelectionMenu;

    string match_uuid;

    //--- Players connected after the lobby ended, each with their own deadline. This used to be one
    //--- shared `ref Timer` on which every late joiner scheduled the same "Disconnect" callback, so a
    //--- second one inside the window silently replaced the first and the first was never kicked.
    protected ref array<ref BattleRoyaleLateJoiner> a_LateJoiners;
    //--- PlainIds (SteamID64) of admins allowed to stay connected mid-match. Checked by
    //--- ScheduleLateJoinKick, which is the only path that can schedule a kick - OnPlayerConnected
    //--- alone was not enough, because OnPlayerTick re-evicts anyone the current state does not hold
    //--- and an exempt admin is, by construction, in no state.
    protected ref array<string> a_LateJoinExempt;

    //--- Next time PushLobbyNames is allowed to send. Self-throttling, so its caller can sit in the
    //--- existing 10 Hz block without a second timer.
    protected int m_NextLobbyNamesMs;

    void BattleRoyaleServer()
    {
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerUnstuck", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestEntityHealthUpdate", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestLeaderboard", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerLoadedIn", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestSpectate", this);
        //--- Admin spectate. All four resolve their actor from the engine-supplied `sender` and all
        //--- four gate on BattleRoyaleSpectators.AdminEligibility, which is the single definition of
        //--- who may do this - see the header of BattleRoyaleSpectators.c.
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateToggle", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateCycle", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateMode", this);
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateCamPos", this);
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
        //--- Nothing to stop: the pending kicks are swept from Update(), not from engine timers, so
        //--- they die with the object. Clearing them is bookkeeping, not teardown.
        if ( a_LateJoiners )
            a_LateJoiners.Clear();
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

        a_LateJoiners = new array<ref BattleRoyaleLateJoiner>;
        a_LateJoinExempt = new array<string>;

        //load config (this may error because GetBattleRoyale would return false)
        BattleRoyaleZoneData m_ZoneData = config_data.GetZoneData();
        BattleRoyaleLobbyData m_LobbyData = config_data.GetLobbyData();
        i_NumRounds = m_ZoneData.num_zones;
        b_EnableSpawnSelectionMenu = m_LobbyData.enable_spawn_selection_menu;

        //--- Build every play-area circle here, as an explicit and logged boot step. It used to
        //--- happen invisibly inside the FIRST BattleRoyaleRound's constructor further down, which
        //--- put the generation log in the middle of the state-machine setup and made it look like a
        //--- per-round cost. Note i_NumRounds is read above and the state list below is sized from
        //--- it, so generation has to observe the same already-validated num_zones - it does, both
        //--- read it through GetZoneData().
        BattleRoyaleZone.PrepareGeneration();

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

			//--- Warn and then disconnect anyone who joined after the lobby ended. Two integer
			//--- comparisons per pending player, and the list is empty in the overwhelming majority
			//--- of ticks.
			UpdateLateJoiners();

			//--- Tell each lobby player who to draw a name over. Self-throttled to 1 Hz and returns
			//--- on one bool outside the lobby, so it costs nothing for the rest of the match.
			PushLobbyNames();

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
			//--- The ONLY driver for spectating: liveness sweep, deferred entry, retarget and the
			//--- 1 Hz keepalive push. Returns immediately when nobody is spectating, and only ever
			//--- touches uid strings, so it cannot fire against a freed player object.
			BattleRoyaleSpectators.GetInstance().Tick();

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

					//--- One call covering every transition, so no individual state has to remember
					//--- to announce itself and a state inserted later cannot forget to.
					BroadcastLobbyPhase();
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

        //--- A player who never set a name in the launcher connects as "Survivor", and the engine
        //--- turns a second one into "Survivor (2)". Queue a Steam lookup; the answer lands a few
        //--- seconds later and rewrites player_name plus vanilla's own cached name. No-op unless
        //--- enable_steam_name_lookup is on.
        //---
        //--- Above the player_name seed below, and that order is load-bearing: this call is also where
        //--- a returning player who has SINCE set a name of their own has their old override dropped,
        //--- and a ResolveIdentity() taken before that decision would bake the stale name into
        //--- player_name with nothing left to undo it.
        BattleRoyaleNameService.RequestForPlayer( player );

        //Copy the display name too: the leaderboard has to render a name for someone who already left.
        player.player_name = BattleRoyaleNameService.ResolveIdentity( player.GetIdentity() );

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

        BattleRoyaleDebugState m_DebugStateObj;

        //--- Note this tests BattleRoyaleDebugState, not BattleRoyaleDebug: BattleRoyaleCountReached
        //--- derives from it too, so joining during the pre-match countdown is accepted and the
        //--- player is added normally. Rejection genuinely begins at the state after that.
        bool in_lobby_state = Class.CastTo(m_DebugStateObj, GetCurrentState());

        //--- Resolved before the placement below, because an admin joining mid-match is not sent to
        //--- the lobby at all.
        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        bool is_admin = (m_GameSettings.admins_steamid64.Find( player.GetIdentity().GetPlainId() ) != -1);

        vector spawn_pos = "0 0 0";
        bool placed_at_zone = false;

        if(!in_lobby_state && is_admin)
        {
            //--- Admins are documented as "immune to kick and can go outside the play area", and they
            //--- already were free to roam - nothing leashes a player the current state does not hold
            //--- (the lobby clamp lives in BattleRoyaleDebugState.OnPlayerTick, which only runs for
            //--- state members). What they could not do was get anywhere: dropped at the lobby centre,
            //--- which is nowhere near the fight and is a long walk with no vehicle. So put them where
            //--- the match actually is.
            placed_at_zone = GetAdminJoinPosition( spawn_pos );
        }

        if(!placed_at_zone)
        {
            //--- The lobby half of GetAdminSpawnPosition, reached directly because the circle half
            //--- above has already been tried and its answer is what `placed_at_zone` records.
            BattleRoyaleDebug m_Debug = BattleRoyaleDebug.Cast( GetState(0) );
            vector debug_pos = m_Debug.GetCenter();

            spawn_pos[0] = Math.RandomFloatInclusive((debug_pos[0] - 5), (debug_pos[0] + 5));
            spawn_pos[2] = Math.RandomFloatInclusive((debug_pos[2] - 5), (debug_pos[2] + 5));
            spawn_pos[1] = GetGame().SurfaceY(spawn_pos[0], spawn_pos[2]);
        }

        player.SetPosition( spawn_pos );

		float dir = Math.RandomFloat(0, 360);
		vector playerDir = vector.YawToVector(dir);
		player.SetDirection( Vector(playerDir[0], 0, playerDir[1]) );

        if(!in_lobby_state)
        {
			//--- The player cannot be disconnected here. Their client has not finished establishing
			//--- its connection yet and GetGame().DisconnectPlayer on that identity crashes the
			//--- server, so the kick is deferred to UpdateLateJoiners.

			if ( is_admin )
			{
				//--- Remembered rather than just returned. The exemption used to be this bare return,
				//--- which lasted exactly one scheduled tick: an admin who is in no state fails
				//--- OnPlayerTick's ContainsPlayer test and got evicted by the branch down there.
				//--- ScheduleLateJoinKick consults this list, so both entry points now honour it.
				a_LateJoinExempt.Insert( player.GetIdentity().GetPlainId() );

				if ( placed_at_zone )
					BattleRoyaleUtils.Info("Admin " + player.GetIdentity().GetName() + " has connected during non-debug state, placed at the active circle " + spawn_pos + ".");
				else
					BattleRoyaleUtils.Info("Admin " + player.GetIdentity().GetName() + " has connected during non-debug state, no circle in play yet so placed at the lobby.");

				return; //allow admins to connect during non-debug state
			}

			//--- Warn, NOT Error. BattleRoyaleUtils.Error and the global Error() both end in Error2(),
			//--- which raises a VM exception and unwinds the stack - which is why the kick that used
			//--- to be scheduled on the line below this one never ran even once.
			BattleRoyaleUtils.Warn("Player " + player.GetIdentity().GetName() + " connected during non-debug state `" + GetCurrentState().GetName() + "`, scheduling disconnect.");
			ScheduleLateJoinKick( player );

            return;
        }

        // only add player if they connect during debug
        if( player.GetIdentity() )
            player.owner_id = player.GetIdentity().GetPlainId(); //cache their id (for connection loss)

        GetCurrentState().AddPlayer(player);

        if( m_ServerData.enable_vigrid_api && m_ServerData.warning_no_uuid && match_uuid == "" )
        	GetCurrentState().MessagePlayerUntranslated( player, "STR_BR_MM_ERROR_REGISTERING_MATCH");
    }

    //--- Where to drop an admin who connected mid-match: the centre of the circle currently in play,
    //--- jittered a little so two admins joining together do not land inside each other.
    //---
    //--- Returns false when no circle is in play yet - the pre-match states (Prepare, StartMatch) and
    //--- the post-match ones - and the caller falls back to the lobby, which is where everyone else
    //--- is at that point anyway.
    //---
    //--- Two casts rather than one because GetActiveZone is not on the base state: BattleRoyaleRound
    //--- has it, and BattleRoyaleLastRound is a sibling of Round (not a subclass) that exposes
    //--- GetPreviousZone instead. Note "active" is the skip-aware, currently-damaging circle, not the
    //--- one being shrunk towards - which is exactly the one an admin wants to be standing in.
    protected bool GetAdminJoinPosition(out vector position)
    {
        BattleRoyaleState state = GetCurrentState();
        if(!state)
            return false;

        BattleRoyaleZone zone;

        BattleRoyaleRound round_state;
        if(Class.CastTo(round_state, state))
            zone = round_state.GetActiveZone();

        BattleRoyaleLastRound last_round_state;
        if(!zone && Class.CastTo(last_round_state, state))
            zone = last_round_state.GetPreviousZone();

        if(!zone || !zone.GetArea())
            return false;

        vector center = zone.GetArea().GetCenter();
        float radius = zone.GetArea().GetRadius();

        //--- Jitter is capped so it stays well inside even the smallest final circle.
        float jitter = 5.0;
        if(radius < 50.0)
            jitter = radius * 0.1;

        position[0] = Math.RandomFloatInclusive((center[0] - jitter), (center[0] + jitter));
        position[2] = Math.RandomFloatInclusive((center[2] - jitter), (center[2] + jitter));
        position[1] = GetGame().SurfaceY(position[0], position[2]);

        return true;
    }

    /**
     *  Where to put an admin who needs to be somewhere useful: the live circle, or the lobby centre
     *  when no circle is in play yet.
     *
     *  The fallback used to live inline in OnPlayerConnected, which was fine while that was the only
     *  caller. AdminRespawn is a second one, and duplicating it there would mean two places that can
     *  disagree about where an admin belongs.
     */
    void GetAdminSpawnPosition(out vector position)
    {
        if(GetAdminJoinPosition( position ))
            return;

        BattleRoyaleDebug debug_state = BattleRoyaleDebug.Cast( GetState(0) );
        if(!debug_state)
            return;

        vector debug_pos = debug_state.GetCenter();

        position[0] = Math.RandomFloatInclusive((debug_pos[0] - 5), (debug_pos[0] + 5));
        position[2] = Math.RandomFloatInclusive((debug_pos[2] - 5), (debug_pos[2] + 5));
        position[1] = GetGame().SurfaceY(position[0], position[2]);
    }

    //--- Remember a uid as exempt from the late-join kick. Idempotent.
    void ExemptFromLateJoinKick(string uid)
    {
        if(uid == "")
            return;
        if(a_LateJoinExempt.Find( uid ) != -1)
            return;

        a_LateJoinExempt.Insert( uid );
    }

    /**
     *  True if this player is allowed to stay connected while holding no state (admins only).
     *
     *  Checks admins_steamid64 DIRECTLY as well as the a_LateJoinExempt list, and that is a fix
     *  rather than belt-and-braces. a_LateJoinExempt is only ever populated by OnPlayerConnected's
     *  mid-match branch, so an admin who connected during the LOBBY and later stopped holding state
     *  - by dying, or by taking the admin respawn - was not on it and got kicked by OnPlayerTick's
     *  not-in-state branch. The steamid list is the actual authority for "is an admin"; the array is
     *  just a cache of the ones we happened to notice on the way in.
     */
    bool IsLateJoinExempt(PlayerBase player)
    {
        if(!player)
            return false;

        //--- player_steamid is cached onto PlayerBase in OnPlayerConnected precisely because
        //--- PlayerIdentity can be gone by the time we want to identify someone.
        if(player.player_steamid != "" && a_LateJoinExempt.Find( player.player_steamid ) != -1)
            return true;

        BattleRoyaleGameData game_settings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(game_settings && game_settings.admins_steamid64 && player.player_steamid != "")
        {
            if(game_settings.admins_steamid64.Find( player.player_steamid ) != -1)
                return true;
        }

        PlayerIdentity identity = player.GetIdentity();
        if(!identity)
            return false;

        if(IsAdminIdentity( identity ))
            return true;

        return (a_LateJoinExempt.Find( identity.GetPlainId() ) != -1);
    }

    //--- Schedule a player who holds no state to be disconnected. Idempotent: safe to call every
    //--- tick, which is exactly what OnPlayerTick does.
    void ScheduleLateJoinKick(PlayerBase player)
    {
        if(!player || !player.GetIdentity())
            return;

        if(IsLateJoinExempt( player ))
            return;

        //--- The last two states are Win and Restart. 8_BattleRoyaleWin.KickWinner calls RemovePlayer
        //--- and DisconnectPlayer back to back, so between them a winner legitimately holds no state
        //--- and OnPlayerTick's not-in-state branch can see them - scheduling a kick there would flash
        //--- "a match is already in progress" at the player who just won it. Nothing is lost by
        //--- skipping the window: 9_BattleRoyaleRestart calls RequestExit a few seconds later anyway.
        //--- Same bound the scoring guards in OnPlayerDisconnected/OnPlayerKilled use.
        if(i_CurrentStateIndex >= m_States.Count() - 2)
            return;

        int configured = BattleRoyaleConfig.GetConfig().GetGameData().late_join_kick_seconds;
        if(configured <= 0)
            return; //--- admin opted out of the timed kick entirely

        for(int i = 0; i < a_LateJoiners.Count(); ++i)
        {
            if(a_LateJoiners[i] && a_LateJoiners[i].player == player)
                return; //--- already pending
        }

        //--- Not Math.Max: it is declared float/float, and rounding an int through a float to clamp
        //--- two ints is a narrowing conversion for no reason.
        int seconds = configured;
        if(seconds < BR_LATE_JOIN_KICK_MIN_SECONDS)
            seconds = BR_LATE_JOIN_KICK_MIN_SECONDS;

        //--- Recorded, but NOT armed: the countdown starts when their client says it is loaded in
        //--- (PlayerLoadedIn), not now. Vanilla reaches InvokeOnConnect from the ClientNew path while
        //--- the client is still loading the world, so arming here spends the grace period on a
        //--- loading screen - the player is disconnected without ever seeing why, and the disconnect
        //--- itself lands on a half-established connection, which is what the original code feared.
        BattleRoyaleLateJoiner entry = new BattleRoyaleLateJoiner();
        entry.player = player;
        entry.plain_id = player.GetIdentity().GetPlainId();
        entry.connect_ms = GetGame().GetTime();
        entry.armed = false;
        entry.warned_final = false;
        a_LateJoiners.Insert( entry );

        BattleRoyaleUtils.Info("Late joiner " + player.GetIdentity().GetName() + " (" + entry.plain_id + ") recorded, waiting for their client to load in before starting the " + seconds + "s countdown");
    }

    //--- Start the grace period for one pending late joiner and tell them why.
    protected void ArmLateJoiner(BattleRoyaleLateJoiner entry, string reason)
    {
        if(!entry || entry.armed)
            return;

        int configured = BattleRoyaleConfig.GetConfig().GetGameData().late_join_kick_seconds;
        int seconds = configured;
        if(seconds < BR_LATE_JOIN_KICK_MIN_SECONDS)
            seconds = BR_LATE_JOIN_KICK_MIN_SECONDS;

        entry.armed = true;
        entry.deadline_ms = GetGame().GetTime() + (seconds * 1000);

        BattleRoyaleUtils.Info("Late-join countdown started for " + entry.plain_id + " (" + reason + "), disconnecting in " + seconds + "s");

        //--- Sent now rather than at connect, so it lands on a player who can actually read it.
        NotifyLateJoiner( entry.player, seconds );
    }

    //--- Client -> server: "I have finished loading and I am controlling my character."
    //--- Registered in the constructor. CF dispatches by method name, so this must keep its name.
    void PlayerLoadedIn(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server || !sender)
            return;

        string sender_id = sender.GetPlainId();
        if(sender_id == "")
            return;

        //--- Tell an admin client that it is one, so it can arm the admin keys and offer the death
        //--- screen's third button. Sent HERE rather than from OnPlayerConnected because that fires
        //--- while the client is still loading the world - measured at 20 s before ClientNew alone -
        //--- and an RPC delivered into that window is simply lost. This handler exists precisely
        //--- because it is the first moment the client is provably listening.
        //---
        //--- Presentation only. Every admin RPC re-checks IsAdminIdentity server-side, so this tells
        //--- the client nothing it can act on that the server will not verify again.
        //---
        //--- THE VERDICT IS COMPUTED ON ITS OWN LINE, and that is not style. Written as
        //--- `new Param1<bool>( IsAdminIdentity( sender ) )` this threw "NULL pointer to instance"
        //--- for every client that loaded in - and because a VM exception unwinds the stack, the
        //--- late-joiner arming loop below never ran either, so the bug reached further than the one
        //--- line it was on. Same family as the measured array-read aliasing bug in this codebase:
        //--- an array read (admins_steamid64.Find) nested inside a call inside a constructor inside
        //--- another call does not evaluate reliably. Keep it flat.
        bool sender_is_admin = IsAdminIdentity( sender );

        ref Param1<bool> admin_flag = new Param1<bool>( sender_is_admin );
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetAdminFlag", admin_flag, true, sender );

        //--- And where the match currently is, for the same reason: this client missed every
        //--- broadcast sent before it connected. Without it a mid-match joiner - which in practice
        //--- means an admin, everyone else being kicked - reads the shipped default and behaves as
        //--- though the lobby were still running. Flat, like the admin verdict above.
        bool in_lobby = IsLobbyPhase();
        ref Param1<bool> lobby_flag = new Param1<bool>( in_lobby );
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetLobbyPhase", lobby_flag, true, sender );

        for(int i = 0; i < a_LateJoiners.Count(); ++i)
        {
            BattleRoyaleLateJoiner entry = a_LateJoiners[i];
            //--- Matched on the sender's own id, never on anything the client chose.
            if(entry && entry.plain_id == sender_id)
            {
                ArmLateJoiner( entry, "client reported loaded in" );
                return;
            }
        }
    }

    protected void NotifyLateJoiner(PlayerBase player, int seconds_left)
    {
        //--- MessagePlayerUntranslated does not require the player to belong to the state - it only
        //--- needs an identity to address the RPC to. Same call shape as the match-uuid warning in
        //--- OnPlayerConnected above.
        BattleRoyaleState state = GetCurrentState();
        if(state)
            state.MessagePlayerUntranslated( player, "STR_BR_LATE_JOIN_KICK", seconds_left.ToString() );
    }

    //--- Swept at 10 Hz from Update(). Walks backwards so a removal cannot move an unvisited entry
    //--- into an index that has already been passed (array.Remove is the unordered variant - it
    //--- swaps in the last element).
    protected void UpdateLateJoiners()
    {
        if(!a_LateJoiners || a_LateJoiners.Count() == 0)
            return;

        int now = GetGame().GetTime();

        for(int i = a_LateJoiners.Count() - 1; i >= 0; --i)
        {
            BattleRoyaleLateJoiner entry = a_LateJoiners[i];

            if(!entry || !entry.player || !entry.player.GetIdentity())
            {
                //--- They left on their own, which is the outcome we were after anyway.
                a_LateJoiners.Remove(i);
                continue;
            }

            //--- Insurance against a future state that adopts joiners: if somebody put this player
            //--- into the running state, they are a participant and must not be disconnected.
            if(GetCurrentState() && GetCurrentState().ContainsPlayer( entry.player ))
            {
                BattleRoyaleUtils.Info("Cancelling late-join disconnect for " + entry.plain_id + ", the current state now holds them.");
                a_LateJoiners.Remove(i);
                continue;
            }

            if(!entry.armed)
            {
                //--- Their client has not reported in. Normally PlayerLoadedIn arms this within a few
                //--- seconds of the loading screen ending; the ceiling is only here so a client that
                //--- never reports (older build, wedged load) is still removed eventually.
                if((now - entry.connect_ms) >= (BR_LATE_JOIN_READY_TIMEOUT_SECONDS * 1000))
                    ArmLateJoiner( entry, "no load-in report after " + BR_LATE_JOIN_READY_TIMEOUT_SECONDS + "s" );

                continue;
            }

            int remaining_ms = entry.deadline_ms - now;

            if(remaining_ms <= 0)
            {
                BattleRoyaleUtils.Info("Disconnecting late joiner " + entry.player.GetIdentity().GetName() + " (" + entry.plain_id + ")");
                GetGame().DisconnectPlayer( entry.player.GetIdentity() );
                a_LateJoiners.Remove(i);
                continue;
            }

            if(!entry.warned_final && remaining_ms <= (BR_LATE_JOIN_FINAL_WARN_SECONDS * 1000))
            {
                entry.warned_final = true;
                //--- Rounded up, so "1" is shown rather than "0" on the last second of the window.
                NotifyLateJoiner( entry.player, (remaining_ms + 999) / 1000 );
            }
        }
    }

    //--- Drop every record of a player who has left, so neither list grows across a session.
    protected void ForgetLateJoiner(PlayerBase player, PlayerIdentity identity)
    {
        string plain_id = "";
        if(identity)
            plain_id = identity.GetPlainId();
        if(plain_id == "" && player)
            plain_id = player.player_steamid;

        for(int i = a_LateJoiners.Count() - 1; i >= 0; --i)
        {
            if(!a_LateJoiners[i])
            {
                a_LateJoiners.Remove(i);
                continue;
            }

            if(a_LateJoiners[i].player == player)
                a_LateJoiners.Remove(i);
            else if(plain_id != "" && a_LateJoiners[i].plain_id == plain_id)
                a_LateJoiners.Remove(i);
        }

        if(plain_id != "")
        {
            int exempt_index = a_LateJoinExempt.Find( plain_id );
            if(exempt_index != -1)
                a_LateJoinExempt.Remove( exempt_index );
        }
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

                        //--- Record the attribution NOW, while we still have the real killer. The
                        //--- health zeroing below fires a second EEKilled whose source is the victim
                        //--- themself; RecordDeath is first-write-wins, so that later call cannot
                        //--- overwrite this with "environment" and break other players' chains.
                        BattleRoyaleSpectators.GetInstance().RecordDeathWithKiller( player, killer );

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
        //--- Unconditional: a late joiner is by definition not in any state, so doing this inside the
        //--- ContainsPlayer branch below would never fire for the players it exists for.
        ForgetLateJoiner(player, identity);
        //--- Cache the uid before anything can free the identity.
        string leaving_uid = "";
        if(player && player.player_steamid != "")
            leaving_uid = player.player_steamid;
        else if(identity)
            leaving_uid = identity.GetPlainId();

        if(GetCurrentState().ContainsPlayer(player))
        {
            //if we are in a round, then we need to call OnPlayerDisconnected (since it's not a state based function we must cast)
            if(i_CurrentStateIndex > 2 && i_CurrentStateIndex < m_States.Count() - 2 )
                GetCurrentState().OnPlayerDisconnected(player);

            GetCurrentState().RemovePlayer(player);
        }

        if(leaving_uid != "")
        {
            //--- Belt-and-braces only: this event is not reliable for a client that controls no
            //--- entity, which is exactly what a spectator is. Tick()'s identity sweep is the
            //--- primary detector and catches it within a second regardless.
            BattleRoyaleSpectators.GetInstance().Remove(leaving_uid);

            //--- Anyone watching the player who just left needs a new target.
            BattleRoyaleSpectators.GetInstance().RetargetAfterLoss(leaving_uid);
        }
    }

    override void OnPlayerKilled(PlayerBase killed, Object killer)
    {
        //--- The ordering below is load-bearing.
        //--- 1 must precede RemovePlayer: the ledger needs the victim's identity and party id.
        //--- 3 must follow it, or the victim is still on the roster and resolves themselves.
        //--- 4 runs last, so existing spectators re-resolve against a roster without the victim.

        //--- 1. Record the death. Cheap, and it runs whether or not spectating is enabled, so the
        //--- killer chain is already populated if an admin turns the feature on mid-match.
        BattleRoyaleSpectators.GetInstance().RecordDeath(killed, killer);

        if(GetCurrentState().ContainsPlayer(killed))
        {
            //if we are in a round, then we need to call onplayerkilled (since it's not a state based function we must cast)
            if(i_CurrentStateIndex > 2 && i_CurrentStateIndex < m_States.Count() - 2 )
                GetCurrentState().OnPlayerKilled(killed, killer);

            //remove player from the state (this would take place in on-disconnect, but some players would choose not to disconnect)
            GetCurrentState().RemovePlayer(killed);

            //--- 3. Register the victim as a spectator, now that they are off the roster.
            BattleRoyaleSpectators.GetInstance().OnDeath(killed);
        }

        //--- 4. Anyone watching the victim needs a new target.
        if(killed && killed.GetIdentity())
            BattleRoyaleSpectators.GetInstance().RetargetAfterLoss(killed.GetIdentity().GetPlainId());
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
                if(temp_disconnecting.Find(player) == -1 && player.GetIdentity())
                {
                    //--- This ensures we only disconnect this player once.
                    temp_disconnecting.Insert(player);

                    //--- Was SendLogoutTime(player, 0), which removed nobody: client-side that only
                    //--- updates an already-open logout menu, and a player who has not pressed
                    //--- Esc -> Exit has none. So a player whose position went to +/-1e6 stayed on the
                    //--- server in that state indefinitely. Unlike the late-join kick there is no
                    //--- grace period to observe here - this player is long past connection setup.
                    BattleRoyaleUtils.Warn("Player " + player.GetIdentity().GetName() + " found at invalid position " + pos + ", disconnecting.");
                    GetGame().DisconnectPlayer( player.GetIdentity() );
                }
            }
        }
        else
        {
            //--- Current state does not hold this player. Either they connected mid-match and
            //--- OnPlayerConnected already scheduled them, or they got here some other way - either
            //--- way they cannot participate, so schedule the disconnect. ScheduleLateJoinKick is
            //--- idempotent and honours the admin exemption, so calling it every tick is fine and the
            //--- temp_disconnecting bookkeeping this branch used to need is gone.
            //---
            //--- This used to call GetGame().SendLogoutTime(player, 0), which does NOTHING here:
            //--- client-side it lands in MissionGameplay.StartLogoutMenu, whose whole body is guarded
            //--- on an m_Logout that only exists once the player has opened Esc -> Exit themselves.
            int life_state = player.GetPlayerState();
            if(life_state == EPlayerStates.ALIVE)
            {
                if(player && player.GetIdentity())
                    ScheduleLateJoinKick(player);
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

    /**
     *  Are the players still gathered in the lobby?
     *
     *  True for the lobby and for the pre-match countdown, false from spawn selection onwards. The
     *  test is the same cast OnPlayerConnected uses to decide whether an arrival is a late joiner:
     *  BattleRoyaleCountReached derives from BattleRoyaleDebugState, so one cast covers both, and
     *  they are precisely the two states in which everyone is standing in the same place.
     */
    bool IsLobbyPhase()
    {
        BattleRoyaleState state = GetCurrentState();
        if(!state)
            return false;

        BattleRoyaleDebugState lobby = BattleRoyaleDebugState.Cast( state );

        return lobby != NULL;
    }

    /**
     *  Push each lobby player the list of people to hang a name over.
     *
     *  ONE PACKET PER RECIPIENT, and their own teammates are dropped from it here rather than
     *  filtered on arrival. Party composition therefore never goes on the wire, and neither do
     *  SteamID64s - the client identifies each subject by NETWORK ID, which is the only handle both
     *  sides agree on for a remote entity. GetIdentity() is not reliably populated client-side for
     *  another player, which is what made the first attempt draw nothing at all.
     *
     *  Names come from PlayerBase.player_name, so they carry the name service's corrections rather
     *  than the "Survivor" the client connected with.
     *
     *  Positions are deliberately NOT sent. Everyone in the lobby is in the same clearing and so
     *  inside every other client's network bubble, which makes the live entity both exact and
     *  per-frame - strictly better than a 1 Hz push, and free.
     */
    protected void PushLobbyNames()
    {
        if(!IsLobbyPhase())
            return;

        int now = GetGame().GetTime();
        if(now < m_NextLobbyNamesMs)
            return;

        m_NextLobbyNamesMs = now + BR_LOBBY_NAMES_PUSH_MS;

        BattleRoyaleState state = GetCurrentState();
        if(!state)
            return;

        array<PlayerBase> population = state.GetPlayers();
        if(!population)
            return;

        int count = population.Count();

        for(int i = 0; i < count; i++)
        {
            PlayerBase recipient = population.Get(i);
            if(!recipient)
                continue;

            PlayerIdentity identity = recipient.GetIdentity();
            if(!identity)
                continue;

            array<PlayerBase> teammates = new array<PlayerBase>();
#ifdef VIGRID_PARTY
            //--- A partition, so a solo recipient simply gets a list containing only themselves.
            teammates = VigridPartyAPI.GetTeammates( recipient, population );
#endif

            array<int> net_low = new array<int>();
            array<int> net_high = new array<int>();
            array<string> names = new array<string>();

            for(int j = 0; j < count; j++)
            {
                if(net_low.Count() >= BR_LOBBY_TAG_MAX_ROWS)
                    break;

                PlayerBase subject = population.Get(j);
                if(!subject)
                    continue;
                if(subject == recipient)
                    continue;
                if(!subject.IsAlive())
                    continue;
                if(teammates.Find( subject ) != -1)
                    continue;

                int subject_low;
                int subject_high;
                subject.GetNetworkID( subject_low, subject_high );

                net_low.Insert( subject_low );
                net_high.Insert( subject_high );
                names.Insert( subject.player_name );
            }

            ref Param3<array<int>, array<int>, array<string>> payload = new Param3<array<int>, array<int>, array<string>>( net_low, net_high, names );
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetLobbyNames", payload, true, identity );
        }
    }

    /**
     *  Tell every connected client where the match is.
     *
     *  Called from the two places a state is activated, so no state has to remember to do it and a
     *  new state added in the middle cannot forget. Broadcast rather than diffed: it is one bool a
     *  handful of times per match, and a client that missed one is a client showing the wrong thing
     *  until the next transition.
     */
    void BroadcastLobbyPhase()
    {
        bool in_lobby = IsLobbyPhase();
        ref Param1<bool> lobby_flag = new Param1<bool>( in_lobby );
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetLobbyPhase", lobby_flag, true );
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
    /**
     *  The dead player pressed "Spectate" instead of waiting out the timeout.
     *
     *  Carries NO payload on purpose. The actor is the engine-supplied `sender`, so there is nothing
     *  a client can put on the wire to make this act on anybody else - which is the rule the comment
     *  on GetPlayerFromIdentity in 0_BattleRoyaleState.c lays down, and the rule the abandoned
     *  VPP port broke by trusting the client-supplied `Object target`.
     */
    void RequestSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type == CallType.Server)
        {
            BattleRoyaleSpectators.GetInstance().RequestSpectate( sender );
        }
    }

    //------------------------------------------------------------------------------------------
    //--- Admin spectate. Same rule as every other admin RPC in this file: the actor is `sender`,
    //--- never anything in the payload, and authorization is re-checked server-side on every call
    //--- rather than trusted from whatever the client believes about itself.
    //------------------------------------------------------------------------------------------

    //! Enter / leave admin spectate, respawning first when the admin is dead. No payload: what the
    //! press means is derived from server state, not sent by the client.
    void AdminSpectateToggle(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        BattleRoyaleSpectators.GetInstance().AdminToggle( sender );
    }

    void AdminSpectateCycle(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1< int > data;
        if( !ctx.Read( data ) ) return;

        if(type != CallType.Server)
            return;

        //--- Normalised rather than trusted: the payload only ever means "next" or "previous", so a
        //--- client sending 10000 steps one place, not ten thousand.
        int direction = 1;
        if(data.param1 < 0)
            direction = -1;

        BattleRoyaleSpectators.GetInstance().CycleTarget( sender, direction );
    }

    void AdminSpectateMode(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1< int > data;
        if( !ctx.Read( data ) ) return;

        if(type != CallType.Server)
            return;

        //--- SetAdminMode rejects anything that is not FOLLOW or FREE, so an out-of-range value from
        //--- a crafted packet is a no-op rather than a mode nothing renders.
        BattleRoyaleSpectators.GetInstance().SetAdminMode( sender, data.param1 );
    }

    /**
     *  The free camera reporting where it is, so the server can keep the admin's body underneath it
     *  and the replication bubble with it.
     *
     *  ~2 Hz and unvalidated as to position, deliberately: the only thing a lying client can achieve
     *  is to move their OWN already-invisible body somewhere else, which is precisely what the honest
     *  path does anyway. It is refused outright for anyone who is not in an admin session.
     */
    void AdminSpectateCamPos(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1< vector > data;
        if( !ctx.Read( data ) ) return;

        if(type != CallType.Server)
            return;

        BattleRoyaleSpectators.GetInstance().SetAdminCamPos( sender, data.param1 );
    }

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

            case BattleRoyaleDiagAction.KILL_SELF:
            {
                //--- The entry point to the entire spectate feature. Without this, dying needs a
                //--- SECOND client to shoot you or a slow wait on zone damage, so every spectate test
                //--- was a three-client test - the same cost the kill feed pays.
                //---
                //--- SetHealth to zero rather than any scripted kill helper, because the ONLY path
                //--- worth exercising is vanilla's: it is EEKilled that BattleRoyaleSpectators hooks,
                //--- and a shortcut that reached RecordDeath directly would test the parts that were
                //--- never in doubt. The killer therefore resolves to the victim themselves, which is
                //--- how a zone death or a suicide already surfaces - so this reproduces the T4
                //--- (nearest living player) tier specifically. A real kill by another player is
                //--- still the only way to exercise T1/T2.
                if(!sender_player)
                {
                    BattleRoyaleUtils.Warn("[Diag] KILL_SELF: sender is not in the current state");
                    break;
                }

                if(BattleRoyaleSpectators.GetInstance().IsRegistered(sender.GetPlainId()))
                {
                    BattleRoyaleUtils.Warn("[Diag] KILL_SELF: already a spectator, ignoring");
                    break;
                }

                BattleRoyaleUtils.Info("[Diag] KILL_SELF for " + GetIdentityLogName(sender));
                sender_player.SetHealth("", "Health", 0.0);
                break;
            }

            case BattleRoyaleDiagAction.LOG_SPECTATORS:
            {
                BattleRoyaleSpectators.GetInstance().LogSpectators();
                break;
            }

            case BattleRoyaleDiagAction.SPECTATE_TP_TARGET:
            {
                //--- The range test the whole bubble question turns on. Walking a target out past
                //--- 1 km takes minutes and usually ends early - the 2026-08-10 run stopped at 929 m
                //--- because a wolf finished the target off - so this puts them at an exact radius
                //--- from the spectator's corpse in one press.
                //---
                //--- Note the sender here is the SPECTATOR, who has no body: sender_player is NULL
                //--- for them by construction (they are in no state), so this deliberately does not
                //--- use it. Everything is resolved from the spectator table instead.
                //---
                //--- READ THE TRACE AS SUSTAINED STATE, NOT AS THE NEXT SAMPLE. A teleported target
                //--- is one of the three documented causes of a transient entity=0 - it is what the
                //--- 4005d62 windows actually were - so the answer is whether entity returns to 1
                //--- and STAYS there at the new distance. Turn Trace Interval down first.
                if(!state || !state.AllowsSpectate())
                {
                    BattleRoyaleUtils.Warn("[Diag] TP Target: the current state does not allow spectating");
                    break;
                }

                vector spectator_death_pos;
                PlayerBase watched_player;
                if(!BattleRoyaleSpectators.GetInstance().GetRangeTestSubject(sender.GetPlainId(), spectator_death_pos, watched_player))
                {
                    BattleRoyaleUtils.Warn("[Diag] TP Target: " + GetIdentityLogName(sender) + " is not spectating a living target");
                    break;
                }

                float tp_radius = data.param2;
                if(tp_radius < 1)
                    tp_radius = 1;

                //--- GetIdentityLogName, not the state's GetPlayerLogName - that one is protected on
                //--- BattleRoyaleState and is not reachable from here.
                BattleRoyaleUtils.Info(string.Format("[Diag] TP Target: %1 -> %2 m from %3", GetIdentityLogName(watched_player.GetIdentity()), tp_radius, spectator_death_pos.ToString()));
                state.BR_DiagTeleportRing(watched_player, spectator_death_pos, tp_radius);
                break;
            }

            case BattleRoyaleDiagAction.SPECTATE_TP_CORPSE:
            {
                //--- THE BUBBLE PROBE, and the whole point of it is that it is a probe and not a fix.
                //---
                //--- Established 2026-08-10: UpdateSpectatorPosition does not move the replication
                //--- bubble, so a spectator sees nothing past ~1 km of where they died. The bubble is
                //--- assumed to sit on the connection's own entity, which is still the corpse - but
                //--- ASSUMED is the operative word, and three explanations have already been wrong in
                //--- this subsystem. If moving the corpse restores replication, that assumption is
                //--- confirmed and a real fix becomes worth designing; if it does not, a whole
                //--- drop-the-inventory-then-carry-the-body design is saved from being built on sand.
                //---
                //--- Why this is safe where the CARRIER was not: nothing is created. The corpse
                //--- already exists, and it moves through BR_SYNC_JUNCTURE_TELEPORT, the same path
                //--- every match-start teleport uses. Both carrier attempts died inside entity
                //--- creation - CreateObjectEx at 0x0, CreatePlayer at 0x9 - neither catchable.
                //---
                //--- It DOES drag the victim's gear along, which is exactly why this is DIAG_DEVELOPER
                //--- only and must not become the shipped behaviour without solving the loot first.
                vector corpse_death_pos;
                PlayerBase corpse_target;
                if(!BattleRoyaleSpectators.GetInstance().GetRangeTestSubject(sender.GetPlainId(), corpse_death_pos, corpse_target))
                {
                    BattleRoyaleUtils.Warn("[Diag] TP Corpse: " + GetIdentityLogName(sender) + " is not spectating a living target");
                    break;
                }

                PlayerBase own_corpse = BattleRoyaleSpectators.GetInstance().FindBodyByUid(sender.GetPlainId());
                if(!own_corpse)
                {
                    BattleRoyaleUtils.Warn("[Diag] TP Corpse: could not find a body for " + GetIdentityLogName(sender) + " - it may already have been cleaned up");
                    break;
                }

                //--- Log what we found BEFORE moving it. If the probe comes back negative, the first
                //--- question is going to be whether this was really the corpse, and "alive=0" plus a
                //--- position matching the death position is what answers it.
                BattleRoyaleUtils.Info(string.Format("[Diag] TP Corpse: body alive=%1 at %2 (died at %3) -> target at %4",
                    own_corpse.IsAlive(), own_corpse.GetPosition().ToString(), corpse_death_pos.ToString(), corpse_target.GetPosition().ToString()));

                state.BR_DiagTeleport(own_corpse, corpse_target.GetPosition());
                break;
            }

            case BattleRoyaleDiagAction.SET_SPECTATE:
            {
                //--- In memory only. BattleRoyaleConfig holds the deserialized instance, so writing
                //--- the field flips the feature for this process without touching the admin's
                //--- general_settings.json - deliberately, because Load() re-saves on next boot and a
                //--- diag toggle must not become a persisted setting.
                BattleRoyaleConfig.GetConfig().GetGameData().spectate_enabled = (data.param2 != 0);
                BattleRoyaleUtils.Info("[Diag] spectate_enabled -> " + BattleRoyaleConfig.GetConfig().GetGameData().spectate_enabled + " (this process only)");
                break;
            }

            case BattleRoyaleDiagAction.SET_ADMIN_SPECTATE:
            {
                //--- In memory only, same reasoning as SET_SPECTATE above. This one exists mainly to
                //--- reach the OFF case: admin_spectate_enabled ships ON, so the test worth having is
                //--- that turning it off makes AdminEligibility refuse.
                BattleRoyaleConfig.GetConfig().GetGameData().admin_spectate_enabled = (data.param2 != 0);
                BattleRoyaleUtils.Info("[Diag] admin_spectate_enabled -> " + BattleRoyaleConfig.GetConfig().GetGameData().admin_spectate_enabled + " (this process only)");
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
