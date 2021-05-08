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
		
		//query for data from leaderboards
		
		BiosUserManager p_UserManager = GetGame().GetUserManager();
		if(!p_UserManager)
		{
			Error("DBR ERROR: p_UserManager = NULL");
			return false;
		}
		BiosUser p_User = p_UserManager.GetTitleInitiator();
		if(!p_User)
		{
			Error("DBR ERROR: p_User = NULL");
			return false;
		}
		string steamid = p_User.GetUid();
		
		BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		LeaderboardPlayer p_PlayerData = api.GetPlayerLeaderboardData(steamid);
		int rank = api.GetRankForRating(p_PlayerData.rating);

		if(p_PlayerData)
		{
			m_TimePlayedValue.SetText(p_PlayerData.totaltimealive.ToString());
			m_RatingValue.SetText(p_PlayerData.rating.ToString());
			m_WinsValue.SetText(p_PlayerData.totalwins.ToString());
			m_KillsValue.SetText(p_PlayerData.totalkills.ToString());
			m_GlobalRankValue.SetText(rank);
		}
		else
		{
			Print("No Simple Data Set, Displaying Blank Stats");
			m_TimePlayedValue.SetText("0:00");
			m_RatingValue.SetText("0");
			m_WinsValue.SetText("0");
			m_KillsValue.SetText("0");
			m_GlobalRankValue.SetText("0");
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