class BattleRoyaleDebug
{
	ref array<PlayerBase> m_DebugPlayers;
	
	ref BattleRoyale br_game;
	
	ref ScriptCallQueue debug_CallQueue;
	
	void BattleRoyaleDebug(BattleRoyale game)
	{
		br_game = game;
		m_DebugPlayers = new array<PlayerBase>();
		debug_CallQueue = new ScriptCallQueue();
		
	}
	
	
	void Init()
	{
		debug_CallQueue.CallLater(this.OnDebugMessage, br_game.m_BattleRoyaleData.wait_for_players * 1000, true);
	}
	
	void OnUpdate(float timespan)
	{
		debug_CallQueue.Tick(timespan);
	}
	
	void OnDebugMessage()
	{
		//if we are not waiting for either round to end
		if(!br_game.m_BattleRoyaleRound.inProgress && !br_game.m_BattleRoyaleRound_2.inProgress)
		{
			int players_needed = br_game.m_BattleRoyaleData.minimum_players - m_DebugPlayers.Count();
			//if we are waiting for more players
			if(players_needed > 0)
			{
				SendMessageAll("DAYZBR: WAITING FOR " + players_needed.ToString() + " PLAYER(S)...",false);
			}
			
		}
	}
	
	
	array<PlayerBase> RemoveAllPlayers()
	{
		ref array<PlayerBase> result_array = new array<PlayerBase>();
		result_array.InsertAll(m_DebugPlayers);
		m_DebugPlayers.Clear();
		return result_array;
	}
	
	void AddPlayer(PlayerBase player)
	{
		m_DebugPlayers.Insert(player);
	}
	void RemovePlayer(PlayerBase player)
	{
		m_DebugPlayers.RemoveItem(player);
	}
	bool ContainsPlayer(PlayerBase player)
	{
		if(m_DebugPlayers.Find(player) >= 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void HealPlayer(PlayerBase player)
	{
		// GetMaxHealth by default seems to only restore health to 100 but unconsciousness etc
		player.SetHealth("", "Health", player.GetMaxHealth("", "Health"));
		player.SetHealth("", "Blood", player.GetMaxHealth("", "Blood"));
		player.SetHealth("", "Shock", player.GetMaxHealth("", "Shock"));
	}
	
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		//This should not happen? if it does, remove them from this class
		BRLOG("DAYZBR: DEBUG PLAYER KILLED -- BUG?");
		m_DebugPlayers.RemoveItem(killed);
	}
	
	void OnPlayerTick(PlayerBase player, float ticktime)
	{
		//Check if it is time to heal the player
		if(player.timeTillNextHealTick <= 0)
		{
			player.timeTillNextHealTick = 5;
			
			// to save some ressources only heal the player but don't clear their stats
			HealPlayer(player);
		}
		player.timeTillNextHealTick = player.timeTillNextHealTick - ticktime;
		
		
		
		//process zone area lock
		vector playerPos = player.GetPosition();
		
		float distance = vector.Distance(playerPos, br_game.m_BattleRoyaleData.debug_position);
		if(distance > 45)
		{
			player.SetPosition(br_game.m_BattleRoyaleData.debug_position);
		}
	}
	
}