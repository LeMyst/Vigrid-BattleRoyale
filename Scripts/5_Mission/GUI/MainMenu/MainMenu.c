modded class MainMenu
{
	protected ref array<string> m_Regions = {"any","na","eu","au"};
	protected int i_CurrentRegion = 0;
	protected TextWidget m_SelectServerLabel;
	protected TextWidget m_OpenWebsiteLabel;
	
	
	override Widget Init()
	{
		super.Init(); // this calls dayz expansion init
		
		m_Logo 						= ImageWidget.Cast( layoutRoot.FindAnyWidget( "dayz_logo" ) );
		//this will fail if we are not initializing a DayZExpansion main menu (as m_Logo only exists there)
		if(!m_Logo.LoadImageFile( 0, "set:battleroyale_gui image:DayZBRLogo_White" ))
			Error("Failed to load imageset image");
		
		string version;
		GetGame().GetVersion( version );
		
		if ( GetDayZGame() )
		{
			string expansion_version = GetDayZGame().GetExpansionClientVersion();
			string dayzbr_version = GetDayZGame().GetBattleRoyaleClientVersion();
			m_Version.SetText( "Client #main_menu_version" + " " + version + " | " + expansion_version + " | " + dayzbr_version );
		}
		else
		{
			m_Version.SetText( "DayZ SA #main_menu_version" + " " + version );
		}
		
		
		if(OnStart())
		{
			m_Stats.InitBRStats(); //need to refresh the stats panel once we successfully query for our player data
			
			Print("You are running a developer build of DayZBR");
		}
		else
		{
			Error("Something Went Wrong! BR Failed To Start?!");
		}
		
		
		m_SelectServerLabel = TextWidget.Cast( layoutRoot.FindAnyWidget( "choose_server_label" ) );
		m_SelectServerLabel.SetText("Select Server");
		//TODO: find a use for this button besides "open website" See: OpenMenuCustomizeCharacter()
		m_OpenWebsiteLabel = TextWidget.Cast( layoutRoot.FindAnyWidget( "customize_character_label" ) );
		m_OpenWebsiteLabel.SetText("Visit DayZBR.Dev");
		
		
		RegionChanged();
		m_LastPlayedTooltip.Show(false);// ensure last played is not shown
		
		return layoutRoot;
	}
	
	bool OnStart()
	{
		
		BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		if(api.GetCurrentPlayer())
			return true;


		Print("DAYZ BATTLE ROYALE INIT");
		
		
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
		
		PlayerData p_PlayerWebData = api.RequestStart(p_User.GetUid(), p_User.GetName());
		if(!p_PlayerWebData)
		{
			Error("FAILED TO GET WEB DATA!");
		}
		
		
		Print("DAYZ BATTLE ROYALE END");
		return true;
	}
	
	override void Play()
	{
		BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		PlayerData p_PlayerWebData = api.GetCurrentPlayer();
		if(!p_PlayerWebData)
		{
			Error("BattleRoyale: PlayerWebData is NULL, cannot matchmake");
			OpenMenuServerBrowser();
			return;
		}
		ServerData p_ServerData = api.RequestMatchmake(p_PlayerWebData, m_Regions.Get(i_CurrentRegion));
		if(!p_ServerData)
		{
			Error("BattleRoyale: ServerData is NULL, cannot matchmake");
			OpenMenuServerBrowser();
			return;
		}

		if(!p_ServerData.CanConnect())
		{
			Error("Result is locked (or wait result)... cannot find viable matchmake");
			OpenMenuServerBrowser();
			return;
		}

		string ip_addr = p_ServerData.GetIP();
		int port = p_ServerData.GetPort();
		
		if(!GetGame().Connect(this, ip_addr, port, "DayZBR_Beta"))
		{
			Print(p_ServerData.connection);
			Error("BattleRoyale: Failed to connect to server");
			OpenMenuServerBrowser();
		}
	}
	
	override void NextCharacter()
	{
		i_CurrentRegion++;
		if(i_CurrentRegion >= m_Regions.Count())
			i_CurrentRegion = 0;
		
		RegionChanged();
	}
	override void PreviousCharacter()
	{
		i_CurrentRegion--;
		if(i_CurrentRegion < 0)
			i_CurrentRegion = m_Regions.Count() - 1;
		
		RegionChanged();
	}
	override void OnChangeCharacter()
	{
		
		super.OnChangeCharacter(); //temporary because I don't want to break any functionality this triggers
		m_OpenWebsiteLabel.SetText("Visit DayZBR.Dev");// OnChangeCharacter replaces this text, this changes it back to the correct value!
		
		
		RegionChanged();
	}
	void RegionChanged()
	{
		string region_text = m_Regions.Get(i_CurrentRegion);
		m_PlayerName.SetText("Region: " + region_text);
	}
	
	
	override void OpenMenuCustomizeCharacter()
	{
		GetGame().OpenURL("http://dayzbr.dev/");
	}
}

