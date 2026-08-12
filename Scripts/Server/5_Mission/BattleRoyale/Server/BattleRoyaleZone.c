#ifdef SERVER
class BattleRoyaleZone
{
    protected ref BattleRoyaleZone m_ParentZone;
    protected ref BattleRoyalePlayArea m_PlayArea;

    protected BattleRoyaleConfig m_Config;
    protected BattleRoyaleZoneData m_ZoneSettings;

    protected float f_ConstantShrink;
    protected int i_ShrinkType;
    protected int i_NumRounds;

    protected float f_Eulers;
    protected float f_Exponent;
    protected ref array<float> a_StaticSizes;
    protected ref array<int> a_StaticTimers;
    protected ref array<int> a_MinPlayers;

    //--- Scratch slot written by GetValidPositionNewCircle and read by the generation loop on the
    //--- very next line. It is NOT the offset for this zone - the loop files it into
    //--- s_PlayAreaDurationOffsets at the right index. Reset at the top of every placement call.
    protected float f_PendingDurationOffset;

    protected bool b_EndInVillages;

    protected ref array<string> a_avoidType;
    protected ref array<string> a_avoidCity;

    protected int i_RoundDurationMinutes;

    protected ref array<vector> polygon_vertices;

    static ref map<int, ref BattleRoyaleZone> m_Zones;

    void BattleRoyaleZone(BattleRoyaleZone parent = NULL)
    {
        m_ParentZone = parent;
    }

    void Init()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleZone Init()");

        m_Config = BattleRoyaleConfig.GetConfig();

        BattleRoyaleGameData m_GameData = m_Config.GetGameData();
        i_NumRounds = m_GameData.num_zones;
        i_RoundDurationMinutes = m_GameData.round_duration_minutes;

        m_ZoneSettings = m_Config.GetZoneData();
        f_ConstantShrink = m_ZoneSettings.constant_scale;
        i_ShrinkType = m_ZoneSettings.shrink_type;
        f_Eulers = m_ZoneSettings.shrink_base;
        f_Exponent = m_ZoneSettings.shrink_exponent;
        a_StaticSizes = m_ZoneSettings.static_sizes;
        a_StaticTimers = m_ZoneSettings.static_timers;
        a_MinPlayers = m_ZoneSettings.min_players;
        b_EndInVillages = m_ZoneSettings.end_in_villages;
        a_avoidType = m_ZoneSettings.end_avoid_type;
        a_avoidCity = m_ZoneSettings.end_avoid_city;

        m_PlayArea = new BattleRoyalePlayArea(Vector(0,0,0), 0.0);

		// Convert final_zone_polygon strings to vectors and check if position is inside the polygon
		if (m_ZoneSettings.restrict_final_zone)
		{
			polygon_vertices = new array<vector>();
			foreach(string v : m_ZoneSettings.final_zone_polygon)
			{
				polygon_vertices.Insert(v.ToVector());
			}
		}

        LogConfiguredZoneWindow();

        //--- Generation can abort (see GetBattleRoyalePlayAreas); keep the placeholder area above
        //--- rather than storing NULL, so nothing null-derefs while the server shuts down.
        BattleRoyalePlayArea generated_area = GetBattleRoyalePlayAreas( i_NumRounds - GetZoneNumber() );
        if(generated_area)
            m_PlayArea = generated_area;
    }

    //--- The three static_* arrays are ordered SMALLEST ZONE FIRST: index 0 is the tiny final circle
    //--- and the last index is the widest opening circle. num_zones therefore selects that many
    //--- tiers from the small end, so lowering it shortens a match by dropping the LARGEST circles
    //--- while always keeping the tight endgame one. Trailing entries being unused is by design;
    //--- an array SHORTER than num_zones is a real misconfiguration and is caught per-lookup.
    static bool s_LoggedZoneWindow;

    protected void LogConfiguredZoneWindow()
    {
        //--- Init() runs once per zone object; the settings are process-wide, so say this once.
        if(s_LoggedZoneWindow)
            return;

        s_LoggedZoneWindow = true;

        BattleRoyaleUtils.Info("[BattleRoyaleZone] num_zones = " + i_NumRounds + " -> using zone_settings entries [0.." + (i_NumRounds - 1) + "] (smallest zone first).");

        LogUnusedTail("static_sizes", a_StaticSizes.Count());
        LogUnusedTail("static_timers", a_StaticTimers.Count());
        LogUnusedTail("min_players", a_MinPlayers.Count());
    }

    protected void LogUnusedTail(string setting_name, int entry_count)
    {
        if(entry_count > i_NumRounds)
            BattleRoyaleUtils.Info("[BattleRoyaleZone] zone_settings." + setting_name + " has " + entry_count + " entries; [" + i_NumRounds + ".." + (entry_count - 1) + "] are unused at num_zones = " + i_NumRounds + ". Raise num_zones to play them.");
        else if(entry_count < i_NumRounds)
            BattleRoyaleUtils.Error("[BattleRoyaleZone] zone_settings." + setting_name + " has only " + entry_count + " entries but num_zones is " + i_NumRounds + "! Zones beyond entry " + (entry_count - 1) + " have no configuration.");
    }

    static ref BattleRoyaleZone GetZone(int x = 1)
    {
        BattleRoyaleZone m_Zone;

        if(!m_Zones)
        {
            m_Zones = new map<int, ref BattleRoyaleZone>();
        }

        int z_Index = x - 1;

        if(!m_Zones.Contains(z_Index))
        {
            BattleRoyaleUtils.Trace("[BattleRoyaleZone] Create zone " + z_Index);
            if(z_Index > 0)
            {
                //m_Zones[z_Index] = new BattleRoyaleZone(m_Zones[z_Index - 1]);
                m_Zone = new BattleRoyaleZone(m_Zones.Get(z_Index - 1));
            } else {
                // First zone
                //m_Zones[0] = new BattleRoyaleZone;
                m_Zone = new BattleRoyaleZone;
                z_Index = 0;
            }
            m_Zone.Init();
            m_Zones.Insert(z_Index, m_Zone);
            return m_Zone;
        } else {
            return m_Zones.Get(z_Index);
        }
    }

    ref BattleRoyalePlayArea GetArea()
    {
        return m_PlayArea;
    }

    ref BattleRoyaleZone GetParent()
    {
        return m_ParentZone;
    }

    void OnActivate(notnull array<PlayerBase> players)
    {
        //This method is run before GetArea() is ever called. This can be used to change the play area size based on players.
        //Note that this on the main thread, therefore it must be performant.
        //we can look at CreatePlayArea / CreatePlayRadius & Init methods for examples of zone size creation
    }

    //returns which # zone this is ( 1 for the first zone )
    int GetZoneNumber()
    {
        int number = 1;
        ref BattleRoyaleZone parent = m_ParentZone;
        while(parent)
        {
            parent = parent.GetParent();
            number++;
        }
        return number;
    }

    //--- Index into the smallest-zone-first settings arrays for this zone. Zone 1 (the widest,
    //--- first-played circle) maps to the highest index; the last zone maps to index 0.
    protected int GetZoneSettingsIndex()
    {
        return i_NumRounds - GetZoneNumber();
    }

    int GetZoneTimer()
    {
        if (i_ShrinkType ==  3)
        {
            //--- Range-check the index we actually use, not the 1-based zone number: any
            //--- static_timers shorter than num_zones used to pass the old guard and read out of range.
            int timer_index = GetZoneSettingsIndex();
            if(timer_index < 0 || timer_index >= a_StaticTimers.Count())
            {
                BattleRoyaleUtils.Error("Not enough static timers! (zone " + GetZoneNumber() + " wants index " + timer_index + ", have " + a_StaticTimers.Count() + ")");
                return 300;
            }

            return a_StaticTimers[timer_index] + GetDurationOffset(timer_index);
        }

        return 60 * i_RoundDurationMinutes;
    }

    int GetZoneMinPlayers()
    {
        int min_players_index = GetZoneSettingsIndex();
        if(min_players_index < 0 || min_players_index >= a_MinPlayers.Count())
        {
            //--- 0 makes GetDynamicStartingZone settle on zone 1, so a short min_players degrades
            //--- to a full-length match instead of an arbitrary one.
            BattleRoyaleUtils.Error("Not enough min players! (zone " + GetZoneNumber() + " wants index " + min_players_index + ", have " + a_MinPlayers.Count() + ")");
            return 0;
        }

        return a_MinPlayers[min_players_index];
    }

    //--- Extra seconds granted to a round whose circle sits far from the one before it, so players
    //--- can actually cross the gap. Indexed exactly like the settings arrays.
    protected float GetDurationOffset(int play_area_index)
    {
        if(!s_PlayAreaDurationOffsets)
            return 0;

        if(play_area_index < 0 || play_area_index >= s_PlayAreaDurationOffsets.Count())
            return 0;

        return s_PlayAreaDurationOffsets[play_area_index];
    }

    bool IsInZone(float x, float z)
    {
        vector center = GetArea().GetCenter();

        float d = (Math.Pow(x - center[0], 2) + Math.Pow(z - center[2], 2));
        float radius_pow = Math.Pow(GetArea().GetRadius(), 2);

        BattleRoyaleUtils.Trace(d);
        BattleRoyaleUtils.Trace(radius_pow);

        return (d < radius_pow);
    }

    protected bool IsSafeZoneCenter(float X, float Z)
    {
        if(GetGame().SurfaceIsSea(X, Z))
            return false;

        if(GetGame().SurfaceIsPond(X, Z))
            return false;
        
        // we try to avoid border locations but dont disallow it
        int world_size = GetGame().GetWorld().GetWorldSize() * 0.85;
        if ( X > world_size || Z > world_size )
            return Math.RandomBool();

        return true;
    }

    static ref array<ref BattleRoyalePlayArea> m_PlayAreas;

    //--- Parallel to m_PlayAreas: extra round seconds earned by the travel into each circle.
    //--- Static for the same reason m_PlayAreas is - the circles are generated once per process and
    //--- every zone object reads the same set.
    static ref array<float> s_PlayAreaDurationOffsets;

    //--- Set when circle placement gives up. Generation is not retried after that: the server is
    //--- already exiting, and re-entering would burn 500 placement attempts per zone on the way out.
    static bool s_GenerationFailed;

	BattleRoyalePlayArea GetBattleRoyalePlayAreas(int zone_number)
	{
		if(s_GenerationFailed)
			return NULL;

		// If we don't have the play areas, we generate them
		if(!m_PlayAreas)
		{
			m_PlayAreas = new array<ref BattleRoyalePlayArea>();
			s_PlayAreaDurationOffsets = new array<float>();

			// Initialize bounding box variables at the method level
			float min_x = float.MAX;
			float max_x = float.LOWEST;
			float min_z = float.MAX;
			float max_z = float.LOWEST;

			// Calculate the bounding box of the polygon if restriction is enabled
			if (m_ZoneSettings.restrict_final_zone && m_ZoneSettings.final_zone_polygon && m_ZoneSettings.final_zone_polygon.Count() >= 3)
			{
				// Iterate through all vertices to find the bounding box
				foreach(vector vtx : polygon_vertices)
				{
					min_x = Math.Min(min_x, vtx[0]);
					max_x = Math.Max(max_x, vtx[0]);
					min_z = Math.Min(min_z, vtx[2]);
					max_z = Math.Max(max_z, vtx[2]);
				}
			}

			BattleRoyaleUtils.Trace("CfgWorlds " + GetGame().GetWorldName());
			vector previous_center;
			for(int i = 0; i < i_NumRounds; i++)
			{
				BattleRoyaleUtils.Trace("Generate Area " + i);
				//--- Was `i > Count()`, which let i == Count() through and then indexed anyway.
				if(i >= a_StaticSizes.Count())
				{
					BattleRoyaleUtils.Error("Not enough static sizes for static zone sizes! (want index " + i + " have " + a_StaticSizes.Count() + ")");
					return AbortGeneration();
				}
				BattleRoyalePlayArea playArea = new BattleRoyalePlayArea(Vector(0,0,0), 0.0);
				float radius = a_StaticSizes[i];
				BattleRoyaleUtils.Trace("radius: " + radius);
				playArea.SetRadius(radius);
				vector area_center;

				//--- Generation runs smallest-first, so i == 0 is the tightest circle: the LAST one
				//--- played. Both the polygon restriction and end_in_villages constrain the endgame.
				if(i == 0)  // Final zone
				{
					BattleRoyaleUtils.Trace("Generate final zone");

					// Get world size
					int world_size = GetGame().GetWorld().GetWorldSize();
					BattleRoyaleUtils.Trace("world_size: " + world_size);

					// Initialize POIs list before attempting to find zones
					if(b_EndInVillages)
					{
						InitializePOIs();
					}

					// Check if final zone polygon restriction is enabled
					if(m_ZoneSettings.restrict_final_zone && m_ZoneSettings.final_zone_polygon && m_ZoneSettings.final_zone_polygon.Count() >= 3)
					{
						BattleRoyaleUtils.Trace("Final zone restricted to polygon with " + m_ZoneSettings.final_zone_polygon.Count() + " vertices");

						bool found_valid_position = false;

						// First try to find a POI within the restricted zone
						if(b_EndInVillages)
						{
							BattleRoyaleUtils.Trace("Trying to find POI in restricted zone");
							// Try to find a POI within the polygon
							int max_poi_attempts = 100;

							string cfg = "CfgWorlds " + GetGame().GetWorldName() + " Names";

							if(s_POI)
							{
								// Try all POIs to find one in the polygon
								for(int poi_idx = 0; poi_idx < s_POI.Count() && !found_valid_position; poi_idx++)
								{
									ref array<float> poi = s_POI.Get(poi_idx);

									// Try multiple positions around this POI
									for(int attempt = 0; attempt < max_poi_attempts && !found_valid_position; attempt++)
									{
										float radius_poi = 10 * Math.Sqrt(Math.RandomFloat(0, 1));
										float theta_poi = Math.RandomFloat(0, 1) * Math.PI2;
										float x_poi = poi[0] + radius_poi * Math.Cos(theta_poi);
										float z_poi = poi[1] + radius_poi * Math.Sin(theta_poi);

										vector test_poi = "0 0 0";
										test_poi[0] = x_poi;
										test_poi[2] = z_poi;

										if(IsValidFinalZonePosition(test_poi) && IsSafeZoneCenter(test_poi[0], test_poi[2]))
										{
											area_center = test_poi;
											area_center[1] = GetGame().SurfaceY(area_center[0], area_center[2]);
											found_valid_position = true;
											BattleRoyaleUtils.Trace("Found valid POI position in polygon: " + area_center + " after " + (attempt + 1) + " attempts");
											break;
										}
									}
								}
							}
							else
							{
								BattleRoyaleUtils.Trace("POI list not initialized yet, will try random position");
							}
						}

						// If no POI found in the restricted zone, try random positions
						if(!found_valid_position)
						{
							BattleRoyaleUtils.Trace("No POI found in restricted zone or POI mode disabled, trying random positions");
							int max_attempts = 500;

							while(max_attempts > 0 && !found_valid_position)
							{
								max_attempts--;
								// Try to generate a position within the polygon

								// Generate a random position within the bounding box
								vector test_pos = "0 0 0";
								test_pos[0] = Math.RandomFloat(min_x, max_x);
								test_pos[2] = Math.RandomFloat(min_z, max_z);

								// Use the IsValidFinalZonePosition helper method to check if position is valid
								// Also check if it's a safe zone (not in water, etc.)
								if(IsValidFinalZonePosition(test_pos) && IsSafeZoneCenter(test_pos[0], test_pos[2]))
								{
									area_center = test_pos;
									area_center[1] = GetGame().SurfaceY(area_center[0], area_center[2]);
									found_valid_position = true;
									BattleRoyaleUtils.Trace("Found valid random position in polygon: " + area_center);
								}
							}
						}

						// If we couldn't find a valid position within the polygon after all attempts
						if(!found_valid_position)
						{
							BattleRoyaleUtils.Error("Could not find a valid position within the specified polygon!");
							// Fall back to the default method
							BattleRoyaleUtils.Trace("Falling back to default method");
							if(b_EndInVillages)
								area_center = GetRandomPOI();
							else
								area_center = GetValidPositionSquare(radius, world_size - radius, radius, world_size - radius);
						}
					}
					else
					{
						// Default behavior if restriction is not enabled
						if(b_EndInVillages)
							area_center = GetRandomPOI();
						else
							area_center = GetValidPositionSquare(radius, world_size - radius, radius, world_size - radius);
					}
				} else {  // Next zones
					BattleRoyalePlayArea previous_area = m_PlayAreas[i - 1];
					area_center = GetValidPositionNewCircle(previous_area.GetCenter(), previous_area.GetRadius(), radius);

					if(area_center == "0 0 0")
					{
						//--- Placement gave up and has already asked the game to exit. A placed
						//--- circle can never sit at the origin (its centre must clear its own
						//--- radius), so this is unambiguous.
						BattleRoyaleUtils.Error("Zone generation aborted at area " + i + " - could not place a circle inside area " + (i - 1) + ".");
						return AbortGeneration();
					}

					//--- The travel this circle demands belongs to the round that moves players INTO
					//--- circle i-1, i.e. play area index i-1, not to this one.
					if(f_PendingDurationOffset > 0)
					{
						BattleRoyaleUtils.Trace("Duration offset " + f_PendingDurationOffset + " applied to area " + (i - 1));
						s_PlayAreaDurationOffsets[i - 1] = f_PendingDurationOffset;
					}
				}

				BattleRoyaleUtils.Trace("area_center x: " + area_center[0]);
				BattleRoyaleUtils.Trace("area_center z: " + area_center[2]);

				playArea.SetCenter(area_center);

				BattleRoyaleUtils.Trace("Zone Data");
				BattleRoyaleUtils.Trace(playArea.GetCenter());
				BattleRoyaleUtils.Trace(playArea.GetRadius());

				m_PlayAreas.Insert(playArea);
				s_PlayAreaDurationOffsets.Insert(0);  //--- kept parallel to m_PlayAreas
			}
		}

		BattleRoyaleUtils.Trace("Return zone number: " + zone_number);
		if(zone_number < 0 || zone_number >= m_PlayAreas.Count())
		{
			BattleRoyaleUtils.Error("Asked for play area " + zone_number + " but only " + m_PlayAreas.Count() + " were generated!");
			return NULL;
		}

		return m_PlayAreas[zone_number];
	}

	//--- Zone generation is unrecoverable: without a full set of circles a match cannot be played.
	//--- Drop the partial registry so it can never be mistaken for a complete one, and latch the
	//--- failure so no later GetZone() restarts the search while the server is shutting down.
	protected BattleRoyalePlayArea AbortGeneration()
	{
		s_GenerationFailed = true;
		m_PlayAreas = NULL;
		s_PlayAreaDurationOffsets = NULL;
		return NULL;
	}

    vector GetValidPositionSquare(float min_x, float max_x, float min_z, float max_z)
    {
        vector new_center = "0 0 0";
        while(true)
        {
            new_center[0] = Math.RandomFloat(min_x, max_x);
            new_center[2] = Math.RandomFloat(min_z, max_z);

            if(!IsSafeZoneCenter(new_center[0], new_center[2]))
                continue;

            new_center[1] = GetGame().SurfaceY(new_center[0], new_center[2]);

            break;
        }

        return new_center;
    }

    //--- Returns "0 0 0" if no circle could be placed. The caller must treat that as fatal; the game
    //--- has already been asked to exit by then.
    vector GetValidPositionNewCircle(vector circle_center, float old_radius, float new_radius)
    {
        f_PendingDurationOffset = 0;  //--- never carry an offset over from the previous circle

        float max_distance = new_radius - old_radius;
        vector new_center = "0 0 0";
        vector potentialpos = "0 0 0";
        float oldX = circle_center[0];
        float oldZ = circle_center[2];
        int max_try = 500;

        while(true)
        {
            max_try--;

            float distance = Math.RandomFloatInclusive(DAYZBR_ZS_MIN_DISTANCE_PERCENT * max_distance, DAYZBR_ZS_MAX_DISTANCE_PERCENT * max_distance); //distance change from previous center

            // Get direction toward map center
            int world_size = GetGame().GetWorld().GetWorldSize();  // Get world size
            float centerDir = Math.Atan2((world_size / 2) - oldX, (world_size / 2) - oldZ);  // Get direction to center of the map based on the old center

            // Limit angle to ±45 degrees from center direction (90-degree arc)
            float angleOffset = Math.RandomFloat(-45, 45) * Math.DEG2RAD;  // Random angle offset in radians, between -45 and 45 degrees
            float moveDir = centerDir + angleOffset;  // Get new direction based on the angle offset

            float dX = distance * Math.Sin(moveDir);  // Calculate the x-component of the movement
            float dZ = distance * Math.Cos(moveDir);  // Calculate the z-component of the movement

            new_center[0] = oldX + dX;  // Calculate new x-coordinate
            new_center[2] = oldZ + dZ;  // Calculate new z-coordinate

            // We check if the (new center+radius) is inside the world
            if(new_center[0] < new_radius || new_center[2] < new_radius || (new_center[0] + new_radius) > world_size || (new_center[2] + new_radius) > world_size)
            {
                BattleRoyaleUtils.Trace("not inside the world " + new_center[0] + " " + new_center[2] + " " + world_size + " " + new_radius);

                if(max_try <= 0)
                {
                    //--- One good candidate is enough - take it rather than failing the match.
                    if ( potentialpos != "0 0 0" )
                    {
                        potentialpos[1] = GetGame().SurfaceY(potentialpos[0], potentialpos[2]);
                        return potentialpos;
                    }

                    //--- Nothing at all was found. Clamping the last rejected position into the
                    //--- world would hand back a circle that is not contained by its parent, so the
                    //--- match would be unplayable anyway - fail loudly instead.
                    BattleRoyaleUtils.Error("Could not place a zone circle of radius " + new_radius + " around " + circle_center + " (radius " + old_radius + ") after 500 attempts. Shutting the server down - a match cannot be played without a full set of circles.");
                    GetGame().RequestExit(0);
                    return "0 0 0";  //--- sentinel: caller aborts generation
                }

                continue;
            }

            if(!IsSafeZoneCenter(new_center[0], new_center[2]))
            {
                BattleRoyaleUtils.Trace("not IsSafeZoneCenter");
                continue;
            }
            
            if ( potentialpos == "0 0 0" )
            {
                potentialpos = new_center;
            }
            else
            {
                // We pick the closest location to the center of the previous center
                float distance_A = Math.AbsFloat(vector.Distance(circle_center, potentialpos));
                float distance_B = Math.AbsFloat(vector.Distance(circle_center, new_center));
                float dist;

                //--- Was `distance_A > distance_B`, which kept A when A was the farther one and kept
                //--- B otherwise - the farther candidate won either way, against the comment above.
                if ( distance_A < distance_B )
                {
                    new_center = potentialpos;
                    dist = distance_A;
                }
                else
                {
                    dist = distance_B;
                }

                //--- Reported to the caller, which files it against the circle players travel FROM.
                if ( dist > 1500 )
                    f_PendingDurationOffset = dist / 6;

                break;
            }
        }

		new_center[1] = GetGame().SurfaceY(new_center[0], new_center[2]);

        return new_center;
    }

    static ref set<ref array<float>> s_POI;

	// Initialize the POIs if they haven't been loaded already
	void InitializePOIs()
	{
		if(s_POI)
			return; // POIs already initialized

		s_POI = new set<ref array<float>>;
		string cfg = "CfgWorlds " + GetGame().GetWorldName() + " Names";

		BattleRoyaleUtils.Trace("Initializing POIs");
		BattleRoyaleUtils.Trace("Avoid Type Count: " + a_avoidType.Count());
		BattleRoyaleUtils.Trace("Avoid City Count: " + a_avoidCity.Count());

		BattleRoyaleUtils.Trace(string.Format("Loading %1 POIs", GetGame().ConfigGetChildrenCount(cfg)));
		for (int i = 0; i < GetGame().ConfigGetChildrenCount(cfg); i++)
		{
			string city;
			GetGame().ConfigGetChildName(cfg, i, city);

			TFloatArray city_position = {};
			GetGame().ConfigGetFloatArray(string.Format("%1 %2 position", cfg, city), city_position);
			string poi_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city));

			if(a_avoidType.Find(poi_type) != -1 || a_avoidCity.Find(city) != -1)
			{
				BattleRoyaleUtils.Trace("Avoiding "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+city_position+" "+poi_type);
				continue;
			}

			vector override_position = m_Config.GetPOIsData().GetOverrodePosition( city );
			if( override_position != "0 0 0" )
			{
				//--- s_POI holds CfgWorlds' 2-element [x, z] pairs, read back as poi[0]/poi[1].
				//--- Writing a 3-element vector here put every overridden POI at z = 0 (the sea).
				city_position = {override_position[0], override_position[2]};
				BattleRoyaleUtils.Trace("Override " + city + " position!");
			}

			BattleRoyaleUtils.Trace("cfg "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+city_position+" "+poi_type);
			s_POI.Insert(city_position);
		}

		BattleRoyaleUtils.Trace("Loaded " + s_POI.Count() + " POIs");
	}

	vector GetRandomPOI()
	{
		// Make sure POIs are initialized
		InitializePOIs();

		string cfg = "CfgWorlds " + GetGame().GetWorldName() + " Names";
		BattleRoyaleUtils.Trace(cfg);

		float radius, theta, x, z;
		while(true)
		{
			ref array<float> poi = s_POI.Get(Math.RandomInt(0, s_POI.Count()));

			radius = 10 * Math.Sqrt( Math.RandomFloat(0, 1) );
			theta = Math.RandomFloat(0, 1) * Math.PI2;
			x = poi[0] + radius * Math.Cos(theta);
			z = poi[1] + radius * Math.Sin(theta);

			if(!IsSafeZoneCenter(x, z))
				continue;

			break;
		}

		vector poi_position = "0 0 0";
		poi_position[0] = x;
		poi_position[2] = z;
		poi_position[1] = GetGame().SurfaceY(poi_position[0], poi_position[2]);

		BattleRoyaleUtils.Trace(poi_position);

		return poi_position;
	}

	bool IsPointInPolygon(vector point, array<vector> polygon)
	{
		if (!polygon || polygon.Count() < 3)
			return false;

		int i, j;
		bool result = false;
		j = polygon.Count() - 1;
		for (i = 0; i < polygon.Count(); i++)
		{
			vector vtx_i = polygon[i];
			vector vtx_j = polygon[j];
			// Only compare x and z coordinates (ignore y/height)
			bool crossesZLine = (vtx_i[2] > point[2]) != (vtx_j[2] > point[2]);

			if (crossesZLine)
			{
				float intersectX = vtx_i[0];
				float zDiff = vtx_j[2] - vtx_i[2];

				if (zDiff != 0)
				{
					float xDiff = vtx_j[0] - vtx_i[0];
					float ratio = (point[2] - vtx_i[2]) / zDiff;
					intersectX = vtx_i[0] + (xDiff * ratio);
				}

				if (point[0] < intersectX)
				{
					result = !result;
				}
			}
			j = i;
		}

		return result;
	}

	bool IsValidFinalZonePosition(vector position)
	{
		// If restriction is not enabled or no polygon is defined, any position is valid
		if (!m_ZoneSettings.restrict_final_zone || !m_ZoneSettings.final_zone_polygon || m_ZoneSettings.final_zone_polygon.Count() < 3)
			return true;

		// Check if the position is within the defined polygon
		return IsPointInPolygon(position, polygon_vertices);
	}
}
