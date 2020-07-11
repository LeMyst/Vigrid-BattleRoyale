class BattleRoyaleBase
{
	void OnPlayerTick(PlayerBase player, float timeslice)
	{}
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{}

	//This is deprecated, should not be used
	BattleRoyaleConfig GetConfig()
	{
		return BattleRoyaleConfig.GetConfig();
	}
}