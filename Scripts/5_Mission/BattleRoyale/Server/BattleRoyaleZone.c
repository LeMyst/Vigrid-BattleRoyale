#define BR_BETA_LOGGING

class BattleRoyaleZone 
{
    ref BattleRoyaleZone m_ParentZone;
    ref BattleRoyalePlayArea m_PlayArea;

    float f_ConstantShrink;

    
    void BattleRoyaleZone(ref BattleRoyaleZone parent = NULL)
    {
        m_ParentZone = parent;
        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
		BattleRoyaleZoneData m_ZoneSettings = m_Config.GetZoneData();

        f_ConstantShrink = m_ZoneSettings.constant_scale;

        m_PlayArea = new BattleRoyalePlayArea;

        Init();
    }
    void Init()
    {
        float p_Rad;
        vector p_Cen = "0 0 0";

        if(m_ParentZone)
        {
            //this zone has a parent
            p_Rad = m_ParentZone.GetArea().GetRadius();
            p_Cen = m_ParentZone.GetArea().GetCenter();
        }
        else
        {
            //This zone is a "full map" zone (ie, the first zone)
            string path = "CfgWorlds " + GetGame().GetWorldName();
            vector temp = GetGame().ConfigGetVector(path + " centerPosition");

            float world_width = temp[0] * 2;
            float world_height = temp[1] * 2;
            Print("World Size Calculation");
            Print(world_width);
            Print(world_height);

            p_Rad = Math.Min(world_height, world_width) / 2;
            float world_center_x = world_width/2;
            float world_center_z = world_height/2;
            float Y = GetGame().SurfaceY(world_center_x, world_center_z);
            
            p_Cen[0] = world_center_x;
            p_Cen[1] = Y;
            p_Cen[2] = world_center_z;
            Print("Initial Play Area");
            Print(p_Cen);
            Print(p_Rad);
        }

        Print("Zone Data");
        m_PlayArea.SetRadius(p_Rad * f_ConstantShrink); //new radius 
        
    //TODO: ensure zone center is not in water
        float distance = Math.RandomFloatInclusive(0, p_Rad - m_PlayArea.GetRadius()); //distance change from previous center
        float oldX = p_Cen[0];
        float oldZ = p_Cen[2];
        Print(distance);
        float moveDir = Math.RandomFloat(0, 360) * Math.DEG2RAD;
        Print(moveDir);
        float dX = distance * Math.Sin(moveDir);
        float dZ = distance * Math.Cos(moveDir);
        Print(dX);
        Print(dZ);
        float newX = oldX + dX;
        float newZ = oldZ + dZ;
        float newY = GetGame().SurfaceY(newX, newZ);

        vector new_center = "0 0 0";
        new_center[0] = newX;
        new_center[1] = newY;
        new_center[2] = newZ;
        m_PlayArea.SetCenter(new_center);

        Print(m_PlayArea.GetCenter());
        Print(m_PlayArea.GetRadius());
    }

    ref BattleRoyalePlayArea GetArea()
    {
        return m_PlayArea;
    }
}