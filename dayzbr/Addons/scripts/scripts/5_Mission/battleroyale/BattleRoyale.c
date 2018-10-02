class BattleRoyale extends BattleRoyaleBase
{
	ref StaticBRData m_BattleRoyaleData;

	ref ScriptCallQueue br_CallQueue;
	//GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.ClientSpawning, 1000, true, newChar);
	
	MissionServer server;
	
	bool RoundStarted;
	
	ref array<PlayerBase> m_AllPlayers;
	ref array<PlayerBase> m_RoundPlayers;
	ref array<PlayerBase> m_DebugPlayers;
	
	ref array<Object> map_Buildings; //stores all cherno buildings
	ref array<EntityAI> last_round_items;
	
	ref array<Object> Zone_Markers;
	
	vector circle_center;
	vector new_center;
	float active_play_area;
	float new_play_area;
	
	int total_markers = 300;
	int marker_count = 0;
	bool spawnMarkers;
	bool updateMarkers;
	bool hasInit;
	bool allowZoneDamage;
	
	void BattleRoyale(MissionServer server_class)
	{
		m_BattleRoyaleData = StaticBRData.LoadDataServer();

		allowZoneDamage = false;
		spawnMarkers = false;
		updateMarkers = false;
		Zone_Markers = new array<Object>();
		active_play_area = m_BattleRoyaleData.play_area_size;
		circle_center = m_BattleRoyaleData.cherno_center;
		new_center = m_BattleRoyaleData.cherno_center;
		hasInit = false;
		map_Buildings = new array<Object>();
		last_round_items = new array<EntityAI>();
		
		br_CallQueue = new ScriptCallQueue(); 
		server = server_class;
		RoundStarted = false;
		m_AllPlayers = new array<PlayerBase>;
		m_RoundPlayers = new array<PlayerBase>;
		m_DebugPlayers = new array<PlayerBase>;
		isUpdating = false;
	}
	void ~BattleRoyale()
	{
		
	}
	
	void log(string msg)
	{
		Print("=============DayZBR LOG================");
		Print("[Log]: " + msg);
		Print("=============DayZBR LOG================");
		
		Debug.Log("=============DayZBR LOG================");
		Debug.Log(msg);
		Debug.Log("=============DayZBR LOG================");
	}
	
	
	bool isUpdating;
	int marker_index;
	void Update_Markers()
	{
		//if we have not spawned markers yet, force that to happen instead
		if(Zone_Markers.Count() == 0)
		{
			updateMarkers = false;
			spawnMarkers = true;
			return;
		}
		if(!isUpdating)
		{
			isUpdating = true;
			marker_index = Zone_Markers.Count();
		}
		
		
		for(int i = 0; i < 5;i++)
		{
			marker_index--;
			
			Object marker = Zone_Markers.Get(marker_index);
			
			float centerX = new_center[0];
			float centerZ = new_center[2];
			float distance = new_play_area;
			
			float deltaAngle = 360.0 / total_markers;
			
			float angle = deltaAngle * marker_index;
			float rads = angle * Math.DEG2RAD;
			
			float dX = Math.Sin(rads) * distance;
			float dZ = Math.Cos(rads) * distance;
			
			float x = centerX + dX;
			float z = centerZ + dZ;
			float y = GetGame().SurfaceY(x,z) + 1.5;
			
			vector objPos = Vector(x,y,z);
			vector objDir = vector.Direction(new_center,objPos).Normalized();
			
			marker.SetPosition(objPos);
			marker.SetDirection(objDir);//MAGICALLY THIS IS WORKING NOW <- )
			
			
			if(marker_index == 0)
			{
				isUpdating = false;
				updateMarkers = false;
				break;
			}
		}
		
		
	}
	void Create_Markers()
	{
		if(marker_count == 0)
		{
			marker_count = total_markers;
		}
		
		//only spawn 5 objects per tick (to reduce lag bcz these spawn during gameplay)
		for(int i = 0;i < 5;i++){
			marker_count--;
			
			
			float centerX = new_center[0];
			float centerZ = new_center[2];
			float distance = new_play_area;
			
			float deltaAngle = 360.0 / total_markers;
			
			float angle = deltaAngle * marker_count;
			float rads = angle * Math.DEG2RAD;
			
			float dX = Math.Sin(rads) * distance;
			float dZ = Math.Cos(rads) * distance;
			
			float x = centerX + dX;
			float z = centerZ + dZ;
			float y = GetGame().SurfaceY(x,z);
			
			vector objPos = Vector(x,y,z);
			Object marker = GetGame().CreateObject("Land_Br_Wall",objPos);
			
			vector objDir = vector.Direction(new_center,objPos).Normalized();
			
			marker.SetDirection(objDir);
			
			Zone_Markers.Insert(marker);
			if(marker_count == 0)
			{
				spawnMarkers = false; //prevent this from firing again
				break;
			}
		}
	}
	void DeleteMarkers()
	{
		for(int i = 0; i < Zone_Markers.Count();i++)
		{
			Object marker = Zone_Markers.Get(i);
			GetGame().ObjectDelete(marker);
		}
		Zone_Markers.Clear();
	}
	
	//New Ticking Code
	void OnUpdate(float timespan)
	{
		br_CallQueue.Tick(timespan);
		if(updateMarkers)
			Update_Markers();
		if(spawnMarkers)
			Create_Markers();
	}
	
	void OnInit()
	{
		if(!hasInit)
			hasInit = true;
		else
			return;
		//Initialize player wait ticker (every 5 seconds we check if we have the required players)
		br_CallQueue.CallLater(this.Tick_WaitingForPlayers, 5000, true); //every 5 seconds, run our wait checker
		br_CallQueue.CallLater(this.Tick_DebugLock,2000,true); //Debug zone distance locker
		
		CreatePlayAreaCircle();
		
		ref array<Object> allObjects = new array<Object>();
		ref array<CargoBase> proxies = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(m_BattleRoyaleData.cherno_center, m_BattleRoyaleData.play_area_size, allObjects, proxies);
		for(int i = 0; i < allObjects.Count();i++)
		{
			Object obj = allObjects.Get(i);
			if(obj.IsBuilding())
			{
				obj.SetHealth(obj.GetMaxHealth());//heal building to max
				map_Buildings.Insert(obj);
			}				
		}
		
	}
	
	void RepairBuildings()
	{
		for(int i = 0; i < map_Buildings.Count();i++)
		{
			Object obj = map_Buildings.Get(i);
			obj.SetHealth(obj.GetMaxHealth());//heal building to max
		}
	}
	
	
	void Tick_DebugLock()
	{
		for(int i = 0; i < m_DebugPlayers.Count();i++)
		{
			PlayerBase target = m_DebugPlayers.Get(i);
			vector playerPos = target.GetPosition();
			
			float distance = vector.Distance(playerPos, m_BattleRoyaleData.debug_position);
			if(distance > 45)
			{
				target.SetPosition(m_BattleRoyaleData.debug_position);
			}
			
		}
	}
	//Pre Round Logic (and starting)
	void Tick_WaitingForPlayers()
	{
		if(m_DebugPlayers.Count() >= m_BattleRoyaleData.minimum_players)
		{
			//Wait is over
			br_CallQueue.Remove(this.Tick_WaitingForPlayers);
			
			SendMessageAll("DAYZBR: PLAYER COUNT REACHED. STARTING GAME IN 30 SECONDS.");
			
			RoundStarted = true;
			active_play_area = m_BattleRoyaleData.play_area_size;
			circle_center = m_BattleRoyaleData.cherno_center;
			new_center = m_BattleRoyaleData.cherno_center;

			br_CallQueue.CallLater(this.Tick_StartRound, 30000, false); //in 30 seconds, start our round function
		}
		else
		{
			SendMessageAll("DAYZBR: WAITING FOR PLAYERS...",false);
		}
	}
	void HeavyRoundStart()
	{
		SpawnCircleCenter();
		SpawnMapLoot();
		RepairBuildings();
		
		br_CallQueue.CallLater(this.FadePlayersOut, 5000, false);
	}
	void Tick_StartRound()
	{
		allowZoneDamage = false;
		m_RoundPlayers.InsertAll(m_DebugPlayers);
		m_DebugPlayers.Clear();
		
		//disable player inputs and fade them to black
		PrepPlayersForTP();
		
		
	}
	
	//Zone Timing Logic
	void Tick_ShrinkZone()
	{
		SendMessageAll("THE NEW ZONE HAS APPEARED. IT WILL LOCK IN 1 MINUTE.");
		
		new_play_area = active_play_area * m_BattleRoyaleData.shrink_coefficient; //Shrink by 85% each round (ex: first tick- 1000m to 850m in diameter)
		//TODO: calculate a new circle_center based on new_play_area
		
		Print("==== ZONE LOGIC ====");
		
		
		float distance = Math.RandomFloatInclusive(0,active_play_area - new_play_area);
		Print(distance);
		float oldX = circle_center[0];
		float oldZ = circle_center[2];
		
		float moveDir = Math.RandomFloat(0,360) * Math.DEG2RAD;
		
		float dX = distance * Math.Sin(moveDir);
		float dZ = distance * Math.Cos(moveDir);
		
		float newX = oldX + dX;
		float newZ = oldZ + dZ
		float newY = GetGame().SurfaceY(newX,newZ);
		
		Print(oldX);
		Print(newX);
		Print(oldZ);
		Print(newZ);
		Print("=====");
		Print(newY);
		//TODO: update new_center with our new coords
		new_center = Vector(newX,newY,newZ);
		
		updateMarkers = true;//force update markers ticks to fire
		
		Print("====================");
		
		br_CallQueue.CallLater(this.Tick_LockZone, 60*1000,false); // in 1 minute, lock the zone
	}
	void Tick_LockZone()
	{
		SendMessageAll("THE ZONE HAS BEEN LOCKED IN.");
		
		circle_center = new_center;
		active_play_area = new_play_area;
		
		Print("==== ZONE LOGIC ====");
		Print(circle_center);
		Print(active_play_area);
		Print("====================");
		
		//TODO: handle 'this is the last zone' logic
		br_CallQueue.CallLater(this.Tick_ShrinkZone, 120*1000,false); // in 2 minutes, shrink the zone again
	}
	
	//Round end logic
	void Tick_CheckRoundEnd()
	{
		int playerCount = m_RoundPlayers.Count();
		
		
		if(!RoundStarted)
		{
			//Round is over, clean up match,
			br_CallQueue.Remove(this.Tick_CheckRoundEnd);
			
			//kill all remaining alive players
			for(int i = 0; i < m_RoundPlayers.Count();i++)
			{
				PlayerBase player = m_RoundPlayers.Get(i);
				player.SetHealth("", "", 0.0);
			}
			
			//Restart The Game
			br_CallQueue.CallLater(this.Tick_WaitingForPlayers, 5000, true); 
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
			
			SendMessage(winner,"YOU WIN DAYZ BR");
			for(int j = 0; j < m_DebugPlayers.Count();j++)
			{
				PlayerBase loser = m_DebugPlayers.Get(j);
				SendMessage(loser,"SOMEONE JUST WON DAYZ BR");
			}
		}
		
		//Immediate round cleanup (these calls need to be killed so they do not happen during the end-of round delay)
		if(!RoundStarted)
		{
			br_CallQueue.Remove(this.Tick_LockZone);
			br_CallQueue.Remove(this.Tick_ShrinkZone);
			DeleteMapLoot();
			DeleteMarkers();
		}
	}
	
	
	//New Network Code
	void SendMessage(PlayerBase target, string msg, bool required = true)
	{
		ref Param1<string> value_string = new Param1<string>(msg);
		GetGame().RPCSingleParam(target,MRPCs.RPC_BR_SEND_CLIENT_MSG,value_string,required,target.GetIdentity());
		
	}
	void SendMessageAll(string msg, bool required = true)
	{
		ref Param1<string> value_string = new Param1<string>(msg);
		GetGame().RPCSingleParam(NULL,MRPCs.RPC_BR_SEND_GLOBAL_MSG,value_string,required,NULL);
	}
	
	//Events
	override void OnPlayerTick(PlayerBase player, float ticktime)
	{
		if(allowZoneDamage && m_RoundPlayers.Find(player) >= 0)
		{
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
					player.timeTillNextDmgTick = 2;
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
	void OnPlayerConnected(PlayerBase player)
	{
		log("DAYZBR: PLAYER CONNECTED");
		player.BR_BASE = this; //Register the player for Player based event calls
		m_DebugPlayers.Insert(player);
		m_AllPlayers.Insert(player);
		player.SetAllowDamage( false );
	}
	
	void OnPlayerDisconnected(PlayerBase player)
	{
		log("DAYZBR: PLAYER DISCONNECTED");
		m_AllPlayers.RemoveItem(player);
		m_RoundPlayers.RemoveItem(player);
		m_DebugPlayers.RemoveItem(player);
		
	}
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		PlayerBase player = killed;
		PlayerBase e_killer = NULL;
		if(killer.IsInherited(PlayerBase))
		{
			e_killer = PlayerBase.Cast(killer);
		}
		int oldPlayerCount = m_RoundPlayers.Count();
		
		//Some logging for debugging
		Print(killed);
		Print(killer);
		log("DAYZBR: PLAYER KILLED");
		m_AllPlayers.RemoveItem(player);
		m_RoundPlayers.RemoveItem(player);
		m_DebugPlayers.RemoveItem(player);
		
		//Player Death Messages
		int newPlayerCount = m_RoundPlayers.Count();
		if(newPlayerCount < oldPlayerCount)
		{
			SendMessageAll(newPlayerCount.ToString() + " PLAYERS REMAIN");
		}
		
	}
	
	
	array<EntityAI> SpawnLootInBackpack(string backpackType, array<string> itemList,vector world_pos)
	{
		//TODO: figure out how to spawn backpacks and fill their inventory
		ref array<EntityAI> spawnedItems = new array<EntityAI>();
		
		Object obj = GetGame().CreateObject(backpackType,world_pos);
		EntityAI backpack = EntityAI.Cast(obj);
		spawnedItems.Insert(backpack);
		
		for(int i = 0; i < itemList.Count();i++)
		{
			string itemName = itemList.Get(i);
			Object itemObj = GetGame().CreateObject(itemName,world_pos);
			EntityAI item = EntityAI.Cast(itemObj);
			
			spawnedItems.Insert(item);
			
			backpack.PredictiveTakeEntityToInventory(FindInventoryLocationType.CARGO,item);
		}
		
		
		
		
		return spawnedItems;
	}
	
	array<EntityAI> SpawnLootAt(array<string> itemList,vector world_pos)
	{
		ref array<EntityAI> outItems = new array<EntityAI>();
		
		for(int i = 0; i < itemList.Count();i++)
		{
			string itemName = itemList.Get(i);
			Object obj = GetGame().CreateObject(itemName,world_pos);
			EntityAI item = EntityAI.Cast(obj);
			
			outItems.Insert(item);
		}
		return outItems;
	}
	
	void SpawnCircleCenter()
	{
		ref array<EntityAI> SpawnedItems = new array<EntityAI>();
		for(int i = 0; i < 5;i++)
		{	
			//Spawn a backpack, fill it with items defined in the Config, place it at cherno center, and insert all items into the SpawnedItems list
			SpawnedItems.InsertAll(SpawnLootInBackpack("AliceBag_Camo",GetBackpackSpawn(),m_BattleRoyaleData.cherno_center));
		}
		//Insert items into garbage collector
		last_round_items.InsertAll(SpawnedItems); //Add items to end-of-round garbage collector
	}
	void DeleteMapLoot()
	{
		for(int i = 0;i < last_round_items.Count();i++)
		{
			EntityAI item = last_round_items.Get(i);
			if(item)
			{
				item.Delete();
			}
		}
		last_round_items.Clear(); //all items are despawned, clear list
	}
	
	bool isGoodSelection(string name)
	{
		//Instant exclusions
		if(name.Contains("wall"))
			return false;
		
		if(name.Contains("proxy"))
			return false;
		
		//Inclusions
		if(name.Contains("loot"))
			return true;
		
		if(name.Contains("small"))
			return true;
		
		if(name.Contains("medium"))
			return true;
		
		if(name.Contains("point"))
			return true;
		
		return false;
	}
	
	array<string> GetBackpackSpawn()
	{
		string cfgPath = "CfgLoot Center_Backpacks ";
			
		int rand = Math.RandomIntInclusive(1,GetGame().ConfigGetInt(cfgPath + "num_items"));
		
		ref array<string> CfgItems = new array<string>();
		GetGame().ConfigGetTextArray( cfgPath + "Gear_" + rand.ToString(), CfgItems );
		
		return CfgItems;
	}
	array<string> GetWeaponSpawn()
	{
		string cfgPath = "CfgLoot Weapons ";
			
		int rand = Math.RandomIntInclusive(1,GetGame().ConfigGetInt(cfgPath + "num_items"));
		
		ref array<string> CfgItems = new array<string>();
		GetGame().ConfigGetTextArray( cfgPath + "Gear_" + rand.ToString(), CfgItems );
		
		return CfgItems;
	}
	
	array<string> GetMedicalSpawn()
	{
		string cfgPath = "CfgLoot Medical ";
			
		int rand = Math.RandomIntInclusive(1,GetGame().ConfigGetInt(cfgPath + "num_items"));
		
		ref array<string> CfgItems = new array<string>();
		GetGame().ConfigGetTextArray( cfgPath + "Gear_" + rand.ToString(), CfgItems );
		
		return CfgItems;
	}
	array<string> GetFoodSpawn()
	{
		string cfgPath = "CfgLoot Food ";
			
		int rand = Math.RandomIntInclusive(1,GetGame().ConfigGetInt(cfgPath + "num_items"));
		
		ref array<string> CfgItems = new array<string>();
		GetGame().ConfigGetTextArray( cfgPath + "Gear_" + rand.ToString(), CfgItems );
		
		return CfgItems;
	}
	
	array<string> GetGearSpawn()
	{
		string cfgPath = "CfgLoot Gear ";
			
		int rand = Math.RandomIntInclusive(1,GetGame().ConfigGetInt(cfgPath + "num_items"));
		
		ref array<string> CfgItems = new array<string>();
		GetGame().ConfigGetTextArray( cfgPath + "Gear_" + rand.ToString(), CfgItems );
		
		return CfgItems;
	}
	
	void SpawnMapLoot()
	{
		for(int i = 0; i < map_Buildings.Count();i++)
		{
			Object obj = map_Buildings.Get(i);
			ref array<string> selections = new array<string>();
			obj.GetSelectionList(selections);
			for(int j = 0; j < selections.Count();j++)
			{
				string selectionName = selections.Get(j);
				if(isGoodSelection(selectionName))
				{
					//For every loot position after the first, run a roll and 50% of the time spawn more
					if(Math.RandomIntInclusive(0,1) == 1 || j == 0) 
					{
						vector model_pos = obj.GetSelectionPosition( selectionName );
						vector world_pos = obj.ModelToWorld( model_pos );
						
						int itemType = Math.RandomIntInclusive(0,3);
						
						ref array<string> Items = new array<string>();
						switch(itemType)
						{
							case 0: //GUN
								Items.InsertAll(GetWeaponSpawn());
								break;
							case 1: //MEDICAL
								Items.InsertAll(GetMedicalSpawn());
								break;
							case 2://FOOD AND DRINK
								Items.InsertAll(GetFoodSpawn());
								break;
							case 3://LAST CATEGORY
								Items.InsertAll(GetGearSpawn());
								break;
						}
						
						ref array<EntityAI> SpawnedItems = SpawnLootAt(Items,world_pos);
						last_round_items.InsertAll(SpawnedItems); //Add items to end-of-round garbage collector
					}
				}
			}
		}
	}
	void CreatePlayAreaCircle()
	{
		float x = m_BattleRoyaleData.cherno_center[0];
		float y = 0;
		float z = m_BattleRoyaleData.cherno_center[2];
		
		//constant angle calcs
		float distance = m_BattleRoyaleData.play_area_size; //10m out from center
		int objCount = 100;//NOTE @ 500m radius, circle circumfrence is 3142m, at this scale, each object will need to be > 3m in size
		float deltaAngle = 360.0 / objCount;
		for(int i = 0; i < objCount;i++)
		{
			//angle calculation
			float angle = deltaAngle * i;
			float rads = angle * Math.DEG2RAD;
			
			//delta position calculation
			float dX = Math.Sin(rads) * distance;
			float dZ = Math.Cos(rads) * distance;
			
			//finalized position
			float objX = x + dX;
			float objZ = z + dZ;
			float objY = GetGame().SurfaceY(objX,objZ);
			
			
			vector objpos = Vector(objX,objY,objZ)
			vector objdir = vector.Direction(m_BattleRoyaleData.cherno_center, objpos).Normalized();
			Print(objdir);
			Object wall = GetGame().CreateObject("Land_Prison_Wall_Large",objpos);
			wall.SetDirection(objdir);
			wall.SetOrientation(wall.GetOrientation()); //Possible fix?
			
			//br_CallQueue.CallLater(wall.SetDirection, Math.RandomIntInclusive(10,1000),false,objdir);  
			//setup random call queues for applying these changes
		}
	}
	
	//Functionality for round starting
	void FadePlayersOut()
	{
		for(int i = 0; i < m_RoundPlayers.Count();i++)
		{
			PlayerBase player = m_RoundPlayers.Get(i);
			GetGame().RPCSingleParam(player,MRPCs.RPC_BR_FADE_OUT,NULL,true,player.GetIdentity());
		}
		br_CallQueue.CallLater(this.NotifyTimeTillStart, 1000, false,5);
	}
	void PrepPlayersForTP()
	{
		for(int i = 0; i < m_RoundPlayers.Count();i++)
		{
			PlayerBase player = m_RoundPlayers.Get(i);
			
			GetGame().RPCSingleParam(player,MRPCs.RPC_BR_FADE_IN,NULL,true,player.GetIdentity());
			
			ref Param1<bool> value_string = new Param1<bool>(true);
			GetGame().RPCSingleParam(player,MRPCs.RPC_BR_SET_INPUT,value_string,true,player.GetIdentity());
		}
		
		br_CallQueue.CallLater(this.TeleportPlayers, 2000, false); //Teleport players
	}
	void StartRoundForPlayers()
	{
		allowZoneDamage = true;
		
		br_CallQueue.CallLater(this.Tick_ShrinkZone, 120*1000,false); // in 2 minutes, start zoning logic
		br_CallQueue.CallLater(this.Tick_CheckRoundEnd, 5000, true);
		
		
		SendMessageAll("LET THE GAMES BEGIN");
		ref Param1<bool> value_string = new Param1<bool>(false);
		GetGame().RPCSingleParam(NULL,MRPCs.RPC_BR_SET_INPUT,value_string,true,NULL);
	}
	void NotifyTimeTillStart(int seconds_remaining)
	{
		SendMessageAll("THE ROUND WILL START IN " + seconds_remaining.ToString());
		seconds_remaining = seconds_remaining - 1;
		
		if(seconds_remaining == 0)
		{
			br_CallQueue.CallLater(this.StartRoundForPlayers, 1000, false,seconds_remaining);
			return;
		}
		br_CallQueue.CallLater(this.NotifyTimeTillStart, 1000, false,seconds_remaining);
		
	}
	void TeleportPlayers()
	{
		float x = m_BattleRoyaleData.cherno_center[0];
		float y = 0.22; //default y coords
		float z = m_BattleRoyaleData.cherno_center[2];
		
		//constant angle calcs
		float distance = 10.0; //10m out from center
		int plrCount = m_RoundPlayers.Count();
		float deltaAngle = 360.0 / plrCount;
		
		for(int i = 0; i < m_RoundPlayers.Count();i++)
		{
			//angle calculation
			float angle = deltaAngle * i;
			float rads = angle * Math.DEG2RAD;
			
			//delta position calculation
			float dX = Math.Sin(rads) * distance;
			float dZ = Math.Cos(rads) * distance;
			
			//finalized position
			float plrX = x + dX;
			float plrZ = z + dZ;
			float plrY = y + GetGame().SurfaceY(plrX,plrZ);
			
			//teleport
			PlayerBase player = m_RoundPlayers.Get(i);
			vector playerPos = Vector(plrX,plrY,plrZ);
			player.SetPosition(playerPos);
			player.SetDirection(vector.Direction(playerPos,m_BattleRoyaleData.cherno_center).Normalized());
			player.SetAllowDamage( true );
		}
		
		
		br_CallQueue.CallLater(this.HeavyRoundStart,2000,false);
		
	}
}