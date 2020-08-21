class BattleRoyaleLoot
{
    protected ref LootReader m_LootReader;
    protected ref ScriptCallQueue m_CallQueue;
    protected bool b_Enabled;


    protected ref map<Entity, ref BattleRoyaleLootableBuilding> m_LootBuildings;

    protected ref map<ref PlayerBase, ref ScriptCallQueue> m_PlayerCallQueues;

    protected ref BattleRoyaleLootData p_LootSettings;


    void BattleRoyaleLoot()
    {
        p_LootSettings = BattleRoyaleLootData.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") );

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

    void UpdatePlayer(ref PlayerBase player, float delta)
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
            if(building_object)
            {
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
                    if(lootable.IsNull())
                        continue;
                    
                    if(!lootable.IsReady())
                        continue;

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
}


class BattleRoyaleLootableBuilding
{
    protected ref LootReader p_LootReader;

    protected Entity p_Building;
    protected bool is_spawned;
    protected ref array<ref BattleRoyaleLootItemData> a_LootData;
    protected ref array<ItemBase> a_SpawnedLoot; //--- 1 to 1 match with a_LootData (as in, the indicies align)
    protected ref array<ref PlayerBase> a_NearPlayers;

    protected bool spawn_loot;

    protected bool is_ready;

    bool IsReady()
    {
        return is_ready;
    }
    bool IsNull()
    {
        return (p_Building == NULL);
    }

    void BattleRoyaleLootableBuilding( Entity building, ref LootReader lr )
    {
        BattleRoyaleLootData p_LootSettings = BattleRoyaleLootData.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") );
        spawn_loot = (Math.RandomFloat(0, 1) < p_LootSettings.chance_to_spawn_building);
        is_ready = false;

        p_Building = building;
        p_LootReader = lr;
        a_LootData = new array<ref BattleRoyaleLootItemData>();
        a_SpawnedLoot = new array<ItemBase>();
        a_NearPlayers = new array<ref PlayerBase>();
        is_spawned = false;

        //use a new thread to initialize this lootable building (AddNearPlayer/RemoveNearPlayer will not be called when is_ready == false)
        if(!spawn_loot)
        {// no loot spawning, so initLoot doesn't need called
            is_ready = true;
            return;
        }
        
        GetGame().GameScript.Call( this, "InitLoot", NULL );
    }

    void AddNearPlayer(ref PlayerBase player)
    {
        if(IsNull())
            return;
        
        if(!spawn_loot)
            return;
        
        if(!is_ready)
            return;

        if(a_NearPlayers.Find(player) == -1)
        {
            Print("First Time player near " + p_Building.GetType());
            a_NearPlayers.Insert(player);
        }
        if(!is_spawned)
        {
            SpawnLoot();
        }
    }
    void RemoveNearPlayer(ref PlayerBase player)
    {
        if(IsNull())
            return;

        if(!spawn_loot)
            return;

        if(!is_ready)
            return;

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
        if(IsNull())
            return;

        if(!spawn_loot)
            return;

        string type_name = p_Building.GetType();
        type_name.ToLower();

        array<vector> loot_positions = p_LootReader.GetAllLootPositions(type_name);

        

        BattleRoyaleLootData p_LootSettings = BattleRoyaleLootData.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") );
        for(int i = 0; i < loot_positions.Count(); i++)
        {
            if(Math.RandomFloat(0, 1) < p_LootSettings.chance_to_spawn_pile)
            {
                vector model_pos = loot_positions.Get(i);
                vector world_pos = p_Building.ModelToWorld(model_pos);

                ref BattleRoyaleLootItemData lootable_item = new BattleRoyaleLootItemData( world_pos );
                a_LootData.Insert(lootable_item);
            }
        }

        is_ready = true;
    }

    //these must ONLY be called when necessary)
    protected void SpawnLoot()
    {
        if(IsNull())
            return;

        if(!spawn_loot)
            return;

        if(is_spawned)
            return;


        Print("SPAWNING LOOT FOR " + p_Building.GetType());

        is_spawned = true;

        a_SpawnedLoot.Clear();
        for(int i = 0; i < a_LootData.Count(); i++)
        {
            ref array<ItemBase> items = a_LootData.Get(i).SpawnItem();
            a_SpawnedLoot.InsertAll(items);
        }
    }
    protected void DespawnLoot() //if this is called when another player is nearby, bugs could occur. Only call when necessary!!!
    {
        if(IsNull())
            return;

        if(!spawn_loot)
            return;

        if(!is_spawned)
            return;

        int i;

        Print("DESPAWNING LOOT FOR " + p_Building.GetType());

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
    static ref map<string, ref array<string>> magazine_mapping; //weapon=>magazine
    static ref map<string, ref array<string>> weapon_slots;//weapon=>slots
    static ref map<string, ref array<string>> slot_attachments;//slot=>attachment

    
    static ref array<string> GetMagazines(string normalized_name)
    {
        string name = normalized_name;
        name.ToLower();

        if(!magazine_mapping)
        {
            magazine_mapping = new map<string, ref array<string>>();
        }
        if(!magazine_mapping.Contains(name))
        {
            //parse magazines for mapping
            string configPath = "CfgWeapons " + normalized_name + " magazines";
            ref array<string> magazines = new array<string>();
            GetGame().ConfigGetTextArray(configPath,magazines);
            magazine_mapping.Insert(name, magazines);
        }
        return magazine_mapping.Get(name);
    }
    static ref array<string> GetSlots(string normalized_name)
    {
        string name = normalized_name;
        name.ToLower();
        if(!weapon_slots)
        {
            weapon_slots = new map<string, ref array<string>>();
        }
        if(!weapon_slots.Contains(name))
        {
            //parse attachments for mapping
            string configPath = "CfgWeapons " + normalized_name + " attachments";
            ref array<string> attachments = new array<string>();
            GetGame().ConfigGetTextArray(configPath,attachments);
            weapon_slots.Insert(name, attachments);
        }
        return weapon_slots.Get(name);
    }
    static ref array<string> GetAttachments(string name, ref BattleRoyaleLootData p_LootSettings)
    {
        if(!slot_attachments)
        {
            slot_attachments = new map<string, ref array<string>>;
        }
        if(slot_attachments.Count() == 0)
        {
            Print("Caching Attachments");
            //for every attachment
            for(int i = 0; i < p_LootSettings.Attachments.Count();i++)
            {
                
                //get its slots out of cfgvehicles
                string attachment_name = p_LootSettings.Attachments[i];
                Print("Caching " + attachment_name);

                string configPath = "cfgVehicles " + attachment_name + " inventorySlot";
                ref array<string> slots = new array<string>();
                GetGame().ConfigGetTextArray(configPath,slots);

                Print("Found Config Slots");
                Print(slots);

                if(slots.Count() == 0)
                {
                    Error("No slots for attachment `" + attachment_name + "`");
                    continue;
                }
                //for every slot
                for(int j = 0; j < slots.Count(); j++)
                {
                    
                    string slot_name = slots[j];
                    slot_name.ToLower();

                    Print("Caching for slot " + slot_name);
                    if(slot_attachments.Contains(slot_name))
                    {
                        //insert it into an existing map entry
                        Print("Entry already exists, inserting " + attachment_name + " into " + slot_name);
                        ref array<string> data = slot_attachments.Get(slot_name);
                        data.Insert(attachment_name);
                    }
                    else
                    {
                        //create a new map entry and add it
                        Print("Entry does not exist, inserting " + slot_name + " with data { " + attachment_name + "}");
                        ref array<string> attachment_values = new array<string>();
                        attachment_values.Insert(attachment_name);
                        slot_attachments.Insert(slot_name, attachment_values);
                    }
                }

            }

            Print("Parse Complete");
            Print(slot_attachments);
        }

        string check = name;
        check.ToLower();

        return slot_attachments.Get(check);
    }

    protected string class_name;
    protected ref array<string> spawn_with;
    protected vector world_pos;

    protected ref BattleRoyaleLootData p_LootSettings;
    
    vector GetSpawnPosition()
    {
        return world_pos;
    }


    void BattleRoyaleLootItemData( vector pos ) //TODO: constructor takes other options for loot types ect
    {
        p_LootSettings = BattleRoyaleLootData.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") );

        spawn_with = new array<string>();
        world_pos = pos;
        InitRandomLootItem();
    }
    void InitRandomLootItem()
    {
        //TODO: random select class name from settings
        float total = 0;
        int i;
        float weight;
        for(i = 0; i < p_LootSettings.Weights.Count(); i++)
        {
            weight = p_LootSettings.Weights[i];
            total += weight;
        }
        float rand = Math.RandomFloatInclusive(0, total);
        float value = 0;
        int type_index = -1;
        for(i = 0; i < p_LootSettings.Weights.Count(); i++)
        {
            weight = p_LootSettings.Weights[i];
            if(rand > value && rand <= (value + weight))
            {
                type_index = i;
                break;
            }
            value += weight;
        }

        if(type_index == -1)
        {
            type_index = 7;
            Error("Failed to select random weight");
        }
        ref array<string> loot_classes = p_LootSettings.Misc_Normal;
        switch(type_index)
        {
            case 0:
                loot_classes = p_LootSettings.Weapons_Rare;
                break;
            case 1:
                loot_classes = p_LootSettings.Weapons_Normal;
                break;
            case 2:
                loot_classes = p_LootSettings.Weapons_Common;
                break;
            case 3:
                loot_classes = p_LootSettings.Gear_Rare;
                break;
            case 4:
                loot_classes = p_LootSettings.Gear_Normal;
                break;
            case 5:
                loot_classes = p_LootSettings.Gear_Common;
                break;
            case 6:
                loot_classes = p_LootSettings.Misc_Rare;
                break;
            case 7:
                loot_classes = p_LootSettings.Misc_Normal;
                break;
            case 8:
                loot_classes = p_LootSettings.Misc_Common;
                break;
            case 9:
                loot_classes = p_LootSettings.Food;
                break;
            case 10:
                loot_classes = p_LootSettings.Drink;
                break;
            case 11:
                loot_classes = p_LootSettings.Medical;
                break;
            case 12:
                loot_classes = p_LootSettings.Ammo;
                break;
            case 13:
                loot_classes = p_LootSettings.Grenades;
                break;
            case 14:
                loot_classes = p_LootSettings.Attachments;
        }


        int index = Math.RandomInt(0, loot_classes.Count());

        class_name = loot_classes[index];
        
        //spawn with mags (cached result for efficiency)
        ref array<string> mags = BattleRoyaleLootItemData.GetMagazines(class_name);
        if(mags.Count() > 0)
        {
            spawn_with.Insert(mags[0]);
            spawn_with.Insert(mags[0]);
            spawn_with.Insert(mags[0]);
        }

        //spawn with attachments (cached result for efficiency)
        ref array<string> slots = BattleRoyaleLootItemData.GetSlots(class_name);
        if(slots.Count() > 0)
        {
            Print("Slots for: " + class_name);
            Print(slots);
            for(i = 0; i < slots.Count(); i++)
            {
                string slot_name = slots[i];
                ref array<string> attachments_for_slot = BattleRoyaleLootItemData.GetAttachments(slot_name, p_LootSettings);
                Print("Attachments for: " + slot_name);
                Print(attachments_for_slot);
                if(!attachments_for_slot)
                {
                    Error("No attachments found for slot `" + slot_name + "`");
                    continue;
                }
                if(attachments_for_slot.Count() > 0)
                {
                    index = Math.RandomInt(0, attachments_for_slot.Count());
                    string attachment_to_spawn = attachments_for_slot[index];

                    spawn_with.Insert(attachment_to_spawn);

                    //spawn battery if necessary
                    string configPath = "cfgVehicles " + attachment_to_spawn + " attachments";
                    ref array<string> sub_attachs = new array<string>();
                    GetGame().ConfigGetTextArray(configPath,sub_attachs);
                    
                    if(sub_attachs.Find("BatteryD") != -1)
                    {
                        spawn_with.Insert("Battery9V");
                    }
                    
                }
            }
            
        }
    }
    ref array<ItemBase> SpawnItem()
    {
        Print("spawning: " + class_name);
        ref array<ItemBase> items = new array<ItemBase>();
        ItemBase item = ItemBase.Cast(GetGame().CreateObject(class_name, world_pos));
        items.Insert(item);

        for(int i = 0; i < spawn_with.Count(); i++)
        {
            string classname = spawn_with.Get(i);
            item = ItemBase.Cast(GetGame().CreateObject(classname, world_pos));
            
            Print("==> spawn with: " + classname);

            items.Insert(item);
        }

        return items;
    }
}