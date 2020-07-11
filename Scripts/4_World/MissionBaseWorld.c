modded class MissionBaseWorld 
{
	protected ref BattleRoyaleBase m_BattleRoyale;
	
	BattleRoyaleBase GetBattleRoyale()
	{
		return m_BattleRoyale;
	}
}


BattleRoyaleBase GetBR()
{
	MissionBaseWorld world = MissionBaseWorld.Cast( GetGame().GetMission() );
	return world.GetBattleRoyale();
}
BattleRoyaleConfig GetBRConfig()
{
	BattleRoyaleBase m_BR = GetBR();
	return m_BR.GetConfig();
}