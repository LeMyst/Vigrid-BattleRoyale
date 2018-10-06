
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
		
		for(int i = 0; i < itemList.Count();i++)
		{
			string itemName = itemList.Get(i);
			Object obj = GetGame().CreateObject(itemName,world_pos);
			//obj.PlaceOnSurface();
			EntityAI item = EntityAI.Cast(obj);
			
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
				if(Math.RandomIntInclusive(0,1) == 1)
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