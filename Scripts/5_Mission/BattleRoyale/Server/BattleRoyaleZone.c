
class BattleRoyaleZone 
{
    ref BattleRoyaleZone m_ParentZone;
    
    float f_ConstantShrink;

    float f_Radius;
    vector v_Center;
    
    
    public BattleRoyaleZone(ref BattleRoyaleZone parent = NULL)
    {
        m_ParentZone = parent;
        BattleRoyaleConfig m_Config = GetBRConfig();
		BattleRoyaleZoneData m_ZoneSettings = m_Config.GetZoneData();

        f_ConstantShrink = m_ZoneSettings.constant_scale;
    }
    void Init()
    {
        float p_Rad;
        vector p_Cen;

        if(m_ParentZone)
        {
            //this zone has a parent
            float p_Rad = m_ParentZone.GetRadius();
            vector p_Cen = m_ParentZone.GetCenter();
        }
        else
        {
            //This zone is a "full map" zone (ie, the first zone)

            float world_width = GetLongitude();
            float world_height = GetLatitude();
            Print("World Size Calculation");
            Print(world_width);
            Print(world_height);

            float p_Rad = Math.Min(world_height, world_width) / 2;
            float Y = GetGame().SurfaceY(newX, newZ);
            v_Center = new Vector( world_width/2, Y, world_height/2 );

        }

        f_Radius = p_Rad * f_ConstantShrink; //new radius 

        float distance = Math.RandomFloatInclusive(0, p_Rad - f_Radius); //distance change from previous center
        float oldX = p_Cent[0];
        float oldZ = p_Cent[1];

        float moveDir = Math.RandomFloat(0, 360) * Math.DEG2RAD;
        float dX = distance * Math.Sin(moveDir);
        float dZ = distance * Math.Cos(moveDir);
        float newX = oldX + dX;
        float newZ = oldZ + dZ;
        float newY = GetGame().SurfaceY(newX, newZ);

        v_Center = Vector(newX, newY, newZ);
    }



    float GetRadius()
    {
        return f_Radius;
    }
    vector GetCenter()
    {
        return v_Center;
    }
}