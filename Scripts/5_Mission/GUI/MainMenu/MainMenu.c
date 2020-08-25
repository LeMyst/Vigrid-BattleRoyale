modded class MainMenu
{
	protected ref array<string> m_Regions = {"any","na","eu","au"};
	protected int i_CurrentRegion = 0;
	protected TextWidget m_SelectServerLabel;
	protected TextWidget m_OpenWebsiteLabel;
	
	protected Widget m_PopupMessage;
	protected TextWidget m_PopupText;
	protected ButtonWidget m_PopupButton;
	protected ref PopupButtonCallback popup_onClick;
	
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
			ClosePopup();
		}


	
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

		ref MatchmakeCallback callback = new MatchmakeCallback( this );
		ref CancelMatchmakingCallback onclick = new CancelMatchmakingCallback( this, callback );
		CreatePopup("Matchmaking...", "Cancel", onclick);

		
		api.RequestMatchmakeAsync(p_PlayerWebData, callback, m_Regions.Get(i_CurrentRegion));
		
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
	
	void CreatePopup(string message, string button_text = "", ref PopupButtonCallback onClickCallback = NULL)
	{
		m_PopupText.SetText(message);
		popup_onClick = onClickCallback;
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
		


		m_PopupMessage.Show(true);
	}
	void ClosePopup()
	{
		m_PopupMessage.Show(false);
	}
	void PopupActionClicked()
	{
		if(!popup_onClick)
		{
			Error("POPUP BUTTON CALLBACK NULL!");
			return;
		}
		popup_onClick.OnButtonClick();
	}
	override bool OnClick( Widget w, int x, int y, int button )
	{
		if( button == MouseState.LEFT )
		{
			if(w == m_PopupButton)
			{
				Print("POPUP BUTTON CLICKED");
				PopupActionClicked();
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

class PopupButtonCallback {
	void OnButtonClick()
	{}
}

class CancelMatchmakingCallback extends PopupButtonCallback
{
	protected ref MainMenu m_MainMenu;
	protected ref MatchmakeCallback m_Matchmaking;
	void CancelMatchmakingCallback(ref MainMenu menu, ref MatchmakeCallback callback)
	{
		m_MainMenu = menu;
		m_Matchmaking = callback;
	} 
	override void OnButtonClick()
	{
		Print("CANCELING MATCHMAKING!");
		m_Matchmaking.Cancel();
		m_MainMenu.ClosePopup();
	}
}
class ClosePopupButtonCallback extends PopupButtonCallback
{
	protected ref MainMenu m_MainMenu;
	void ClosePopupButtonCallback(ref MainMenu menu)
	{
		m_MainMenu = menu;
	} 
	override void OnButtonClick()
	{
		m_MainMenu.ClosePopup();
	}
}

class ConnectingToNetworkCallback extends BattleRoyaleOnStartCallback
{
	protected ref MainMenu m_MainMenu;
	void ConnectingToNetworkCallback(ref MainMenu menu)
	{
		m_MainMenu = menu;
	}
	override void OnError( int errorCode )
	{
		if(errorCode == 5)
		{
			//some generic error code
		}
		m_MainMenu.CreatePopup("Failed to connect! Error " + errorCode.ToString());
	}
	override void OnTimeout()
	{
		m_MainMenu.CreatePopup("Failed to connect! Timed out!");
	}
	override void OnSuccess( PlayerData data )
	{
		m_MainMenu.ClosePopup();
		m_MainMenu.GetStats().InitBRStats(); //need to refresh the stats panel once we successfully query for our player data
	}
}
class MatchmakeCallback extends BattleRoyaleMatchmakeCallback
{
	protected bool is_canceled;

	protected ref MainMenu m_MainMenu;
	void Cancel()
	{
		is_canceled = true;
	}
	void MatchmakeCallback(ref MainMenu menu)
	{
		is_canceled = false;
		m_MainMenu = menu;
	}
	override void OnError( int errorCode )
	{
		if(is_canceled)
		{
			return;
		}
		ref ClosePopupButtonCallback onclick = new ClosePopupButtonCallback( m_MainMenu );
		m_MainMenu.CreatePopup("Failed to connect! Error " + errorCode.ToString(), "Close", onclick);
	}
	override void OnTimeout()
	{
		if(is_canceled)
		{
			return;
		}
		ref ClosePopupButtonCallback onclick = new ClosePopupButtonCallback( m_MainMenu );
		m_MainMenu.CreatePopup("Failed to connect! Timed out!", "Close", onclick);
	}
	override void OnSuccess( ref ServerData data )
	{
		ref ServerData p_ServerData = data;

		if(is_canceled) 
		{
			return;
		}
		if(p_ServerData == NULL)
		{
			Error("BattleRoyale: ServerData is NULL, cannot matchmake");
			m_MainMenu.ClosePopup();
			m_MainMenu.OpenMenuServerBrowser();
			return;
		}

		if(!p_ServerData.CanConnect())
		{
			Error("Result is locked (or wait result)... cannot find viable matchmake");
			m_MainMenu.ClosePopup();
			m_MainMenu.OpenMenuServerBrowser();
			return;
		}

		string ip_addr = p_ServerData.GetIP();
		int port = p_ServerData.GetPort();
		
		if(!GetGame().Connect(m_MainMenu, ip_addr, port, "DayZBR_Beta"))
		{
			Print(p_ServerData.connection);
			Error("BattleRoyale: Failed to connect to server");
			m_MainMenu.OpenMenuServerBrowser();
		}

		m_MainMenu.ClosePopup();
	}
}