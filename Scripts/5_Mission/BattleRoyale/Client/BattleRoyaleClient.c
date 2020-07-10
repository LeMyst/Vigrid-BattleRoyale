class BattleRoyaleClient extends BattleRoyaleBase
{
	#ifdef BR_BETA_LOGGING
	bool print_once_tick = true;
	#endif
	void BattleRoyaleClient()
	{
		Init();
	}
	void Init()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleClient::Init()");
		#endif
	}
	
	
	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		#ifdef BR_BETA_LOGGING
		if(print_once_tick)
		{
			print_once_tick = false;
			BRPrint("BattleRoyaleClient::OnPlayerTick() => Running!");
		}
		#endif
		
	}
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleClient::OnPlayerKilled()");
		#endif
		
	}
}