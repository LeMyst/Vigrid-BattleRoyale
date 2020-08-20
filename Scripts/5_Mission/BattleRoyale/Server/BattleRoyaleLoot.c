class BattleRoyaleLoot
{
    protected ref LootReader m_LootReader;
    protected ref ScriptCallQueue m_CallQueue;
    protected bool b_Enabled;


    protected ref map<Entity, ref BattleRoyaleLootableBuilding> m_LootBuildings;

    protected ref map<ref PlayerBase, ref ScriptCallQueue> m_PlayerCallQueues;


    void BattleRoyaleLoot()
    {
        b_Enabled = false;
        m_LootBuildings = new map<Entity, ref BattleRoyaleLootableBuilding>();
        m_PlayerCallQueues = new map<ref PlayerBase, ref ScriptCallQueue>();
        m_CallQueue = new ScriptCallQueue;
        m_LootReader = new LootReader();
        //TODO: calculate mission path dynamically (or use json files for config) (currently bugged in dayz)
		m_LootReader.ReadAsync("$CurrentDir:mpmissions\\BattleRoyale.ChernarusPlusGloom\\mapgroupproto.xml"); 
    }
    void Update(float delta)
    {
        m_CallQueue.Tick(delta);
    }

    void UpdatePlayer(PlayerBase player, float delta)
    {
        ref ScriptCallQueue call_queue;
        if(!m_PlayerCallQueues.Contains(player))
        {
            call_queue = new ScriptCallQueue;
            m_PlayerCallQueues.Insert(player, call_queue);
            call_queue.CallLater(this.HandleLoot, 500, true, player);
        }
        call_queue = m_PlayerCallQueues.Get(player);
        if(b_Enabled)
        {
            
            call_queue.Tick(delta);
            
           // HandleLoot(player);//maybe slap this on a script call queue to save performance
        }
    }

    void Start()
    {
        b_Enabled = true;
    }
    void Stop()
    {
        b_Enabled = false;
    }

    

    protected void HandleLoot(PlayerBase player)
    {
        float spawn_radius = 50;
        
        ref array<Object> buildings = new array<Object>();
		ref array<CargoBase> proxies = new array<CargoBase>();

        float despawn_radius = spawn_radius + 20;//20m is enough for any tick rate I bet
        vector player_pos = player.GetPosition();
        
        player_pos[1] = 0;

        GetGame().GetObjectsAtPosition(player_pos, despawn_radius, buildings, proxies);

        for(int i = 0; i < buildings.Count(); i++)
        {
            Entity building_object = Entity.Cast(buildings.Get(i));
            vector obj_pos = building_object.GetPosition();
            obj_pos[1] = 0;
            string type_name = building_object.GetType();
            if(m_LootReader.ContainsObject(type_name))
            {
                if(!m_LootBuildings.Contains( building_object ))
                {
                    m_LootBuildings.Insert( building_object, new BattleRoyaleLootableBuilding( building_object, m_LootReader ) );
                }
                ref BattleRoyaleLootableBuilding lootable = m_LootBuildings.Get(building_object);

                //--- loot may be here
                float distance = vector.Distance(player_pos, obj_pos);
                if(distance <= spawn_radius)
                {
                    //--- spawns loot piles if necessary
                    lootable.AddNearPlayer( player );
                }
                else
                {
                    //--- despawn loot piles if necessary
                    lootable.RemoveNearPlayer( player );
                }
            }
        }
    }
}


class BattleRoyaleLootableBuilding
{
    protected ref LootReader p_LootReader;

    protected Entity p_Building;
    protected bool is_spawned;
    protected ref array<ref BattleRoyaleLootItemData> a_LootData;
    protected ref array<ItemBase> a_SpawnedLoot; //--- 1 to 1 match with a_LootData (as in, the indicies align)
    protected ref array<ref PlayerBase> a_NearPlayers;

    void BattleRoyaleLootableBuilding( Entity building, ref LootReader lr )
    {
        p_Building = building;
        p_LootReader = lr;
        a_LootData = new array<ref BattleRoyaleLootItemData>();
        a_SpawnedLoot = new array<ItemBase>();
        a_NearPlayers = new array<ref PlayerBase>();
        is_spawned = false;

        InitLoot();
    }

    void AddNearPlayer(ref PlayerBase player)
    {
        if(a_NearPlayers.Find(player) == -1)
        {
            a_NearPlayers.Insert(player);
        }
        if(!is_spawned)
        {
            SpawnLoot();
        }
    }
    void RemoveNearPlayer(ref PlayerBase player)
    {
        a_NearPlayers.RemoveItem(player);
        while(a_NearPlayers.Find(NULL) != -1)
        {
            a_NearPlayers.RemoveItem(NULL);
        }
        if(a_NearPlayers.Count() == 0 && is_spawned)
        {
            DespawnLoot();
        }
    }

    void InitLoot()
    {
        string type_name = p_Building.GetType();
        type_name.ToLower();

        array<vector> loot_positions = p_LootReader.GetAllLootPositions(type_name);

        //TODO: use xml for more complex loot spawning techniques


        for(int i = 0; i < loot_positions.Count(); i++)
        {
            vector model_pos = loot_positions.Get(i);
            vector world_pos = p_Building.ModelToWorld(model_pos);

            ref BattleRoyaleLootItemData lootable_item = new BattleRoyaleLootItemData( world_pos );
            a_LootData.Insert(lootable_item);
        }
    }

    //these must ONLY be called when necessary)
    protected void SpawnLoot()
    {
        if(is_spawned)
            return;
        is_spawned = true;

        a_SpawnedLoot.Clear();
        for(int i = 0; i < a_LootData.Count(); i++)
        {
            ItemBase item = a_LootData.Get(i).SpawnItem();
            a_SpawnedLoot.Insert(item);
        }
    }
    protected void DespawnLoot() //if this is called when another player is nearby, bugs could occur. Only call when necessary!!!
    {
        if(!is_spawned)
            return;

        int i;

        array< ref BattleRoyaleLootItemData > remove_these = new array< ref BattleRoyaleLootItemData >();
        for(i = 0; i < a_LootData.Count(); i++)
        {
            ref ItemBase item = a_SpawnedLoot.Get(i);
            ref BattleRoyaleLootItemData lootable_item = a_LootData.Get(i);

            if(item)
            {
                vector should_be_at = lootable_item.GetSpawnPosition();
                vector is_at = item.GetPosition();
                float distance = vector.Distance(should_be_at, is_at);
                if(distance > 1)
                {
                    //object moved, remove it from our list (don't despawn!)
                    remove_these.Insert(lootable_item);
                }
                else
                {
                    GetGame().ObjectDelete( item ); //despawn our item
                }
            }
            else
            {
                remove_these.Insert(lootable_item); //Null item needs cleaned from list
            }
            
        }
        //clean our loot data
        for(i = 0; i < remove_these.Count(); i++)
        {
            a_LootData.RemoveItem(remove_these.Get(i));
        }

        a_SpawnedLoot.Clear();
        is_spawned = false;
    }
}

//--- TODO: this is for cached loot item data
class BattleRoyaleLootItemData
{
    protected string class_name;
    protected ref array<string> attachments;
    protected vector world_pos;
    
    vector GetSpawnPosition()
    {
        return world_pos;
    }


    void BattleRoyaleLootItemData( vector pos ) //TODO: constructor takes other options for loot types ect
    {
        attachments = new array<string>();
        world_pos = pos;
        InitRandomLootItem();
    }
    void InitRandomLootItem()
    {
        //TODO: random select class name from settings
        class_name = "AK101";

        //TODO: handle attachemented items like magazine & optic?
    }
    ItemBase SpawnItem()
    {
        ItemBase item = ItemBase.Cast(GetGame().CreateObject(class_name, world_pos));

        //todo: spawn attachments
        for(int i = 0; i < attachments.Count(); i++)
        {
            string attachment_classname = attachments.Get(i);
        }

        return item;
    }
}