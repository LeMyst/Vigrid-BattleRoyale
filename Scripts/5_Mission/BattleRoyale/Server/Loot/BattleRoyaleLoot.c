
class BattleRoyaleLoot 
{
    protected bool b_Enabled;
    protected bool b_IsProcessing;
    protected ref ScriptCallQueue m_CallQueue;

    protected ref array<ref PlayerBase> m_Players;
    protected ref map<Object, ref BattleRoyaleLootableBuilding> m_LootableBuildings;

    

    void BattleRoyaleLoot()
    {
        m_CallQueue = new ScriptCallQueue;
        m_LootableBuildings = new map<Object, ref BattleRoyaleLootableBuilding>();
        m_Players = new array<ref PlayerBase>();
        b_IsProcessing = false;

        m_CallQueue.CallLater(this.HandleTick, 500, true);

        LootReader.GetReader().ReadAsync("$CurrentDir:mpmissions\\BattleRoyale.ChernarusPlusGloom\\mapgroupproto.xml");

        BattleRoyaleLootData.GetData().Load();
    }
    void ~BattleRoyaleLoot()
    {
        Print("BattleRoyaleLoot Deconstructor Called!");

        delete m_LootableBuildings;
        delete m_CallQueue;
        delete m_Players;
    }

    void Update(float delta)
    {
        if(b_Enabled)
        {
            m_CallQueue.Tick(delta);
        }
    }
    
    void AddPlayer(ref PlayerBase player)
    {
        if(m_Players.Find(player) == -1)
        {
            m_Players.Insert(player);
        }
    }
    void RemovePlayer(ref PlayerBase player)
    {
        m_Players.RemoveItem(player);
    }


    void Start()
    {
        b_Enabled = true;
    }
    void Stop()
    {
        b_Enabled = false;
    }
    bool IsEnabled()
    {
        return b_Enabled;
    }

    void HandleTick()
    {
        if(!b_IsProcessing) //this prevents a second thread from spinning up if processloot takes longer than 0.5s
        {
            b_IsProcessing = true;

            GetGame().GameScript.Call( this, "ProcessLoot", NULL ); 
        }
        else
        {
            Print("Warning! Loot System Can't Keep Up!");
        }
        
    }
    void ProcessLoot()
    {
        //clone players into a new list so we can process them without interrupts from RemovePlayer()
        ref array<ref PlayerBase> m_PlayerClone = new array<ref PlayerBase>();
        int i;
        ref PlayerBase player;
        for(i = 0; i < m_Players.Count(); i++)
        {
            player = m_Players[i];
            m_PlayerClone.Insert(player);
        }

        for(i = 0; i < m_PlayerClone.Count(); i++)
        {
            player = m_PlayerClone[i];
            if(player)
            {
                ProcessPlayerLoot(player); //can't put this on it's own thread bcz players near eachother will have issues with code stepping on eachother
            }
        }

        b_IsProcessing = false;
    }
    void ProcessPlayerLoot(ref PlayerBase player)
    {
        vector player_pos = player.GetPosition();
        player_pos[1] = 0;

        float spawn_radius = 20; //within 20m? spawn
        float despawn_radius = 25;  //outside 25m? despawn

        array<Object> buildings = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
        GetGame().GetObjectsAtPosition(player_pos, despawn_radius + 5, buildings, proxies);

        for(int i = 0; i < buildings.Count(); i++)
        {
            Object building_object = buildings[i];
            if(building_object)
            {
                vector obj_pos = building_object.GetPosition();
                obj_pos[1] = 0; //we need a 2D distance, not 3D
                string type_name = building_object.GetType();
                if(LootReader.GetReader().ContainsObject(type_name))
                {
                    if(!m_LootableBuildings.Contains(building_object))
                    {
                        Print("First time player near building");
                        Print(building_object);
                        m_LootableBuildings.Insert(building_object, new BattleRoyaleLootableBuilding( building_object ));
                    }
                    //TODO: we may need to serialized the building object in case dayz hands us unique instances every time GetObjectsAtPosition returns
                    ref BattleRoyaleLootableBuilding lootable_building = m_LootableBuildings.Get(building_object);
                    

                    float dist = vector.Distance(player_pos, obj_pos);
                    if(dist < spawn_radius)
                    {
                        //handle spawning loot
                        lootable_building.AddNearPlayer(player);
                    }
                    if(dist > despawn_radius)
                    {
                        //handle despawning loot
                        lootable_building.RemoveNearPlayer(player);
                    }
                }
            }
            else
            {
                Error("ObjectsAtPosition returned NULL object");
            }
            
        }
    }
}



class BattleRoyaleLootableBuilding 
{
    protected Object m_Object;
    protected ref array<ref PlayerBase> a_NearPlayers;
    protected ref array<ref BattleRoyaleLootPile> a_LootPiles;
    protected bool b_Active;
    protected bool b_Initialized;

    

    void BattleRoyaleLootableBuilding( Object obj )
    {
        m_Object = obj;
        a_NearPlayers = new array<ref PlayerBase>();
        a_LootPiles = new array<ref BattleRoyaleLootPile>();
        b_Initialized = false;

        float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_building;
        b_Active = (Math.RandomFloat(0, 1) < odds);
    } 
    void ~BattleRoyaleLootableBuilding()
    {
        Print("BattleRoyaleLootableBuilding deconstructor called");
        delete a_NearPlayers;
        delete a_LootPiles;
    }

    Object GetObject()
    {
        return m_Object;
    }

    protected void InitLoot()
    {
        ref array<vector> model_positions = LootReader.GetReader().GetAllLootPositions(m_Object.GetType());
        for(int i = 0; i < model_positions.Count(); i++)
        {
            a_LootPiles.Insert( new BattleRoyaleLootPile( model_positions[i], this ) );
        }
        b_Initialized = true;
    }

    void AddNearPlayer(ref PlayerBase player)
    {
        if(a_NearPlayers.Find(player) == -1)
        {
            a_NearPlayers.Insert(player);
            if(a_NearPlayers.Count() == 1)
            {
                SpawnLoot();
            }
        }
    }
    void RemoveNearPlayer(ref PlayerBase player)
    {
        if(a_NearPlayers.Find(player) != -1)
        {
            a_NearPlayers.RemoveItem(player);
            if(a_NearPlayers.Count() == 0)
            {
                DespawnLoot();
            }
        }   
    }
    protected void SpawnLoot()
    {
        if(!b_Active)
            return;

        if(!b_Initialized)
            InitLoot();
        
        for(int i = 0; i < a_LootPiles.Count(); i++)
        {
            ref BattleRoyaleLootPile loot_pile = a_LootPiles[i];

            loot_pile.Spawn();
        }
    }
    protected void DespawnLoot()
    {
        if(!b_Active)
            return;

        for(int i = 0; i < a_LootPiles.Count(); i++)
        {
            ref BattleRoyaleLootPile loot_pile = a_LootPiles[i];

            loot_pile.Despawn();
        }
    }
}

class BattleRoyaleLootPile
{
    protected ref BattleRoyaleLootableBuilding m_Parent;
    protected vector v_ModelPosition;
    protected vector v_WorldPosition;

    protected bool b_IsSpawned;
    protected bool b_Active;

    protected ref BattleRoyaleLootEntry m_Entry;

    protected ref array<string> class_names;
    protected ref array<ItemBase> spawned_items;


    void BattleRoyaleLootPile(vector model_pos, ref BattleRoyaleLootableBuilding parent )
    {
        spawned_items = new array<ItemBase>();
        class_names = new array<string>();
        v_ModelPosition = model_pos;
        m_Parent = parent;
        b_IsSpawned = false;

        v_WorldPosition = "0 0 0";


        float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_pile;
        b_Active =(Math.RandomFloat(0, 1) < odds); 
    }
    
    vector GetWorldPos()
    {
        return m_Parent.GetObject().ModelToWorld( v_ModelPosition );
    }

    void Init()
    {
        v_WorldPosition = GetWorldPos(); //calculate our world position
        //select a random loot entry! (this is all done in the background)
        m_Entry = BattleRoyaleLootData.GetData().GetRandomFieldByWeight().GetRandomEntryByWeight();

        string class_name =  m_Entry.GetRandomStyle();
        class_names.Insert( class_name ); //insert base item name
        class_names.InsertAll( m_Entry.spawn_with ); //insert all items that need to spawn with

        int i;
        int index;

        if(m_Entry.SpawnWithMagazines())
        {
            //cached lookup of all possible magazines
            ref array<string> all_mags = BattleRoyaleLootData.GetData().GetAllMagazines( class_name );

            if(all_mags.Count() > 0)
            {
                int num_mags = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).num_mags_to_spawn_with;

                for(i = 0; i < num_mags; i++)
                {
                    index = Math.RandomInt(0, all_mags.Count());
                    class_names.Insert( all_mags[index] );
                }
            }
            else
            {
                Error("Trying to spawn `" + class_name + "` with mags, but none are found!");
            }
            
        }
        if(m_Entry.SpawnWithAmmo())
        {

            ref array<string> all_ammo = BattleRoyaleLootData.GetData().GetAllAmmo( class_name );
            
            if(all_ammo.Count() > 0)
            {
                int num_ammo = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).num_ammoboxs_to_spawn_with;

                for(i = 0; i < num_ammo; i++)
                {
                    index = Math.RandomInt(0, all_ammo.Count());
                    class_names.Insert( all_ammo[index] );
                }
            }
            else
            {
                Error("Trying to spawn `" + class_name + "` with ammo, but none are found!");
            }

        }
        if(m_Entry.SpawnWithAttachments())
        {
            ref array<string> all_attachments = BattleRoyaleLootData.GetData().GetAllAttachments( class_name );

            if(all_attachments.Count() > 0)
            {
                float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_attachment;

                for(i = 0; i < all_attachments.Count(); i++)
                {
                    float roll = Math.RandomFloat(0, 1);
                    if(roll < odds)
                    {
                        class_names.Insert( all_attachments[i] );
                    }
                }
            }
            else
            {
                Error("Trying to spawn `" + class_name + "` with attachments, but none are found!");
            }
        }
    }

    protected ItemBase CreateItem(string class_name)
    {
        return ItemBase.Cast(GetGame().CreateObject(class_name, v_WorldPosition));
    }

    void Spawn()
    {
        if(!b_Active)
            return;

        if(b_IsSpawned)
        {
            Error("Trying to spawn loot that is already spawned!");
            return;
        }
        if(spawned_items.Count() != 0)
        {
            Error("Trying to spawn loot but some items are already spawned!");
            return;
        }
        
        if(!m_Entry)
            Init();
        
        

        Print("Spawning Items");
        for(int i = 0; i < class_names.Count(); i++)
        {
            Print("==> " + class_names[i]);
            ItemBase item = CreateItem( class_names[i] );
            if(item)
            {
                spawned_items.Insert(item);
            }
            else
            {
                Error("Failed to create item `" + class_names[i] + "`!");
            }
        }

        b_IsSpawned = true;
    }
    void Despawn()
    {
        if(!b_Active)
            return;

        if(!b_IsSpawned)
        {
            Error("Trying to despawn loot that is not spawned!");
            return;
        }
        int i;
        
        bool was_moved = false;
        for(i = 0; i < spawned_items.Count(); i++)
        {
            if(!(spawned_items[i]))
            {
                was_moved = true;
                break;
            }
            vector item_pos = spawned_items[i].GetPosition();
            float dist = vector.Distance( item_pos, v_WorldPosition);
            if(dist > 1)
            {
                was_moved = true;
                break;
            }
        }
        if(was_moved)
        {
            b_Active = false;
            return;
        }
        for(i = 0; i < spawned_items.Count(); i++)
        {
            GetGame().ObjectDelete( spawned_items[i] );
        }
        spawned_items.Clear();
    }

}