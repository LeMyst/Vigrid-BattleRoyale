modded class MainMenu
{
	protected ref array<string> m_Regions = {"any","na","eu","au"}; //TODO: return these values in the "OnStart" web request
	protected int i_CurrentRegion = 0;
	protected TextWidget m_SelectServerLabel;
	protected TextWidget m_OpenWebsiteLabel;
	
	protected Widget m_PopupMessage;
	protected TextWidget m_PopupText;
	protected ButtonWidget m_PopupButton;
	protected ButtonWidget m_PopupButton_2;
	protected ref PopupButtonCallback popup_onClick;
	protected ref PopupButtonCallback popup_onClick2;
	
	protected ImageWidget m_Logo;

	override Widget Init()
	{
		super.Init(); // this calls dayz expansion init
		
		//ensure popup message is initialized
		if(!m_PopupMessage)
		{

			m_PopupMessage = GetGame().GetWorkspace().CreateWidgets( "BattleRoyale/GUI/layouts/widgets/popup_message.layout", layoutRoot );
			m_PopupText = TextWidget.Cast( m_PopupMessage.FindAnyWidget( "MessageText" ) );
			m_PopupButton = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton" ) );
			m_PopupButton_2 = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton_2" ) ); //TODO: update this for new layout (with new button)
		}
		ClosePopup(); //if init is called twice, close any popup that exists


	
		m_Logo 						= ImageWidget.Cast( layoutRoot.FindAnyWidget( "dayz_logo" ) );
		if(!m_Logo.LoadImageFile( 0, "set:battleroyale_gui image:DayZBRLogo_White" ))
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
		m_OpenWebsiteLabel.SetText("Visit DayZBR.Dev");
		
		
		RegionChanged();
		m_LastPlayedTooltip.Show(false);// ensure last played is not shown
		
		return layoutRoot;
	}
	
	MainMenuStats GetStats()
	{
		return m_Stats;
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
		

		//--- connecting to BattleRoyale network UI
		CreatePopup("Connecting to the BattleRoyale Network...");

		ref ConnectingToNetworkCallback callback = new ConnectingToNetworkCallback( this );
		api.RequestStartAsync(p_User.GetUid(), p_User.GetName(), callback);

		/*
		PlayerData p_PlayerWebData = api.RequestStart(p_User.GetUid(), p_User.GetName());
		if(!p_PlayerWebData)
		{
			Error("FAILED TO GET WEB DATA!");
		}
		
		
		Print("DAYZ BATTLE ROYALE END");
		*/
		return true;
	}
	
	string GetSelectedRegion()
	{
		return m_Regions.Get(i_CurrentRegion);
	}

	override void Play()
	{
		BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		PlayerData p_PlayerWebData = api.GetCurrentPlayer();
		if(!p_PlayerWebData)
		{
			ref ClosePopupButtonCallback closecallback = new ClosePopupButtonCallback( m_MainMenu );
			CreatePopup("Network Error - Code " + DAYZBR_NETWORK_ERRORCODE_NULL_PLAYER_DATA.ToString(), "Close", closecallback);
			return;
		}

		ref MatchmakeCallback callback = new MatchmakeCallback( this );
		ref CancelMatchmakingCallback onclick = new CancelMatchmakingCallback( this, callback );
		CreatePopup("Matchmaking...", "Cancel", onclick);

		
		api.RequestMatchmakeAsync(p_PlayerWebData, callback, GetSelectedRegion());
		
		/*
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
		*/
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
			m_PopupButton_2.SetText(button_text);
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
		GetGame().OpenURL("http://dayzbr.dev/");
	}
}


