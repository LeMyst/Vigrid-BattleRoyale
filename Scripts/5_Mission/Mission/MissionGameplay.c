#define BR_BETA_LOGGING

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

	override void OnUpdate( float timeslice )
	{	
		Man player = GetGame().GetPlayer();
		PlayerBase playerPB = PlayerBase.Cast(player);

		super.OnUpdate( timeslice ); //no more using fade out because it causes way to much compatibility issues, instead we'll use widgets
	}
}