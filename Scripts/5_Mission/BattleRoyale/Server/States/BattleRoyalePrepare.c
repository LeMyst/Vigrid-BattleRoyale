class BattleRoyalePrepare extends BattleRoyaleState
{
    protected ref array<PlayerBase> m_PlayerList;
    protected ref array<string> a_StartingClothes;
    protected ref array<string> a_StartingItems;
    protected vector world_center;

    private BattleRoyaleGameData m_GameSettings;

    private ref array<ref Town> villages;
    private int villages_index;

    ref array<string> avoid_city_spawn = {"Camp_Shkolnik", "Settlement_Kumyrna", "Ruin_Voron", "Settlement_Skalisty", "Settlement_Novoselki", "Settlement_Dubovo", "Settlement_Vysotovo"};

    void BattleRoyalePrepare()
    {
        m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            a_StartingClothes = m_GameSettings.player_starting_clothes;
            a_StartingItems = m_GameSettings.player_starting_items;
        }
        else
        {
            Error("GameSettings is NULL!");
            a_StartingClothes = {"TrackSuitJacket_Red", "TrackSuitPants_Red", "JoggingShoes_Red"};
            a_StartingItems = {"HuntingKnife", "BandageDressing", "Compass"};
        }

        m_PlayerList = new array<PlayerBase>();

        string path = "CfgWorlds " + GetGame().GetWorldName();
        world_center = GetGame().ConfigGetVector(path + " centerPosition");

        villages_index = 0;
    }

    override void Activate()
    {
        super.Activate();

        //if(BattleRoyaleAPI.GetAPI().ShouldUseApi())
        //{
        //    //this is still like not working properly or something
        //    ServerData m_ServerData = BattleRoyaleAPI.GetAPI().GetCurrentServer();
        //    while(!m_ServerData || m_ServerData.locked == 0)
        //    {
        //        Print("Attempting to lock the server!");
        //        BattleRoyaleAPI.GetAPI().ServerSetLock(true); //Lock the server
        //        m_ServerData = BattleRoyaleAPI.GetAPI().GetServer(m_ServerData._id);
        //    }
        //}

        //TODO: spawn & setup drop plane
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(true), true); //fade out screen

        //we process on a static list so when players possibly disconnect during this phase we don't error out or skip any clients
        m_PlayerList.InsertAll( m_Players ); //this gave an error when using m_Players

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input on all clients (we'll do this on the server in another thread)

        Print("Reset date time to random");
        int year, month, day, hour, minute;
        GetGame().GetWorld().GetDate(year, month, day, hour, minute);
        GetGame().GetWorld().SetDate(year, month, day, Math.RandomIntInclusive(7, 14), 0);

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
        for (int i = 0; i < cCount; i++) {
            EntityAI clothes = process_player.GetInventory().CreateAttachment(a_StartingClothes[i]);
            if(!item_spawned && clothes.GetInventory().GetCargo())
            {
                int iCount = a_StartingItems.Count();
                for (int j = 0; j < iCount; j++) {
                    clothes.GetInventory().CreateEntityInCargo(a_StartingItems[j]);
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

                    if(!area.IsAreaOverlap(new BattleRoyalePlayArea(town_entry.Position, village_pad)))
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
            foreach(Town village : villages)
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
            if(!BattleRoyaleZone.GetZone(1).IsInZone(x, z))
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

        float x = Math.RandomFloatInclusive((village_x - village_pad), (village_x + village_pad));
        float z = Math.RandomFloatInclusive((village_z - village_pad), (village_z + village_pad));
        float y = GetGame().SurfaceY(x, z);

        Print("Trying to spawn player to " + village.Name + " (" + Town.GetTownTypeString(village.Type) + ") with a radius of " + village_pad);

        return Vector(x, y, z);
    }

    protected ref Param2<vector, Town> GetRandomSpawnPosition()
    {
        vector random_pos = "0 0 0";
        Town village;

        int spawn_try = 1;

        while(true)
        {
            Print("Try " + spawn_try);
            spawn_try = spawn_try + 1;
            float edge_pad = 0.1;

            float x = Math.RandomFloatInclusive((world_center[0] * edge_pad), (world_center[0] * 2) - (world_center[0] * edge_pad));
            float z = Math.RandomFloatInclusive((world_center[1] * edge_pad), (world_center[1] * 2) - (world_center[1] * edge_pad));
            float y = GetGame().SurfaceY(x, z);
            random_pos[0] = x;
            random_pos[1] = y;
            random_pos[2] = z;

            village = NULL;
            bool check_zone = true;
            if (m_GameSettings.spawn_in_villages && spawn_try <= 50)
            {
                if(m_GameSettings.spawn_in_first_zone)
                {
                    BattleRoyaleUtils.Trace("Spawn in village area");
                    village = GetRandomVillage(BattleRoyaleZone.GetZone(1).GetArea(), true);
                    check_zone = false;  // We got a village in zone, don't need to check if the player will spawn in zone
                }
                else
                    village = GetRandomVillage();

                if (village != NULL && village.Name != "")
                {
                    random_pos = GetRandomVillagePosition(village);
                } else {
                    Print("Another fucked up village!");
                    continue;
                }
            }

            if(!IsSafeForTeleport(random_pos[0], random_pos[1], random_pos[2], check_zone))
                continue;

            break;
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
            BattleRoyaleUtils.Trace("Teleport player " + player.GetIdentity().GetName() + " to position " + position);

            float x = position[0] + Math.RandomFloatInclusive(-3.0, 3.0);
            float z = position[2] + Math.RandomFloatInclusive(-3.0, 3.0);
            float y = GetGame().SurfaceY(x, z);

            TeleportPlayer(player, Vector(x, y, z), village);
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
            float dir = Math.RandomFloat(0,360); //non-inclusive, 360==0
            vector playerDir = vector.YawToVector(dir);
            player.SetDirection(Vector(playerDir[0], 0, playerDir[1]));
        }
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

#ifdef SCHANAMODPARTY
        BattleRoyaleUtils.Trace("Mod party enabled");

        ref array<ref set<PlayerBase>> teleport_groups = GetGroups();

        // Teleport groups
        int pGroupCount = teleport_groups.Count();
        BattleRoyaleUtils.Trace("Groups: " + pGroupCount);
        for (i = 0; i < pGroupCount; i++) {
            BattleRoyaleUtils.Trace("Teleport group " + i);
            if (process_player) TeleportGroup(teleport_groups.Get(i));
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
        foreach(PlayerBase player : players)
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
