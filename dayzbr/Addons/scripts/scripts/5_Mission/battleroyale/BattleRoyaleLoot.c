
class BattleRoyaleLoot
{
	bool isRunning;
	bool spawn_loot;
	bool spawn_bags;
	
	
	void BattleRoyaleLoot()
	{
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
	
	void SpawnLoot()
	{
		this.spawn_loot = true;
		this.spawn_bags = true;
		this.isRunning = true;
	}
	
	void ProcessGroundLootSpawn()
	{
			//this gets called on every tick. to stop, set spawn_loot to false
	} 
	void ProcessBagLootSpawn()
	{
		//this gets called on every tick. to stop, set spawn_bags to false
	}
	
	//TODO:
	void CleanLoot()
	{
		
	}
}