
class BattleRoyaleLoot
{
	bool isRunning;
	bool spawn_loot;
	bool spawn_bags;
	
	
	ref array<EntityAI> last_round_items;
	ref array<Object> map_Buildings;
	int master_index;
	
	
	
	
	void BattleRoyaleLoot()
	{
		last_round_items = new array<EntityAI>();
		
		this.isRunning = false;
		
		this.spawn_loot = false;
		this.spawn_bags = false;
	}
	
	void OnUpdate(float ticktime)
	{
		if(this.isRunning)
		{
			if(this.spawn_loot)
			{
				ProcessGroundLootSpawn();
			}
			else if(this.spawn_bags)
			{
				ProcessBagLootSpawn();
			}
			else
			{
				//we are done whatever we were doing, mark the class as so
				this.isRunning = false; 
			}
		}
	}
	
	void SpawnLoot(array<Object> buildings)
	{
		this.map_Buildings = buildings;
		this.master_index = map_Buildings.Count();
		this.spawn_loot = true;
		this.spawn_bags = true;
		this.isRunning = true;
	}
	
	void SpawnRandomItemAtPosition(Object building, vector model_pos)
	{
		vector world_pos = building.ModelToWorld( model_pos );
		
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
		last_round_items.InsertAll(SpawnedItems); //add to garbage collection
	}

	array<EntityAI> SpawnLootAt(array<string> itemList,vector world_pos)
	{
		ref array<EntityAI> outItems = new array<EntityAI>();
		
		/*
		TODO: Automatic item attachment
		
		1) if a weapon is spawned, store it in a variable
		2) if another item is spawned, check if the weapon supports this as an attachment (cfg searches)
		3) try to spawn the item as an attachment (createAttachment)
		4) if that fails, spawn it on ground (how to check fail?)
		
		
		
		*/
		
		for(int i = 0; i < itemList.Count();i++)
		{
			string itemName = itemList.Get(i);

			Object obj;
			EntityAI item;
			if(itemName.Contains("M4A1"))
			{
				item = GetGame().CreateObject(itemName,world_pos);
				//item.GetInventory().CreateAttachment("M4_Suppressor");
				//item.GetInventory().CreateAttachment("M4_CarryHandleOptic");
				//outItems.Insert(item.GetInventory().CreateAttachment("M4_MPBttstck_Black"));
				//outItems.Insert(item.GetInventory().CreateAttachment("M4_RISHndgrd_Black"));
			}
			else if(itemName == ("AKM"))
			{
				item = GetGame().CreateObject(itemName,world_pos);
				//item.GetInventory().CreateAttachment("AK_Suppressor");
				//outItems.Insert(item.GetInventory().CreateAttachment("AK_PlasticBttstck_Black"));
				//outItems.Insert(item.GetInventory().CreateAttachment("AK_RailHndgrd_Black"));
			}
			else
			{
				obj = GetGame().CreateObject(itemName,world_pos);
				item = EntityAI.Cast(obj);
			}
			
			//obj.PlaceOnSurface();
			
			outItems.Insert(item);
		}
		return outItems;
	}
	
	void SpawnItemsForBuilding(Object building)
	{
		string ObjectType = building.GetType();
		string path = "CfgLootPositions " + ObjectType;
		
		//check we have a loot position for this building
		if(GetGame().ConfigIsExisting(path))
		{
			int count = GetGame().ConfigGetChildrenCount(path);
			for(int i = 0; i < count;i++)
			{
				//TODO randomize if we can spawn here
				if(Math.RandomIntInclusive(0,1) == 1 || i == 0)
				{
					string pos_name;
					GetGame().ConfigGetChildName(path, i, pos_name);
					
					vector model_pos = GetGame().ConfigGetVector(path + " " + pos_name);
					
					SpawnRandomItemAtPosition(building,model_pos);
				}
			}
			
		}
	}
	
	
	void ProcessGroundLootSpawn()
	{
		for(int i = 0; i < 2; i++)
		{
			master_index--;
			Object building = map_Buildings.Get(master_index);
			SpawnItemsForBuilding(building);
			if(master_index == 0)
			{
				this.master_index = map_Buildings.Count();
				this.spawn_loot = false;
				return;
			}
		}
	} 
	void ProcessBagLootSpawn()
	{
		//this gets called on every tick. to stop, set spawn_bags to false
		spawn_bags = false;
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
		
		string classPath = cfgPath + "Gear_" + rand.ToString();
		
		ref array<string> CfgItems = new array<string>();
		
		
		int itemSlots = GetGame().ConfigGetChildrenCount(classPath);
		for(int i = 1; i <= itemSlots; i++)
		{
			//roll for a spawn on this slot
			string itemSlotPath = classPath + " Item_" + i.ToString();
			float chance = GetGame().ConfigGetFloat(itemSlotPath + " chance");
			if(Math.RandomFloatInclusive(0, 1) < chance)
			{
				//calculate number of this type to spawn
				int min = GetGame().ConfigGetInt(itemSlotPath + " min_spawn");
				int max = GetGame().ConfigGetInt(itemSlotPath + " max_spawn");
				int num_to_spawn = Math.RandomIntInclusive(min,max);
				
				//figure out which type to spawn
				ref array<string> CfgTypes = new array<string>();
				GetGame().ConfigGetTextArray(itemSlotPath + " Types", CfgTypes);
				
				min = 0;
				max = CfgTypes.Count() - 1;
				int index = Math.RandomIntInclusive(min,max);
				string itemTypeToSpawn = CfgTypes.Get(index);
				
				for(int j = 0; j < num_to_spawn;j++)
				{
					CfgItems.Insert(itemTypeToSpawn);
				}
			}
		}
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
	
	
	void CleanLoot()
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
}