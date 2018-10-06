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
		//if we are not waiting for a round to end
		if(!br_game.m_BattleRoyaleRound.inProgress)
		{
			int players_needed = br_game.m_BattleRoyaleData.minimum_players - m_DebugPlayers.Count();
			//if we are waiting for more players
			if(players_needed > 0)
			{
				SendMessageAll("DAYZBR: WAITING FOR " + players_needed.ToString() + " PLAYER(S)...",false);
			}
			
		}
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
	
	void HealPlayer(PlayerBase player, bool fast)
	{
		// GetMaxHealth by default seems to only restore health to 100 but unconsciousness etc
		player.SetHealth("", "Health", player.GetMaxHealth("", "Health"));
		player.SetHealth("", "Blood", player.GetMaxHealth("", "Blood"));
		player.SetHealth("", "Shock", player.GetMaxHealth("", "Shock"));
		
		if (!fast)
		{
			// GetStatStomachSolid + GetStatStomachWater > 1000 == STUFFED!
			player.GetStatStomachSolid().Set(250);
			player.GetStatStomachWater().Set(250);
			
			// for bone regen: water = 2500 and energy = 4000 so 5000 should be ok
			player.GetStatWater().Set(5000);
			player.GetStatEnergy().Set(5000);
			// is get max an good idea?
			// player.GetStatWater().Set(player.GetStatWater().GetMax());
			// player.GetStatEnergy().Set(player.GetStatEnergy().GetMax());
			
			
			// default body temperature is  37.4 -> HYPOTHERMIC_TEMPERATURE_TRESHOLD = 35.8
			player.GetStatTemperature().Set(37.4);
			
			// BURNING_TRESHOLD = 199 -> 100 should be fine
			player.GetStatHeatComfort().Set(100);
			
			// seems unused
			// player.GetStatHeatIsolation().Set(100);
			
			// we don't want shaking -> limit is 0.008
			player.GetStatTremor().Set(player.GetStatTremor().GetMin());
			
			// wet if > 0.2
			player.GetStatWet().Set(0);
			
			// unknow effect, don't alter yet
			// player.GetStatStomachEnergy().Set(100);
			// player.GetStatDiet().Set(100);
			
			// think max stamima does not break the game
			player.GetStatStamina().Set(player.GetStatStamina().GetMax());
			
			// required for repairing and stuff, so no need to change for godmode
			//player.GetStatSpecialty().Set(100);
		}
	}
	
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		//This should not happen? if it does, remove them from this class
		m_DebugPlayers.RemoveItem(killed);
	}
	
	void OnPlayerTick(PlayerBase player, float ticktime)
	{
		//Check if it is time to heal the player
		if(player.timeTillNextHealTick <= 0)
		{
			player.timeTillNextHealTick = 5;
			
			// to save some ressources only heal the player but don't clear their stats
			HealPlayer(player, true);
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