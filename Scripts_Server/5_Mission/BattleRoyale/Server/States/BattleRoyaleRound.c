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

#ifdef SCHANAMODPARTY
    int i_MaxPartySize;
    autoptr set<string> remaining_parties
#endif

    protected ref array<ref Timer> m_MessageTimers;
    protected ref Timer m_NewZoneLockTimer;
    protected ref Timer m_RoundTimeUpTimer;

    //If this is NULL, we assume previous state is debug
    //a battle royale round represents a playing state with a play area
    void BattleRoyaleRound(ref BattleRoyaleState previous_state)
    {
        m_PreviousState = previous_state;

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
        i_RoundTimeInSeconds = 60 * m_GameSettings.round_duration_minutes;

        lock_notif_min =  m_GameSettings.zone_notification_minutes;
        lock_notif_sec =  m_GameSettings.zone_notification_seconds;

        i_DamageTickTime = m_GameSettings.zone_damage_tick_seconds;
        f_Damage = m_GameSettings.zone_damage_delta;
        i_NumZones = m_GameSettings.num_zones;

        b_ArtillerySound = m_GameSettings.artillery_sound;

        b_DoZoneDamage = m_GameSettings.enable_zone_damage;

        m_MessageTimers = new array<ref Timer>;

        b_AirdropEnabled = m_GameSettings.airdrop_enabled;
        i_AirdropIgnoreLastZones = m_GameSettings.airdrop_ignore_last_zones;

#ifdef SCHANAMODPARTY
        i_MaxPartySize = GetSchanaPartyServerSettings().GetMaxPartySize();
#endif

        Init();
    }

    void Init()
    {
        Print(GetName() + " Init!");
        b_ZoneLocked = false;
        m_Zone = new BattleRoyaleZone;

        if(GetPreviousZone())
        {
            int previous_zone_number = Math.Floor(GetPreviousZone().GetZoneNumber());
            m_Zone = m_Zone.GetZone(previous_zone_number + 1);
        } else {
            Print("No previous zone, default to 1");
            m_Zone = m_Zone.GetZone(1); // Can't add dynamic num zone stuff here
        }

        // Update zone timer
        i_RoundTimeInSeconds = m_Zone.GetZoneTimer();
        Print("Create round " + GetName());

        if( GetPreviousZone() )
            Print("- Previous zone number: " + Math.Floor(GetPreviousZone().GetZoneNumber()));

        Print("- Duration: " + i_RoundTimeInSeconds);

        //dear god i hope i really don't have to keep this, but it should work
        zone_num = m_Zone.GetZoneNumber() * 1.0; //returns 1-max (inclusive)
        float num_zones = i_NumZones * 1.0;
        Print("- Num zone: " + m_Zone.GetZoneNumber() + "/" + i_NumZones);

        int min_players = m_Zone.GetZoneMinPlayers();
        Print("- Min players: " + min_players);

        //scale zone damage so it is FULL power in the final zone, and linearly decreases as we decrease zone #
        f_Damage = f_Damage * ( zone_num / num_zones );
        Print("- Damage scale: " + f_Damage);
    }

    override string GetName()
    {
        return DAYZBR_SM_GAMEPLAY_NAME + " (" + zone_num + ")";
    }

    override void Activate()
    {
        //we just activated this round (players not yet transfered from previous state)
        int time_till_end = i_RoundTimeInSeconds * 1000;
        int time_till_lock = time_till_end * 0.75;
        int time_between_lock_and_end = time_till_end - time_till_lock;
        Print(GetName() + " Activate with a duration of " + i_RoundTimeInSeconds + " seconds with a lock at " + time_till_lock / 1000 + " seconds (so " + time_between_lock_and_end / 1000 + " seconds after lock before end) !");

        int i;
        int min;
        int sec;
        int val;

		if( m_PreviousState.i_StartingZone < zone_num )
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
            m_PreviousArea = GetPreviousZone().GetArea();

			//tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( m_PreviousArea ), true);
        }

        ref BattleRoyalePlayArea m_ThisArea = NULL;
        if(GetZone())
        {
            GetZone().OnActivate( GetPlayers() ); //hand players over to the zone (for complex zone size/position calculation)
            m_ThisArea = GetZone().GetArea();
        }

        //tell the client the next play area and play artillery sound
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param2<ref BattleRoyalePlayArea, bool>( m_ThisArea, b_ArtillerySound ), true);

        //end state event
        m_RoundTimeUpTimer = AddTimer( time_till_end / 1000.0, this, "OnRoundTimeUp", NULL, false);

        //message players saying the new zone has appeared & notify them if they're outside the play area (hopefully this won't lag the server)
        for(i = 0; i < GetPlayers().Count(); i++)
        {
            PlayerBase player = GetPlayers()[i];
            if(player)
            {
                vector playerPos = player.GetPosition();
                playerPos[1] = 0;
                vector next_pos = m_ThisArea.GetCenter();
                next_pos[1] = 0;
                float dist = vector.Distance(playerPos, next_pos);
                if(dist > m_ThisArea.GetRadius())
                {
                    MessagePlayer(player, DAYZBR_MSG_NEW_ZONE_OUTSIDE);
                }
                else if (m_Zone.GetZoneNumber() != 1)
                {
                    MessagePlayer(player, DAYZBR_MSG_NEW_ZONE_INSIDE);
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
        m_NewZoneLockTimer.Stop();
        m_RoundTimeUpTimer.Stop();
        for(int i = 0; i < m_MessageTimers.Count(); i++)
            m_MessageTimers[i].Stop();
        
        //we just deactivated this round (players not yet transfered from previous state)
        super.Deactivate();
    }

    override bool IsComplete() //return true when this state is complete & ready to transfer to the next state
    {
        if(!BATTLEROYALE_SOLO_GAME)
        {
            ref array<PlayerBase> players = GetPlayers();

            if(IsActive())
            {
#ifdef SCHANAMODPARTY
                if(i_MaxPartySize < 1 || players.Count() <= i_MaxPartySize)
                {
					if( GetGroups().Count() <= 1 )
					{
						Print(GetName() + " IsComplete (Groups)!");
						Deactivate();
					}
                }
#else
                if(players.Count() <= 1)
                {
                    Print(GetName() + " IsComplete (Players)!");
                    Deactivate();
                }
#endif
            }
        }
        return super.IsComplete();
    }

#ifdef SCHANAMODPARTY
    bool AllPlayersSameParty(set<string> players)
    {
        //BattleRoyaleUtils.Trace("AllPlayersSameParty");
        SchanaPartyManagerServer manager = GetSchanaPartyManagerServer();
        autoptr map<string, autoptr set<string>> s_parties = manager.GetParties();
        remaining_parties = new set<string>;
        foreach (auto id, auto party_ids: s_parties) {  // For each party
            foreach (string member: party_ids)  // For each member of the party
            {
                if(players.Find(member) != -1)  // If the member is alive
                {
                    remaining_parties.Insert(id);  // Insert the party inside the remaining parties

                    if (remaining_parties.Count() > 1)
                        return false;
                }
            }
        }

        //Print(remaining_parties);

        if (remaining_parties.Count() == 1)
            return true;

        return false;
    }
#endif

    override bool SkipState(BattleRoyaleState _previousState)
    {
        if( _previousState.i_StartingZone > zone_num )
        {
            Print("[State Machine] Skipping State `" + _previousState.i_StartingZone + "` > `" + zone_num + "`");
            return true;
        }

        if( BATTLEROYALE_SOLO_GAME )
            return false;

        //only one (or less) players remaining, must skip to win state
        // TODO: toggle to debug game
        if(_previousState.GetPlayers().Count() <= 1)
            return true;

#ifdef SCHANAMODPARTY
        if(_previousState.GetGroups().Count() <= 1)
            return true;
#endif

        return false;
    }

    override void OnPlayerKilled(PlayerBase player, Object source)
    {
        super.OnPlayerKilled( player, source );
    }

    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        BattleRoyaleZone current_zone = GetActiveZone();
        if(current_zone && b_DoZoneDamage)
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
                    MessagePlayer(player, DAYZBR_MSG_TAKING_DAMAGE);
                    //GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "TakeZoneDamage", new Param1<bool>(true), true, player.GetIdentity());
				    player.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_PAIN_LIGHT);
                    //TODO: determine if this last health tick will kill the player
                    player.DecreaseHealthCoef( f_Damage ); //TODO: delta this by the # of zones that have ticked (more zones = more damage)
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

    BattleRoyaleZone GetPreviousZone()
    {
        BattleRoyaleRound prev_round;
        if(Class.CastTo(prev_round, m_PreviousState))
        {
            return prev_round.GetZone();
//            if( m_Zone.GetZoneNumber() > m_PreviousState.i_StartingZone )
//                return prev_round.GetZone();
//            else
//                Print(m_Zone.GetZoneNumber() + " <= " + m_PreviousState.i_StartingZone);
        } else {
            Print("Can't cast m_PreviousState to BattleRoyaleRound!");
        }

        return NULL;
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

        //tell the client the current area is now this area
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( m_ThisArea ), true);
        //tell the client we don't know the next play area
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param2<ref BattleRoyalePlayArea, bool>( NULL, false), true);
        //tell the client how much time until the next zone appears
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>( seconds ) , true);
    }

    void OnRoundTimeUp()
    {
        Deactivate();
    }

    void SpawnAirdrop()
    {
#ifdef EXPANSIONMODMISSIONS
    	BattleRoyalePlayArea future_play_area = GetZone().GetArea();
		vector future_play_area_center = future_play_area.GetCenter();
    	float future_play_area_radius = future_play_area.GetRadius();

		for(int airdrop_try = 1; airdrop_try <= 200; airdrop_try++)
		{
			float distance = Math.RandomFloatInclusive(future_play_area_radius * 0.1, future_play_area_radius * 0.9);
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
        ref array<string> bad_surface_types = {
            "nam_seaice",
            "nam_lakeice_ext"
        };

        string surface_type;
        GetGame().SurfaceGetType(x, z, surface_type);
        if(bad_surface_types.Find(surface_type) != -1)
            return false;

        return true;
    }

    void SpawnAirdropMessage()
    {
        MessagePlayers("An Airdrop is being deployed in the area!");
    }

    // Round messaging
    //TODO: add these to dayzbrconstants and use String Replace to insert the seconds or minutes values
    void NotifyTimeTillLockSeconds(int seconds)
    {
        string message = "The current zone will lock in " + seconds.ToString() + " ";
        if(seconds > 1)
            message += "seconds";
        else
            message += "second";

        MessagePlayers(message);
    }

    void NotifyTimeTillLockMinutes(int minutes)
    {
        string message = "The current zone will lock in " + minutes.ToString() + " ";
        if(minutes > 1)
            message += "minutes";
        else
            message += "minute";

        MessagePlayers(message);
    }

    void NotifyTimeTillNewZoneSeconds(int seconds)
    {
        string message = "A new zone will appear in " + seconds.ToString() + " ";
        if(seconds > 1)
            message += "seconds";
        else
            message += "second";

        MessagePlayers(message);
    }

    void NotifyTimeTillNewZoneMinutes(int minutes)
    {
        string message = "A new zone will appear in " + minutes.ToString() + " ";
        if(minutes > 1)
            message += "minutes";
        else
            message += "minute";

        MessagePlayers(message);
    }

    void NotifyTimeTillNoZoneSeconds(int seconds)
    {
        string message = "The last zone will appear in " + seconds.ToString() + " ";
        if(seconds > 1)
            message += "seconds";
        else
            message += "second";

        MessagePlayers(message);
    }

    void NotifyTimeTillNoZoneMinutes(int minutes)
    {
        string message = "The last zone will appear in " + minutes.ToString() + " ";
        if(minutes > 1)
            message += "minutes";
        else
            message += "minute";

        MessagePlayers(message);
    }
}
