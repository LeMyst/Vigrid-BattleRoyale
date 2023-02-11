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

        m_PlayArea = new BattleRoyalePlayArea;

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

    static ref map<int, ref BattleRoyaleZone> m_Zones;

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
        Print("BattleRoyaleZone Init()");
        float p_Rad;
        vector p_Cen = "0 0 0";

        //figure out what the previous zone was
        if(m_ParentZone)
        {
            //this zone has a parent
            Print("Create Another Zone");
            p_Rad = m_ParentZone.GetArea().GetRadius();
            p_Cen = m_ParentZone.GetArea().GetCenter();
        }
        else
        {
            Print("Create First Zone");
            //This zone is a "full map" zone (ie, the first zone)
            string path = "CfgWorlds " + GetGame().GetWorldName();
            Print(path);
            vector temp = GetGame().ConfigGetVector(path + " centerPosition");

            float world_width = temp[0] * 2;
            float world_height = temp[1] * 2;

            p_Rad = Math.Min(world_height, world_width) / 2;
            float world_center_x = world_width/2;
            float world_center_z = world_height/2;
            float Y = GetGame().SurfaceY(world_center_x, world_center_z);

            p_Cen[0] = world_center_x;
            p_Cen[1] = Y;
            p_Cen[2] = world_center_z;
        }

        CreatePlayArea(p_Rad, p_Cen);
    }

    void OnActivate(notnull ref array<PlayerBase> players)
    {
        //This method is run before GetArea() is ever called. This can be used to change the play area size based on players.
        //Note that this on the main thread, therefore it must be performant.
        //we can look at CreatePlayArea / CreatePlayRadius & Init methods for examples of zone size creation
    }

    protected void CreatePlayArea(float p_Rad, vector p_Cen)
    {
        float new_radius = CreatePlayRadius(p_Rad);
        m_PlayArea.SetRadius(new_radius);

        vector new_center = "0 0 0";
        float oldX = p_Cen[0];
        float oldZ = p_Cen[2];
        float max_distance = p_Rad - new_radius; // TODO: Define default size for last zone

        while(true)
        {
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
            if(IsSafeZoneCenter(newX, newZ))
            {
                break;
            }
        }

        m_PlayArea.SetCenter(new_center);

        Print("Zone Data");
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

    protected bool IsSafeZoneCenter(float X, float Z)
    {
        if(GetGame().SurfaceIsSea(X, Z))
            return false;

        if(GetGame().SurfaceIsPond(X, Z))
            return false;

        //put any extra checks here

        return true;
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
}
