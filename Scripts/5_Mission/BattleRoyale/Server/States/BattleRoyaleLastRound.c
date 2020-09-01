class BattleRoyaleLastRound extends BattleRoyaleState 
{
    ref BattleRoyaleState m_PreviousSate; 
    
    int i_RoundTimeInSeconds;
	bool b_DoZoneDamage;
	int i_DamageTickTime;
	float f_Damage;

	array<int> lock_notif_min;
	array<int> lock_notif_sec;

    bool b_IsZoneLocked;


    void BattleRoyaleLastRound(ref BattleRoyaleState previous_state)
	{
        m_PreviousSate = previous_state;

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
		BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
		i_RoundTimeInSeconds = 60 * m_GameSettings.round_duration_minutes;
		
		lock_notif_min =  m_GameSettings.zone_notification_minutes;
		lock_notif_sec =  m_GameSettings.zone_notification_seconds;

		i_DamageTickTime = m_GameSettings.zone_damage_tick_seconds;
		f_Damage = m_GameSettings.zone_damage_delta;
		b_DoZoneDamage = m_GameSettings.enable_zone_damage;
        b_IsZoneLocked = false;
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
				m_CallQueue.CallLater(this.NotifyTimeToEndMinutes, val, false, min);
		}
		for(i = 0; i < lock_notif_sec.Count();i++)
		{
			sec = lock_notif_sec[i];
			val = time_till_lock - (sec*1000);
			if(val > 0)
				m_CallQueue.CallLater(this.NotifyTimeToEndSeconds, val, false, sec); //30 seconds until zone locks
		}


		//timer before 
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>((i_RoundTimeInSeconds)/2), true); 

		//lock zone event
		m_CallQueue.CallLater(this.LockFinalZone, time_till_lock); //set timer to lock new zone after X seconds


        //send play area to clients
		ref BattleRoyalePlayArea m_PreviousArea = NULL;
		if(GetPreviousZone())
			m_PreviousArea = GetPreviousZone().GetArea();
        
		
		//tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( m_PreviousArea ), true);
		//tell the client the future zone is NULL (no future zone)
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param1<ref BattleRoyalePlayArea>( NULL ), true);

        super.Activate();
    }
    override void Deactivate()
	{
		//we just deactivated this round (players not yet transfered from previous state)
		super.Deactivate();
	}
    override bool SkipState(BattleRoyaleState m_PreviousState) 
	{
		//only one (or less) players remaining, must skip to win state
		if(m_PreviousState.GetPlayers().Count() <= 1)
			return true;
		
		return false;
	}
	override string GetName()
	{
		return DAYZBR_SM_LAST_ROUND_NAME;
	}
    override bool IsComplete() //return true when this state is complete & ready to transfer to the next state
	{
		if(GetPlayers().Count() <= 1 && IsActive())
		{
			Deactivate();
		}
		return super.IsComplete();
	}

    void OnPlayerKilled(PlayerBase player, Object killer)
	{
		if(ContainsPlayer(player))
		{
			RemovePlayer(player);
		}
		
		if(killer)
		{
			EntityAI killer_entity;
			if(Class.CastTo(killer_entity, killer))
			{
				PlayerBase pbKiller;
				if(!Class.CastTo(pbKiller, killer_entity))
				{
					Man root_player = killer_entity.GetHierarchyRootPlayer();
					if(root_player)
					{
						pbKiller = PlayerBase.Cast( root_player );
					}
				}

				if(pbKiller && pbKiller.GetIdentity())
				{
					if(ContainsPlayer(pbKiller))
					{
						//RPC client to add kill count
						GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", new Param1<int>(1), true, pbKiller.GetIdentity(),pbKiller);
					}
					else
					{
						Error("Killer does not exist in the current game state!");
					}
					
				}
			}
		}
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

    void LockFinalZone()
    {
		//TODO: this doesn't fucking work | if zone is null, no damage occurs
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( NULL ), true);
        b_IsZoneLocked = true;
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

	//TODO: both of these need added to battleroyaleconstants & use string replace to append minutes.ToString()
    void NotifyTimeToEndMinutes(int minutes)
	{
		string message = "The zone will disappear in " + minutes.ToString() + " ";
		if(minutes > 1)
			message += "minutes";
		else
			message += "minute";
		
		MessagePlayers(message);
	}
    void NotifyTimeToEndSeconds(int seconds)
	{
		string message = "The zone will disappear in " + seconds.ToString() + " ";
		if(seconds > 1)
			message += "seconds";
		else
			message += "second";
		
		MessagePlayers(message);
	}
}