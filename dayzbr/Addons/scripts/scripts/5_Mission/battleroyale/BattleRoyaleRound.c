
class BattleRoyaleRound
{
	ref BattleRoyaleZone m_BattleRoyaleZone;
	ref BattleRoyaleLoot m_BattleRoyaleLoot;
	ref BattleRoyale br_game;
	ref array<PlayerBase> m_RoundPlayers;
	ref array<PlayerBase> m_DeadBodies;
	
	bool inProgress;
	bool allowZoneDamage;
	
	ref ScriptCallQueue round_CallQueue;
	
	void BattleRoyaleRound(BattleRoyale game)
	{
		br_game = game;
		inProgress = false;
		m_BattleRoyaleZone = new BattleRoyaleZone(this);
		m_BattleRoyaleLoot = new BattleRoyaleLoot();
		m_RoundPlayers = new array<PlayerBase>();
		m_DeadBodies = new array<PlayerBase>();
		
		round_CallQueue = new ScriptCallQueue();
	}
	
	void Init()
	{
		
	}
	
	
	void AddPlayer(PlayerBase player)
	{
		m_RoundPlayers.Insert(player);
	}
	void RemovePlayer(PlayerBase player)
	{
		m_RoundPlayers.RemoveItem(player);
	}
	bool ContainsPlayer(PlayerBase player)
	{
		if(m_RoundPlayers.Find(player) >= 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	
	
	
	void OnUpdate(float ticktime)
	{
		m_BattleRoyaleZone.OnUpdate(ticktime);
		m_BattleRoyaleLoot.OnUpdate(ticktime);
	}
	
	
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		PlayerBase player = killed;
		PlayerBase e_killer = NULL;
		if(killer.IsInherited(PlayerBase))
		{
			e_killer = PlayerBase.Cast(killer);
		}
		
		int oldPlayerCount = m_RoundPlayers.Count();
		m_RoundPlayers.RemoveItem(player);
		m_DeadBodies.Insert(player);
		
		int newPlayerCount = m_RoundPlayers.Count();
		if(newPlayerCount < oldPlayerCount)
		{
			SendMessageAll(newPlayerCount.ToString() + " PLAYERS REMAIN");
		}
	}
	
	void OnPlayerTick(PlayerBase player, float ticktime)
	{
		if(allowZoneDamage)
		{
			
			//TODO: zone logic is needed for this
			
			/*
			vector center = circle_center;
			float distance = active_play_area;
			
			vector playerPos = player.GetPosition();
			
			float x1 = playerPos[0];
			float z1 = playerPos[2];
			float x2 = center[0];
			float z2 = center[2];
			
			float distance2d = Math.Sqrt(Math.Pow(x2-x1,2) + Math.Pow(z2-z1,2));
			if(distance2d > distance)
			{
				if(player.timeTillNextDmgTick <= 0)
				{
					Print("==== PLAYER OUT OF ZONE ====");
					Print(distance2d);
					Print(distance);
					Print("============================");
					player.DecreaseHealthCoef(0.1); //TODO: delta this by the # of zones that have ticked (more zones = more damage)
					player.timeTillNextDmgTick = 5;
					SendMessage(player,"YOU ARE TAKING ZONE DAMAGE",false);
				}
				else
				{
					player.timeTillNextDmgTick -= ticktime;
				}
				
			}
			else
			{
				player.timeTillNextDmgTick = 0;
			}
			*/
		}
	}
	
	
	void EndRound()
	{
		//Handle Round End
	}
	
	
	void PlayerCountReached()
	{
		inProgress = true;
		SendMessageAll("DAYZBR: PLAYER COUNT REACHED. STARTING GAME IN " + br_game.m_BattleRoyaleData.start_timer.ToString() + " SECONDS.");
		round_CallQueue.CallLater(this.StartRound, br_game.m_BattleRoyaleData.start_timer * 1000, false);
		
	}
	
	void StartRound()
	{
		allowZoneDamage = false;
		ref array<PlayerBase> round_players = br_game.m_BattleRoyaleDebug.RemoveAllPlayers();
		m_RoundPlayers.InsertAll(round_players);
		//TODO: start the round
		
		SendMessageAll("DAYZBR: PLAYER COUNT REACHED. STARTING GAME IN " + br_game.m_BattleRoyaleData.start_timer.ToString() + " SECONDS.");
	}
	
}