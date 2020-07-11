#define BR_BETA_LOGGING

class BattleRoyaleRound extends BattleRoyaleState 
{
	ref BattleRoyaleState m_PreviousSate; 
	ref BattleRoyaleZone m_Zone;
	int i_RoundTimeInSeconds; 
	bool b_TimeUp;
	bool b_IsFirstRound;
	bool b_ZoneLocked;

	array<int> lock_notif_min;
	array<int> lock_notif_sec;

	//If this is NULL, we assume previous state is debug 
	//a battle royale round represents a playing state with a play area
	/*
		
	
	*/
	void BattleRoyaleRound(ref BattleRoyaleState previous_state)
	{
		m_PreviousSate = previous_state;

		BattleRoyaleConfig m_Config = GetBRConfig();
		BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
		i_RoundTimeInSeconds = 60 * m_GameSettings.round_duration_minutes;
		
		lock_notif_min =  m_GameSettings.zone_notification_minutes;
		lock_notif_sec =  m_GameSettings.zone_notification_seconds;

		Init();
	}
	
	
	void Init()
	{
		b_IsFirstRound = false;
		b_TimeUp = false;
		b_ZoneLocked = false;

		m_Zone = new BattleRoyaleZone(GetPreviousZone());
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


		//TODO: send clients the current & previous zone data so they can render (both render here)


		super.Activate();
	}
	override void Deactivate()
	{
		//we just deactivated this round (players not yet transfered from previous state)
		super.Deactivate();
	}
	
	override bool IsComplete() //return true when this state is complete & ready to transfer to the next state
	{
		return b_TimeUp;
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
		
	}
	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		BattleRoyaleZone current_zone = GetActiveZone();
		if(current_zone)
		{
			float radius = current_zone.GetRadius();
			vector center = current_zone.GetCenter();

			vector playerPos = player.GetPosition();
			//TODO: distance needs to be done in 2D, not 3D
			float distance = vector.Distance(playerPos, center); 
			if(distance >= radius)
			{
				//TODO: player outside zone, damage them
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

		//TODO: tell clients to hide previous zone data (if necessary)


	}
	void OnRoundTimeUp()
	{
		b_TimeUp = true;
	}



	//TODO: Round messaging
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