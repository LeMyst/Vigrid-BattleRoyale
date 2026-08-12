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

    /**
     *  May a player who dies while THIS state is running enter spectate?
     *
     *  Default no. The lobby, countdown, spawn selection and prepare phases have no match to watch,
     *  and the win/restart states are past the point where a new spectator makes sense - a death in
     *  any of them keeps the pre-existing behaviour exactly.
     *
     *  A dedicated predicate rather than reusing BattleRoyaleServer's
     *  `i_CurrentStateIndex > 2 && i_CurrentStateIndex < m_States.Count() - 2` range test, which
     *  shifts by one depending on whether enable_spawn_selection_menu inserted state 3.
     */
    bool AllowsSpectate()
    {
        return false;
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

    //--- Identity-safe name for logging. A PlayerBase can outlive its PlayerIdentity across a
    //--- coroutine yield or a deferred call, and a Trace argument is evaluated whatever the log
    //--- level - so an inlined GetIdentity().GetName() throws from inside a disabled log call and
    //--- takes the caller with it. Lives here rather than in one state because both the Prepare
    //--- coroutine and the unstuck path below need it.
    protected string GetPlayerLogName(PlayerBase player)
    {
        if(!player)
            return "<null player>";

        PlayerIdentity identity = player.GetIdentity();
        if(!identity)
            return "<null identity>";

        return BattleRoyaleNameService.ResolveIdentity(identity);
    }

    //--- Whether this state answers an F2 unstuck request at all. False by default, so Prepare, the
    //--- rounds and Win keep refusing exactly as they always have; only the lobby and the warm-up
    //--- opt in. The client sends the RPC unconditionally (BattleRoyaleClient.Unstuck), so this is
    //--- the only gate.
    bool AllowsUnstuck()
    {
        return false;
    }

    //--- Where an unstuck request may drop this player. Ring search outward from where they stand,
    //--- which is what keeps the teleport short enough to read as "shoved free" rather than as a
    //--- relocation. check_zone is off because an unstuck must work wherever the player currently
    //--- is - refusing to free somebody because they are already outside the circle is backwards.
    protected bool FindUnstuckPosition(PlayerBase player, out vector position)
    {
        position = "0 0 0";

        if(!player)
            return false;

        vector player_position = player.GetPosition();

        for(int search_pos = 0; search_pos < 50; search_pos++)
        {
            float radius = 10.0 + (search_pos * 0.5);
            float angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
            float x = player_position[0] + ( radius * Math.Cos(angle) );
            float z = player_position[2] + ( radius * Math.Sin(angle) );
            float y = GetGame().SurfaceY(x, z);

            if( !IsSafeForTeleport(x, y, z, false) )
                continue;

            position = Vector(x, y, z);
            return true;
        }

        return false;
    }

    //--- Cooldown gate, so F2 cannot be held down as free fast-travel. wait_unstuck alone is not
    //--- enough: it is cleared the moment the teleport lands, which was fine while unstuck only
    //--- existed during the warm-up but is not once the lobby - where players idle for minutes -
    //--- can answer it too.
    void DeferredUnstuck( PlayerBase player )
    {
        //--- Reached from an RPC handler, so the subject may already be gone.
        if( !player )
            return;

        if( player.wait_unstuck )
        {
            MessagePlayerUntranslated( player, "STR_BR_ALREADY_REQUESTED_UNSTUCK" );
            return;
        }

        if( GetGame().GetTickTime() < player.next_unstuck_time )
        {
            MessagePlayerUntranslated( player, "STR_BR_ALREADY_REQUESTED_UNSTUCK" );
            return;
        }

        //--- Note the cooldown is NOT started here. A request that finds nowhere to land refuses and
        //--- leaves the player exactly as stuck as they were, so charging them 30 s for it would
        //--- punish the one case where retrying immediately is the right thing to do. Unstuck()
        //--- starts the clock once a teleport has actually been sent.
        player.wait_unstuck = true;

        MessagePlayerUntranslated( player, "STR_BR_UNSTUCK_TELEPORTATION" );
        BattleRoyaleUtils.Trace( GetPlayerLogName( player ) + " asked for an unstuck teleportation." );
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "Unstuck", Math.RandomFloat(1, 3) * 1000 , false, new Param1<PlayerBase>( player ));
    }

    void Unstuck( PlayerBase player )
    {
        //--- 1-3 seconds have passed since DeferredUnstuck queued this, and a player can disconnect
        //--- inside that window. The failure branch below used to dereference GetIdentity() with no
        //--- guard on either, which was survivable while only the warm-up could reach it.
        if( !player )
            return;

        vector unstuck_position;
        if( !FindUnstuckPosition( player, unstuck_position ) )
        {
            BattleRoyaleUtils.Warn( GetPlayerLogName( player ) + " unstuck failed at " + player.GetPosition() );
            player.wait_unstuck = false;
            return;
        }

#ifdef DIAG_DEVELOPER
        //--- The one sample the juncture cannot give: what the command state was while the player
        //--- was still AT the ladder. If it reads Ladder here and not at "juncture", the command was
        //--- replaced underneath the animation, which is the case HumanCommandLadder.Exit exists for.
        player.BR_LogTeleportState("pre-unstuck");
#endif

        vector playerDir = vector.YawToVector( Math.RandomFloat(0, 360) );
        vector direction = Vector(playerDir[0], 0, playerDir[1]);

        ScriptJunctureData pCtx = new ScriptJunctureData;
        pCtx.Write( unstuck_position );
        pCtx.Write( direction );
        player.SendSyncJuncture( BR_SYNC_JUNCTURE_TELEPORT, pCtx );
        player.SetSynchDirty();
        player.wait_unstuck = false;

        //--- Charged only for a teleport that was actually sent - see the note in DeferredUnstuck.
        player.next_unstuck_time = GetGame().GetTickTime() + BR_UNSTUCK_COOLDOWN_SECONDS;
    }

#ifdef DIAG_DEVELOPER
    /**
     *  Diag only: put `player` at `position` down the same sync juncture the unstuck path uses.
     *
     *  Deliberately NOT a SetPosition. The juncture is what carries the teleport to the client half
     *  as well, and it is the code path the mod's own teleport bugs live in - a debug teleport that
     *  went around it would be testing something nobody ships.
     *
     *  The height is left alone: the juncture's server half applies BR_TELEPORT_DROP_HEIGHT itself,
     *  and adding it here as well would stack two seating epsilons.
     */
    void BR_DiagTeleport( PlayerBase player, vector position )
    {
        if( !player )
            return;

        position[1] = GetGame().SurfaceY( position[0], position[2] );

        vector playerDir = vector.YawToVector( Math.RandomFloat(0, 360) );
        vector direction = Vector(playerDir[0], 0, playerDir[1]);

        ScriptJunctureData pCtx = new ScriptJunctureData;
        pCtx.Write( position );
        pCtx.Write( direction );
        player.SendSyncJuncture( BR_SYNC_JUNCTURE_TELEPORT, pCtx );
        player.SetSynchDirty();

        BattleRoyaleUtils.Info( "[Diag] teleported " + GetPlayerLogName( player ) + " to " + position );
    }

    /**
     *  Put `player` exactly `radius` metres from `origin`, for the spectator range test.
     *
     *  The bearing is not the caller's business - only the distance is - so this walks a ring of
     *  candidate bearings and takes the first that survives IsSafeForTeleport, which already knows
     *  about sea, ponds and bad surfaces. Starting from the player's CURRENT bearing relative to
     *  origin keeps the teleport as short as it can be, so the camera's jump is the distance change
     *  and not a jump across the map as well.
     *
     *  Two passes. The first keeps check_zone, because a target dumped outside the circle starts
     *  taking zone damage immediately and a range test wants the target alive for the 20-30 s the
     *  measurement needs. The second drops it, since at 1200 m from a corpse there frequently IS no
     *  in-circle answer - that pass warns, because a target quietly bleeding out changes what the
     *  trace means.
     */
    bool BR_DiagTeleportRing( PlayerBase player, vector origin, float radius )
    {
        if( !player )
            return false;

        vector offset = player.GetPosition() - origin;
        offset[1] = 0;

        float start_yaw = 0;
        if( offset.LengthSq() > 1.0 )
            start_yaw = offset.VectorToAngles()[0];

        int step = 0;
        int pass = 0;
        //--- 24 bearings, 15 degrees apart. Enough to find a gap around a lake without making this a
        //--- search: the spawn code's hundreds-of-candidates approach is not warranted for a button.
        for( pass = 0; pass < 2; pass++ )
        {
            bool check_zone = ( pass == 0 );

            for( step = 0; step < 24; step++ )
            {
                float yaw = start_yaw + ( step * 15.0 );
                vector dir = vector.YawToVector( yaw );
                vector candidate = origin + ( Vector( dir[0], 0, dir[1] ) * radius );
                candidate[1] = GetGame().SurfaceY( candidate[0], candidate[2] );

                if( !IsSafeForTeleport( candidate[0], candidate[1], candidate[2], check_zone ) )
                    continue;

                if( !check_zone )
                    BattleRoyaleUtils.Warn( "[Diag] TP Target: no in-circle spot at " + radius + " m, using one OUTSIDE the zone - the target will take zone damage" );

                BR_DiagTeleport( player, candidate );
                return true;
            }
        }

        BattleRoyaleUtils.Warn( "[Diag] TP Target: no safe spot at " + radius + " m from " + origin + " on any of 24 bearings" );
        return false;
    }
#endif

	// Maybe this should be moved to another class, maybe not
    int GetDynamicStartingZone(int num_players)
    {
		//--- Answer the memo before touching config. Same num_players always gives the same zone, so
		//--- the repeated calls from the spawn search cost nothing after the first.
		if ( num_players == i_DynamicZoneMemoPlayers )
			return i_DynamicZoneMemoResult;

		int resolved_zone = 1;  // Default to 1 if dynamic zones are not enabled

    	BattleRoyaleZoneData m_ZoneSettings = BattleRoyaleConfig.GetConfig().GetZoneData();
		if ( m_ZoneSettings.use_dynamic_zones )
		{
			// Return the first zone based on number of registered players
			int last_try_zone = 1;

			//--- Starting at zone Z plays zones Z..num_zones, i.e. (num_zones - Z + 1) of them.
			//--- Solving for min_zone_num gives Z = num_zones - min_zone_num + 1; the old test
			//--- omitted the +1 and so guaranteed one zone more than configured. Clamped so a
			//--- min_zone_num >= num_zones still means "play them all" rather than never matching.
			int floor_zone = Math.Max(1, m_ZoneSettings.num_zones - m_ZoneSettings.min_zone_num + 1);

			BattleRoyaleUtils.Trace("Number of players registered: " + num_players);
			for(int i_zone = 1; i_zone < m_ZoneSettings.num_zones; i_zone++)
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
        BattleRoyaleLobbyData m_LobbySettings = BattleRoyaleConfig.GetConfig().GetLobbyData();

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

        if(m_LobbySettings)
        {
            i_HealTickTime = m_LobbySettings.debug_heal_tick_seconds;
        }
        else
        {
            Error("LOBBY SETTINGS IS NULL");
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

    //--- Same ring search as the base state, but no candidate outside the lobby disc is allowed, and
    //--- there is always an answer.
    //---
    //--- Two reasons the clamp is here rather than left to OnPlayerTick's containment above. It
    //--- would catch an escapee anyway, but only by yanking them back to the centre a frame later -
    //--- a visible double teleport. More importantly it exempts admins (CanGoOutsideLobby), so for
    //--- an admin nothing would catch it at all and an unstuck could quietly drop them into the
    //--- world mid-lobby. Making "cannot leave the lobby" a property of this method covers both.
    //---
    //--- 2D distance, matching OnPlayerTick and for the same reason its comment gives.
    override protected bool FindUnstuckPosition(PlayerBase player, out vector position)
    {
        position = "0 0 0";

        if(!player)
            return false;

        vector player_position = player.GetPosition();

        for(int search_pos = 0; search_pos < 50; search_pos++)
        {
            float radius = 10.0 + (search_pos * 0.5);
            float angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
            float x = player_position[0] + ( radius * Math.Cos(angle) );
            float z = player_position[2] + ( radius * Math.Sin(angle) );
            float y = GetGame().SurfaceY(x, z);

            //--- The ring grows to 34.5 m, so on a small lobby most of it lies outside. Reject
            //--- before the expensive geometry test in IsSafeForTeleport, not after.
            if( vector.Distance( Vector( x, 0, z ), Vector( v_Center[0], 0, v_Center[2] ) ) > f_Radius )
                continue;

            if( !IsSafeForTeleport(x, y, z, false) )
                continue;

            position = Vector(x, y, z);
            return true;
        }

        //--- Nothing safe inside the disc. Fall back to the placement OnPlayerTick already uses to
        //--- put an escapee back, so a lobby unstuck never fails: being shoved to the middle of the
        //--- lobby is the correct outcome for a player who cannot otherwise move.
        //---
        //--- Vetted first, though, which it never used to be. This is where a lobby unstuck almost
        //--- always lands - the ring search above only tends to succeed on a wide open lobby - and
        //--- the position was taken raw from SurfaceY with no geometry test of any kind. A centre
        //--- that happens to sit under a prop seats the capsule inside it, and the player is freed
        //--- from being stuck into being stuck. That is the fault BR_TELEPORT_DROP_HEIGHT spent a
        //--- metre of clearance hiding.
        float centre_x = v_Center[0];
        float centre_z = v_Center[2];
        float centre_y = GetGame().SurfaceY(centre_x, centre_z);

        for(int centre_try = 0; centre_try < 10; centre_try++)
        {
            centre_x = Math.RandomFloatInclusive((v_Center[0] - 5), (v_Center[0] + 5));
            centre_z = Math.RandomFloatInclusive((v_Center[2] - 5), (v_Center[2] + 5));
            centre_y = GetGame().SurfaceY(centre_x, centre_z);

            if( !IsSafeForTeleport(centre_x, centre_y, centre_z, false) )
                continue;

            BattleRoyaleUtils.Trace("No safe unstuck spot inside the lobby, falling back to a vetted position near the centre.");

            position = Vector(centre_x, centre_y, centre_z);
            return true;
        }

        //--- Even the centre is obstructed. Land there regardless: refusing outright would leave a
        //--- genuinely stuck player with no way out at all, and that is the worse of the two.
        BattleRoyaleUtils.Warn("No safe unstuck spot anywhere in the lobby, including around the centre. Using the last centre candidate unvetted.");

        position = Vector(centre_x, centre_y, centre_z);
        return true;
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
