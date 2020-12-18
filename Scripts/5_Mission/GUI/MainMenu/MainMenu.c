modded class MainMenu
{
	protected ref array<string> m_Regions; //TODO: return these values in the "OnStart" web request
	protected int i_CurrentRegion = 0;
	protected TextWidget m_SelectServerLabel;
	protected TextWidget m_OpenWebsiteLabel;
	
	protected Widget m_PopupMessage;
	protected RichTextWidget m_PopupText;
	protected ButtonWidget m_PopupButton;
	protected ButtonWidget m_PopupButton_2;
	protected ref PopupButtonCallback popup_onClick;
	protected ref PopupButtonCallback popup_onClick2;
	
	protected ImageWidget m_Logo;

	protected bool b_IsConnected;


	void SetConnected( bool status )
	{
		b_IsConnected = status;
	}

	//initialize the BR client without a connection to BattleRoyaleAPI
	void OfflineInit()
	{
		Print("INITIALIZING BR IN OFFLINE MODE");
		UpdateRegionsOffline();
		SetConnected(false);

	}
	protected void UpdateRegionsOffline()
	{
		m_Regions = {"any"};
		i_CurrentRegion = 0;
		RegionChanged();
	}

	void UpdateRegions()
	{
		ref RegionData region_data = BattleRoyaleAPI.GetAPI().GetRegionData();
		if(region_data)
		{
			m_Regions = region_data.regions;
			//TODO: reload i_CurrentRegion from it's config file
		}
		else
		{
			//default regions
			m_Regions = {"any","na","eu","au"};
		}
		if(i_CurrentRegion >= m_Regions.Count())
		{
			i_CurrentRegion = 0;
		}
		//update display text based on i_CurrentRegion and m_Regions
		RegionChanged();
	}

	override Widget Init()
	{
		Print("INITIALIZING BR IN ONLINE MODE");
		b_IsConnected = false;

		super.Init(); // this calls dayz expansion init
		
		//ensure popup message is initialized
		if(!m_PopupMessage)
		{

			m_PopupMessage = GetGame().GetWorkspace().CreateWidgets( "BattleRoyale/GUI/layouts/widgets/popup_message.layout", layoutRoot );
			m_PopupText = RichTextWidget.Cast( m_PopupMessage.FindAnyWidget( "MessageText" ) );
			m_PopupButton = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton" ) );
			m_PopupButton_2 = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton_2" ) ); //TODO: update this for new layout (with new button)
		}
		ClosePopup(); //if init is called twice, close any popup that exists

		UpdateRegions();
	
		m_Logo 						= ImageWidget.Cast( layoutRoot.FindAnyWidget( "dayz_logo" ) );
		if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
			Error("Failed to load imageset image");
		
		m_NewsfeedPanel.Show( false ); //--- don't show dayz expansion news feed
		
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
		m_OpenWebsiteLabel.SetText( BATTLEROYALE_VISIT_WEBSITE_MESSAGE );
		
		
		RegionChanged();
		m_LastPlayedTooltip.Show(false);// ensure last played is not shown
		
		return layoutRoot;
	}
	
	override void ShowNewsfeed(bool state)
	{
		super.ShowNewsfeed( false ); //completely disable news feeds
	}

	MainMenuStats GetStats()
	{
		return m_Stats;
	}

	bool OnStart(bool force_restart = false)
	{
		
		BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		if(api.GetCurrentPlayer() && !force_restart)
		{
			b_IsConnected = true; //player exists in the api, so we connected to the network successfully
			return true;
		}

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
		

		//--- connecting to BattleRoyale network UI
		CreatePopup( DAYZBR_CONNECTING_TO_NETWORK_MSG );

		ref ConnectingToNetworkCallback callback = new ConnectingToNetworkCallback( this );
		api.RequestStartAsync(p_User.GetUid(), p_User.GetName(), callback);


		return true;
	}
	
	string GetSelectedRegion()
	{
		return m_Regions.Get(i_CurrentRegion);
	}

	override void Play()
	{
		ref ClosePopupButtonCallback closecallback = new ClosePopupButtonCallback( this );
		if(!b_IsConnected)
		{
			ref RetryNetworkConnectCallback retry_connect = new RetryNetworkConnectCallback( this );
			CreatePopup("You are not connected to the BR Network!", "Close", closecallback, "Connect", retry_connect);
			return;
		}
		BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		PlayerData p_PlayerWebData = api.GetCurrentPlayer();
		if(!p_PlayerWebData)
		{
			CreatePopup("Network Error - Code " + DAYZBR_NETWORK_ERRORCODE_NULL_PLAYER_DATA.ToString(), "Close", closecallback);
			return;
		}

		ref MatchmakeCallback callback = new MatchmakeCallback( this );
		ref CancelMatchmakingCallback onclick = new CancelMatchmakingCallback( this, callback );
		CreatePopup( DAYZBR_MATCHMAKING_MSG, "Cancel", onclick);

		
		api.RequestMatchmakeAsync(p_PlayerWebData, callback, GetSelectedRegion());
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
	override void OnChangeCharacter(bool create_character = true)
	{
		
		super.OnChangeCharacter(create_character); //temporary because I don't want to break any functionality this triggers
		m_OpenWebsiteLabel.SetText( BATTLEROYALE_VISIT_WEBSITE_MESSAGE );// OnChangeCharacter replaces this text, this changes it back to the correct value!
		
		
		RegionChanged();
	}
	void RegionChanged()
	{
		string region_text = m_Regions.Get(i_CurrentRegion);
		m_PlayerName.SetText("Region: " + region_text);
	}
	
	void CreatePopup(string message, string button_text = "", ref PopupButtonCallback onClickCallback = NULL, string button_text_2 = "",  ref PopupButtonCallback onClickCallback_2 = NULL )
	{
		m_PopupText.SetText(message);
		popup_onClick = onClickCallback;
		popup_onClick2 = onClickCallback_2;
		if(button_text != "")
		{
			//Show button
			m_PopupButton.SetText(button_text);
			m_PopupButton.Show(true);
		}
		else
		{
			//hide button
			m_PopupButton.Show(false);
		}
		if(button_text_2 != "")
		{
			//Show button #2
			m_PopupButton_2.SetText(button_text_2);
			m_PopupButton_2.Show(true);
		}
		else
		{
			//hide button
			m_PopupButton_2.Show(false);
		}
		


		m_PopupMessage.Show(true);
	}
	void ClosePopup()
	{
		m_PopupMessage.Show(false);
	}
	void PopupActionClicked(int button_num)
	{
		if(button_num == 1)
		{
			if(!popup_onClick)
			{
				Error("POPUP BUTTON CALLBACK NULL!");
				return;
			}
			popup_onClick.OnButtonClick();
		}
		else
		{
			if(!popup_onClick2)
			{
				Error("POPUP BUTTON 2 CALLBACK NULL!");
				return;
			}
			popup_onClick2.OnButtonClick();
		}
		
	}
	override bool OnClick( Widget w, int x, int y, int button )
	{
		if( button == MouseState.LEFT )
		{
			if(w == m_PopupButton)
			{
				Print("POPUP BUTTON CLICKED");
				PopupActionClicked(1);
				return true;
			}
			if(w == m_PopupButton_2)
			{
				Print("POPUP BUTTON 2 CLICKED");
				PopupActionClicked(2);
				return true;
			}
		}
		return super.OnClick(w, x, y, button);
	}

	override bool OnMouseEnter( Widget w, int x, int y )
	{
		if( IsFocusable( w ) )
		{
			ColorHighlight( w );
			return true;
		}
		return false;
	}

	override void OpenMenuCustomizeCharacter()
	{
		GetGame().OpenURL( BATTLEROYALE_WEBSITE );
	}
	
	override void LoadMods()
	{
		super.LoadMods(); //initialize like normal

		//our goal here is to only show BR as a simplemod entry
		ref array<ref ModInfo> modArray = new array<ref ModInfo>;		
		GetGame().GetModInfos( modArray );
		for(int i = 0; i < modArray.Count(); i++)
		{
			if(modArray[i].GetName().Contains("BattleRoyale"))
			{
				LoadBRModEntry(modArray[i]);
				return;
			}
		}

		//No BR mod found? ya just delete the lists....
		if( m_ModsSimple )
			delete m_ModsSimple;
		if( m_ModsDetailed )
			delete m_ModsDetailed;
	}
	private void LoadBRModEntry(ref ModInfo dbr_mod_info)
	{
		if( m_ModsSimple )
			delete m_ModsSimple;
			
		ref array<ref ModInfo> modArray = new array<ref ModInfo>;	
		modArray.Insert(dbr_mod_info);
		m_ModsSimple = new ModsMenuSimple(modArray, layoutRoot.FindAnyWidget("ModsSimple"), m_ModsDetailed);
	}
}


