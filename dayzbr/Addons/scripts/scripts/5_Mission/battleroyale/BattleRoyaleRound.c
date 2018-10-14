
class BattleRoyaleRound
{
	ref StaticBRData m_BattleRoyaleData;
	ref BattleRoyaleDebug m_BattleRoyaleDebug;
	ref BattleRoyaleZoneManager m_BattleRoyaleZoneManager;

	ref BattleRoyaleZone m_BattleRoyaleZone;
	ref BattleRoyaleLoot m_BattleRoyaleLoot;

	ref array<PlayerBase> m_RoundPlayers;
	ref array<PlayerBase> m_DeadBodies;

	bool inProgress;
	bool allowZoneDamage;
	
	ref ScriptCallQueue round_CallQueue;
	
	
	//used for ticking
	bool Prepare_Players;
	bool Teleport_Players;
	bool Wait_For_Loot;
	bool Repair_Buildings;
	bool Fade_Players;
	bool RoundStarted;
	int master_index;
	
	
	void BattleRoyaleRound(StaticBRData staticdata, BattleRoyaleDebug debugarea, BattleRoyaleZoneManager zonemanager)
	{
		m_BattleRoyaleData = staticdata;
		m_BattleRoyaleDebug = debugarea;
		m_BattleRoyaleZoneManager = zonemanager;

		// initialize with an ranzom zone just in case ...
		m_BattleRoyaleZone = m_BattleRoyaleZoneManager.getRandomZoneFromPool();
		Print("INITIALIZED ROUND WITH ZONE: " + m_BattleRoyaleZone.GetZoneName());

		inProgress = false;
		m_BattleRoyaleLoot = new BattleRoyaleLoot();
		m_RoundPlayers = new array<PlayerBase>();
		m_DeadBodies = new array<PlayerBase>();

		round_CallQueue = new ScriptCallQueue();
		
		//used for ticking
		Prepare_Players = false;
		Teleport_Players = false;
		Wait_For_Loot = false;
		Repair_Buildings = false;
		Fade_Players = false;
		RoundStarted = false;
	}
	
	void Init()
	{
		return;
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
		round_CallQueue.Tick(ticktime);
		m_BattleRoyaleZone.OnUpdate(ticktime);
		m_BattleRoyaleLoot.OnUpdate(ticktime);
		
		//Round ticking
		PrepPlayersTick();
		TeleportPlayersTick();
		ProcessLootTick();
		RepairBuildingsTick();
		FadePlayersOutTick();
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
			if(m_BattleRoyaleData.death_show_names > 0 && killed && killed.GetIdentity())
			{
				if(e_killer && e_killer.GetIdentity())
				{
					// show names > 1 means also show the distance too
					if(m_BattleRoyaleData.death_show_names > 1)
					{
						float distance = vector.Distance(e_killer.GetPosition(), killed.GetPosition());

						SendMessageAll(e_killer.GetIdentity().GetName() + " KILLED " + killed.GetIdentity().GetName() + " FROM " + distance.ToString() + " METER " + newPlayerCount.ToString() + " PLAYERS REMAIN");														
					}
					else
					{
						SendMessageAll(e_killer.GetIdentity().GetName() + " KILLED " + killed.GetIdentity().GetName() + "! " + newPlayerCount.ToString() + " PLAYERS REMAIN");
					}
				}
				else
				{
					SendMessageAll(killed.GetIdentity().GetName() + " DIED! " + newPlayerCount.ToString() + " PLAYERS REMAIN");
				}
			}
			else
			{
				SendMessageAll(newPlayerCount.ToString() + " PLAYERS REMAIN");
			}
		}
	}
	
	void OnPlayerTick(PlayerBase player, float ticktime)
	{
		if(allowZoneDamage)
		{
			
			vector center = m_BattleRoyaleZone.GetCurrentCenter();
			float distance = m_BattleRoyaleZone.GetCurrentSize();
			
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
		}
	}
	
	
	
	void HealPlayer(PlayerBase player)
	{
		// GetMaxHealth by default seems to only restore health to 100 but unconsciousness etc
		player.SetHealth("", "Health", player.GetMaxHealth("", "Health"));
		player.SetHealth("", "Blood", player.GetMaxHealth("", "Blood"));
		player.SetHealth("", "Shock", player.GetMaxHealth("", "Shock"));
		
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
	
	
	
	void EndRound()
	{
		//Handle Round End
		inProgress = false;
	}
	
	
	void PlayerCountReached()
	{
		inProgress = true;
		
		SendMessageAll("DAYZBR: PLAYER COUNT REACHED. STARTING GAME IN " + m_BattleRoyaleData.start_timer.ToString() + " SECONDS.");
		
		round_CallQueue.CallLater(this.StartRound, m_BattleRoyaleData.start_timer * 1000, false);
	}
	
	void StartRound()
	{
		allowZoneDamage = false;

		// get a zone
		m_BattleRoyaleZone = m_BattleRoyaleZoneManager.getRandomZoneFromPool();

		// get all players
		ref array<PlayerBase> round_players = m_BattleRoyaleDebug.RemoveAllPlayers();
		m_RoundPlayers.InsertAll(round_players);
		
		BRLOG("STARTING ROUND IN ZONE " + m_BattleRoyaleZone.GetZoneName());
		m_DeadBodies.Clear();
		master_index = m_RoundPlayers.Count();
		Prepare_Players = true;
	}
	
	
	//TODO: if a player disconnects during this time, it could cause a player to get skipped in this queue. That would be bad. We need to fix
	void PrepPlayersTick()
	{
		if(Prepare_Players)
		{
			for(int i = 0; i < 5;i++)
			{
				master_index--;
				
				PlayerBase player = m_RoundPlayers.Get(master_index);
				if(player)
				{
					HealPlayer(player);
					
					player.RemoveAllItems();
					//For each item that exist in the array, we spawn on the player.
					for(int counter = 0; counter < br_game.m_BattleRoyaleData.Player_Items.Count();counter++)
					{	
						player.GetInventory().CreateInInventory(br_game.m_BattleRoyaleData.Player_Items[counter]);
					}
					
					GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ScreenFadeIn", NULL, true, player.GetIdentity(), player );
					GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true, player.GetIdentity(), player );
				}
				
				if(master_index == 0)
				{
					Prepare_Players = false;
					master_index = m_RoundPlayers.Count();
					BRLOG("PLAYER PREP DONE");
					Teleport_Players = true;
					return;
				}
			}
		}
	}
	void TeleportPlayersTick()
	{
		if(Teleport_Players)
		{
			for(int i = 0; i < 5; i++)
			{
				master_index--;
				PlayerBase player = m_RoundPlayers.Get(master_index);
				if(player)
				{
					float x = m_BattleRoyaleZone.GetCenter()[0];
					float y = 0.22; //default y coords
					float z = m_BattleRoyaleZone.GetCenter()[2];
					
					//constant angle calcs
					float distance = 10.0; //10m out from center
					int plrCount = m_RoundPlayers.Count();
					float deltaAngle = 360.0 / plrCount;
				
					//angle calculation
					float angle = deltaAngle * master_index;
					float rads = angle * Math.DEG2RAD;
					
					//delta position calculation
					float dX = Math.Sin(rads) * distance;
					float dZ = Math.Cos(rads) * distance;
					
					//finalized position
					float plrX = x + dX;
					float plrZ = z + dZ;
					float plrY = y + GetGame().SurfaceY(plrX,plrZ);
					
					//teleport
					vector playerPos = Vector(plrX,plrY,plrZ);
					player.SetPosition(playerPos);
					vector playerDir = vector.Direction(playerPos,m_BattleRoyaleZone.GetCenter()).Normalized();
					player.SetDirection(playerDir);

					player.GetInputController().OverrideMovementSpeed( true, 0 );
					player.GetInputController().OverrideMeleeEvade( true, false );
					player.GetInputController().OverrideRaise( true, false );

					float headingChange = playerDir.VectorToYaw() - player.GetInputController().GetHeadingAngle();

					player.GetInputController().OverrideAimChangeY( true, headingChange );
					player.GetInputController().OverrideMovementAngle( true, 0 );
				}
				if(master_index == 0)
				{
					Teleport_Players = false;
					master_index = m_RoundPlayers.Count();
					BRLOG("PLAYER TELEPORT DONE");
					m_BattleRoyaleLoot.SpawnLoot(m_BattleRoyaleZone.map_Buildings); //Start loot spawner
					Wait_For_Loot = true;
					return;
				}
			}
		}
	}
	void ProcessLootTick()
	{
		if(Wait_For_Loot)
		{
			//wait for loot spawner to complete and then move on
			if(!m_BattleRoyaleLoot.isRunning)
			{
				Wait_For_Loot = false;
				master_index = m_BattleRoyaleZone.map_Buildings.Count();
				BRLOG("LOOT DONE");
				Repair_Buildings = true;
				return;
			}
		}
	}
	void RepairBuildingsTick()
	{
		if(Repair_Buildings)
		{
			for(int i = 0; i < 5; i++)
			{
				master_index--;
				Object obj = m_BattleRoyaleZone.map_Buildings.Get(master_index);
				obj.SetHealth(obj.GetMaxHealth());
				
				if(master_index == 0)
				{
					Repair_Buildings = false;
					master_index = m_RoundPlayers.Count();
					BRLOG("BUILDING REPAIR DONE");
					Fade_Players = true;
					return;
				}
			}
		}
	}

	void FadePlayersOutTick()
	{
		if(Fade_Players)
		{
			for(int i = 0; i < 5; i++)
			{
				master_index--;
				
				PlayerBase player = m_RoundPlayers.Get(master_index);
				
				player.GetInputController().OverrideAimChangeY( false, 0 );
				player.GetInputController().OverrideMovementAngle( false, 0 );

				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ScreenFadeOut", NULL, true, player.GetIdentity(), player );
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "AllowLookInput", NULL, true, player.GetIdentity(), player );
				
				if(master_index == 0)
				{
					Fade_Players = false;
					BRLOG("FADE PLAYERS DONE");

					int start_countdown_time = 5;

					round_CallQueue.CallLater(this.NotifyTimeTillStart, 5000, false, start_countdown_time);
					return;
				}
			}
		}
	}
	
	void NotifyTimeTillStart(int seconds_remaining)
	{
		SendMessageAll("THE ROUND WILL START IN " + seconds_remaining.ToString());
		seconds_remaining = seconds_remaining - 1;
		
		if(seconds_remaining == 0)
		{
			round_CallQueue.CallLater(this.StartRoundForPlayers, 1000, false,seconds_remaining);
			BRLOG("TIME TILL START DONE");
			return;
		}
		round_CallQueue.CallLater(this.NotifyTimeTillStart, 1000, false,seconds_remaining);
	}
	
	void StartRoundForPlayers()
	{
		allowZoneDamage = true;
		
		m_BattleRoyaleZone.StartZoning();
		
		RoundStarted = true;
		round_CallQueue.CallLater(this.CheckRoundEnd, m_BattleRoyaleData.check_round_end*1000, true);
		
		BRLOG("LET THE GAMES BEGIN");
		SendMessageAll("LET THE GAMES BEGIN");

		for(int i = 0; i < m_RoundPlayers.Count(); i++)
		{
			PlayerBase player = m_RoundPlayers.Get( i );

			ref Param1<bool> value_string = new Param1<bool>(false);
			
			player.GetInputController().SetDisabled( value_string.param1 );
			player.GetInputController().OverrideMovementSpeed( value_string.param1, 0 );
			player.GetInputController().OverrideMeleeEvade( value_string.param1, false );
			player.GetInputController().OverrideRaise( value_string.param1, false );
			player.GetInputController().OverrideMovementAngle( value_string.param1, 0 );

			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", value_string, true, player.GetIdentity(), player );
		}
	}
	
	void CheckRoundEnd()
	{
		int playerCount = m_RoundPlayers.Count();
		
		
		if(!RoundStarted)
		{
			//Round is over, clean up match,
			round_CallQueue.Remove(this.CheckRoundEnd);
			
			//kill all remaining alive players
			for(int i = 0; i < m_RoundPlayers.Count();i++)
			{
				PlayerBase player = m_RoundPlayers.Get(i);
				player.SetHealth("", "", 0.0);
			}
			
			round_CallQueue.CallLater(this.CleanBodies, 5000, false);
			
			
		}
		
		
		
		if(playerCount == 0)
		{
			//We fucked up no player to win the match ?
			RoundStarted = false;

		}
		else if(playerCount == 1)
		{
			PlayerBase winner = m_RoundPlayers.Get(0);
			RoundStarted = false;
			BRLOG("ROUND OVER");
			SendMessage(winner,"YOU WIN DAYZ BR");
			for(int j = 0; j < m_BattleRoyaleDebug.m_DebugPlayers.Count();j++)
			{
				PlayerBase loser = m_BattleRoyaleDebug.m_DebugPlayers.Get(j);
				SendMessage(loser,"SOMEONE JUST WON DAYZ BR");
			}
		}
		
		//Immediate round cleanup (these calls need to be killed so they do not happen during the end-of round delay)
		if(!RoundStarted)
		{
			m_BattleRoyaleZone.StopZoning();
			m_BattleRoyaleLoot.CleanLoot();
		}
	}
	
	void CleanBodies()
	{
		BRLOG("CLEANING BODIES");
		for(int i = 0; i < m_DeadBodies.Count();i++)
		{
			m_DeadBodies.Get(i).Delete();
		}
		EndRound();
	}
}