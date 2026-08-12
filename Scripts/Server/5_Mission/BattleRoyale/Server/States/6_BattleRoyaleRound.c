#ifdef SERVER
class BattleRoyaleRound: BattleRoyaleState
{
    ref BattleRoyaleState m_PreviousState;
    ref BattleRoyaleZone m_Zone;
    int i_RoundTimeInSeconds;
    bool b_ZoneLocked;
    bool b_DoZoneDamage;
    int i_DamageTickTime;
    float f_Damage;
    float zone_num;
    int i_NumZones;
    bool b_ArtillerySound;
    array<int> lock_notif_min;
    array<int> lock_notif_sec;
    bool b_AirdropEnabled;
    int i_AirdropIgnoreLastZones;

    protected ref array<ref Timer> m_MessageTimers;
    protected ref Timer m_NewZoneLockTimer;
    protected ref Timer m_RoundTimeUpTimer;

    //If this is NULL, we assume previous state is debug
    //a battle royale round represents a playing state with a play area
    void BattleRoyaleRound(BattleRoyaleState previous_state)
    {
        m_PreviousState = previous_state;

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
        BattleRoyaleZoneData m_ZoneSettings = m_Config.GetZoneData();
        i_RoundTimeInSeconds = 60 * m_GameSettings.round_duration_minutes;

        lock_notif_min = m_ZoneSettings.zone_notification_minutes;
        lock_notif_sec = m_ZoneSettings.zone_notification_seconds;

        i_DamageTickTime = m_ZoneSettings.zone_damage_tick_seconds;
        f_Damage = m_ZoneSettings.zone_damage_delta;
        i_NumZones = m_ZoneSettings.num_zones;

        b_ArtillerySound = m_GameSettings.artillery_sound;

        b_DoZoneDamage = m_ZoneSettings.enable_zone_damage;

        m_MessageTimers = new array<ref Timer>;

        b_AirdropEnabled = m_GameSettings.airdrop_enabled;
        i_AirdropIgnoreLastZones = m_GameSettings.airdrop_ignore_last_zones;

        Init();
    }

    void Init()
    {
        BattleRoyaleUtils.Trace(GetName() + " Init!");
        b_ZoneLocked = false;
        m_Zone = new BattleRoyaleZone;

        //--- Chaining, NOT the played order: use the round we were constructed against even if it
        //--- later gets skipped, or every round after a skip would collapse onto zone 1.
        BattleRoyaleRound chained_round = GetChainedPreviousRound();
        if(chained_round && chained_round.GetZone())
        {
            int previous_zone_number = Math.Floor(chained_round.GetZone().GetZoneNumber());
            m_Zone = m_Zone.GetZone(previous_zone_number + 1);
        } else {
            BattleRoyaleUtils.Trace("No previous zone, default to 1");
            m_Zone = m_Zone.GetZone(1); // Can't add dynamic num zone stuff here
        }

        // Update zone timer
        i_RoundTimeInSeconds = m_Zone.GetZoneTimer();
        BattleRoyaleUtils.Trace("Create round " + GetName());

        if( chained_round && chained_round.GetZone() )
            BattleRoyaleUtils.Trace("- Previous zone number: " + Math.Floor(chained_round.GetZone().GetZoneNumber()));

        BattleRoyaleUtils.Trace("- Duration: " + i_RoundTimeInSeconds);

        //dear god i hope i really don't have to keep this, but it should work
        zone_num = m_Zone.GetZoneNumber() * 1.0; //returns 1-max (inclusive)
        float num_zones = i_NumZones * 1.0;
        BattleRoyaleUtils.Trace("- Num zone: " + m_Zone.GetZoneNumber() + "/" + i_NumZones);

        int min_players = m_Zone.GetZoneMinPlayers();
        BattleRoyaleUtils.Trace("- Min players: " + min_players);

        //scale zone damage so it is FULL power in the final zone, and linearly decreases as we decrease zone #
        f_Damage = f_Damage * ( zone_num / num_zones );
        BattleRoyaleUtils.Trace("- Damage scale: " + f_Damage);
    }

    override string GetName()
    {
        return "Gameplay State (" + zone_num + ")";
    }

    override bool AllowsSpectate()
    {
        return true;
    }

    override void Activate()
    {
        //we just activated this round (players not yet transfered from previous state)
        int time_till_end = i_RoundTimeInSeconds * 1000;
        int time_till_lock = time_till_end * 0.80; // Time before lock, changed from 0.75 to 0.80
        int time_between_lock_and_end = time_till_end - time_till_lock;
        BattleRoyaleUtils.Trace(GetName() + " Activate with a duration of " + i_RoundTimeInSeconds + " seconds with a lock at " + time_till_lock / 1000 + " seconds (so " + time_between_lock_and_end / 1000 + " seconds after lock before end) !");

        int i;
        int min;
        int sec;
        int val;

		if( GetDynamicStartingZone(i_NumStartingPlayers) < zone_num )
		{
			//--- notification message timers
			for(i = 0; i < lock_notif_min.Count();i++)
			{
				min = lock_notif_min[i];
				val = time_till_lock - (min*60*1000);
				if(val > 0)
					m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeTillLockMinutes", new Param1<int>( min ), false) );
			}

			for(i = 0; i < lock_notif_sec.Count();i++)
			{
				sec = lock_notif_sec[i];
				val = time_till_lock - (sec*1000);
				if(val > 0)
					m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeTillLockSeconds", new Param1<int>( sec ), false) );
			}
        }

        //timer before zone locks
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>( time_till_lock / 1000 ), true);

        //lock zone event
        m_NewZoneLockTimer = AddTimer(time_till_lock / 1000.0, this, "LockNewZone", new Param1<int>( time_between_lock_and_end / 1000 ), false);

        if (m_Zone.GetZoneNumber() < i_NumZones)  // Not the last zone
        {
            //--- notification message timers
            for(i = 0; i < lock_notif_min.Count();i++)
            {
                min = lock_notif_min[i];
                val = time_till_end - (min*60*1000);
                if(val > 0)
                    m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeTillNewZoneMinutes", new Param1<int>( min ), false) );
            }

            for(i = 0; i < lock_notif_sec.Count();i++)
            {
                sec = lock_notif_sec[i];
                val = time_till_end - (sec*1000);
                if(val > 0)
                    m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeTillNewZoneSeconds", new Param1<int>( sec ), false) );
            }
        } else {  // The last zone, no new zone
            //--- notification message timers
            for(i = 0; i < lock_notif_min.Count();i++)
            {
                min = lock_notif_min[i];
                val = time_till_end - (min*60*1000);
                if(val > 0)
                    m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeTillNoZoneMinutes", new Param1<int>( min ), false) );
            }

            for(i = 0; i < lock_notif_sec.Count();i++)
            {
                sec = lock_notif_sec[i];
                val = time_till_end - (sec*1000);
                if(val > 0)
                    m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeTillNoZoneSeconds", new Param1<int>( sec ), false) );
            }

			//send play area to clients
			ref BattleRoyalePlayArea m_PreviousArea = NULL;
			if(GetPreviousZone())
				m_PreviousArea = GetPreviousZone().GetArea();

			//tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
			//--- Nothing to re-send when no round has played yet: the clients were never given a
			//--- current area, and the circle of a skipped round must not be advertised as one.
			if(m_PreviousArea)
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param2<vector, float>( m_PreviousArea.GetCenter(), m_PreviousArea.GetRadius() ), true);
        }

        ref BattleRoyalePlayArea m_ThisArea = NULL;
        if(GetZone())
        {
            GetZone().OnActivate( GetPlayers() ); //hand players over to the zone (for complex zone size/position calculation)
            m_ThisArea = GetZone().GetArea();
        }

        //tell the client the next play area and play artillery sound
        //--- Guarded for the same reason the m_PreviousArea send above is: GetZone() may hand back
        //--- nothing, and this used to dereference it unconditionally.
        if(m_ThisArea)
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param3<vector, float, bool>( m_ThisArea.GetCenter(), m_ThisArea.GetRadius(), b_ArtillerySound ), true);

        //end state event
        m_RoundTimeUpTimer = AddTimer( time_till_end / 1000.0, this, "OnRoundTimeUp", NULL, false);

        //message players saying the new zone has appeared & notify them if they're outside the play area (hopefully this won't lag the server)
        for(i = 0; i < GetPlayers().Count(); i++)
        {
            PlayerBase player = GetPlayers()[i];
            if(player && m_ThisArea)
            {
                vector playerPos = player.GetPosition();
                playerPos[1] = 0;
                vector next_pos = m_ThisArea.GetCenter();
                next_pos[1] = 0;
                float dist = vector.Distance(playerPos, next_pos);
                if(dist > m_ThisArea.GetRadius())
                {
                    MessagePlayerUntranslated(player, "STR_BR_NEW_ZONE_OUTSIDE");
                }
                else if (m_Zone.GetZoneNumber() != 1)
                {
                    MessagePlayerUntranslated(player, "STR_BR_NEW_ZONE_INSIDE");
                }
            }
        }

        // Spawn airdrop
        if( b_AirdropEnabled && m_Zone.GetZoneNumber() <= (i_NumZones - i_AirdropIgnoreLastZones) ) // Don't have airdrop in the last X zones
        	SpawnAirdrop();

        super.Activate();
    }

    override void Deactivate()
    {
        if ( m_NewZoneLockTimer && m_NewZoneLockTimer.IsRunning() )
            m_NewZoneLockTimer.Stop();

        if ( m_RoundTimeUpTimer && m_RoundTimeUpTimer.IsRunning() )
            m_RoundTimeUpTimer.Stop();

        for(int i = 0; i < m_MessageTimers.Count(); i++)
        {
            if ( m_MessageTimers[i] && m_MessageTimers[i].IsRunning() )
                m_MessageTimers[i].Stop();
        }
        
        //we just deactivated this round (players not yet transfered from previous state)
        super.Deactivate();
    }

	override bool IsComplete() //return true when this state is complete & ready to transfer to the next state
	{
		if(!IsActive())
			return super.IsComplete();

		//--- The round ends when a single group is left standing, not a single player: a
		//--- surviving party has already won. Without the party addon every player is their own
		//--- group, so the group test simply mirrors the player test.
		bool playersLessThanTwo = GetPlayers().Count() <= 1;
		bool groupsLessThanTwo = playersLessThanTwo;
#ifdef VIGRID_PARTY
		groupsLessThanTwo = VigridPartyAPI.GetGroupCount( GetPlayers() ) <= 1;
#endif

		if(playersLessThanTwo || groupsLessThanTwo)
		{
			string reason;
			if(playersLessThanTwo && groupsLessThanTwo)
				reason = "(Players/Groups)";
			else if(playersLessThanTwo)
				reason = "(Players)";
			else
				reason = "(Groups)";

			BattleRoyaleUtils.Trace(GetName() + " IsComplete " + reason + "!");
			Deactivate();
		}

		return super.IsComplete();
	}

    override bool SkipState(BattleRoyaleState _previousState)
    {
        //only one (or less) players remaining, must skip to win state
        // TODO: toggle to debug game
        if(_previousState.GetPlayers().Count() <= 1)
            return true;

#ifdef VIGRID_PARTY
        if(VigridPartyAPI.GetGroupCount( _previousState.GetPlayers() ) <= 1)
            return true;
#endif

        if( GetDynamicStartingZone(i_NumStartingPlayers) > zone_num )
        {
            BattleRoyaleUtils.Trace("[State Machine] Skipping State `" + GetDynamicStartingZone(i_NumStartingPlayers) + "` > `" + zone_num + "`");
            return true;
        }

        return false;
    }

    override void OnPlayerKilled(PlayerBase player, Object source)
    {
        super.OnPlayerKilled( player, source );
    }

    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        BattleRoyaleZone current_zone = GetActiveZone();

        //--- A zero-radius area is the placeholder circle at the world origin that a zone falls back
        //--- to when generation could not produce a real one. Without this guard `distance >= radius`
        //--- is true for every player on the map, so the entire lobby takes zone damage from the
        //--- first round onward - which is the loudest symptom a broken generation ever had.
        bool zone_is_real = false;
        if(current_zone && current_zone.GetArea())
            zone_is_real = (current_zone.GetArea().GetRadius() > 0);

        if(zone_is_real && b_DoZoneDamage)
        {
            float radius = current_zone.GetArea().GetRadius();
            vector center = current_zone.GetArea().GetCenter();

            vector playerPos = player.GetPosition();

            //distance needs to be done in 2D, not 3D, set Z coord to 0 so this can be done
            playerPos[1] = 0;
            center[1] = 0;
            float distance = vector.Distance(playerPos, center);
            if(distance >= radius)
            {
                if(player.time_until_damage <= 0)
                {
                    //DAMAGE
                	MessagePlayerUntranslated(player, "STR_BR_TAKING_DAMAGE");
				    player.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_PAIN_LIGHT);
                    //TODO: determine if this last health tick will kill the player
                    player.DecreaseHealthCoef( f_Damage ); //TODO: delta this by the # of zones that have ticked (more zones = more damage)
#ifdef KILLFEED
                    //--- Scripted damage reaches EEKilled with the player as their own killer, so
                    //--- without this the kill feed cannot tell a zone death from starvation.
                    KillFeedAPI.NoteEnvironmentalDamage( player, KillFeedCause.ZONE );
#endif
                    player.time_until_damage = i_DamageTickTime; //reset timer
                }
                player.time_until_damage -= timeslice;
            }
            else
            {
                //if the player leaves the zone damage area, their damage ticktime will slowly incremement until it reaches the max value (5)
                //this way you can't just keep jumping in and out of the zone to edgeplay
                player.time_until_damage = Math.Min(i_DamageTickTime, player.time_until_damage + timeslice);
            }
        }

        // Unlimited stamina
        //player.GetStatStamina().Set(CfgGameplayHandler.GetStaminaMax());

        super.OnPlayerTick(player, timeslice);
    }

    //handle zoning stuff
    BattleRoyaleZone GetZone()
    {
        return m_Zone;
    }

    //--- The round this one was chained to when the state list was built. NOT skip-aware, and must
    //--- not become so: chaining happens at construction, before any state can be skipped, and it
    //--- is what tells this round which zone number it owns.
    BattleRoyaleRound GetChainedPreviousRound()
    {
        BattleRoyaleRound prev_round;
        if(Class.CastTo(prev_round, m_PreviousState))
            return prev_round;

        //--- NULL here is the NORMAL case for the first round that runs: its previous state is
        //--- BattleRoyaleStartMatch, not a round. This used to log a Trace, which was reached once
        //--- per player per tick through GetActiveZone() -> GetPreviousZone(); on a DIAG server
        //--- BattleRoyaleUtils.Trace also mirrors every line into in-game chat over an RPC, so it
        //--- was a per-tick broadcast that visibly lagged the server. The NULL contract is
        //--- documented on GetPreviousZone() below and every caller already handles it.
        return NULL;
    }

    //--- The circle that was actually in play before this round, or NULL when this is the first
    //--- round that ran. With dynamic starting zones the rounds ahead of the starting one are
    //--- skipped, yet they were still constructed with a fully generated zone - so the cast used to
    //--- succeed and hand back a circle that never activated and was never sent to any client.
    BattleRoyaleZone GetPreviousZone()
    {
        BattleRoyaleRound prev_round = GetChainedPreviousRound();
        if(!prev_round)
            return NULL;

        if(prev_round.WasSkipped())
            return NULL;

        return prev_round.GetZone();
    }

    BattleRoyaleZone GetActiveZone() //returns NULL if first zone & not locked
    {
        if(b_ZoneLocked)
            return GetZone();
        else
            return GetPreviousZone();
    }

    bool IsLocked()
    {
        return b_ZoneLocked;
    }

    void LockNewZone(int seconds)
    {
        b_ZoneLocked = true;

        //send play area to clients
        ref BattleRoyalePlayArea m_PreviousArea = NULL;
        if(GetPreviousZone())
            m_PreviousArea = GetPreviousZone().GetArea();

        ref BattleRoyalePlayArea m_ThisArea = NULL;
        if(GetZone())
            m_ThisArea = GetZone().GetArea();

        //--- Both sends dereference m_ThisArea, which the block above is allowed to leave NULL.
        if(m_ThisArea)
        {
            //tell the client the current area is now this area
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param2<vector, float>( m_ThisArea.GetCenter(), m_ThisArea.GetRadius() ), true);
            //tell the client we don't know the next play area
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param3<vector, float, bool>( m_ThisArea.GetCenter(), m_ThisArea.GetRadius(), b_ArtillerySound ), true);
        }
        //tell the client how much time until the next zone appears
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>( seconds ) , true);
    }

    void OnRoundTimeUp()
    {
        //--- Deferred: this is a timer callback, i.e. inside TimerQueue.Tick. See
        //--- BattleRoyaleState.DeactivateDeferred().
        DeactivateDeferred();
    }

    void SpawnAirdrop()
    {
#ifdef EXPANSIONMODMISSIONS
    	BattleRoyalePlayArea future_play_area = GetZone().GetArea();
		vector future_play_area_center = future_play_area.GetCenter();
    	float future_play_area_radius = future_play_area.GetRadius();

		for(int airdrop_try = 1; airdrop_try <= 200; airdrop_try++)
		{
			float distance = Math.RandomFloatInclusive(future_play_area_radius * 0.4, future_play_area_radius * 0.8);
			float moveDir = Math.RandomFloat(0, 360) * Math.DEG2RAD;

			float dX = distance * Math.Sin(moveDir);
			float dZ = distance * Math.Cos(moveDir);

			vector airdrop_position;
			airdrop_position[0] = future_play_area_center[0] + dX;
			airdrop_position[2] = future_play_area_center[2] + dZ;

			if( IsSafeForAirdrop( airdrop_position[0], airdrop_position[2] ) )
				break;
		}
		airdrop_position[1] = GetGame().SurfaceY( airdrop_position[0], airdrop_position[2] );
    	ExpansionMissionModule.s_Instance.CallAirdrop( airdrop_position );
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName( this, "SpawnAirdropMessage", 15000, false );
#endif
    }

    protected bool IsSafeForAirdrop(float x, float z)
    {
        // Avoid the sea
        if(GetGame().SurfaceIsSea(x, z))
            return false;

		// Avoid the ponds
        if(GetGame().SurfaceIsPond(x, z))
            return false;

		// Avoid namalsk ice (and others)
        //--- The list used to be hardcoded here. It now comes from zone_settings.avoid_surface_types,
        //--- shared with the zone generator so an admin adding a bad surface for their map fixes both
        //--- at once rather than only one of them.
        if(BattleRoyaleZone.IsBadSurfaceType(x, z))
            return false;

        return true;
    }

	void SpawnAirdropMessage()
	{
		MessagePlayersUntranslated("STR_BR_AIR_DEPLOYED");
	}

	// Round messaging
	void NotifyTimeTillLockSeconds(int seconds)
	{
		if(seconds > 1)
			MessagePlayersUntranslated("STR_BR_CURRENT_ZONE_WILL_LOCK_IN_SECONDS", seconds.ToString());
		else
			MessagePlayersUntranslated("STR_BR_CURRENT_ZONE_WILL_LOCK_IN_SECOND", seconds.ToString());
	}

	void NotifyTimeTillLockMinutes(int minutes)
	{
		if(minutes > 1)
			MessagePlayersUntranslated("STR_BR_CURRENT_ZONE_WILL_LOCK_IN_MINUTES", minutes.ToString());
		else
			MessagePlayersUntranslated("STR_BR_CURRENT_ZONE_WILL_LOCK_IN_MINUTE", minutes.ToString());
	}

	void NotifyTimeTillNewZoneSeconds(int seconds)
	{
		if(seconds > 1)
			MessagePlayersUntranslated("STR_BR_A_NEW_ZONE_WILL_APPEAR_IN_SECONDS", seconds.ToString());
		else
			MessagePlayersUntranslated("STR_BR_A_NEW_ZONE_WILL_APPEAR_IN_SECOND", seconds.ToString());
	}

	void NotifyTimeTillNewZoneMinutes(int minutes)
	{
		if(minutes > 1)
			MessagePlayersUntranslated("STR_BR_A_NEW_ZONE_WILL_APPEAR_IN_MINUTES", minutes.ToString());
		else
			MessagePlayersUntranslated("STR_BR_A_NEW_ZONE_WILL_APPEAR_IN_MINUTE", minutes.ToString());
	}

	void NotifyTimeTillNoZoneSeconds(int seconds)
	{
		if(seconds > 1)
			MessagePlayersUntranslated("STR_BR_THE_LAST_ZONE_WILL_APPEAR_IN_SECONDS", seconds.ToString());
		else
			MessagePlayersUntranslated("STR_BR_THE_LAST_ZONE_WILL_APPEAR_IN_SECOND", seconds.ToString());
	}

	void NotifyTimeTillNoZoneMinutes(int minutes)
	{
		if(minutes > 1)
			MessagePlayersUntranslated("STR_BR_THE_LAST_ZONE_WILL_APPEAR_IN_MINUTES", minutes.ToString());
		else
			MessagePlayersUntranslated("STR_BR_THE_LAST_ZONE_WILL_APPEAR_IN_MINUTE", minutes.ToString());
	}
}
