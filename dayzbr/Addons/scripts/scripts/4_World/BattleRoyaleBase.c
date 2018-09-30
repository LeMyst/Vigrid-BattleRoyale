class BattleRoyaleBase
{
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		Print("ERROR: RUNNING BASE ON KILLED");
	}
	void OnPlayerTick(PlayerBase player, float ticktime)
	{
		Print("ERROR: RUNNING BASE ON TICK");
	}
}