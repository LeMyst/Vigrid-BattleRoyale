modded class MissionBaseWorld
{
    ref BattleRoyaleBase m_BattleRoyale;

    BattleRoyaleBase GetBattleRoyale()
    {
        return m_BattleRoyale;
    }
}

BattleRoyaleBase GetBR()
{
    MissionBaseWorld world = MissionBaseWorld.Cast( GetGame().GetMission() );

    if (!world)
    {
        BattleRoyaleUtils.Warn("GetBR() => FAILED TO GET MISSION");
        return NULL;
    }

    return world.GetBattleRoyale();
}
