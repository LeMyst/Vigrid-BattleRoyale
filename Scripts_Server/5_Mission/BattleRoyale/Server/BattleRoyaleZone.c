#ifdef SERVER
class BattleRoyaleZone
{
    protected ref BattleRoyaleZone m_ParentZone;
    protected ref BattleRoyalePlayArea m_PlayArea;

    protected float f_ConstantShrink;
    protected int i_ShrinkType;
    protected int i_NumRounds;

    protected float f_Eulers;
    protected float f_Exponent;
    protected ref array<float> a_StaticSizes;
    protected ref array<int> a_StaticTimers;
    protected float f_durationOffset;
    protected ref array<int> a_MinPlayers;

    protected bool b_EndInVillages;

    protected ref array<string> a_avoidType;
    protected ref array<string> a_avoidCity;

    protected int i_RoundDurationMinutes;

    static ref map<int, ref BattleRoyaleZone> m_Zones;

    void BattleRoyaleZone(ref BattleRoyaleZone parent = NULL)
    {
        m_ParentZone = parent;
    }

    void Create()
    {
        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleZoneData m_ZoneSettings = m_Config.GetZoneData();

        BattleRoyaleConfig config_data = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData m_GameData = config_data.GetGameData();
        i_NumRounds = m_GameData.num_zones;
        i_RoundDurationMinutes = m_GameData.round_duration_minutes;

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

        Init();
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
            Print("[BattleRoyaleZone] Create zone " + z_Index);
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
            m_Zone.Create();
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

    void Init()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleZone Init()");
        m_PlayArea = GetBattleRoyalePlayAreas( i_NumRounds - GetZoneNumber() );
    }

    void OnActivate(notnull ref array<PlayerBase> players)
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

    int GetZoneTimer()
    {
        if (i_ShrinkType ==  3)
        {
            float x = GetZoneNumber();
            if(x > a_StaticTimers.Count())
            {
                Error("Not enough static timers! (want " + x + " have " + a_StaticTimers.Count() + ")");
                return 300;
            }
            return a_StaticTimers[i_NumRounds - x] + f_durationOffset;
        }

        return 60 * i_RoundDurationMinutes;
    }

    int GetZoneMinPlayers()
    {
        return a_MinPlayers[i_NumRounds - GetZoneNumber()];
    }

    bool IsInZone(float x, float z)
    {
        vector center = GetArea().GetCenter();

        float d = (Math.Pow(x - center[0], 2) + Math.Pow(z - center[2], 2));
        float radius_pow = Math.Pow(GetArea().GetRadius(), 2);

        Print(d);
        Print(radius_pow);

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

    BattleRoyalePlayArea GetBattleRoyalePlayAreas(int zone_number)
    {
        if(!m_PlayAreas)
        {
            m_PlayAreas = new array<ref BattleRoyalePlayArea>();

            BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
            BattleRoyaleZoneData m_ZoneSettings = m_Config.GetZoneData();
            Print("CfgWorlds " + GetGame().GetWorldName());
            vector previous_center;
            for(int i = 0; i < i_NumRounds; i++)
            {
                BattleRoyaleUtils.Trace("Generate Area " + i);
                if(i > a_StaticSizes.Count())
                {
                    Error("Not enough static sizes for static zone sizes! (want " + i + " have " + a_StaticSizes.Count() + ")");
                }
                BattleRoyalePlayArea playArea = new BattleRoyalePlayArea(Vector(0,0,0), 0.0);
                float radius = a_StaticSizes[i];
                BattleRoyaleUtils.Trace("radius: " + radius);
                playArea.SetRadius(radius);
                vector area_center;

                if(i == 0)  // First zone
                {
                    BattleRoyaleUtils.Trace("Generate first zone");

                    // Get world size
                    int world_size = GetGame().GetWorld().GetWorldSize();
                    BattleRoyaleUtils.Trace("world_size: " + world_size);

                    if(b_EndInVillages)
                        area_center = GetRandomPOI();
                    else
                        area_center = GetValidPositionSquare(radius, world_size - radius, radius, world_size - radius);
                } else {
                    BattleRoyalePlayArea previous_area = m_PlayAreas[i - 1];
                    area_center = GetValidPositionNewCircle(previous_area.GetCenter(), previous_area.GetRadius(), radius);
                }

                BattleRoyaleUtils.Trace("area_center x: " + area_center[0]);
                BattleRoyaleUtils.Trace("area_center z: " + area_center[2]);

                playArea.SetCenter(area_center);

                BattleRoyaleUtils.Trace("Zone Data");
                Print(playArea.GetCenter());
                Print(playArea.GetRadius());

                m_PlayAreas.Insert(playArea);
            }
        }

        BattleRoyaleUtils.Trace("Return zone number: " + zone_number);
        return m_PlayAreas[zone_number];
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

    vector GetValidPositionNewCircle(vector circle_center, float old_radius, float new_radius)
    {
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
            float moveDir = Math.RandomFloat(DAYZBR_ZS_MIN_ANGLE, DAYZBR_ZS_MAX_ANGLE) * Math.DEG2RAD; //direction from previous center

            float dX = distance * Math.Sin(moveDir);
            float dZ = distance * Math.Cos(moveDir);

            new_center[0] = oldX + dX;
            new_center[2] = oldZ + dZ;

            // We check if the (new center+radius) is inside the world
            int world_size = GetGame().GetWorld().GetWorldSize();

            if(new_center[0] < new_radius || new_center[2] < new_radius || (new_center[0] + new_radius) > world_size || (new_center[2] + new_radius) > world_size)
            {
                BattleRoyaleUtils.Trace("not inside the world " + new_center[0] + " " + new_center[2] + " " + world_size + " " + new_radius);

                if(max_try <= 0)
                {
                    if ( potentialpos != "0 0 0" )
                        return potentialpos;

                    if(new_center[0] < new_radius)
                        new_center[0] = new_radius;

                    if(new_center[2] < new_radius)
                        new_center[2] = new_radius;

                    if((new_center[0] + new_radius) > world_size)
                        new_center[0] = world_size - new_radius;

                    if((new_center[2] + new_radius) > world_size)
                        new_center[2] = world_size - new_radius;

					if ( (new_radius - old_radius) < vector.Distance( Vector( new_center[0], 0, new_center[2] ), Vector( circle_center[0], 0, circle_center[2] ) ) )
					{
						BattleRoyaleUtils.Trace("try to find the maximum distance we can use to have circle inside one another");
						distance = new_radius - old_radius; // We take the maximum distance we should have
						moveDir = Math.Atan2( (world_size / 2) - circle_center[0], (world_size / 2) - circle_center[2] ); // We got the direction to the center of the map, we can improve this

						dX = distance * Math.Sin(moveDir);
						dZ = distance * Math.Cos(moveDir);

						new_center[0] = oldX + dX;
						new_center[2] = oldZ + dZ;
					}

                    BattleRoyaleUtils.Trace("max_try for finding new_center, sad...");
                    // TODO: crash the server if no good zone found?
                    //GetGame().RequestExit(0);
                    break;
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

                if ( distance_A > distance_B )
                {
                    new_center = potentialpos;
                    dist = distance_A;
                }
                else
                {
                    dist = distance_B;
                }

                if ( dist > 1500 )
                    f_durationOffset = dist / 6;

                break;
            }
        }

		new_center[1] = GetGame().SurfaceY(new_center[0], new_center[2]);

        return new_center;
    }

    static ref set<ref array<float>> s_POI;

    vector GetRandomPOI()
    {
        string cfg = "CfgWorlds " + GetGame().GetWorldName() + " Names";
        BattleRoyaleUtils.Trace(cfg);

        BattleRoyaleUtils.Trace("Avoid Type Count: " + a_avoidType.Count());
        BattleRoyaleUtils.Trace("Avoid City Count: " + a_avoidCity.Count());

        if(!s_POI)
        {
            s_POI = new set<ref array<float>>;
            for (int i = 0; i < GetGame().ConfigGetChildrenCount(cfg); i++)
            {
                string city;
                GetGame().ConfigGetChildName(cfg, i, city);

                TFloatArray float_array = {};
                GetGame().ConfigGetFloatArray(string.Format("%1 %2 position", cfg, city), float_array);
                string poi_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city));

				if(a_avoidType.Find(poi_type) != -1 || a_avoidCity.Find(city) != -1)
				{
					BattleRoyaleUtils.Trace("Avoiding "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+float_array+" "+poi_type);
					continue;
				}

                BattleRoyaleUtils.Trace("cfg "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+float_array+" "+poi_type);
                s_POI.Insert(float_array);
            }
        }

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

        Print(poi_position);

        return poi_position;
    }
}
