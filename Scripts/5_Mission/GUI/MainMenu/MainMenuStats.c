modded class MainMenuStats {
	
	protected TextWidget m_TimePlayedLabel;
	protected TextWidget m_TimePlayedValue;
	
	protected TextWidget m_RatingLabel;
	protected TextWidget m_RatingValue;
	
	protected TextWidget m_WinsLabel;
	protected TextWidget m_WinsValue;
	
	protected TextWidget m_KillsLabel;
	protected TextWidget m_KillsValue;
	
	protected TextWidget m_GlobalRankLabel;
	protected TextWidget m_GlobalRankValue;
	
	protected TextWidget m_Title;
	
	void InitBRStats() 
	{
		m_Title = TextWidget.Cast( m_Root.FindAnyWidget("character_stats_text") );
		m_Title.SetText("DAYZBR #main_menu_statistics");
		
		//--- ensure our label widgets are defined so we can overwrite them
		m_TimePlayedLabel 	= TextWidget.Cast( m_Root.FindAnyWidget( "TimeSurvivedLabel" ) );
		m_RatingLabel	 	= TextWidget.Cast( m_Root.FindAnyWidget( "PlayersKilledLabel" ) );
		m_WinsLabel			= TextWidget.Cast( m_Root.FindAnyWidget( "InfectedKilledLabel" ) );
		m_KillsLabel 		= TextWidget.Cast( m_Root.FindAnyWidget( "DistanceTraveledLabel" ) );
		m_GlobalRankLabel 	= TextWidget.Cast( m_Root.FindAnyWidget( "LongRangeShotLabel" ) );
		
		m_TimePlayedValue 	= m_TimeSurvivedValue;
		m_RatingValue 		= m_PlayersKilledValue;
		m_WinsValue 		= m_InfectedKilledValue;
		m_KillsValue 		= m_DistanceTraveledValue;
		m_GlobalRankValue 	= m_LongRangeShotValue;
		//all of this is subject to be reworked
		m_TimePlayedLabel.SetText("Time played"); 
		m_RatingLabel.SetText("ELO rating");
		m_WinsLabel.SetText("Matches won"); 
		m_KillsLabel.SetText("Players killed");
		m_GlobalRankLabel.SetText("Global rank");
		
		//simple data from player web request
		BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		PlayerData p_SimpleData = api.GetCurrentPlayer();
			
		if(p_SimpleData)
		{
			m_TimePlayedValue.SetText("<Incomplete>");
			m_RatingValue.SetText("<Incomplete>";
			m_WinsValue.SetText("<Incomplete>");
			m_KillsValue.SetText("<Incomplete>");
			m_GlobalRankValue.SetText("<Incomplete>");
		}
		else
		{
			Print("No Simple Data Set, Displaying Blank Stats");
			m_TimePlayedValue.SetText("");
			m_RatingValue.SetText("");
			m_WinsValue.SetText("");
			m_KillsValue.SetText("");
			m_GlobalRankValue.SetText("");
		}
		
		m_Root.Show( true ); //online mode, make sure this is visible
	}
	void InitOffline()
	{
		m_Root.Show( false ); //offline mode, don't show this box at all
	}
	override void UpdateStats()
	{
		InitBRStats();
	}
}