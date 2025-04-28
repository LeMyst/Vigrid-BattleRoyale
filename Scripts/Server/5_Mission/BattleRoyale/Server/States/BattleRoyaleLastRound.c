#ifdef SERVER
class BattleRoyaleLastRound: BattleRoyaleState
{
    ref BattleRoyaleState m_PreviousState;

    int i_RoundTimeInSeconds;
    bool b_DoZoneDamage;
    int i_DamageTickTime;
    float f_Damage;

    array<int> lock_notif_min;
    array<int> lock_notif_sec;

    bool b_IsZoneLocked;

    protected ref Timer m_FinalZoneLockTimer;
    protected ref array<ref Timer> m_MessageTimers;

    void BattleRoyaleLastRound(BattleRoyaleState previous_state)
    {
        m_PreviousState = previous_state;

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();

        lock_notif_min =  m_GameSettings.zone_notification_minutes;
        lock_notif_sec =  m_GameSettings.zone_notification_seconds;

        i_DamageTickTime = m_GameSettings.zone_damage_tick_seconds;
        f_Damage = m_GameSettings.zone_damage_delta;
        b_DoZoneDamage = m_GameSettings.enable_zone_damage;
        b_IsZoneLocked = false;

        m_MessageTimers = new array<ref Timer>;

        BattleRoyaleZoneData m_ZoneSettings = m_Config.GetZoneData();
        if (m_ZoneSettings.shrink_type == 3)
        {
            i_RoundTimeInSeconds = m_ZoneSettings.static_timers[0];
        } else {
            i_RoundTimeInSeconds = 60 * m_GameSettings.round_duration_minutes;
        }
    }

    override void Activate()
    {
        int time_till_lock = (i_RoundTimeInSeconds * 1000) / 2;

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
                m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeToEndMinutes", new Param1<int>( min ), false) ); //we need to store the object in case it's automatically deconstructed ?
        }
        for(i = 0; i < lock_notif_sec.Count();i++)
        {
            sec = lock_notif_sec[i];
            val = time_till_lock - (sec*1000);
            if(val > 0)
                m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeToEndSeconds", new Param1<int>( sec ), false) ); //we need to store the object in case it's automatically deconstructed ?
        }

        //timer before
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>((i_RoundTimeInSeconds)/2), true);

        //lock zone event
        m_FinalZoneLockTimer = AddTimer( time_till_lock / 1000.0, this, "LockFinalZone", NULL, false);

        //send play area to clients
        ref BattleRoyalePlayArea m_PreviousArea = NULL;
        if(GetPreviousZone())
            m_PreviousArea = GetPreviousZone().GetArea();

        //tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param2<vector, float>( m_PreviousArea.GetCenter(), m_PreviousArea.GetRadius() ), true);

        //tell the client the future zone is NULL (no future zone)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param3<vector, float, bool>( "0 0 0", 0.0, false ), true);

        super.Activate();
    }

    override void Deactivate()
    {
        if ( m_FinalZoneLockTimer && m_FinalZoneLockTimer.IsRunning() )
        {
            m_FinalZoneLockTimer.Stop();
        }

        for(int i = 0; i < m_MessageTimers.Count(); i++)
        {
            if ( m_MessageTimers[i] && m_MessageTimers[i].IsRunning() )
            {
                m_MessageTimers[i].Stop();
            }
        }

        //we just deactivated this round (players not yet transfered from previous state)
        super.Deactivate();
    }

    override bool SkipState(BattleRoyaleState _previousState)
    {
        //only one (or less) players remaining, must skip to win state
        if(_previousState.GetPlayers().Count() > 1)
            return false;

#ifdef SCHANAMODPARTY
        if(_previousState.GetGroups().Count() > 1)
            return false;
#endif

        return true;
    }

    override string GetName()
    {
        return DAYZBR_SM_LAST_ROUND_NAME;
    }

    override bool IsComplete() //return true when this state is complete & ready to transfer to the next state
    {
        if(IsActive())
        {
            if(GetPlayers().Count() < 2)
                Deactivate();

#ifdef SCHANAMODPARTY
            if(GetGroups().Count() < 2)
                Deactivate();
#endif
        }

        return super.IsComplete();
    }

    override void OnPlayerKilled(PlayerBase player, Object source)
    {
        super.OnPlayerKilled( player, source );
    }

    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        //determine if the player needs to take damage
        bool do_damage = b_IsZoneLocked;
        if(!do_damage)
        {
            BattleRoyaleZone current_zone = GetPreviousZone();
            if(b_DoZoneDamage)
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
                    do_damage = true;
                }
            }
        }

        if(do_damage)
        {
            if(player.time_until_damage <= 0)
            {
                //DAMAGE
                MessagePlayerUntranslated(player, "STR_BR_TAKING_DAMAGE");
                player.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_PAIN_HEAVY);
                player.DecreaseHealthCoef( f_Damage );
                player.time_until_damage = i_DamageTickTime; //reset timer
            }
            player.time_until_damage -= timeslice;
        }
        else
        {
            player.time_until_damage = Math.Min(i_DamageTickTime, player.time_until_damage + timeslice);
        }

        super.OnPlayerTick(player, timeslice);
    }

    bool IsLocked()
    {
        return b_IsZoneLocked;
    }

    void LockFinalZone()
    {
        //TODO: this doesn't fucking work | if zone is null, no damage occurs
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param2<vector, float>( "0 0 0", 0.0 ), true);
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param3<vector, float, bool>( "0 0 0", 0.0, false ), true);
        b_IsZoneLocked = true;
    }

    BattleRoyaleZone GetPreviousZone()
    {
        BattleRoyaleRound prev_round;
        if(Class.CastTo(prev_round, m_PreviousState))
        {
            return prev_round.GetZone();
        }

        return NULL;
    }

    //TODO: both of these need added to battleroyaleconstants & use string replace to append minutes.ToString()
    void NotifyTimeToEndMinutes(int minutes)
    {
		MessagePlayersUntranslated("STR_BR_ZONE_WILL_DISAPPEAR_MINUTE", minutes.ToString());
    }

    void NotifyTimeToEndSeconds(int seconds)
    {
		MessagePlayersUntranslated("STR_BR_ZONE_WILL_DISAPPEAR_SECOND", seconds.ToString());
    }
}
