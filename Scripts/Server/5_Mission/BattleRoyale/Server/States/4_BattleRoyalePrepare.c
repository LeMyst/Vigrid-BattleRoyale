#ifdef SERVER
class BattleRoyalePrepare: BattleRoyaleState
{
    protected ref array<PlayerBase> a_PlayerList;

    protected ref array<string> a_StartingClothes;
    protected ref array<string> a_StartingItems;
    protected ref array<string> a_AvoidCitySpawn;
    protected bool b_EnableSpawnSelectionMenu;

	private BattleRoyaleConfig m_Config;
    private BattleRoyaleGameData m_GameSettings;
    private BattleRoyaleSpawnsData m_SpawnsSettings;
    private BattleRoyaleServerData m_ServerData;
    private BattleRoyalePOIsData m_POIsSettings;

    private ref array<ref NamedLocation> villages;
    private int i_VillagesIndex;

    private string last_village_spawn = "";

    ref map<string, vector> m_OverrideSpawnPositions;

    void BattleRoyalePrepare()
    {
    	m_Config = BattleRoyaleConfig.GetConfig();

        m_GameSettings = m_Config.GetGameData();
        m_SpawnsSettings = m_Config.GetSpawnsData();
        m_ServerData = m_Config.GetServerData();
        m_POIsSettings = m_Config.GetPOIsData();

		a_StartingClothes = m_GameSettings.player_starting_clothes;
		a_StartingItems = m_GameSettings.player_starting_items;
		a_AvoidCitySpawn = m_SpawnsSettings.avoid_city_spawn;
		b_EnableSpawnSelectionMenu = m_GameSettings.enable_spawn_selection_menu;

        a_PlayerList = new array<PlayerBase>();

        i_VillagesIndex = 0;
    }

    override void Activate()
    {
        super.Activate();

		if ( m_ServerData.enable_vigrid_api )
		{
			StartMatchWebhook matchWebhook = new StartMatchWebhook( m_ServerData.webhook_jwt_token );
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
			matchWebhook.startMatch( br_instance.match_uuid );
		}

        //TODO: spawn & setup drop plane
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(true), true); //fade out screen

        //we process on a static list so when players possibly disconnect during this phase we don't error out or skip any clients
        a_PlayerList.InsertAll( m_Players ); //this gave an error when using m_Players

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input on all clients (we'll do this on the server in another thread)

        BattleRoyaleUtils.Trace("Reset date time to random");
        int year, month, day, hour, minute;
        GetGame().GetWorld().GetDate(year, month, day, hour, minute);
        GetGame().GetWorld().SetDate(year, month, day, Math.RandomIntInclusive(6, 12), 0);

		// Resend hide Spawn Selection UI RPC in case some players didn't get it
		if (b_EnableSpawnSelectionMenu)
		{
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "HideSpawnSelection", NULL, true);
		}

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
        return "Prepare State";
    }

    /**
     *  Strip the lobby loadout.
     *
     *  This deliberately does NOT call vanilla PlayerBase.RemoveAllItems(), which throws one
     *  "LocalDestroyEntity: No inventory location" VM exception per player every match. Two reasons,
     *  both visible in playerbase.c:1600-1613 and inventory.c:1346-1361:
     *
     *   - It enumerates PREORDER, which lists a container *before* its contents, and then destroys
     *     in that order. Destroying a container takes its children with it, so by the time the loop
     *     reaches those children they have no inventory location left and LocalDestroyEntity errors.
     *     Walking the same PREORDER list backwards fixes it: in preorder an ancestor always precedes
     *     its descendants, so in reverse every item is destroyed before anything that contains it.
     *
     *   - It ignores the hands slot. LocalDestroyEntity errors on a HANDS location too, with
     *     "player has to handle this" - the engine expects a hands transition instead. Nothing was
     *     ever held during our test runs, so only the first fault actually fired, but a player who
     *     is holding something when the match starts would hit this one.
     *
     *  POSTORDER traversal would express the ordering directly, but it appears zero times in the
     *  whole vanilla tree (PREORDER appears six), so it is unexercised engine behaviour. Reversing a
     *  PREORDER walk relies only on the traversal vanilla itself depends on.
     */
    protected bool DeleteAllItems(PlayerBase process_player)
    {
        if ( process_player == NULL )
            return false;

        GameInventory inventory = process_player.GetInventory();
        if ( !inventory )
            return false;

        //--- Hands first, through the API the engine sanctions for it.
        if ( process_player.GetEntityInHands() )
            process_player.LocalDestroyEntityInHands();

        if ( inventory.CountInventory() <= 0 )
            return true;

        array<EntityAI> items = new array<EntityAI>();
        inventory.EnumerateInventory( InventoryTraversalType.PREORDER, items );

        //--- Backwards: deepest items first, so nothing is destroyed after its container.
        for ( int i = items.Count() - 1; i >= 0; i-- )
        {
            ItemBase item = ItemBase.Cast( items.Get(i) );
            if ( !item )
                continue;

            //--- Same exclusion vanilla makes: the character itself shows up in the enumeration.
            if ( item.IsInherited( SurvivorBase ) )
                continue;

            inventory.LocalDestroyEntity( item );
        }

        return true;
    }

    protected bool AddStartItems(PlayerBase process_player)
    {
        DeleteAllItems(process_player);

		array<int> player_starting_items_shortcut = m_GameSettings.player_starting_items_shortcut;

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
                    if( new_item )
					{
						// set health to the maximum
						new_item.SetHealthMax();

						// If item is in the shortcut list, set it to the hotbar
						int shortcut_index = player_starting_items_shortcut.Find(j);
						if(shortcut_index != -1)
						{
							process_player.SetQuickBarEntityShortcut(new_item, shortcut_index);
						}
					}
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

//				vector override_position = m_POIsSettings.GetOverrodePosition( city );
//
//				if( override_position != "0 0 0" )
//				{
//					city_position = override_position;
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
				if(a_AvoidCitySpawn.Find(town_entry.Name) != -1)
					continue;

				if(m_SpawnsSettings.spawn_in_first_zone && area != NULL)
				{
					float village_pad;

					if(use_radius)
					{
						// TODO: Move this to configuration file
						if (town_entry.Type == NamedLocation.CITY)
							village_pad = 300.0;
						else if (town_entry.Type == NamedLocation.CAPITAL)
							village_pad = 500.0;
						else
							village_pad = 150.0;
					}
					else
						village_pad = 0.0;

					// Check if city area is in the area, otherwise skip it
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

			//--- The list is built once and cached, and the starting-zone filter above is applied
			//--- during that build. If it removed everything, the list stays empty for the rest of
			//--- the match and every later call can only return NULL - so say so once, here, rather
			//--- than letting the caller rediscover it 200 times per player.
			if(villages.Count() == 0)
				BattleRoyaleUtils.Warn("No town survived the spawn filters, village spawning is unavailable for this match. Falling back to random positions inside the zone.");
        }

        if(villages.Count() > 0)
        {
            if(i_VillagesIndex >= villages.Count())
                i_VillagesIndex = 0;

            return villages[i_VillagesIndex++];
        }
        else
            return NULL;
    }

    //--- True once the cached village list has been built and came out empty. NULL from
    //--- GetRandomVillage() means exactly this and nothing else, and it can never recover within a
    //--- match, so it is a reason to stop retrying rather than to try again.
    protected bool HasNoUsableVillages()
    {
        return villages && villages.Count() == 0;
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
        // Use Math.Pow with power > 1 to concentrate points toward the center
        // Higher values (2, 3, 4) will concentrate points more heavily near center
        radius = village_pad * Math.Pow(Math.RandomFloat(0, 1), 2); // Using power of 2
		angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
        x = village_x + ( radius * Math.Cos(angle) );
        z = village_z + ( radius * Math.Sin(angle) );
        y = GetGame().SurfaceY(x, z);

        // Get the distance between the center of the village and the random point
        float distance = Math.Sqrt(Math.Pow(x - village_x, 2) + Math.Pow(z - village_z, 2));
        BattleRoyaleUtils.Trace("Distance from village center: " + distance + " (from an initial radius of " + village_pad + ")");

        BattleRoyaleUtils.Trace("Trying to spawn player to " + village.Name + " (" + village.Type + ") with a radius of " + village_pad);

        return Vector(x, y, z);
    }

    protected ref Param2<vector, NamedLocation> GetRandomSpawnPosition()
    {
        vector random_pos = "0 0 0";
        NamedLocation village;
        int spawn_zone_number = GetDynamicStartingZone(i_NumStartingPlayers);

        if (m_SpawnsSettings.spawn_in_villages)
        {
            vector village_pos = "0 0 0";
            BattleRoyaleUtils.Trace("Spawn in zone " + spawn_zone_number);
            ref BattleRoyalePlayArea spawn_area = BattleRoyaleZone.GetZone(spawn_zone_number).GetArea();
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
                    {
                    	BattleRoyaleUtils.Trace("Same village as last spawn, we skip it");
                    	continue;
					}

					// Override village position from config file using the technical name (e.g. "Settlement_Novoselki")
					vector override_position = BattleRoyaleConfig.GetConfig().GetPOIsData().GetOverrodePosition( village.GetName() );
					if( override_position != "0 0 0" )
					{
						BattleRoyaleUtils.Trace("Override position " + override_position);
						village.Position[0] = override_position[0];
						village.Position[2] = override_position[2];
					}

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
                    //--- GetRandomVillage() returns NULL only when the cached list is empty, and the
                    //--- list is built once per match - so no further attempt can succeed. Stop here
                    //--- and let the random-position fallback below run, instead of burning the
                    //--- remaining attempts. This was costing 200 guaranteed failures per player.
                    if(HasNoUsableVillages())
                        break;

                    BattleRoyaleUtils.Trace("Another fucked up village!");
                }
            }
        }

        // If at this step we always have a zero vector, try to find a random one
        if( random_pos == "0 0 0" )
        {
        	BattleRoyaleUtils.Trace("Trying to spawn at a random position");

            //--- Sample INSIDE the starting circle rather than sampling the whole map and rejecting
            //--- what falls outside it. The old form drew from the full world and then asked
            //--- IsInZone, but the circle is a tiny fraction of the map - zone 3 at radius 562.5 on
            //--- Sakhal covers well under 1% of the sampled area - so a hit needed ~100-200 draws and
            //--- roughly one player in six exhausted the budget entirely. Measured on two live runs:
            //--- 164 and ~120 draws when it worked, and one player stranded 4.4 km outside the zone
            //--- when it did not. Drawing from the disc makes every draw a candidate, so the only
            //--- thing that can still fail is a circle with no safe ground in it.
            int max_random_spawn_try = 200;
            vector out_of_zone_fallback = "0 0 0";
            float x;
            float y;
            float z;

            //--- A failed generation leaves a zero-radius placeholder rather than NULL, and both
            //--- shapes would make the disc draw meaningless - so fall straight through to the
            //--- map-wide search instead of dereferencing or sampling a point.
            ref BattleRoyalePlayArea start_area = BattleRoyaleZone.GetZone(spawn_zone_number).GetArea();
            vector zone_center = "0 0 0";
            float zone_radius = 0.0;
            if(start_area)
            {
                zone_center = start_area.GetCenter();
                zone_radius = start_area.GetRadius();
            }
            else
                BattleRoyaleUtils.Warn("Starting zone " + spawn_zone_number + " has no play area, falling back to a map-wide spawn search.");

            for(int random_spawn_try = 1; zone_radius > 0.0 && random_spawn_try <= max_random_spawn_try; random_spawn_try++)
            {
                BattleRoyaleUtils.Trace("Try to spawn at random position " + random_spawn_try);

                //--- Uniform over the disc's *area*: the sqrt is what stops draws bunching up around
                //--- the centre, which a plain uniform radius would do. YawToVector gives the unit
                //--- direction, matching how the rest of this file builds a heading.
                float spawn_angle = Math.RandomFloatInclusive(0.0, 360.0);
                float spawn_dist = zone_radius * Math.Sqrt(Math.RandomFloatInclusive(0.0, 1.0));
                vector spawn_dir = vector.YawToVector(spawn_angle);

                x = zone_center[0] + (spawn_dir[0] * spawn_dist);
                z = zone_center[2] + (spawn_dir[1] * spawn_dist);
                y = GetGame().SurfaceY(x, z);

                random_pos[0] = x;
                random_pos[1] = y;
                random_pos[2] = z;

                //--- check_zone is off because the draw is inside the circle by construction; asking
                //--- again would only pay for the test twice.
                if(IsSafeForTeleport(x, y, z, false))
                    return new Param2<vector, NamedLocation>(random_pos, village);
            }

            //--- Nothing safe inside the circle at all - it is all water, ice or blocked. Widen to
            //--- the whole map and take the first safe spot, accepting a spawn outside the zone:
            //--- neither caller checks the returned vector, so giving up with "0 0 0" would teleport
            //--- the player to the map origin, which is worse than a long run inward.
            float edge_pad = 0.1;
            int world_size = GetGame().GetWorld().GetWorldSize();

            for(int wide_spawn_try = 1; wide_spawn_try <= max_random_spawn_try; wide_spawn_try++)
            {
                x = Math.RandomFloatInclusive((world_size * edge_pad), world_size - (world_size * edge_pad));
                z = Math.RandomFloatInclusive((world_size * edge_pad), world_size - (world_size * edge_pad));
                y = GetGame().SurfaceY(x, z);

                if(!IsSafeForTeleport(x, y, z, false))
                    continue;

                out_of_zone_fallback[0] = x;
                out_of_zone_fallback[1] = y;
                out_of_zone_fallback[2] = z;
                break;
            }

            //--- random_pos still holds the last candidate tried, which failed. Discard it.
            random_pos = out_of_zone_fallback;

            if(random_pos == "0 0 0")
                BattleRoyaleUtils.Warn("Found no safe position inside zone " + spawn_zone_number + " and none anywhere on the map after " + max_random_spawn_try + " attempts each. The player will not be moved.");
            else
                BattleRoyaleUtils.Warn("Found no safe position inside zone " + spawn_zone_number + " after " + max_random_spawn_try + " attempts - the circle has no usable ground. Spawning outside the zone at " + random_pos + " instead.");
        }

        return new Param2<vector, NamedLocation>(random_pos, village);
    }

    protected vector GetRandomSafePosition(vector position = "0 0 0", float radius = 100.0)
	{
		BattleRoyaleUtils.Trace("GetRandomSafePosition() with position " + position + " and radius " + radius);

		vector random_pos = "0 0 0";
		int max_spawn_try = 50;
		for(int i = 0; i < max_spawn_try; i++)
		{
			BattleRoyaleUtils.Trace("Try to spawn at random position " + i);

			random_pos[0] = position[0] + Math.RandomFloatInclusive(-radius-i, radius+i);
			random_pos[2] = position[2] + Math.RandomFloatInclusive(-radius-i, radius+i);
			random_pos[1] = GetGame().SurfaceY(random_pos[0], random_pos[2]);

			if(!IsSafeForTeleport(random_pos[0], random_pos[1], random_pos[2], false))
			{
				BattleRoyaleUtils.Trace("Position " + random_pos + " is not safe, we try again");
				random_pos = "0 0 0";  // Reset random_pos to zero vector
				continue;
			}

			break;
		}

		return random_pos;
	}

    //--- Identity-safe name for logging. A PlayerBase can outlive its PlayerIdentity across one of
    //--- the ProcessPlayers coroutine's yields, and a Trace argument is evaluated whatever the log
    //--- level - so an inlined GetIdentity().GetName() throws from inside a disabled log call and
    //--- takes the whole coroutine with it.
    protected string GetPlayerLogName(PlayerBase player)
    {
        if(!player)
            return "<null player>";

        PlayerIdentity identity = player.GetIdentity();
        if(!identity)
            return "<null identity>";

        return identity.GetName();
    }

    protected void Teleport(PlayerBase process_player)
    {
        ref Param2<vector, NamedLocation> random_pos = GetRandomSpawnPosition();
        vector position = random_pos.param1;
        NamedLocation village = random_pos.param2;

        TeleportPlayer(process_player, position, village);
    }

    /**
     * Groups of the current roster, largest concept first: a party is one group, and a player with
     * no party is a group of one. Without the party addon compiled in every player is their own
     * group, which lets the teleport and reporting paths share a single loop instead of carrying a
     * mirrored #else branch each.
     */
    protected array<ref array<PlayerBase>> BuildPartyGroups()
    {
#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetGroups( GetPlayers() );
#else
        array<ref array<PlayerBase>> groups = new array<ref array<PlayerBase>>();
        ref array<PlayerBase> roster = GetPlayers();

        foreach (PlayerBase roster_player : roster)
        {
            if (!roster_player)
                continue;

            ref array<PlayerBase> solo = new array<PlayerBase>();
            solo.Insert(roster_player);
            groups.Insert(solo);
        }

        return groups;
#endif
    }

    protected void TeleportGroup(array<PlayerBase> group)
    {
        ref Param2<vector, NamedLocation> random_pos = GetRandomSpawnPosition();
        vector position = random_pos.param1;
        NamedLocation village = random_pos.param2;

        //--- A zero vector means the spawn search failed outright. The per-player scatter below
        //--- would turn it into a position *near* the origin, which TeleportPlayer's own zero guard
        //--- would then not recognise - so stop here instead and leave the group where it is.
        if(position == "0 0 0")
        {
            BattleRoyaleUtils.Warn("TeleportGroup got a zero position, leaving the group where they are.");
            return;
        }

        int tmpNbPlayers = group.Count();
        for(int i = 0; i < tmpNbPlayers; i++)
        {
            PlayerBase player = group.Get(i);
            if(player)
            {
                BattleRoyaleUtils.Trace("Teleport player " + GetPlayerLogName(player) + " to position " + position);

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
        //--- Runs inside the ProcessPlayers coroutine, which yields between players. A player can
        //--- disconnect across a yield and leave a live PlayerBase with a null PlayerIdentity, and
        //--- an exception here aborts the coroutine before Deactivate() - stranding the match in
        //--- Prepare for good. Same reason the webhook loop below guards both.
        if(!player)
        {
            BattleRoyaleUtils.Warn("TeleportPlayer called with no player, skipping.");
            return;
        }

        //--- The spawn search reports total failure as a zero vector. Teleporting there would drop
        //--- the player at the map origin, so leave them where they are instead.
        if(position == "0 0 0")
        {
            BattleRoyaleUtils.Warn("TeleportPlayer got a zero position for " + GetPlayerLogName(player) + ", leaving them where they are.");
            return;
        }

        BattleRoyaleUtils.Trace("Spawn player " + GetPlayerLogName(player) + " at " + position);

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

        //--- Counted from the snapshot, not from the live roster. Activate() states the rule: "we
        //--- process on a static list so when players possibly disconnect during this phase we
        //--- don't error out or skip any clients". Taking the bound from GetPlayers() broke it in
        //--- both directions - a disconnect before this line made the loops skip the tail of the
        //--- snapshot, and the spawn-selection loop below indexed the live array, which shrinks
        //--- across every Sleep(100) yield and so read out of range.
        int pCount = a_PlayerList.Count();

        for (i = 0; i < pCount; i++) {
            process_player = a_PlayerList[i];
            if (process_player)
            	DisablePlayerInput(process_player);

            Sleep(100);
        }
        BattleRoyaleUtils.Trace("Players are disabled");

        a_PlayerList.ShuffleArray();

        for (i = 0; i < pCount; i++) {
            process_player = a_PlayerList[i];
            if (process_player) GiveStartingItems(process_player);

            Sleep(100);
        }
        BattleRoyaleUtils.Trace("Gave starting items");

        BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
        array<ref map<string, string>> parties_list = new array<ref map<string, string>> ();

		if (b_EnableSpawnSelectionMenu)
		{
			BattleRoyaleUtils.Trace("Spawn selection menu enabled");
			for (i = 0; i < pCount; i++) {
				//--- The snapshot, like every other loop in this method. This was the one place
				//--- that indexed the live roster.
				process_player = a_PlayerList[i];
				if (process_player)
				{
					float f_SpawnSelectionRadius = m_GameSettings.spawn_selection_radius;
					vector position = process_player.GetSpawnPos();
					string player_name = GetPlayerLogName(process_player);

					if( position == vector.Zero )
					{
						BattleRoyaleUtils.Trace("Player " + player_name + " didn't select a spawn point, we randomly teleport them");
						Teleport(process_player);
						Sleep(100);
						continue;
					}

					BattleRoyaleUtils.Trace("Try to spawn player " + player_name + " at " + position + " with a radius of " + f_SpawnSelectionRadius);

					vector random_position = GetRandomSafePosition(position, f_SpawnSelectionRadius);

					if ( random_position == vector.Zero )
					{
						BattleRoyaleUtils.Trace("No safe position found, we randomly teleport the player");
						Teleport(process_player);
					}
					else
					{
						BattleRoyaleUtils.Trace("Found a safe position " + random_position);
						TeleportPlayer(process_player, random_position);
					}

					Sleep(100);
				}
			}
		}
		else
		{
			BattleRoyaleUtils.Trace("Spawn selection menu disabled");

			//--- Solo players come back as groups of one, so this one loop replaces the pair of
			//--- mirrored branches that used to sit here.
			array<ref array<PlayerBase>> teleport_groups = BuildPartyGroups();
			int pGroupCount = teleport_groups.Count();

			teleport_groups.ShuffleArray();
			BattleRoyaleUtils.Trace("Groups: " + pGroupCount);

			for (i = 0; i < pGroupCount; i++) {
				BattleRoyaleUtils.Trace("Teleport group " + i);
				array<PlayerBase> group = teleport_groups.Get(i);

				if ( group.Count() > 1 )
				{
					TeleportGroup( group );
				} else {
					process_player = group.Get(0);
					if (process_player) Teleport(process_player);
				}
				Sleep(100);
			}
		}
        BattleRoyaleUtils.Trace("Teleported players");

		if ( m_ServerData.enable_vigrid_api )
		{
			//--- One map per group, keyed by SteamID64. Without parties every group holds one
			//--- player, which reproduces exactly what the non-party branch used to send - so the
			//--- payload shape is identical either way and there is a single encoding loop.
			array<ref array<PlayerBase>> api_groups = BuildPartyGroups();
			int apiGroupCount = api_groups.Count();

			for (i = 0; i < apiGroupCount; i++) {
				array<PlayerBase> api_group = api_groups.Get(i);
				map<string, string> party = new map<string, string>();

				int tmpNbPlayers = api_group.Count();
				for(int j = 0; j < tmpNbPlayers; j++)
				{
					process_player = api_group.Get(j);

					//--- The identity guard used to be missing on the non-party path, where a
					//--- player disconnecting mid-preparation would dereference null.
					if ( !process_player )
						continue;
					if ( !process_player.GetIdentity() )
						continue;

					BattleRoyaleUtils.Trace( process_player.GetIdentity().GetPlainId() );

					// Encode player name to Base16
					CF_StringStream string_stream = CF_StringStream( process_player.GetIdentity().GetPlainName() );
					CF_Base16Stream base16_stream = CF_Base16Stream();
					string_stream.CopyTo( base16_stream );
					party.Insert( process_player.GetIdentity().GetPlainId(), base16_stream.Encode() );
				}

				if ( party.Count() > 0 )
					parties_list.Insert( party );
			}

#ifdef BR_TRACE_ENABLED
			Print( parties_list );
#endif

			PartiesWebhook partiesWebhook = new PartiesWebhook( m_ServerData.webhook_jwt_token );
			partiesWebhook.postParties( br_instance.match_uuid, parties_list );
			BattleRoyaleUtils.Trace("Parties list sent");
		}

        // plz fix this
        Sleep(1000);

        for (i = 0; i < pCount; i++) {
            process_player = a_PlayerList[i];
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

    //--- Index loop with a null check, not a foreach - see the note on the BattleRoyaleDebugState
    //--- override this mirrors. m_Players is already cleared by the time this runs, so a throw
    //--- here loses the roster during state migration.
    override ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> players = super.RemoveAllPlayers();
        for(int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = players[i];
            if(!player)
                continue;

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
