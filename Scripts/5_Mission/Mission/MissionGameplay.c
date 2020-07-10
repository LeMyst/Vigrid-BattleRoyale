modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
		
		#ifdef BR_BETA_LOGGING
		BRPrint("MissionGameplay::OnInit()");
		#endif
		
		m_BattleRoyale = new BattleRoyaleClient;
	}
}