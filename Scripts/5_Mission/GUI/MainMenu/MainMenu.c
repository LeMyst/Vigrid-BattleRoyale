modded class MainMenu
{
	protected Widget m_PopupMessage;
	protected RichTextWidget m_PopupText;
	protected ButtonWidget m_PopupButton;
	protected ButtonWidget m_PopupButton_2;
	protected ref PopupButtonCallback popup_onClick;
	protected ref PopupButtonCallback popup_onClick2;

	protected ImageWidget m_Logo;

	override Widget Init()
	{
		Print("INITIALIZING BR IN ONLINE MODE");

		super.Init(); // this calls dayz expansion init

		//ensure popup message is initialized
		if(!m_PopupMessage)
		{

			m_PopupMessage = GetGame().GetWorkspace().CreateWidgets( "DayZBR-Mod/GUI/layouts/widgets/popup_message.layout", layoutRoot );
			m_PopupText = RichTextWidget.Cast( m_PopupMessage.FindAnyWidget( "MessageText" ) );
			m_PopupButton = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton" ) );
			m_PopupButton_2 = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton_2" ) ); //TODO: update this for new layout (with new button)
		}
		ClosePopup(); //if init is called twice, close any popup that exists

		m_Logo = ImageWidget.Cast( layoutRoot.FindAnyWidget( "dayz_logo" ) );
		if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
			Error("Failed to load imageset image");

		//m_NewsfeedPanel.Show( false ); //--- don't show dayz expansion news feed

		m_ChooseServer.Show( false );

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

		m_LastPlayedTooltip.Show(false);// ensure last played is not shown

		return layoutRoot;
	}

//	override void ShowNewsfeed(bool state)
//	{
//		super.ShowNewsfeed( false ); //completely disable news feeds
//	}

	bool OnStart(bool force_restart = false)
	{

		//BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		//if(api.GetCurrentPlayer() && !force_restart)
		//{
		//	b_IsConnected = true; //player exists in the api, so we connected to the network successfully
		//	return true;
		//}

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
		//CreatePopup( DAYZBR_CONNECTING_TO_NETWORK_MSG );

		//api.RequestStartAsync(p_User.GetUid(), p_User.GetName(), this, "RequestStartCallback");


		return true;
	}

	override void Play()
	{
		ref ClosePopupButtonCallback closecallback = new ClosePopupButtonCallback( this );
		//if(!b_IsConnected)
		//{
		//	ref RetryNetworkConnectCallback retry_connect = new RetryNetworkConnectCallback( this );
		//	CreatePopup("You are not connected to the BR Network!", "Close", closecallback, "Connect", retry_connect);
		//	return;
		//}
		//BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		//BRPlayerData p_PlayerWebData = api.GetCurrentPlayer();
		//if(!p_PlayerWebData)
		//{
		//	CreatePopup("Network Error - Code " + DAYZBR_NETWORK_ERRORCODE_NULL_PLAYER_DATA.ToString(), "Close", closecallback);
		//	return;
		//}

		//ref Match"makeAction mmaction = new MatchmakeAction( this );
		//ref CancelMatchmakingCallback onclick = new CancelMatchmakingCallback( this, mmaction );
		//CreatePopup( DAYZBR_MATCHMAKING_MSG, "Cancel", onclick);

		//api.RequestMatchmakeAsync(p_PlayerWebData, mmaction, "OnMatchmakeComplete", GetSelectedRegion());
		//GetGame().Connect(this, "127.0.0.1", 2302, "");
		Print("Play()");
		g_Game.ConnectToBR();
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


