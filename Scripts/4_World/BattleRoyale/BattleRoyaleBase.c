class BattleRoyaleBase
{
	void BattleRoyaleBase()
	{}

	void OnPlayerTick(PlayerBase player, float timeslice)
	{}
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{}
	void Update(float delta)
	{}
	
	ref MatchData GetMatchData()
	{
		return NULL;
	}
	
	bool IsDebug() 
	{
		return true;
	}
}
