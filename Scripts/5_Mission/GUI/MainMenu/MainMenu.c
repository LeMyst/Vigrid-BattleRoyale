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
		
		
		//note OnStart() is currently blocking
		//TODO: only run OnStart if the player data does not already exist (it runs every time main menu wigit is initialized xd)
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
		Print("DAYZ BATTLE ROYALE INIT");
		
		if(!GetDayZGame())
		{
			Error("DBR ERROR: GetDayZGame() = NULL");
			return false;
		}
		
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
		
		string web_result = GetDayZGame().RequestStart(p_User.GetUid(), p_User.GetName());
		if(web_result == "")
		{
			Error("DBR ERROR: web_result = NULL");
			return false;
		}
		
		JsonSerializer m_Serializer = new JsonSerializer;
		PlayerData p_PlayerWebData;
		string error;
		
		if(!m_Serializer.ReadFromString( p_PlayerWebData, web_result, error ))
		{
			Print("DBR ERROR: JSON Failed To Parse!");
			Error(error);
			return false;
		}
		
		GetDayZGame().SetWebPlayer(p_PlayerWebData); //store our player data
		
		Print("DAYZ BATTLE ROYALE END");
		
		return true;
	}
	
	override void Play()
	{
		if(!GetDayZGame())
		{
			Error("GetDayZGame() == null in Play()");
			OpenMenuServerBrowser();
			return;
		}
		
		string result = GetDayZGame().RequestMatchmake(m_Regions.Get(i_CurrentRegion));
		if(result == "")
		{
			Error("Matchmaking Failed!");
			OpenMenuServerBrowser();
			return;
		}
		Print(result);
		JsonSerializer m_Serializer = new JsonSerializer;
		ServerData p_ServerData;
		string error;
		if(!m_Serializer.ReadFromString( p_ServerData, result, error ))
		{
			Print("DBR ERROR: JSON Failed To Parse!");
			Error(error);
			OpenMenuServerBrowser();
			return;
		}
		
		string connect_data = p_ServerData.connection;	
		Print(p_ServerData);
		Print(connect_data);
		
		TStringArray parts = new TStringArray;
		connect_data.Split(":",parts);
		
		string ip_addr = parts.Get(0);
		int port = parts.Get(1).ToInt();
		if(!GetGame().Connect(this, ip_addr, port, "DayZBR_Beta"))
		{
			Print(connect_data);
			Error("Failed to connect to server");
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

