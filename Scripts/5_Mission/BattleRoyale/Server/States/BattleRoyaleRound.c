class BattleRoyaleRound extends BattleRoyaleState 
{
	ref BattleRoyaleState m_PreviousSate; 
	ref BattleRoyaleZone m_Zone;
	int i_RoundTimeInSeconds; 
	bool b_IsFirstRound;
	bool b_ZoneLocked;
	bool b_DoZoneDamage;
	int i_DamageTickTime;
	float f_Damage;
	int i_NumZones;
	array<int> lock_notif_min;
	array<int> lock_notif_sec;

	//If this is NULL, we assume previous state is debug 
	//a battle royale round represents a playing state with a play area
	/*
		
	
	*/
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

		b_DoZoneDamage = m_GameSettings.enable_zone_damage;

		Init();
	}
	
	
	void Init()
	{
		b_IsFirstRound = false;
		b_ZoneLocked = false;

		m_Zone = new BattleRoyaleZone(GetPreviousZone());

		//dear god i hope i really don't have to keep this, but it should work
		float zone_num = m_Zone.GetZoneNumber() * 1.0; //returns 1-max (inclusive)
		float num_zones = i_NumZones * 1.0;

		//scale zone damage so it is FULL power in the final zone, and linearly decreases as we decrease zone # 
		f_Damage = f_Damage * ( zone_num / num_zones );
	}
	override string GetName()
	{
		return DAYZBR_SM_GAMEPLAY_NAME;
	}
	override void Activate()
	{
		//we just activated this round (players not yet transfered from previous state)
		int time_till_end = i_RoundTimeInSeconds * 1000;
		int time_till_lock = time_till_end / 2;

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
				m_CallQueue.CallLater(this.NotifyTimeTillLockMinutes, val, false, min);
		}
		for(i = 0; i < lock_notif_sec.Count();i++)
		{
			sec = lock_notif_sec[i];
			val = time_till_lock - (sec*1000);
			if(val > 0)
				m_CallQueue.CallLater(this.NotifyTimeTillLockSeconds, val, false, sec); //30 seconds until zone locks
		}


		//timer before zone locks
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>((i_RoundTimeInSeconds)/2), true); 

		//lock zone event
		m_CallQueue.CallLater(this.LockNewZone, time_till_lock); //set timer to lock new zone after X seconds


		//--- notification message timers
		for(i = 0; i < lock_notif_min.Count();i++)
		{
			min = lock_notif_min[i];
			val = time_till_end - (min*60*1000);
			if(val > 0)
				m_CallQueue.CallLater(this.NotifyTimeTillNewZoneMinutes, val, false, min);
		}
		for(i = 0; i < lock_notif_sec.Count();i++)
		{
			sec = lock_notif_sec[i];
			val = time_till_end - (sec*1000);
			if(val > 0)
				m_CallQueue.CallLater(this.NotifyTimeTillNewZoneSeconds, val, false, sec); //30 seconds until zone locks
		}

		//end state event
		m_CallQueue.CallLater(this.OnRoundTimeUp, time_till_end); //set timer to end round after X seconds


		//send play area to clients
		ref BattleRoyalePlayArea m_PreviousArea = NULL;
		if(GetPreviousZone())
			m_PreviousArea = GetPreviousZone().GetArea();

		ref BattleRoyalePlayArea m_ThisArea = NULL;
		if(GetZone())
		{
			GetZone().OnActivate( GetPlayers() ); //hand players over to the zone (for complex zone size/position calculation)
			m_ThisArea = GetZone().GetArea();
		}
		//tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( m_PreviousArea ), true);
		//tell the client the next play area
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param1<ref BattleRoyalePlayArea>( m_ThisArea ), true);

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
				else
				{
					MessagePlayer(player, DAYZBR_MSG_NEW_ZONE_INSIDE);
				}
			}
		}

		super.Activate();
	}
	override void Deactivate()
	{
		//we just deactivated this round (players not yet transfered from previous state)
		super.Deactivate();
	}
	
	override bool IsComplete() //return true when this state is complete & ready to transfer to the next state
	{

		//when Player count <= 1 -> automatically clean up anything in the CallQueue (removing it) and complete this state
		if(GetPlayers().Count() <= 1 && IsActive())
		{
			//clean call queue?
			m_CallQueue.Remove(this.OnRoundTimeUp);
			m_CallQueue.Remove(this.NotifyTimeTillNewZoneSeconds);
			m_CallQueue.Remove(this.NotifyTimeTillNewZoneMinutes);
			m_CallQueue.Remove(this.LockNewZone);
			m_CallQueue.Remove(this.NotifyTimeTillLockSeconds);
			m_CallQueue.Remove(this.NotifyTimeTillLockMinutes);
			Deactivate();
		}
		return super.IsComplete();
		
	}
	override bool SkipState(BattleRoyaleState m_PreviousState) 
	{
		//only one (or less) players remaining, must skip to win state
		if(m_PreviousState.GetPlayers().Count() <= 1)
			return true;
		
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
					ref array<string> killed_with = new array<string>();
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
							//TODO: figure out why this error is firing, it shouldn't 
							/*if(!match_data.ContainsDeath(player_steamid))
							{
								killed_with.Insert( "Bugged Killer" );
								match_data.CreateDeath( player_steamid, player_position, time, "", killed_with, Vector(0,0,0) );
							}
							return;*/
						}

						GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", new Param1<int>(1), true, pbKiller.GetIdentity(),pbKiller);
					
						if(!match_data.ContainsDeath(player_steamid))
						{
							if(!is_vehicle)
							{
								EntityAI itemInHands = pbKiller.GetHumanInventory().GetEntityInHands();
								killed_with.Insert( itemInHands.GetType() );
								//TODO: insert all attachments associated with the item in hands
							}
							else
							{
								killed_with.Insert( killer_entity.GetType() ); //vehicle kill
							}
							
							match_data.CreateDeath( player_steamid, player_position, time, "", killed_with, killer_position );
						}
						
						
					}
					else
					{
						//killer is an entity, but not a player
						if(!match_data.ContainsDeath(player_steamid))
						{
							killed_with.Insert( killer_entity.GetType() );
							match_data.CreateDeath( player_steamid, player_position, time, "", killed_with, killer_position );
						}
					}
					
				}
				else
				{
					//Unhandled killer case (not an entity?)
					if(!match_data.ContainsDeath(player_steamid))
					{
						match_data.CreateDeath( player_steamid, player_position, time, "", new array<string>(), killer_position );
					}
				}
				
			}
			else
			{
				//null killer
				if(!match_data.ContainsDeath(player_steamid))
				{
					match_data.CreateDeath( player_steamid, player_position, time, "", new array<string>(), Vector(0,0,0) );
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
							ref array<string> killed_with = new array<string>();
							killed_with.Insert( "Zone Damage" );
							match_data.CreateDeath( player_steamid, player_position, time, "", killed_with, Vector(0,0,0) );
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
		else
		{
			b_IsFirstRound = true;
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
	

	void LockNewZone()
	{
		b_ZoneLocked = true;

		//send play area to clients
		ref BattleRoyalePlayArea m_PreviousArea = NULL;
		if(GetPreviousZone())
			m_PreviousArea = GetPreviousZone().GetArea();

		ref BattleRoyalePlayArea m_ThisArea = NULL;
		if(GetZone())
			m_ThisArea = GetZone().GetArea();
			
		BattleRoyaleServer.Cast( GetBR() ).GetMatchData().CreateZone(m_ThisArea.GetCenter(), m_ThisArea.GetRadius(), GetGame().GetTime());
			
		//tell the client the current area is now this area
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( m_ThisArea ), true);
		//tell the client we don't know the next play area
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param1<ref BattleRoyalePlayArea>( NULL ), true);
		//tell the client how much time until the next zone appears
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>((i_RoundTimeInSeconds)/2), true); 

	}
	void OnRoundTimeUp()
	{
		Deactivate();
	}



	// Round messaging
	//TODO: add these to dayzbrconstants and use String Replace to insert the seconds or minutes values
	void NotifyTimeTillLockSeconds(int seconds)
	{
		string message = "The new zone will lock in " + seconds.ToString() + " ";
		if(seconds > 1)
			message += "seconds";
		else
			message += "second";
		
		MessagePlayers(message);
	}
	void NotifyTimeTillLockMinutes(int minutes)
	{
		string message = "The new zone will lock in " + minutes.ToString() + " ";
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
}