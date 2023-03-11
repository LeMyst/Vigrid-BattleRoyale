class BattleRoyaleZone
{
    static float f_WorldRadius = -1;

    protected ref BattleRoyaleZone m_ParentZone;
    protected ref BattleRoyalePlayArea m_PlayArea;

    protected float f_ConstantShrink;
    protected int i_ShrinkType;
    protected int i_NumRounds;

    protected float f_Eulers;
    protected float f_Exponent;
    protected ref array<float> a_StaticSizes;
    protected ref array<int> a_StaticTimers;

    protected bool b_EndInVillages;

    protected int i_RoundDurationMinutes;

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
        b_EndInVillages = m_ZoneSettings.end_in_villages;

        m_PlayArea = new BattleRoyalePlayArea(Vector(0,0,0), 0.0);

        Init();
    }

    static ref map<int, ref BattleRoyaleZone> m_Zones;

    static ref BattleRoyaleZone GetZone(int x = 1)
    {
        BattleRoyaleZone m_Zone;

        if(!m_Zones)
        {
            m_Zones = new map<int, ref BattleRoyaleZone>();
        }

        int z_Index = x - 1;

        if(!m_Zones.Contains(z_Index)) {
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

#ifdef OLD_AREA_SYSTEM
        float p_Rad;
        vector p_Cen = "0 0 0";

        //figure out what the previous zone was
        if(m_ParentZone)
        {
            //this zone has a parent
            BattleRoyaleUtils.Trace("Create Another Zone");
            p_Rad = m_ParentZone.GetArea().GetRadius();
            p_Cen = m_ParentZone.GetArea().GetCenter();
        }
        else
        {
            BattleRoyaleUtils.Trace("Create First Zone");
            //This zone is a "full map" zone (ie, the first zone)
            string path = "CfgWorlds " + GetGame().GetWorldName();
            Print(path);
            vector temp = GetGame().ConfigGetVector(path + " centerPosition");

            float world_width = temp[0] * 2;
            float world_height = temp[1] * 2;
            BattleRoyaleUtils.Trace("world_width: " + world_width);
            BattleRoyaleUtils.Trace("world_height: " + world_height);

            //p_Rad = Math.Min(world_height, world_width) / 2;
            p_Rad = CreatePlayRadius(Math.Min(world_height, world_width) / 2);
            BattleRoyaleUtils.Trace("max world_center_x: " + (world_width - p_Rad));
            BattleRoyaleUtils.Trace("max world_center_z: " + (world_height - p_Rad));
            float world_center_x = Math.RandomFloat(p_Rad, (world_width - p_Rad));
            float world_center_z = Math.RandomFloat(p_Rad, (world_height - p_Rad));
            float Y = GetGame().SurfaceY(world_center_x, world_center_z);
            BattleRoyaleUtils.Trace("world_center_x: " + world_center_x);
            BattleRoyaleUtils.Trace("world_center_z: " + world_center_z);

            p_Cen[0] = world_center_x;
            p_Cen[1] = Y;
            p_Cen[2] = world_center_z;
        }
        BattleRoyaleUtils.Trace("pRad: " + p_Rad);
        BattleRoyaleUtils.Trace("p_Cen: " + p_Cen);

        CreatePlayArea(p_Rad, p_Cen);
#else
        m_PlayArea = GetBattleRoyalePlayAreas( i_NumRounds - GetZoneNumber() );
#endif
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
            return a_StaticTimers[i_NumRounds - x];
        }

        return 60 * i_RoundDurationMinutes;
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

        // put any extra checks here

        return true;
    }

#ifdef OLD_AREA_SYSTEM
    protected void CreatePlayArea(float p_Rad, vector p_Cen)
    {
        float new_radius = CreatePlayRadius(p_Rad);
        m_PlayArea.SetRadius(new_radius);

        vector new_center = "0 0 0";
        float oldX = p_Cen[0];
        float oldZ = p_Cen[2];
        float max_distance = p_Rad - new_radius; // TODO: Define default size for last zone

        int max_try = 100;

        while(true)
        {
            max_try = max_try - 1;
            float distance = Math.RandomFloatInclusive(DAYZBR_ZS_MIN_DISTANCE_PERCENT * max_distance, DAYZBR_ZS_MAX_DISTANCE_PERCENT * max_distance); //distance change from previous center
            float moveDir = Math.RandomFloat(DAYZBR_ZS_MIN_ANGLE, DAYZBR_ZS_MAX_ANGLE) * Math.DEG2RAD; //direction from previous center

            float dX = distance * Math.Sin(moveDir);
            float dZ = distance * Math.Cos(moveDir);

            float newX = oldX + dX;
            float newZ = oldZ + dZ;

            float newY = GetGame().SurfaceY(newX, newZ);

            new_center[0] = newX;
            new_center[1] = newY;
            new_center[2] = newZ;

            //check if new_center is valid (not in water)
            if(!IsSafeZoneCenter(newX, newZ))
                continue;

            break;
        }

        m_PlayArea.SetCenter(new_center);

        BattleRoyaleUtils.Trace("Zone Data");
        Print(m_PlayArea.GetCenter());
        Print(m_PlayArea.GetRadius());
    }

    protected float CreatePlayRadius(float p_Rad) //p_Rad is the previous play areas radius
    {
        float m = i_NumRounds + 1; //7 total, rounds, 8 is needed for our calculation because X=8 would result in Y=0 and we want to avoid that
        float x = GetZoneNumber(); //this needs to start at 1 (because x=0 for r=initial play area  == initial play area)
        float r = GetWorldRadius();

        switch(i_ShrinkType)
        {
            case 1: //exponential
                // code for wolfram alpha: plot (r/-(e^3))*(e^((3/m)*x)+(-(e^3))) from x=0 to 30, r=500, m=30
                float e = f_Eulers;
                float exp = f_Exponent;

                float yoffset = -1.0 * Math.Pow(e, exp); // -(e^3)
                float sizefactor = r / yoffset; // -(r/(e^3))
                float shrinkexp = (exp / m) * x; // (3/m)*x
                float shrinkfactor = Math.Pow(e, shrinkexp) + yoffset; //e^((3/m)*x) + (-(e^3))

                return sizefactor * shrinkfactor; //(-(r/(e^3)))*(e^((3/m)*x) + (-(e^3)))

            case 2: //linear
                // code for wolfram alpha: plot -(r/m)*x+r from x=0 to 30, r=500, m=30
                float gradient = -1.0 * (r / m);
                return gradient * x + r;

            case 3: //static sizes (lift from array in config)
                if(x > a_StaticSizes.Count())
                {
                    Error("Not enough static sizes for static zone sizes! (want " + x + " have " + a_StaticSizes.Count());
                    return 10000;
                }
                return a_StaticSizes[i_NumRounds - x];
            default:
                return p_Rad * f_ConstantShrink;
        }

        return -1;
    }

    protected float GetWorldRadius()
    {
        if(BattleRoyaleZone.f_WorldRadius == -1)
        {
            string path = "CfgWorlds " + GetGame().GetWorldName();
            vector temp = GetGame().ConfigGetVector(path + " centerPosition");

            float world_width = temp[0] * 2;
            float world_height = temp[1] * 2;

            BattleRoyaleZone.f_WorldRadius = Math.Min(world_height, world_width) / 2;
        }
        return BattleRoyaleZone.f_WorldRadius;
    }

#else

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
                    vector temp = GetGame().ConfigGetVector("CfgWorlds " + GetGame().GetWorldName() + " centerPosition");

                    // Get world size
                    float world_width = temp[0] * 2;
                    float world_height = temp[1] * 2;
                    BattleRoyaleUtils.Trace("world_width: " + world_width);
                    BattleRoyaleUtils.Trace("world_height: " + world_height);

                    if(b_EndInVillages)
                    {
                        area_center = GetRandomPOI();
                    } else {
                        area_center = GetValidPositionSquare(radius, world_width, radius, world_height);
                    }
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
        int max_try = 100;
        vector new_center = "0 0 0";
        while(true)
        {
            max_try = max_try - 1;

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
        float oldX = circle_center[0];
        float oldZ = circle_center[2];
        int max_try = 500;

        while(true)
        {
            max_try = max_try - 1;

            float distance = Math.RandomFloatInclusive(DAYZBR_ZS_MIN_DISTANCE_PERCENT * max_distance, DAYZBR_ZS_MAX_DISTANCE_PERCENT * max_distance); //distance change from previous center
            float moveDir = Math.RandomFloat(DAYZBR_ZS_MIN_ANGLE, DAYZBR_ZS_MAX_ANGLE) * Math.DEG2RAD; //direction from previous center

            float dX = distance * Math.Sin(moveDir);
            float dZ = distance * Math.Cos(moveDir);

            new_center[0] = oldX + dX;
            new_center[2] = oldZ + dZ;
            new_center[1] = GetGame().SurfaceY(new_center[0], new_center[2]);

            // We check if the (new center+radius) is inside the world
            vector temp = GetGame().ConfigGetVector("CfgWorlds " + GetGame().GetWorldName() + " centerPosition");
            float world_width = temp[0] * 2;
            float world_height = temp[1] * 2;
            if(new_center[0] < new_radius || new_center[2] < new_radius || (new_center[0] + new_radius) > world_width || (new_center[2] + new_radius) > world_height)
            {
                BattleRoyaleUtils.Trace("not inside the world " + new_center[0] + " " + new_center[2] + " " + world_width + " " + world_height + " " + new_radius);

                if(max_try <= 0)
                {
                    if(new_center[0] < new_radius)
                    {
                        new_center[0] = new_radius;
                    }
                    if(new_center[2] < new_radius)
                    {
                        new_center[2] = new_radius;
                    }
                    if((new_center[0] + new_radius) > world_width)
                    {
                        new_center[0] = world_width-new_radius;
                    }
                    if((new_center[2] + new_radius) > world_height)
                    {
                        new_center[2] = world_height-new_radius;
                    }
                    BattleRoyaleUtils.Trace("max_try for finding new_center, sad...");
                    break;
                }

                continue;
            }

            if(!IsSafeZoneCenter(new_center[0], new_center[2]))
            {
                BattleRoyaleUtils.Trace("not IsSafeZoneCenter");
                continue;
            }

            break;
        }

        return new_center;
    }

    static ref set<ref array<float>> s_POI;

    vector GetRandomPOI() {
        string cfg = "CfgWorlds " + GetGame().GetWorldName() + " Names";
        BattleRoyaleUtils.Trace(cfg);
        if(!s_POI)
        {
            s_POI = new set<ref array<float>>;
            for (int i = 0; i < GetGame().ConfigGetChildrenCount(cfg); i++) {
                string city;
                GetGame().ConfigGetChildName(cfg, i, city);
                TFloatArray float_array = {};
                GetGame().ConfigGetFloatArray(string.Format("%1 %2 position", cfg, city), float_array);
                string poi_type = GetGame().ConfigGetTextOut(string.Format("%1 %2 type", cfg, city));

                ref array<string> avoid_type = {"DeerStand", "FeedShack", "Marine"};
                ref array<string> avoid_city = {"Camp_Shkolnik", "Ruin_Voron", "Settlement_Skalisty", "Ruin_Storozh", "Local_MB_PrisonIsland"};  // , "Local_Drakon", "Local_Otmel"

                if(avoid_type.Find(poi_type) != -1 || avoid_city.Find(city) != -1)
                    continue;

                BattleRoyaleUtils.Trace("cfg "+city+" "+GetGame().ConfigGetTextOut(string.Format("%1 %2 name", cfg, city))+" "+float_array+" "+poi_type);
                s_POI.Insert(float_array);
            }
        }

        ref array<float> poi = s_POI.Get(Math.RandomInt(0, s_POI.Count()));

        float radius, theta, x, z;
        while(true)
        {
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
#endif
}
