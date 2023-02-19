class BattleRoyalePrepare extends BattleRoyaleState
{
    protected ref array<PlayerBase> m_PlayerList;
    protected ref array<string> a_StartingClothes;
    protected ref array<string> a_StartingItems;
    protected vector world_center;

    private BattleRoyaleGameData m_GameSettings;

    private ref array<ref Town> villages;

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

    protected Town GetRandomVillage()
    {
        // https://github.com/InclementDab/DayZ-Dabs-Framework/blob/production/DabsFramework/Scripts/3_Game/DabsFramework/Town/TownFlags.c
        if(!villages)
        {
            villages = new array<ref Town>;

            //ref array<ref Town> temp_villages = Town.GetMapTowns(TownFlags.CAPITAL | TownFlags.CITY | TownFlags.VILLAGE | TownFlags.CAMP);
            ref array<ref Town> temp_villages = new array<ref Town>;

            string world_name = "";
            GetGame().GetWorldName(world_name);
            string cfg = "CfgWorlds " + world_name + " Names";
            Print(cfg);
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

                Print("cfg "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+city_position+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city)));

                Town town_entry();
                town_entry.Entry = city;
                town_entry.Type = Town.GetTownFlag(GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city)));
                town_entry.Name = GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city));
                town_entry.Position = city_position;
                temp_villages.Insert(town_entry);
            }

            Print(temp_villages);
            for(int j = 0; j < temp_villages.Count(); j++)
            {
                ref Town temp_village = temp_villages[j];
                Print("- " + j + ". " + temp_village.Name + " (" + Town.GetTownTypeString(temp_village.Type) + ")");

                if(temp_village.Name == "" || Town.GetTownTypeString(temp_village.Type) == "") // useless ?
                    continue;

                if(temp_village.Name == "Kumyrna")  // Ruins
                    continue;

                if(temp_village.Name == "Skalisty")  // South-east island
                    continue;

                if(temp_village.Name == "Shkolnik")  // North-east BR lobby
                    continue;

                villages.Insert(temp_village);
            }

            Print(villages);
        }
        Town random_village = villages[Math.RandomInt(0, villages.Count())];
        return random_village;
    }

    protected void Teleport(PlayerBase process_player)
    {
        vector random_pos = "0 0 0";
        Town village;

        BattleRoyaleZone m_Zone = new BattleRoyaleZone;
        m_Zone = m_Zone.GetZone(1);

        int spawn_try = 1;

        while(true)
        {
            Print("Try " + spawn_try);
            spawn_try = spawn_try + 1;
            float edge_pad = 0.1;

            float x = Math.RandomFloatInclusive((world_center[0] * edge_pad), (world_center[0] * 2) - (world_center[0] * edge_pad));
            float z = Math.RandomFloatInclusive((world_center[1] * edge_pad), (world_center[1] * 2) - (world_center[1] * edge_pad));
            float y = GetGame().SurfaceY(x, z);

            if (m_GameSettings.spawn_in_villages && spawn_try < 100)
            {
                village = GetRandomVillage();
                if (village.Name != "")
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

                    x = Math.RandomFloatInclusive((village_x - village_pad), (village_x + village_pad));
                    z = Math.RandomFloatInclusive((village_z - village_pad), (village_z + village_pad));
                    y = GetGame().SurfaceY(x, z);

                    Print("Trying to spawn player " + process_player.GetIdentity().GetName() + " to " + village.Name + " (" + Town.GetTownTypeString(village.Type) + ") with a radius of " + village_pad);
                } else {
                    Print("Another fucked up village!");
                    continue;
                }
            }

            random_pos[0] = x;
            random_pos[1] = y;
            random_pos[2] = z;

            //check if random_pos is bad
            if(GetGame().SurfaceIsSea(x, z))
                continue;

            if(GetGame().SurfaceIsPond(x, z))
                continue;

            if(GetGame().SurfaceRoadY(x, z) != y)
                continue;

            if(!m_Zone.IsInZone(x, z))
                continue;

            ref array<string> bad_surface_types = {
                "nam_seaice",
                "nam_lakeice_ext"
            };

            string surface_type;
            GetGame().SurfaceGetType(x, z, surface_type);
            if(bad_surface_types.Find(surface_type) != -1)
                continue;

            vector start = random_pos + Vector( 0, 5, 0 );
            vector end = random_pos;
            float radius = 2.0;

            PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
            Object m_HitObject;
            vector m_HitPosition;
            vector m_HitNormal;
            float m_HitFraction;
            bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );

            if(m_Hit)
                continue;

            break;
        }

        Print("Spawn player " + process_player.GetIdentity().GetName() + " at " + random_pos);

        //set pos
        process_player.SetPosition(random_pos);

        //TODO: make sure the retarded game engine doesn't keep the player in a swimming state ????
        //TODO: force stand up

        if (village)
        {
            process_player.SetDirection(vector.Direction(process_player.GetPosition(), village.Position));
        } else {
            //random direction
            float dir = Math.RandomFloat(0,360); //non-inclusive, 360==0
            vector playerDir = vector.YawToVector(dir);
            process_player.SetDirection(Vector(playerDir[0], 0, playerDir[1]));
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

        for (i = 0; i < pCount; i++) {
            process_player = m_PlayerList[i];
            if (process_player) Teleport(process_player);

            Sleep(100);
        }
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
