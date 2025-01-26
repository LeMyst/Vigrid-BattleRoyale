#ifdef SERVER
class BattleRoyalePrepare: BattleRoyaleState
{
    protected ref array<PlayerBase> m_PlayerList;
    protected ref array<string> a_StartingClothes;
    protected ref array<string> a_StartingItems;

	private BattleRoyaleConfig m_Config;
    private BattleRoyaleGameData m_GameSettings;
    private BattleRoyaleSpawnsData m_SpawnsSettings;
    private BattleRoyaleServerData m_ServerData;

    private ref array<ref NamedLocation> villages;
    private int villages_index;

    private string last_village_spawn = "";

    ref array<string> avoid_city_spawn;

    ref map<string, vector> m_OverrideSpawnPositions;

    void BattleRoyalePrepare()
    {
        m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        m_SpawnsSettings = BattleRoyaleConfig.GetConfig().GetSpawnsData();
        m_ServerData = BattleRoyaleConfig.GetConfig().GetServerData();
        if(m_GameSettings)
        {
            a_StartingClothes = m_GameSettings.player_starting_clothes;
            a_StartingItems = m_GameSettings.player_starting_items;
            avoid_city_spawn = m_SpawnsSettings.avoid_city_spawn;
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

        LockServerWebhook serverWebhook = new LockServerWebhook( m_ServerData.webhook_jwt_token );
        serverWebhook.LockServer();

        //TODO: spawn & setup drop plane
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(true), true); //fade out screen

        //we process on a static list so when players possibly disconnect during this phase we don't error out or skip any clients
        m_PlayerList.InsertAll( m_Players ); //this gave an error when using m_Players

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input on all clients (we'll do this on the server in another thread)

        BattleRoyaleUtils.Trace("Reset date time to random");
        int year, month, day, hour, minute;
        GetGame().GetWorld().GetDate(year, month, day, hour, minute);
        GetGame().GetWorld().SetDate(year, month, day, Math.RandomIntInclusive(6, 12), 0);

        BattleRoyaleZoneData m_ZoneSettings = BattleRoyaleConfig.GetConfig().GetZoneData();
		if ( m_ZoneSettings.use_dynamic_zones )
		{
			// Found the first zone based on number of registered players
			int pCount = m_PlayerList.Count();
			int last_try_zone = 1;
			BattleRoyaleUtils.Trace("Number of players registered: " + pCount);
			for(int i_zone = 1; i_zone < m_GameSettings.num_zones; i_zone++)
			{
				BattleRoyaleUtils.Trace("Try zone: " + i_zone);
				last_try_zone = i_zone;
				BattleRoyaleUtils.Trace("Min player for zone: " + BattleRoyaleZone.GetZone(i_zone).GetZoneMinPlayers());
				if(BattleRoyaleZone.GetZone(i_zone).GetZoneMinPlayers() < pCount)
				{
					BattleRoyaleUtils.Trace("It's a match! " + i_zone);
					break;
				}
				if(i_zone == m_GameSettings.num_zones - m_ZoneSettings.min_zone_num)
				{
					BattleRoyaleUtils.Trace("Reach the minimum! " + i_zone);
					break;
				}
				BattleRoyaleUtils.Trace("No chance, we continue...");
			}
			i_StartingZone = last_try_zone;
        }

        BattleRoyaleUtils.Trace("Starting zone will be " + i_StartingZone);

        GetGame().GameScript.Call(this, "ProcessPlayers", NULL); //Spin up a new thread to process giving players items and teleporting them
    }

    override void Deactivate()
    {
        super.Deactivate();

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(false), true); //fade out screen
    }

    override bool IsComplete()
    {
        //BattleRoyaleUtils.Trace(GetName() + " IsComplete!");
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

    protected void DisablePlayerInput(PlayerBase process_player)
    {
        process_player.DisableInput(true);
    }

    protected NamedLocation GetRandomVillage(BattleRoyalePlayArea area = NULL, bool use_radius = false)
    {
		BattleRoyaleUtils.Trace("GetRandomVillage()");
		if( !m_OverrideSpawnPositions )
		{
			m_OverrideSpawnPositions = new map<string, vector>();

			foreach(BattleRoyaleOverrideSpawnPosition position: m_SpawnsSettings.override_spawn_positions)
			{
				BattleRoyaleUtils.Trace(position.city_name + " " + position.new_position);
				vector temp_pos;
				temp_pos[0] = position.new_position[0];
				temp_pos[2] = position.new_position[1];
				temp_pos[1] = GetGame().SurfaceY( temp_pos[0], temp_pos[2] );
				m_OverrideSpawnPositions.Set( position.city_name, temp_pos );
			}
		}

        // https://github.com/InclementDab/DayZ-Dabs-Framework/blob/production/DabsFramework/Scripts/3_Game/DabsFramework/!Core/NamedLocation.c
        if(!villages)
        {
            villages = new array<ref NamedLocation>;

            //ref array<ref NamedLocation> temp_villages = NamedLocation.GetMapTowns(TownFlags.CAPITAL | TownFlags.CITY | TownFlags.VILLAGE | TownFlags.CAMP);

            string world_name = "";
            GetGame().GetWorldName(world_name);
            string cfg = "CfgWorlds " + world_name + " Names";
            BattleRoyaleUtils.Trace(cfg);
            for (int i = 0; i < GetGame().ConfigGetChildrenCount(cfg); i++) {
				string city;
				GetGame().ConfigGetChildName(cfg, i, city);
//				vector city_position;
				// TODO: Override city position from config file

//				if( m_OverrideSpawnPositions.Contains(city) )
//				{
//					city_position = m_OverrideSpawnPositions.Get( city );
//					BattleRoyaleUtils.Trace("Override " + city + " position!");
//				}
//				else
//				{
//					TFloatArray float_array = {};
//					GetGame().ConfigGetFloatArray(string.Format("%1 %2 position", cfg, city), float_array);
//					city_position[0] = float_array[0]; city_position[2] = float_array[1];
//					city_position[1] = GetGame().SurfaceY(city_position[0], city_position[2]);
//				}


				string town_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city));
				if(town_type != "Capital" && town_type != "City" && town_type != "Village")
					continue;

				BattleRoyaleUtils.Trace("cfg "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city)));

				NamedLocation town_entry = new NamedLocation(string.Format("%1 %2", cfg, city));
//				town_entry.Entry = city;
//				town_entry.Type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city));
//				town_entry.Name = GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city));
//				town_entry.Position = city_position;

				BattleRoyaleUtils.Trace("- " + i + ". " + town_entry.Name + " (" + town_entry.Type + ")");

				if(town_entry.Name == "" || town_entry.Type == "") // useless ?
					continue;

				// Check if city Entry is not in the avoid spawn list
				if(avoid_city_spawn.Find(town_entry.Name) != -1)
					continue;

				if(m_SpawnsSettings.spawn_in_first_zone && area != NULL)
				{
					float village_pad;

					if(use_radius)
					{
						if (town_entry.Type == NamedLocation.CITY)
							village_pad = 300.0;
						else if (town_entry.Type == NamedLocation.CAPITAL)
							village_pad = 500.0;
						else
							village_pad = 150.0;
					}
					else
						village_pad = 0.0;

					if(!area.IsAreaOverlap(new BattleRoyalePlayArea(town_entry.Position, village_pad), m_SpawnsSettings.extra_spawn_radius))
						continue;
				}

				int pond = 1;
				if(town_entry.Type == NamedLocation.CAPITAL)
					pond = 5;
				else if(town_entry.Type == NamedLocation.CITY)
					pond = 3;

				for(int p = 0; p < pond; p++)
					villages.Insert(town_entry);
			}

			// Add weighting


			// Randomize order of villages
			villages.ShuffleArray();

			BattleRoyaleUtils.Trace("Final village list:");
			foreach(NamedLocation village: villages)
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

    protected vector GetRandomVillagePosition(NamedLocation village)
    {
        float village_x = village.Position[0];
        float village_z = village.Position[2];
        float village_pad;

        if (village.Type == NamedLocation.CITY)
            village_pad = 300.0;
        else if (village.Type == NamedLocation.CAPITAL)
            village_pad = 500.0;
        else
            village_pad = 100.0;

        float radius, angle, x, z, y;
        radius = village_pad * Math.Sqrt( Math.RandomFloat(0, 1) );
        angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
        x = village_x + ( radius * Math.Cos(angle) );
        z = village_z + ( radius * Math.Sin(angle) );
        y = GetGame().SurfaceY(x, z);

        BattleRoyaleUtils.Trace("Trying to spawn player to " + village.Name + " (" + village.Type + ") with a radius of " + village_pad);

        return Vector(x, y, z);
    }

    protected ref Param2<vector, NamedLocation> GetRandomSpawnPosition()
    {
        vector random_pos = "0 0 0";
        NamedLocation village;

        if (m_SpawnsSettings.spawn_in_villages)
        {
            vector village_pos = "0 0 0";
            BattleRoyaleUtils.Trace("Spawn in zone " + i_StartingZone);
            ref BattleRoyalePlayArea spawn_area = BattleRoyaleZone.GetZone(i_StartingZone).GetArea();
            for(int village_spawn_try = 1; village_spawn_try <= 200; village_spawn_try++)
            {
                BattleRoyaleUtils.Trace("Try to spawn in village " + village_spawn_try);

                village = NULL;
                bool check_zone = true;
                if(m_SpawnsSettings.spawn_in_first_zone)
                {
                    village = GetRandomVillage(spawn_area, true);
                    check_zone = false;  // We got a village in zone, don't need to check if the player will spawn in zone
                }
                else
                    village = GetRandomVillage();

                if (village != NULL && village.Name != "")
                {
                    BattleRoyaleUtils.Trace("Found village " + village.Name);
                    vector search_for_village = "0 0 0";

                    if( village.Name == last_village_spawn )
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
                    if( village.Type == NamedLocation.CAPITAL || village.Type == NamedLocation.CITY )
                    	last_village_spawn = village.Name; // Save last village spawn to avoid it next time
                    break;
                } else {
                    BattleRoyaleUtils.Trace("Another fucked up village!");
                }
            }
        }

        // If at this step we always have a zero vector, try to find a random one
        if( random_pos == "0 0 0" )
        {
            int random_spawn_try = 1;
            while(true)
            {
                BattleRoyaleUtils.Trace("Try to spawn at random position " + random_spawn_try);
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

        return new Param2<vector, NamedLocation>(random_pos, village);
    }

    protected void Teleport(PlayerBase process_player)
    {
        ref Param2<vector, NamedLocation> random_pos = GetRandomSpawnPosition();
        vector position = random_pos.param1;
        NamedLocation village = random_pos.param2;

        TeleportPlayer(process_player, position, village);
    }

    protected void TeleportGroup(ref set<PlayerBase> group)
    {
        ref Param2<vector, NamedLocation> random_pos = GetRandomSpawnPosition();
        vector position = random_pos.param1;
        NamedLocation village = random_pos.param2;

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
                    BattleRoyaleUtils.Trace("Try Group " + spawn_try);
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

    protected void TeleportPlayer(PlayerBase player, vector position, NamedLocation village = NULL)
    {
        BattleRoyaleUtils.Trace("Spawn player " + player.GetIdentity().GetName() + " at " + position);

        //TODO: make sure the retarded game engine doesn't keep the player in a swimming state ????
        //TODO: force stand up

		vector direction;

        if (village)
        {
            direction = vector.Direction(player.GetPosition(), village.Position);
        } else {
            //random direction
            float dir = Math.RandomFloat(0, 360); //non-inclusive, 360==0
            vector playerDir = vector.YawToVector(dir);
            direction = Vector(playerDir[0], 0, playerDir[1]);
        }

		ScriptJunctureData pCtx = new ScriptJunctureData;
		pCtx.Write( position );
		pCtx.Write( direction );
		player.SendSyncJuncture( 88, pCtx );
    }

    void ProcessPlayers()
    {
        BattleRoyaleUtils.Trace("Starting to process players...");
        int i;
        PlayerBase process_player;
        int pCount = m_PlayerList.Count();

        for (i = 0; i < pCount; i++) {
            process_player = m_PlayerList[i];
            if (process_player)
            	DisablePlayerInput(process_player);

            Sleep(100);
        }
        BattleRoyaleUtils.Trace("Players are disabled");

        m_PlayerList.ShuffleArray();

        for (i = 0; i < pCount; i++) {
            process_player = m_PlayerList[i];
            if (process_player) GiveStartingItems(process_player);

            Sleep(100);
        }
        BattleRoyaleUtils.Trace("Gave starting items");

        BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
        array<ref map<string, string>> parties_list = new array<ref map<string, string>> ();
#ifdef SCHANAMODPARTY
        BattleRoyaleUtils.Trace("Mod party enabled");

        // ref array ref set, what in the seven fucks is this ?
        ref array<ref set<PlayerBase>> teleport_groups = GetGroups();

        // Teleport groups
        int pGroupCount = teleport_groups.Count();
        ref set<PlayerBase> group;

        // Send parties list to API server
        for (i = 0; i < pGroupCount; i++) {
            group = teleport_groups.Get(i);
#ifdef BR_TRACE_ENABLED
			Print( group );
#endif
            map<string, string> party = new map<string, string>();
			int tmpNbPlayers = group.Count();
			for(int j = 0; j < tmpNbPlayers; j++)
			{
            	process_player = group.Get(j);
#ifdef BR_TRACE_ENABLED
				Print( process_player );
#endif
				if ( process_player && process_player.GetIdentity() )
				{
					BattleRoyaleUtils.Trace( process_player.GetIdentity().GetPlainId() );
					CF_StringStream string_stream = CF_StringStream( process_player.GetIdentity().GetPlainName() );
					CF_Base16Stream base16_stream = CF_Base16Stream();
					string_stream.CopyTo( base16_stream );
					party.Insert( process_player.GetIdentity().GetPlainId(), base16_stream.Encode() );
				}
			}
#ifdef BR_TRACE_ENABLED
			Print( party );
#endif
			parties_list.Insert( party );
        }

        teleport_groups.ShuffleArray();
        BattleRoyaleUtils.Trace("Groups: " + pGroupCount);
        for (i = 0; i < pGroupCount; i++) {
            BattleRoyaleUtils.Trace("Teleport group " + i);
            group = teleport_groups.Get(i);
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

            map<string, string> party = new map<string, string>();
			CF_StringStream string_stream = CF_StringStream( process_player.GetIdentity().GetPlainName() );
			CF_Base16Stream base16_stream = CF_Base16Stream();
			string_stream.CopyTo( base16_stream );
			party.Insert( process_player.GetIdentity().GetPlainId(), base16_stream.Encode() )
#ifdef BR_TRACE_ENABLED
			Print( party );
#endif
			parties_list.Insert( party );

            Sleep(100);
        }
#endif
        BattleRoyaleUtils.Trace("Teleported players");

#ifdef BR_TRACE_ENABLED
		Print( parties_list );
#endif
        PartiesWebhook partiesWebhook = new PartiesWebhook( m_ServerData.webhook_jwt_token );
        partiesWebhook.postParties( br_instance.match_uuid, parties_list );
        BattleRoyaleUtils.Trace("Parties list sent");

        // plz fix this
        Sleep(1000);

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
