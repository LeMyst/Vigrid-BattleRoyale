class BattleRoyaleBase
{
	ref BattleRoyaleConfig m_Config;

	void OnPlayerTick(PlayerBase player, float timeslice)
	{}
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{}

	BattleRoyaleConfig GetConfig()
	{
		if(!m_Config)
		{
			m_Config = new BattleRoyaleConfig;
			m_Config.Load();
		}
		return m_Config;
	}
}