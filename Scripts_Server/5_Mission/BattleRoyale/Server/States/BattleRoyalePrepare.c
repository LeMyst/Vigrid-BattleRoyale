#ifdef SERVER
class BattleRoyalePrepare: BattleRoyaleState
{
    protected ref array<PlayerBase> m_PlayerList;
    protected ref array<string> a_StartingClothes;
    protected ref array<string> a_StartingItems;

    private BattleRoyaleGameData m_GameSettings;

    private ref array<ref Town> villages;
    private int villages_index;

    private string last_village_spawn = "";

    ref array<string> avoid_city_spawn;

    void BattleRoyalePrepare()
    {
        m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            a_StartingClothes = m_GameSettings.player_starting_clothes;
            a_StartingItems = m_GameSettings.player_starting_items;
            avoid_city_spawn = m_GameSettings.avoid_city_spawn;
        }
        else
        {
            Error("GameSettings is NULL!");
            a_StartingClothes = {"TrackSuitJacket_Red", "TrackSuitPants_Red", "JoggingShoes_Red"};
            a_StartingItems = {"HuntingKnife", "BandageDressing", "Compass"};
            avoid_city_spawn = {"Camp_Shkolnik", "Hill_Zelenayagora", "Settlement_Kumyrna", "Ruin_Voron", "Settlement_Skalisty", "Settlement_Novoselki", "Settlement_Dubovo", "Settlement_Vysotovo"};
        }

        m_PlayerList = new array<PlayerBase>();

        string path = "CfgWorlds " + GetGame().GetWorldName();

        villages_index = 0;
    }

    override void Activate()
    {
        super.Activate();

        //TODO: spawn & setup drop plane
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(true), true); //fade out screen

        //we process on a static list so when players possibly disconnect during this phase we don't error out or skip any clients
        m_PlayerList.InsertAll( m_Players ); //this gave an error when using m_Players

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input on all clients (we'll do this on the server in another thread)

        Print("Reset date time to random");
        int year, month, day, hour, minute;
        GetGame().GetWorld().GetDate(year, month, day, hour, minute);
        GetGame().GetWorld().SetDate(year, month, day, Math.RandomIntInclusive(6, 13), 0);

#ifdef DYNAMIC_NUM_ZONES
        // Found the first zone based on number of registered players
        int pCount = m_PlayerList.Count();
        int last_try_zone = 1;
        Print("Number of players registered: " + pCount);
        BattleRoyaleZoneData m_ZoneSettings = BattleRoyaleConfig.GetConfig().GetZoneData();
        for(int i_zone = 1; i_zone < m_GameSettings.num_zones; i_zone++)
        {
            Print("Try zone: " + i_zone);
            last_try_zone = i_zone;
            Print("Min player for zone: " + BattleRoyaleZone.GetZone(i_zone).GetZoneMinPlayers());
            if(BattleRoyaleZone.GetZone(i_zone).GetZoneMinPlayers() < pCount)
            {
                Print("It's a match! " + i_zone);
                break;
            }
            if(i_zone == m_GameSettings.num_zones - m_ZoneSettings.min_zone_num)
            {
                Print("Reach the minimum! " + i_zone);
                break;
            }
            Print("No chance, we continue...");
        }
        i_StartingZone = last_try_zone;
#endif

        Print("Starting zone will be " + i_StartingZone);

        GetGame().GameScript.Call(this, "ProcessPlayers", NULL); //Spin up a new thread to process giving players items and teleporting them
    }

    override void Deactivate()
    {
        super.Deactivate();

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(false), true); //fade out screen
    }

    override bool IsComplete()
    {
        //Print(GetName() + " IsComplete!");
        return super.IsComplete();
    }

    override string GetName()
    {
        return DAYZBR_SM_PREPARE_NAME;
    }

    protected bool DeleteAllItems(PlayerBase process_player)
    {
        if ( process_player == NULL )
            return false;
        
        if (process_player.GetInventory().CountInventory() > 0)
        {
            process_player.RemoveAllItems();
        }

        return true;
    }

    protected bool AddStartItems(PlayerBase process_player)
    {
        DeleteAllItems(process_player);

        int cCount = a_StartingClothes.Count();
        bool item_spawned = false;
        EntityAI new_item;
        for (int i = 0; i < cCount; i++)
        {
            EntityAI clothes = process_player.GetInventory().CreateAttachment(a_StartingClothes[i]);
            if(!item_spawned && clothes && clothes.GetInventory() && clothes.GetInventory().GetCargo())
            {
                int iCount = a_StartingItems.Count();
                for (int j = 0; j < iCount; j++)
                {
                    new_item = clothes.GetInventory().CreateEntityInCargo(a_StartingItems[j]);
                    if( a_StartingItems[j] == "HuntingKnife" )
                        process_player.SetQuickBarEntityShortcut(new_item, 0);
                }
                item_spawned = true;
            }
        }

        return true;
    }

    protected void GiveStartingItems(PlayerBase process_player)
    {
        //drop the item out of the player's hands before we delete it
        if(process_player.GetItemInHands())
        {
            ItemBase item_inhands = process_player.GetItemInHands();
            process_player.ServerDropEntity( item_inhands );
            //GetGame().GameScript.Call(GetGame(), "ObjectDelete", item_inhands);
            GetGame().ObjectDelete( item_inhands );
        }
        DeleteAllItems(process_player);
        AddStartItems(process_player);
    }

    protected void DisableInput(PlayerBase process_player)
    {
        process_player.DisableInput(true);
    }

    protected Town GetRandomVillage(BattleRoyalePlayArea area = NULL, bool use_radius = false)
    {
        // https://github.com/InclementDab/DayZ-Dabs-Framework/blob/production/DabsFramework/Scripts/3_Game/DabsFramework/Town/TownFlags.c
        if(!villages)
        {
            villages = new array<ref Town>;

            //ref array<ref Town> temp_villages = Town.GetMapTowns(TownFlags.CAPITAL | TownFlags.CITY | TownFlags.VILLAGE | TownFlags.CAMP);

            string world_name = "";
            GetGame().GetWorldName(world_name);
            string cfg = "CfgWorlds " + world_name + " Names";
            BattleRoyaleUtils.Trace(cfg);
            for (int i = 0; i < GetGame().ConfigGetChildrenCount(cfg); i++) {
                string city;
                GetGame().ConfigGetChildName(cfg, i, city);
                vector city_position;
                // TODO: Override city position from config file
                TFloatArray float_array = {};
                GetGame().ConfigGetFloatArray(string.Format("%1 %2 position", cfg, city), float_array);
                city_position[0] = float_array[0]; city_position[2] = float_array[1];
                city_position[1] = GetGame().SurfaceY(city_position[0], city_position[2]);

                string town_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city));
                if(town_type != "Capital" && town_type != "City" && town_type != "Village")
                    continue;

                BattleRoyaleUtils.Trace("cfg "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+city_position+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city)));

                Town town_entry();
                town_entry.Entry = city;
                town_entry.Type = Town.GetTownFlag(GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city)));
                town_entry.Name = GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city));
                town_entry.Position = city_position;

                BattleRoyaleUtils.Trace("- " + i + ". " + town_entry.Name + " (" + Town.GetTownTypeString(town_entry.Type) + ")");

                if(town_entry.Name == "" || Town.GetTownTypeString(town_entry.Type) == "") // useless ?
                    continue;

                // Check if city Entry is not in the avoid spawn list
                if(avoid_city_spawn.Find(town_entry.Entry) != -1)
                    continue;

                if(m_GameSettings.spawn_in_first_zone && area != NULL)
                {
                    float village_pad;

                    if(use_radius)
                    {
                        if (town_entry.Type == TownFlags.CITY)
                            village_pad = 300.0;
                        else if (town_entry.Type == TownFlags.CAPITAL)
                            village_pad = 500.0;
                        else
                            village_pad = 150.0;
                    }
                    else
                        village_pad = 0.0;

                    if(!area.IsAreaOverlap(new BattleRoyalePlayArea(town_entry.Position, village_pad), m_GameSettings.extra_spawn_radius))
                        continue;
                }

                int pond = 1;
                if(town_entry.Type == TownFlags.CAPITAL)
                    pond = 5;
                else if(town_entry.Type == TownFlags.CITY)
                    pond = 3;

                for(int p = 0; p < pond; p++)
                    villages.Insert(town_entry);
            }

            // Add weighting


            // Randomize order of villages
            villages.ShuffleArray();

            BattleRoyaleUtils.Trace("Final village list:");
            foreach(Town village: villages)
            {
                BattleRoyaleUtils.Trace("- " + village.Name + " (" + village.Type + ")");
            }
        }

        if(villages.Count() > 0)
        {
            //return villages[Math.RandomInt(0, villages.Count())];
            if(villages_index > villages.Count()+1)
                villages_index = 0;

            return villages[villages_index++];
        }
        else
            return NULL;
    }

    protected bool IsSafeForTeleport(float x, float y, float z, bool check_zone = true)
    {
        //check if random_pos is bad
        if(GetGame().SurfaceIsSea(x, z))
            return false;

        if(GetGame().SurfaceIsPond(x, z))
            return false;

        if(GetGame().SurfaceRoadY(x, z) != y)
            return false;

        if(check_zone && m_GameSettings.spawn_in_first_zone)
        {
            if(!BattleRoyaleZone.GetZone(i_StartingZone).IsInZone(x, z))
                return false;
        }

        ref array<string> bad_surface_types = {
            "nam_seaice",
            "nam_lakeice_ext"
        };

        string surface_type;
        GetGame().SurfaceGetType(x, z, surface_type);
        if(bad_surface_types.Find(surface_type) != -1)
            return false;

        vector position;
        position[0] = x;
        position[1] = y;
        position[2] = z;

        vector start = position + Vector( 0, 5, 0 );
        vector end = position;
        float radius = 2.0;

        PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
        Object m_HitObject;
        vector m_HitPosition;
        vector m_HitNormal;
        float m_HitFraction;
        bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );

        if(m_Hit)
            return false;

        // New Geometry check
        array<Object> excludedObjects = new array<Object>();
        array<Object> collidedObjects = new array<Object>();
        vector box_position;
        box_position[0] = position[0];
        box_position[1] = position[1]; // + 1
        box_position[2] = position[2];
        if( GetGame().IsBoxCollidingGeometry(box_position, "0 0 0", "1 5 1", ObjIntersectFire, ObjIntersectGeom, excludedObjects, collidedObjects) )
        {
            if( collidedObjects.Count() > 0)
            {
                BattleRoyaleUtils.Trace("New IsSafeForTeleport Geometry check is true !");
                Print( collidedObjects );
                for (int i = 0; i < collidedObjects.Count(); ++i)
                {
                    string objectClass = collidedObjects.Get(i).GetType();
                    BattleRoyaleUtils.Trace( "objectClass: " + objectClass );
                }

                string text = "";
                foreach (Object object: collidedObjects)
                    text += " | " + Object.GetDebugName(object);

                return false;
            }
        }

        return true;
    }

    protected vector GetRandomVillagePosition(Town village)
    {
        float village_x = village.Position[0];
        float village_z = village.Position[2];
        float village_pad;

        if (village.Type == TownFlags.CITY)
            village_pad = 300.0;
        else if (village.Type == TownFlags.CAPITAL)
            village_pad = 500.0;
        else
            village_pad = 100.0;

        float radius, angle, x, z;
        radius = village_pad * Math.Sqrt( Math.RandomFloat(0, 1) );
        angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
        x = village_x + ( radius * Math.Cos(angle) );
        z = village_z + ( radius * Math.Sin(angle) );

        float y = GetGame().SurfaceY(x, z);

        Print("Trying to spawn player to " + village.Name + " (" + Town.GetTownTypeString(village.Type) + ") with a radius of " + village_pad);

        return Vector(x, y, z);
    }

    protected ref Param2<vector, Town> GetRandomSpawnPosition()
    {
        vector random_pos = "0 0 0";
        Town village;

        if (m_GameSettings.spawn_in_villages)
        {
            vector village_pos = "0 0 0";
            BattleRoyaleUtils.Trace("Spawn in zone " + i_StartingZone);
            ref BattleRoyalePlayArea spawn_area = BattleRoyaleZone.GetZone(i_StartingZone).GetArea();
            for(int village_spawn_try = 1; village_spawn_try <= 200; village_spawn_try++)
            {
                BattleRoyaleUtils.Trace("Try to spawn in village " + village_spawn_try);

                village = NULL;
                bool check_zone = true;
                if(m_GameSettings.spawn_in_first_zone)
                {
                    village = GetRandomVillage(spawn_area, true);
                    check_zone = false;  // We got a village in zone, don't need to check if the player will spawn in zone
                }
                else
                    village = GetRandomVillage();

                if (village != NULL && village.Name != "")
                {
                    BattleRoyaleUtils.Trace("Found village " + village.Entry);
                    vector search_for_village = "0 0 0";

                    if( village.Entry == last_village_spawn )
                    	continue;

                    for(int search_pos = 1; search_pos <= 50; search_pos++)
                    {
                        BattleRoyaleUtils.Trace("Try to find a position in village " + search_pos);
                        search_for_village = GetRandomVillagePosition(village);

                        if(!IsSafeForTeleport(search_for_village[0], search_for_village[1], search_for_village[2], check_zone))
                            continue;

                        village_pos = search_for_village;
                        break;
                    }

                    if( village_pos == "0 0 0" )
                        continue;

                    BattleRoyaleUtils.Trace("Found village position " + village_pos);
                    random_pos = village_pos; // Found a valid village position
                    if( village.Type == TownFlags.CAPITAL || village.Type == TownFlags.CITY )
                    	last_village_spawn = village.Entry; // Save last village spawn to avoid it next time
                    break;
                } else {
                    Print("Another fucked up village!");
                }
            }
        }

        // If at this step we always have a zero vector, try to find a random one
        if( random_pos == "0 0 0" )
        {
            int random_spawn_try = 1;
            while(true)
            {
                Print("Try to spawn at random position " + random_spawn_try);
                random_spawn_try++;
                float edge_pad = 0.1;

                int world_size = GetGame().GetWorld().GetWorldSize();

                float x = Math.RandomFloatInclusive((world_size * edge_pad), world_size - (world_size * edge_pad));
                float z = Math.RandomFloatInclusive((world_size * edge_pad), world_size - (world_size * edge_pad));
                float y = GetGame().SurfaceY(x, z);

                random_pos[0] = x;
                random_pos[1] = y;
                random_pos[2] = z;

                if(!IsSafeForTeleport(random_pos[0], random_pos[1], random_pos[2], true))
                    continue;

                break;
            }
        } else {
            BattleRoyaleUtils.Trace("Already found a random pos from a village!");
        }

        return new Param2<vector, Town>(random_pos, village);
    }

    protected void Teleport(PlayerBase process_player)
    {
        ref Param2<vector, Town> random_pos = GetRandomSpawnPosition();
        vector position = random_pos.param1;
        Town village = random_pos.param2;

        TeleportPlayer(process_player, position, village);
    }

    protected void TeleportGroup(ref set<PlayerBase> group)
    {
        ref Param2<vector, Town> random_pos = GetRandomSpawnPosition();
        vector position = random_pos.param1;
        Town village = random_pos.param2;

        int tmpNbPlayers = group.Count();
        for(int i = 0; i < tmpNbPlayers; i++)
        {
            PlayerBase player = group.Get(i);
            if(player)
            {
                BattleRoyaleUtils.Trace("Teleport player " + player.GetIdentity().GetName() + " to position " + position);

                int spawn_try = 1;
                while(true)
                {
                    Print("Try Group " + spawn_try);
                    spawn_try = spawn_try + 1;
                    float x = position[0] + Math.RandomFloatInclusive(-5.0, 5.0);
                    float z = position[2] + Math.RandomFloatInclusive(-5.0, 5.0);
                    float y = GetGame().SurfaceY(x, z);

                    if( IsSafeForTeleport(x, y, z, false) )
                        break;

                    if( spawn_try > 50 )
                        break;
                }

                TeleportPlayer(player, Vector(x, y, z), village);
            }
        }
    }

    protected void TeleportPlayer(PlayerBase player, vector position, Town village = NULL)
    {
        BattleRoyaleUtils.Trace("Spawn player " + player.GetIdentity().GetName() + " at " + position);

        //set pos
        player.SetPosition(position);

        //TODO: make sure the retarded game engine doesn't keep the player in a swimming state ????
        //TODO: force stand up

        if (village)
        {
            player.SetDirection(vector.Direction(player.GetPosition(), village.Position));
        } else {
            //random direction
            float dir = Math.RandomFloat(0, 360); //non-inclusive, 360==0
            vector playerDir = vector.YawToVector(dir);
            player.SetDirection(Vector(playerDir[0], 0, playerDir[1]));
        }

        player.SetSynchDirty();
    }

    void ProcessPlayers()
    {
        BattleRoyaleUtils.Trace("Starting to process players...");
        int i;
        PlayerBase process_player;

        int pCount = m_PlayerList.Count();
        for (i = 0; i < pCount; i++) {
            process_player = m_PlayerList[i];
            if (process_player) DisableInput(process_player);

            Sleep(100);
        }
        BattleRoyaleUtils.Trace("Players are disabled");

        m_PlayerList.ShuffleArray();

#ifdef SCHANAMODPARTY
        BattleRoyaleUtils.Trace("Mod party enabled");

        // ref array ref set, what in the seven fucks is this ?
        ref array<ref set<PlayerBase>> teleport_groups = GetGroups();

        // Teleport groups
        int pGroupCount = teleport_groups.Count();
        teleport_groups.ShuffleArray();
        BattleRoyaleUtils.Trace("Groups: " + pGroupCount);
        for (i = 0; i < pGroupCount; i++) {
            BattleRoyaleUtils.Trace("Teleport group " + i);
            ref set<PlayerBase> group = teleport_groups.Get(i);
            if ( group.Count() > 1 )
            {
                TeleportGroup( group );
            } else {
                process_player = group.Get(0);
                if (process_player) Teleport(process_player);
            }
            Sleep(100);
        }
#else
        for (i = 0; i < pCount; i++) {
            process_player = m_PlayerList[i];
            if (process_player) Teleport(process_player);

            Sleep(100);
        }
#endif
        BattleRoyaleUtils.Trace("Teleported players");

        // plz fix this
        Sleep(1000);

        for (i = 0; i < pCount; i++) {
            process_player = m_PlayerList[i];
            if (process_player) GiveStartingItems(process_player);

            Sleep(100);
        }
        
        BattleRoyaleUtils.Trace("Gave starting items");

        for (i = 0; i < pCount; i++) {
            process_player = m_PlayerList[i];
            if (process_player) process_player.ResetPlayer(true);

            Sleep(100);
        }

        BattleRoyaleUtils.Trace("Healed players");
        Deactivate();
    }

    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        super.OnPlayerTick(player, timeslice);

        if(player.time_until_heal <= 0)
        {
            player.time_until_heal = 5;
            player.Heal();
        }

        player.time_until_heal -= timeslice;
    }

    override void AddPlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(false); //all players in this state are god mode
            player.Heal();
        }

        super.AddPlayer(player);
    }

    override ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> players = super.RemoveAllPlayers();
        foreach(PlayerBase player: players)
        {
            player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }

        return players;
    }

    override void RemovePlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }

        super.RemovePlayer(player);
    }
}
