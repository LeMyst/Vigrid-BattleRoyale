class BattleRoyaleRound extends BattleRoyaleState
{
    ref BattleRoyaleState m_PreviousSate;
    ref BattleRoyaleZone m_Zone;
    int i_RoundTimeInSeconds;
    bool b_ZoneLocked;
    bool b_DoZoneDamage;
    int i_DamageTickTime;
    float f_Damage;
    int i_NumZones;
    bool b_ArtillerySound;
    array<int> lock_notif_min;
    array<int> lock_notif_sec;

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
        m_PreviousSate = previous_state;

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
            m_Zone = m_Zone.GetZone(1);
        }

        // Update zone timer
        i_RoundTimeInSeconds = m_Zone.GetZoneTimer();
        Print("Create round " + GetName());
        Print("- Duration : " + i_RoundTimeInSeconds);

        //dear god i hope i really don't have to keep this, but it should work
        float zone_num = m_Zone.GetZoneNumber() * 1.0; //returns 1-max (inclusive)
        float num_zones = i_NumZones * 1.0;
        Print("- Num zone : " + m_Zone.GetZoneNumber() + "/" + i_NumZones);

        //scale zone damage so it is FULL power in the final zone, and linearly decreases as we decrease zone #
        f_Damage = f_Damage * ( zone_num / num_zones );
        Print("- Damage scale : " + f_Damage);
    }

    override string GetName()
    {
        return DAYZBR_SM_GAMEPLAY_NAME;
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
        }

        //end state event
        m_RoundTimeUpTimer = AddTimer( time_till_end / 1000.0, this, "OnRoundTimeUp", NULL, false);

        //send play area to clients
        ref BattleRoyalePlayArea m_PreviousArea = NULL;
        if(GetPreviousZone())
            m_PreviousArea = GetPreviousZone().GetArea();

        ref BattleRoyalePlayArea m_ThisArea = NULL;
        if(GetZone())
        {
            GetZone().OnActivate( GetPlayers() ); //hand players over to the zone (for complex zone size/position calculation)
            m_ThisArea = GetZone().GetArea();

            BattleRoyaleServer.Cast( GetBR() ).GetMatchData().ShowZone(m_ThisArea.GetCenter(), m_ThisArea.GetRadius(), GetGame().GetTime());
        }

        //tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( m_PreviousArea ), true);

        //tell the client the next play area and play artillery sound
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param2<ref BattleRoyalePlayArea, bool>( m_ThisArea, b_ArtillerySound ), true);

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
                    set<string> players_id = new set<string>;
                    foreach(PlayerBase player : players)
                    {
                        if(player && player.GetIdentity())
                            players_id.Insert(player.GetIdentity().GetId());
                    }
                    if(players.Count() <= 1 || AllPlayersSameParty(players_id))
                    {
                        Print(GetName() + " IsComplete!");
                        Deactivate();
                    }
                }
#else
                if(players.Count() <= 1)
                {
                    Print(GetName() + " IsComplete!");
                    // TODO: toggle to debug game
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
        foreach (auto id, auto party_ids : s_parties) {  // For each party
            foreach (string member : party_ids)  // For each member of the party
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

    override bool SkipState(BattleRoyaleState m_PreviousState)
    {
        if(BATTLEROYALE_SOLO_GAME)
            return false;

        //only one (or less) players remaining, must skip to win state
        // TODO: toggle to debug game
        if(m_PreviousState.GetPlayers().Count() <= 1)
            return true;

#ifdef SCHANAMODPARTY
        if(m_PreviousState.GetGroups().Count() <= 1)
            return true;
#endif

        return false;
    }

    void OnPlayerKilled(PlayerBase player, Object killer)
    {
        ref MatchData match_data = BattleRoyaleServer.Cast( GetBR() ).GetMatchData();

        if(ContainsPlayer(player))
        {
            RemovePlayer(player);
        }
        else
        {
            Error("Unknown player killed! Not in current state?");
        }

        if(player.GetIdentity())
        {
            string player_steamid = player.GetIdentity().GetPlainId();
            vector player_position = player.GetPosition();
            int time = GetGame().GetTime(); //MS since mission start (we'll calculate UNIX timestamp on the webserver)

            if(killer)
            {
                EntityAI killer_entity;
                if(Class.CastTo(killer_entity, killer))
                {
                    string killed_with = "";
                    vector killer_position = killer_entity.GetPosition();

                    bool is_vehicle = false;

                    PlayerBase pbKiller;
                    if(!Class.CastTo(pbKiller, killer_entity))
                    {
                        Man root_player = killer_entity.GetHierarchyRootPlayer();
                        if(root_player)
                        {
                            pbKiller = PlayerBase.Cast( root_player );
                            is_vehicle = true;
                        }
                    }

                    if(pbKiller && pbKiller.GetIdentity())
                    {
                        if(!ContainsPlayer( pbKiller ))
                        {
                            Error("Killer does not exist in the current game state!");
                        }

                        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", new Param1<int>(1), true, pbKiller.GetIdentity(),pbKiller);

                        if(!match_data.ContainsDeath(player_steamid))
                        {
                            if(!is_vehicle)
                            {
                                EntityAI itemInHands = pbKiller.GetHumanInventory().GetEntityInHands();
                                killed_with = itemInHands.GetType();
                            }
                            else
                            {
                                killed_with = killer_entity.GetType();
                            }

                            string killer_steamid = "";
                            if(pbKiller.GetIdentity())
                            {
                                killer_steamid = pbKiller.GetIdentity().GetPlainId();
                            }

                            match_data.CreateDeath( player_steamid, player_position, time, killer_steamid, killed_with, killer_position );
                        }
                    }
                    else
                    {
                        //killer is an entity, but not a player (we have a few subsystems for figuring out the killer id)
                        if(!match_data.ContainsDeath(player_steamid))
                        {
                            killed_with = killer_entity.GetType();

                            Grenade_Base grenade;
                            if(Class.CastTo( grenade, killer_entity ))
                            {
                                match_data.CreateDeath( player_steamid, player_position, time, grenade.GetDZBRActivator(), killed_with, killer_position );
                                return;
                            }

                            match_data.CreateDeath( player_steamid, player_position, time, "", killed_with, killer_position );
                        }
                    }
                }
                else
                {
                    //Unhandled killer case (not an entity?)
                    if(!match_data.ContainsDeath(player_steamid))
                    {
                        match_data.CreateDeath( player_steamid, player_position, time, "", "Environment", killer_position );
                    }
                }
            }
            else
            {
                //null killer
                if(!match_data.ContainsDeath(player_steamid))
                {
                    match_data.CreateDeath( player_steamid, player_position, time, "", "Environment", Vector(0,0,0) );
                }
            }
        }
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
                    bool b_WillKillThisTick = false;
                    if(b_WillKillThisTick)
                    {
                        string player_steamid = player.GetIdentity().GetPlainId();
                        vector player_position = player.GetPosition();
                        int time = GetGame().GetTime();
                        ref MatchData match_data = BattleRoyaleServer.Cast( GetBR() ).GetMatchData();
                        //--- this ensures the leaderboard logs this player's death as zone damage
                        if(!match_data.ContainsDeath(player_steamid))
                        {
                            match_data.CreateDeath( player_steamid, player_position, time, "", "Zone", Vector(0,0,0) );
                        }
                    }
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

        /*if(player.GetIdentity())
        {
            if(player.time_until_move <= 0)
            {
                //send movement update
                string steamid = player.GetIdentity().GetPlainId();
                vector dirvector = player.GetDirection();
                dirvector[1] = 0;
                dirvector = dirvector.Normalized(); //renormalize
                float angle_rads = Math.Atan2(dirvector[0], dirvector[2]);
                //clamp range (-pi, pi]
                if (angle_rads > Math.PI)
                {
                    angle_rads -= 2 * Math.PI;
                }
                else if (angle_rads <= -Math.PI)
                {
                    angle_rads += 2 * Math.PI;
                }
                float angle = angle_rads * Math.RAD2DEG;

                BattleRoyaleServer.Cast( GetBR() ).GetMatchData().Movement(steamid, player.GetPosition(), angle, GetGame().GetTime() );

                player.time_until_move = 5;
            }
            else
            {
                player.time_until_move -= timeslice;
            }
        }*/

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
        if(Class.CastTo(prev_round, m_PreviousSate))
        {
            return prev_round.GetZone();
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

        BattleRoyaleServer.Cast( GetBR() ).GetMatchData().LockZone(m_ThisArea.GetCenter(), m_ThisArea.GetRadius(), GetGame().GetTime());

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
        string message = "The last zone will disappear in " + seconds.ToString() + " ";
        if(seconds > 1)
            message += "seconds";
        else
            message += "second";

        MessagePlayers(message);
    }

    void NotifyTimeTillNoZoneMinutes(int minutes)
    {
        string message = "The last zone will disappear in " + minutes.ToString() + " ";
        if(minutes > 1)
            message += "minutes";
        else
            message += "minute";

        MessagePlayers(message);
    }
}
