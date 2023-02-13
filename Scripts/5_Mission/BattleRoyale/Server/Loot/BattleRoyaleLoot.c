#ifdef BR_LOOT
class BattleRoyaleLoot
{
    protected bool b_Enabled;
    protected bool b_IsProcessing;
    protected ref Timer m_TickTimer;

    protected ref array<PlayerBase> m_Players;
    protected ref map<Object, ref BattleRoyaleLootableBuilding> m_LootableBuildings;

    void BattleRoyaleLoot()
    {
        Print("BattleRoyaleLoot");
        m_TickTimer = new Timer;

        m_LootableBuildings = new map<Object, ref BattleRoyaleLootableBuilding>();
        m_Players = new array<PlayerBase>();
        b_IsProcessing = false;

        m_TickTimer.Run( 0.5, this, "HandleTick", NULL, true);

        string mission_name = BattleRoyaleConfig.GetConfig().GetGameData().mission;
        Print( "Read $CurrentDir:mpmissions\\" + mission_name + "\\mapgroupproto.xml" );
        LootReader.GetReader().ReadAsync( "$mission:mapgroupproto.xml" );

        BattleRoyaleLootData.GetData(); //--- this will call LootData.Load() so this is enough
    }

    void ~BattleRoyaleLoot()
    {
        m_TickTimer.Stop();
        delete m_LootableBuildings;
        delete m_Players;
    }

    void Update(float delta)
    {
    }

    void AddPlayer(PlayerBase player)
    {
        if(m_Players.Find(player) == -1)
        {
            m_Players.Insert(player);
        }
    }

    void RemovePlayer(PlayerBase player)
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
        if(!b_Enabled)
            return;

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
        ref array<PlayerBase> m_PlayerClone = new array<PlayerBase>();
        int i;
        PlayerBase player;

        m_PlayerClone.InsertAll( m_Players );

        //Note: the code below existed because NULL entries were causing errors when using a foreach (we haven't tested the above code however)
        /*
        for(i = 0; i < m_Players.Count(); i++)
        {
            player = m_Players[i];
            m_PlayerClone.Insert(player);
        }
        */

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

    void ProcessPlayerLoot(PlayerBase player)
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
                        Print("First time player near that building");
                        Print(building_object);
                        m_LootableBuildings.Insert(building_object, new BattleRoyaleLootableBuilding( building_object ));
                    }
                    //we may need to serialized the building object in case dayz hands us unique instances every time GetObjectsAtPosition returns
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
#endif //BR_LOOT
