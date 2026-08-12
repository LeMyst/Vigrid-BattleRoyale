#ifdef SERVER
class BattleRoyaleState: Timeable
{
    protected ref array<PlayerBase> m_Players;
    protected bool b_IsActive;
    protected bool b_IsPaused;
    protected bool b_IsDebug;

    //--- Set by the state machine when this state is passed over entirely. A skipped state never
    //--- activates, so anything it owns - a round's circle above all - was never in play and was
    //--- never sent to a client. Distinct from b_IsActive, which only says "not running *now*".
    protected bool b_WasSkipped;

	// Track the number of players in the game when the game starts - shared between all states
    static int i_NumStartingPlayers = 0;

    //--- Memo for GetDynamicStartingZone(). Its answer depends only on num_players plus config and
    //--- the generated circles, and both are fixed for the life of the process, so recomputing it is
    //--- pure waste. It was being called once per spawn candidate through IsSafeForTeleport, which
    //--- meant two config fetches and a zone scan several hundred times per player.
    //--- -1 means "nothing memoized yet"; a real player count is never negative.
    protected int i_DynamicZoneMemoPlayers = -1;
    protected int i_DynamicZoneMemoResult = 1;

    //static int i_StartingZone = 1; // Default zone is the biggest one

    //--- Plain game setting, unrelated to whether parties exist. It used to sit behind the party
    //--- mod's #ifdef, which meant a build without that mod could not honour it at all.
    bool hide_players_endgame = false;

    string GetName()
    {
        return "Unknown State";
    }

    void BattleRoyaleState()
    {
        m_Players = new array<PlayerBase>();

        b_IsActive = false;
        b_IsPaused = false;
        b_IsDebug = false;

        //--- Drives the player/group counter on every client's HUD. This used to be registered
        //--- only when the party mod was present, so a build without it never refreshed the
        //--- counter at all - the panel simply froze on whatever it was first told.
        AddTimer(5.0, this, "OnPlayerCountChanged", NULL, true);

        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            hide_players_endgame = m_GameSettings.hide_players_endgame;
        }
    }

    void Update(float timeslice)
    {

    }

    //state controls
    void Activate()
    {
    	BattleRoyaleUtils.Debug(string.Format("BattleRoyaleState::Activate: %1", GetName()));
        //Note: this is called AFTER players are added
        b_IsActive = true;
    }

    void Deactivate()
    {
    	BattleRoyaleUtils.Debug(string.Format("BattleRoyaleState::Deactivate: %1", GetName()));
        //Note: this is called BEFORE players are removed
        //--- stop all repeating timers
        StopTimers();

        b_IsActive = false;
    }

    /**
     *  Deactivate() on the next frame instead of right now.
     *
     *  **Call this instead of Deactivate() from inside a timer callback.**
     *
     *  Stopping a Timer removes it from vanilla's TimerQueue (`TimerBase.SetRunning` ->
     *  `m_timerQueue.Remove`, tools.c:351-375), and Deactivate() stops several at once - the state's
     *  own one-shots plus every looping timer via StopTimers(). Meanwhile `TimerQueue.Tick`
     *  (tools.c:407-420) snapshots Count() *before* its loop and then walks indices downward:
     *
     *      int count = Count();
     *      for (int i = count - 1; i >= 0; i--)
     *          Get(i).Tick(timeslice);
     *
     *  A one-shot timer has already removed itself by the time its callback runs (`TimerBase.Tick`
     *  calls SetRunning(false) at tools.c:287, before OnTimer()), so the snapshot is stale by one
     *  the moment we are entered. Each further timer the callback stops shrinks the array again,
     *  and once Count() drops below the loop's index, Get(i) returns NULL - the
     *  "NULL pointer to instance / Class: 'TimerQueue' / Function: 'Tick'" exception.
     *
     *  Deferring by one frame takes every removal out of TimerQueue.Tick. Nothing observable
     *  changes: the driver only polls IsComplete() at 10 Hz anyway (BattleRoyaleServer.c:195), so a
     *  state that signals completion a frame earlier or later transitions on the same tick.
     *
     *  Deactivate() reached from IsComplete() or from a script coroutine does NOT need this - those
     *  do not run inside TimerQueue.Tick.
     */
    void DeactivateDeferred()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "Deactivate", 0, false);
    }

    bool IsActive()
    {
        return b_IsActive;
    }

    void SetSkipped(bool skipped)
    {
        b_WasSkipped = skipped;
    }

    bool WasSkipped()
    {
        return b_WasSkipped;
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

    bool SkipState(BattleRoyaleState _previousState)  //if true, we will skip activating/deactivating this state entirely
    {
        return false;
    }

    ref array<PlayerBase> GetPlayers()
    {
        return m_Players;
    }

    void AddPlayer(PlayerBase player)
    {
    	BattleRoyaleUtils.Info(string.Format("BattleRoyaleState::AddPlayer: %1", player.GetIdentityName()));
        m_Players.Insert( player );
        OnPlayerCountChanged();
    }

    void RemovePlayer(PlayerBase player)
    {
    	BattleRoyaleUtils.Info(string.Format("BattleRoyaleState::RemovePlayer: %1", player.GetIdentityName()));

        //--- The single leaderboard recording point. Every way out of a match funnels through here -
        //--- killed, disconnected, disconnected while unconscious, force-logged out, or kicked as
        //--- the winner - and all three subclass overrides chain to this. RemoveAllPlayers()
        //--- deliberately does not, so state migration never scores.
        //---
        //--- RecordExit is safe to call unconditionally: it gates on its own ranking latch and
        //--- dedupes by SteamID64, which matters because a kill reaches this twice (once from
        //--- OnPlayerKilled, once from BattleRoyaleServer).
        BattleRoyaleLeaderboard.GetInstance().RecordExit(player);

        m_Players.RemoveItem(player);
        OnPlayerCountChanged();
    }

    ref array<PlayerBase> RemoveAllPlayers()
    {
    	BattleRoyaleUtils.Debug(string.Format("BattleRoyaleState::RemoveAllPlayers: removed %1 players", m_Players.Count()));
        ref array<PlayerBase> result_array = new array<PlayerBase>();
        result_array.InsertAll(m_Players);
        m_Players.Clear();
        OnPlayerCountChanged();
        return result_array;
    }

    bool ContainsPlayer(PlayerBase player)
    {
        return (m_Players.Find(player) >= 0);
    }

    //--- Resolve the engine-supplied RPC `sender` to a player tracked by THIS state.
    //--- Server RPC handlers must use this instead of the `Object target` they receive:
    //--- `target` is chosen by the client and can name any other player, `sender` cannot be spoofed.
    //--- Returns NULL when the identity is unusable or the sender is not part of this state,
    //--- so the membership test that ContainsPlayer used to provide is inherent here.
    PlayerBase GetPlayerFromIdentity(PlayerIdentity identity)
    {
        if(!identity)
            return NULL;

        string sender_id = identity.GetId();
        if(sender_id == "")
            return NULL;

        for(int i = 0; i < m_Players.Count(); ++i)
        {
            PlayerBase candidate = m_Players.Get(i);
            if(!candidate)
                continue;

            PlayerIdentity candidate_identity = candidate.GetIdentity();
            if(!candidate_identity)
                continue;

            if(candidate_identity.GetId() == sender_id)
                return candidate;
        }

        return NULL;
    }

	void OnPlayerTick(PlayerBase player, float timeslice)
	{
	}

	//player count changed event handler
	protected void OnPlayerCountChanged()
	{
		//BattleRoyaleUtils.Trace("OnPlayerCountChanged()");
		if(IsActive())
		{
			int nb_players, nb_groups;

			nb_players = GetPlayers().Count();

			//--- Sentinels the client decodes in BattleRoyaleHud.SetCount: -2 hides the group
			//--- panel entirely (no party system), -1 shows "???" (endgame concealment).
			nb_groups = -2;
			int groups_count = nb_players;

#ifdef VIGRID_PARTY
			groups_count = VigridPartyAPI.GetGroupCount( GetPlayers() );
			nb_groups = groups_count;

			if(nb_players < 10 && hide_players_endgame && !b_IsDebug)
				nb_groups = -1;
#endif

			//--- Placement follows groups when parties exist and raw player count otherwise.
			UpdateTopPosition( groups_count );

			//BattleRoyaleUtils.Trace(string.Format("OnPlayerCountChanged: %1 %2", nb_players, nb_groups));
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", new Param2<int, int>( nb_players, nb_groups ), true);
		}
	}

    void UpdateTopPosition( int position )
    {
		for(int i = 0; i < GetPlayers().Count(); i++)
		{
			PlayerBase player = GetPlayers()[i];
			player.SetBRPosition( position );
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetTopPosition", new Param1<int>( position ), true, player.GetIdentity() );
		}
    }

    //CreateNotification( ref StringLocaliser title, ref StringLocaliser text, string icon, int color, float time, PlayerIdentity identity ) ()
    void MessagePlayers(string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
    {
		BattleRoyaleUtils.Info(string.Format("MessagePlayers: %1", message));
        StringLocaliser text = new StringLocaliser( message );
        ExpansionNotification(DAYZBR_MSG_TITLE, text, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, time).Create();
    }

    void MessagePlayer(PlayerBase player, string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
    {
        if(player)
        {
            PlayerIdentity identity = player.GetIdentity();
            if(identity)
            {
            	BattleRoyaleUtils.Info(string.Format("MessagePlayer: %1 %2", identity.GetName(), message));
                StringLocaliser text = new StringLocaliser( message );
                ExpansionNotification(DAYZBR_MSG_TITLE, text, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, time).Create(identity);
            }
        }
    }

	/*
	 * Send a message to all players, with standard message duration
	 * @param message The message to send
	 */
    void MessagePlayersUntranslated(string message, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		MessagePlayersUntranslatedTimed(message, DAYZBR_MSG_TIME, param1, param2, param3, param4, param5);
	}

	/*
	 * Send a message to all players, with custom message duration
	 * @param message The message to send
	 * @param time The time to display the message for
	 */
	void MessagePlayersUntranslatedTimed(string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		BattleRoyaleUtils.Info(string.Format("MessagePlayersUntranslated: %1", message));
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>( message, time, param1, param2, param3, param4, param5 ), true);
	}

    /*
	 * Send a message to a specific player, with standard message duration
	 * @param player The player to send the message to
	 * @param message The message to send
	 */
	void MessagePlayerUntranslated(PlayerBase player, string message, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		MessagePlayerUntranslatedTimed(player, message, DAYZBR_MSG_TIME, param1, param2, param3, param4, param5);
	}

	/*
	 * Send a message to a specific player, with custom message duration
	 * @param player The player to send the message to
	 * @param message The message to send
	 * @param time The time to display the message for
	 */
	void MessagePlayerUntranslatedTimed(PlayerBase player, string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
	    if(player)
	    {
	    	if(player.GetIdentity())
	    	{
	    		BattleRoyaleUtils.Info(string.Format("MessagePlayerUntranslated: %1 %2", player.GetIdentity().GetName(), message));
	    		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>( message, time, param1, param2, param3, param4, param5 ), true, player.GetIdentity());
	    	}
	    }
	}

    //--- Ordered cheapest test first. Spawn searches call this hundreds of times per player, so a
    //--- candidate that is in the sea must be thrown out before anything expensive runs. The zone
    //--- test used to come first even though it is the costliest check in here.
    protected bool IsSafeForTeleport(float x, float y, float z, bool check_zone = true)
    {
        // Avoid the sea
        if(GetGame().SurfaceIsSea(x, z))
            return false;

		// Avoid the ponds
        if(GetGame().SurfaceIsPond(x, z))
            return false;

    	// Check if in zone (if needed)
        if(check_zone)
        {
            if(!BattleRoyaleZone.GetZone(GetDynamicStartingZone(i_NumStartingPlayers)).IsInZone(x, z))
                return false;
        }

		// Avoid the roads
//        if(GetGame().SurfaceRoadY(x, z) != y)
//            return false;

		// Avoid namalsk ice (and others)
        ref array<string> bad_surface_types = {
            "nam_seaice",
            "nam_lakeice_ext"
        };

        string surface_type;
        GetGame().SurfaceGetType(x, z, surface_type);
        if(bad_surface_types.Find(surface_type) != -1)
            return false;

		// Avoid objects
        vector position;
        position[0] = x;
        position[1] = y;
        position[2] = z;

        vector start = position + Vector( 0, 10, 0 );
        vector end = position;
        float radius = 2.5;

        PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
        Object m_HitObject;
        vector m_HitPosition;
        vector m_HitNormal;
        float m_HitFraction;
        bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );

        if(m_Hit)
            return false;

        // New Geometry check
        array<Object> excludedObjects = new array<Object>();
        array<Object> collidedObjects = new array<Object>();
        vector box_position;
        box_position[0] = position[0];
        box_position[1] = position[1];
        box_position[2] = position[2];
        if( GetGame().IsBoxCollidingGeometry(box_position, "0 0 0", "2 10 2", ObjIntersectFire, ObjIntersectGeom, excludedObjects, collidedObjects) )
        {
            if( collidedObjects.Count() > 0)
            {
                BattleRoyaleUtils.Trace("New IsSafeForTeleport Geometry check is true !");
#ifdef BR_TRACE_ENABLED
                Print( collidedObjects );
#endif
                for (int i = 0; i < collidedObjects.Count(); ++i)
                {
                    string objectClass = collidedObjects.Get(i).GetType();
                    BattleRoyaleUtils.Trace( "objectClass: " + objectClass );
                }

                string text = "";
                foreach (Object object: collidedObjects)
                    text += " | " + Object.GetDebugName(object);
            }

			return false;
        }

        return true;
    }

	// Maybe this should be moved to another class, maybe not
    int GetDynamicStartingZone(int num_players)
    {
		//--- Answer the memo before touching config. Same num_players always gives the same zone, so
		//--- the repeated calls from the spawn search cost nothing after the first.
		if ( num_players == i_DynamicZoneMemoPlayers )
			return i_DynamicZoneMemoResult;

		int resolved_zone = 1;  // Default to 1 if dynamic zones are not enabled

    	BattleRoyaleZoneData m_ZoneSettings = BattleRoyaleConfig.GetConfig().GetZoneData();
		BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
		if ( m_ZoneSettings.use_dynamic_zones )
		{
			// Return the first zone based on number of registered players
			int last_try_zone = 1;

			//--- Starting at zone Z plays zones Z..num_zones, i.e. (num_zones - Z + 1) of them.
			//--- Solving for min_zone_num gives Z = num_zones - min_zone_num + 1; the old test
			//--- omitted the +1 and so guaranteed one zone more than configured. Clamped so a
			//--- min_zone_num >= num_zones still means "play them all" rather than never matching.
			int floor_zone = Math.Max(1, m_GameSettings.num_zones - m_ZoneSettings.min_zone_num + 1);

			BattleRoyaleUtils.Trace("Number of players registered: " + num_players);
			for(int i_zone = 1; i_zone < m_GameSettings.num_zones; i_zone++)
			{
				BattleRoyaleUtils.Trace("Try zone: " + i_zone);
				last_try_zone = i_zone;

				//--- Resolved once: this used to be evaluated twice per iteration, once for the
				//--- trace and once for the test, and each call re-enters the zone registry.
				int zone_min_players = BattleRoyaleZone.GetZone(i_zone).GetZoneMinPlayers();
				BattleRoyaleUtils.Trace("Min player for zone: " + zone_min_players);
				if(zone_min_players < num_players)
				{
					BattleRoyaleUtils.Trace("It's a match! " + i_zone);
					break;
				}
				if(i_zone == floor_zone)
				{
					BattleRoyaleUtils.Trace("Reach the minimum! " + i_zone);
					break;
				}
				BattleRoyaleUtils.Trace("No chance we continue...");
			}

			resolved_zone = last_try_zone;
		}

		i_DynamicZoneMemoPlayers = num_players;
		i_DynamicZoneMemoResult = resolved_zone;

		return resolved_zone;
	}

	void OnPlayerDisconnected(PlayerBase player)
	{
		if(ContainsPlayer( player ))
		{
			RemovePlayer( player );
		}
		else
		{
			Error("Unknown player disconnected! Not in current state?");
		}

		BattleRoyaleServerData m_ServerData = BattleRoyaleConfig.GetConfig().GetServerData();

		if ( m_ServerData.enable_vigrid_api )
		{
			BattleRoyaleUtils.Trace("ScoreWebhook: Sending player score");
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
			ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
			scoreWebhook.Send( br_instance.match_uuid, player.player_steamid, player.GetBRPosition() );
		}
	}

	void OnPlayerKilled( PlayerBase player, Object source )
	{
		if( ContainsPlayer( player ) )
		{
			RemovePlayer( player );
		}
		else
		{
			Error("Unknown player killed! Not in current state?");
		}

		if (!player || !source)
		{
	        BattleRoyaleUtils.Trace("DEBUG: PlayerKilled() player/source does not exist");
	        return;
    	}

		if( player.GetIdentity() )
		{
			map<string, string> json_data = new map<string, string>();
			json_data.Insert( "victim", player.GetIdentity().GetPlainId() );
			json_data.Insert( "victim_position", player.GetPosition().ToString() );
			vector killer_position = "0 0 0";

			BattleRoyaleServerData m_ServerData = BattleRoyaleConfig.GetConfig().GetServerData();
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();

			if ( m_ServerData.enable_vigrid_api )
			{
				BattleRoyaleUtils.Trace("ScoreWebhook: Sending player score");
				ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
				scoreWebhook.Send( br_instance.match_uuid, player.GetIdentity().GetPlainId(), player.GetBRPosition() );
			}

			// Does the source is a carrier and the carrier a Player?
			PlayerBase playerSource = PlayerBase.Cast( EntityAI.Cast( source ).GetHierarchyParent() );
			if (!playerSource)
			{
				// If not, does the source is a Player?
				playerSource = PlayerBase.Cast( source );
			}

			if (player == source)	// deaths not caused by another object (starvation, dehydration)
			{
				// Killed by environmental causes but the the player directly
				json_data.Insert( "killer", "environment" );
			}
			else if ( source.IsInherited(Grenade_Base) || source.IsInherited(LandMineTrap) )
			{
				string killer = "";
				EnScript.GetClassVar(source, "m_ActivatorId", -1, killer);
				json_data.Insert( "killer", killer );
				json_data.Insert( "weapon", source.GetType() );
			}
			else {
				json_data.Insert( "killer_position", source.GetPosition().ToString() );

				if (playerSource)
				{
					json_data.Insert( "killer", playerSource.GetIdentity().GetPlainId() );
					GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", new Param1<int>(1), true, playerSource.GetIdentity(), playerSource);
					//--- Server-side tally for the leaderboard. The RPC above only ever told the
					//--- killer's own client, so nothing on this side could score kills before.
					playerSource.br_kills = playerSource.br_kills + 1;
					if (source.IsWeapon() || source.IsMeleeWeapon())
					{
						json_data.Insert( "weapon", source.GetType() );
						if ( !source.IsMeleeWeapon() )
						{
							json_data.Insert( "distance", vector.Distance( player.GetPosition(), playerSource.GetPosition() ).ToString() );
						}
					}
					else
					{
						json_data.Insert( "weapon", "fist" );
					}
				}
				else
				{
					//rest, Animals, Zombies
					json_data.Insert( "killer", source.GetType() );
				}
			}

			if ( m_ServerData.enable_vigrid_api )
			{
				EventWebhook eventWebhook = new EventWebhook( m_ServerData.webhook_jwt_token );
				eventWebhook.Send( br_instance.match_uuid, "player.kill", JsonFileLoader<map<string, string>>.JsonMakeData( json_data ) );
			}
		}
	}
}

// Base state for the Debug Zone.
// This handles healing / godmode / and teleporting
class BattleRoyaleDebugState: BattleRoyaleState
{
    protected vector v_Center;
    protected float f_Radius;
    protected int i_HealTickTime;
    protected ref array<string> a_AdminsList;

    void BattleRoyaleDebugState()
    {
        BattleRoyaleSpawnsData m_SpawnsSettings = BattleRoyaleConfig.GetConfig().GetSpawnsData();
        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();

        if(m_SpawnsSettings && m_GameSettings)
        {
            v_Center = m_SpawnsSettings.spawn_point;
            f_Radius = m_SpawnsSettings.radius;
            a_AdminsList = m_GameSettings.admins_steamid64;
        }
        else
        {
            Error("DEBUG SETTINGS IS NULL!");
            GetGame().RequestExit(0);  // Exit the game
        }

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

    override void Activate()
    {
        //Note: this is called AFTER players are added
        b_IsDebug = true;
        super.Activate();
    }

    override string GetName()
    {
        return "Unknown Debug State";
    }

    /*
    override void AddPlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(false); //all players in this state are god mode
            player.Heal();
        }

        super.AddPlayer(player);
    }

    //--- Index loop with a null check, not a foreach. The only caller (BattleRoyaleServer.Update)
    //--- already walks the result this way and logs "null player in RemoveAllPlayers result!", so
    //--- nulls demonstrably occur. super.RemoveAllPlayers() has already cleared m_Players by the
    //--- time this runs, so a throw here loses the whole roster mid-migration: the next state
    //--- activates empty and OnPlayerTick force-logs-out every survivor.
    override ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> players = super.RemoveAllPlayers();
        for(int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = players[i];
            if(!player)
                continue;

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
    }
    */

    //--- debug states must lock players into the debug zone & heal them
    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        super.OnPlayerTick(player, timeslice);

		// Try to do only a 2D distance between points and not 3D (to avoid the teleport bug on Takistan)
        vector player_position = player.GetPosition();
        float distance = vector.Distance( Vector( player_position[0], 0, player_position[2] ), Vector( v_Center[0], 0, v_Center[2] ) );

        if(distance > f_Radius && !CanGoOutsideLobby(player))
        {
			vector spawn_pos = "0 0 0";
			spawn_pos[0] = Math.RandomFloatInclusive((v_Center[0] - 5), (v_Center[0] + 5));
			spawn_pos[2] = Math.RandomFloatInclusive((v_Center[2] - 5), (v_Center[2] + 5));
			spawn_pos[1] = GetGame().SurfaceY(spawn_pos[0], spawn_pos[2]);

            player.SetPosition(spawn_pos);

			float dir = Math.RandomFloat(0, 360);
			vector playerDir = vector.YawToVector(dir);
			player.SetDirection( Vector(playerDir[0], 0, playerDir[1]) );
        }

        if(player.time_until_heal <= 0)
        {
            player.time_until_heal = i_HealTickTime;
            player.Heal();
        }
        player.time_until_heal -= timeslice;
    }

    bool CanGoOutsideLobby(PlayerBase player)
    {
        if(!player)
        {
            Error("Null player in CanGoOutsideLobby");
            return false;
        }
        PlayerIdentity identity = player.GetIdentity();

        if(!identity)
            return false;

        string steamid = identity.GetPlainId();
        if(steamid == "")
        {
            Error("Blank SteamId from identity!");
            return false;
        }

        return (a_AdminsList.Find(steamid) != -1);
    }

    //TODO:
    //DEPRECATED: should pull value from config
    //GOAL: make this protected
    vector GetCenter()
    {
        v_Center[1] = GetGame().SurfaceY(v_Center[0], v_Center[2]);
        return v_Center;
    }

    //DEPRECATED: should pull value from config
    //GOAL: make this protected
    float GetRadius()
    {
        return f_Radius;
    }
}
