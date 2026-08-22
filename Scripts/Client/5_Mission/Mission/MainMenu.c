#ifndef SERVER
modded class MainMenu
{
	protected ImageWidget m_Logo;
	protected ref array<ref ModInfo> modList = new array<ref ModInfo>();
	protected TextWidget m_PlayButtonLabel;
	protected bool b_MatchMakingAvailable;

	protected Widget m_PopupMessage;
	protected RichTextWidget m_PopupText;
	protected ButtonWidget m_PopupButton;
	protected ButtonWidget m_PopupButton_2;
	protected ref PopupButtonCallback popup_onClick;
	protected ref PopupButtonCallback popup_onClick2;

	protected BattleRoyaleMatchMakingState m_MatchMakingState;

	protected int m_MatchMakingTryCount;

#ifdef DIAG
	//--- one-shot for the whole client process, not per menu instance - see TryAutoConnect.
	protected static bool s_AutoConnectDone;
#endif

	void ~MainMenu()
	{
		//--- A "come back later" response queues StartMatchMaking on the SYSTEM call queue, which
		//--- lives for the whole process. Without this the pending retry outlives the menu it
		//--- belongs to and re-enters the blocking POST_now against a torn-down widget tree.
		//--- GetGame() is checked because a destructor can run on the way out.
		if ( GetGame() )
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).RemoveByName( this, "StartMatchMaking" );
	}

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/MainMenu/main_menu.layout");

		m_Play						= layoutRoot.FindAnyWidget("play");
		m_PlayButtonLabel			= TextWidget.Cast(m_Play.FindAnyWidget("play_label"));
		m_ChooseServer				= layoutRoot.FindAnyWidget("choose_server");
		m_CustomizeCharacter		= layoutRoot.FindAnyWidget("customize_character");
		m_PlayVideo					= layoutRoot.FindAnyWidget("play_video");
		m_Tutorials					= layoutRoot.FindAnyWidget("tutorials");
		m_TutorialButton			= layoutRoot.FindAnyWidget("tutorial_button");
		m_MessageButton				= layoutRoot.FindAnyWidget("message_button");
		m_SettingsButton			= layoutRoot.FindAnyWidget("settings_button");
		m_Exit						= layoutRoot.FindAnyWidget("exit_button");
		m_PrevCharacter				= layoutRoot.FindAnyWidget("prev_character");
		m_NextCharacter				= layoutRoot.FindAnyWidget("next_character");

		m_DlcFrame 					= layoutRoot.FindAnyWidget("dlc_Frame");
		m_Version					= TextWidget.Cast(layoutRoot.FindAnyWidget("version"));
		m_ModdedWarning				= TextWidget.Cast(layoutRoot.FindAnyWidget("ModdedWarning"));
		m_CharacterRotationFrame	= layoutRoot.FindAnyWidget("character_rotation_frame");

		m_LastPlayedTooltip			= layoutRoot.FindAnyWidget("last_server_info");
		m_LastPlayedTooltip.Show(false);
		m_LastPlayedTooltipLabel	= m_LastPlayedTooltip.FindAnyWidget("last_server_info_label");
		m_LastPlayedTooltipName 	= TextWidget.Cast(m_LastPlayedTooltip.FindAnyWidget("last_server_info_name"));
		m_LastPlayedTooltipIP		= TextWidget.Cast(m_LastPlayedTooltip.FindAnyWidget("last_server_info_ip"));
		m_LastPlayedTooltipPort		= TextWidget.Cast(m_LastPlayedTooltip.FindAnyWidget("last_server_info_port"));

		m_LastPlayedTooltipTimer	= new WidgetFadeTimer();

		m_Stats						= new MainMenuStats(layoutRoot.FindAnyWidget("character_stats_root"));

		m_Mission					= MissionMainMenu.Cast(GetGame().GetMission());

		m_LastFocusedButton 		= m_Play;

		m_ScenePC					= m_Mission.GetIntroScenePC();

		if (m_ScenePC)
		{
			m_ScenePC.ResetIntroCamera();
		}

		m_PlayVideo.Show(false);

		m_PlayerName				= TextWidget.Cast(layoutRoot.FindAnyWidget("character_name_text"));

		// Set Version
		string version;
		GetGame().GetVersion(version);
		m_Version.SetText("#main_menu_version" + " " + version);

		GetGame().GetUIManager().ScreenFadeOut(0);

		SetFocus(null);

		Refresh();

		LoadMods();
	//		PopulateDlcFrame();

		GetDayZGame().GetBacklit().MainMenu_OnShow();
		GetGame().GetMission().GetOnModMenuVisibilityChanged().Insert(ShowDlcFrame);

		g_Game.SetLoadState(DayZLoadState.MAIN_MENU_CONTROLLER_SELECT);

		// Get mod list
		GetGame().GetModInfos(modList);

		m_Logo = ImageWidget.Cast( layoutRoot.FindAnyWidget( "dayz_logo" ) );
		if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
			Error("Failed to load imageset image");

		//m_NewsfeedPanel.Show( false ); //--- don't show dayz expansion news feed

		//m_ChooseServer.Show( false ); // Don't show choose server
		m_TutorialButton.Show( false ); // Don't show tutorial button
		//m_MessageButton.Show( false ); // Don't show credits button
		array<string> funny_strings = {
			"Bro, it's DayZ but with Battle Royale stuff!",
			"Welcome to the wild world of DayZ Battle Royale, where the only certainty is uncertainty.",
			"Thou whom read this message, S1Q.",
			"Next time, try to aim better...",
			"Just so you know, you're allergic to bullets.",
			"You Miss 100% Of The Shots You Don't Take.",
			"I should have improved the mod instead of writing this sentence.",
			"Remember, bushes aren't bulletproof.",
			"If at first, you don't succeed, respawn and try again.",
			"Pro tip: Running in a straight line isn't the best strategy.",
			"Warning: May cause sudden addiction to adrenaline and respawn buttons.",
			"Just a friendly reminder: camping in this game is for tents, not players.",
			"Remember, it's not about how many kills you get, but how stylishly you avoid death.",
			"Embrace the chaos, but don't forget to reload.",
			"Just when you thought it was safe to loot...",
			"In this game, karma has a faster firing rate than any weapon."
		};
		m_ModdedWarning.SetText(funny_strings.GetRandomElement());
		//m_ModdedWarning.Show( false ); // Hide modded message

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

		//ensure popup message is initialized
		if(!m_PopupMessage)
		{
			m_PopupMessage = GetGame().GetWorkspace().CreateWidgets( "Vigrid-BattleRoyale/GUI/widgets/popup_message.layout", layoutRoot );
			m_PopupText = RichTextWidget.Cast( m_PopupMessage.FindAnyWidget( "MessageText" ) );
			m_PopupButton = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton" ) );
			m_PopupButton_2 = ButtonWidget.Cast( m_PopupMessage.FindAnyWidget( "PopupButton_2" ) ); //TODO: update this for new layout (with new button)
		}
		ClosePopup(); //if init is called twice, close any popup that exists

		BR_OverrideUI();

		m_MatchMakingState = BattleRoyaleMatchMakingState.None;
		m_MatchMakingTryCount = 0;

		if (IsMatchMakingAvailable())
		{
			m_PlayButtonLabel.SetText("#STR_BR_MM_MATCHMAKING");
		}
		else
		{
			m_PlayButtonLabel.SetText("#main_menu_play");
		}

#ifdef DIAG
		TryAutoConnect();
#endif

		return layoutRoot;
	}

	void BR_OverrideUI()
	{
	//		m_Play.Show(false);
		m_ChooseServer.Show(false);
		m_TutorialButton.Show(false);
		m_DlcFrame.Show(false);

		if (m_DisplayedDlcHandler)
			m_DisplayedDlcHandler.ShowInfoPanel(false);
	}

	private bool IsMatchMakingAvailable()
	{
		// Change the play button text
		m_PlayButtonLabel.SetText("#STR_BR_MM_LOGIN");

		// MatchMaking Webhook
		MatchMakingWebhook matchMakingWebhook = new MatchMakingWebhook();
		// Check if the matchmaking is available by checking if the IP and port is part of Vigrid hive
		// Get server host and port from CLI arguments
		string m_ConnectAddress;
		string m_ConnectPort;
		GetCLIParam("connect", m_ConnectAddress);
		GetCLIParam("port", m_ConnectPort);

		if (m_ConnectAddress.Contains(":"))
		{
			array<string> connectParts = new array<string>;
			m_ConnectAddress.Split(":", connectParts);
			if (connectParts.Count() >= 2)
			{
				m_ConnectAddress = connectParts[0];
				m_ConnectPort = connectParts[1];
			}
		}

		b_MatchMakingAvailable = matchMakingWebhook.isMatchMakingAvailable( m_ConnectAddress, m_ConnectPort );
		BattleRoyaleUtils.Trace("MatchMaking Available: " + b_MatchMakingAvailable);

		return b_MatchMakingAvailable;
	}

	override void FilterDlcs(inout array<ref ModInfo> modArray)
	{
		BR_OverrideUI();
	}

	// Hide the DLC frame
	override void ShowDlcFrame(bool show)
	{
		super.ShowDlcFrame(false);
	}

	// Disable the play button
	override void Play()
	{

	}

	// Disable the server browser button
	override void OpenMenuServerBrowser()
	{

	}

	bool OnStart(bool force_restart = false)
	{
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

		Print("DBR: User: " + p_User.GetUid() + " | " + p_User.GetName());

		return true;
	}

	override void LoadMods()
	{
		super.LoadMods(); //initialize like normal

		//No BR mod found? ya just delete the lists....
		if( m_ModsSimple )
			delete m_ModsSimple;

		if( m_ModsDetailed )
			delete m_ModsDetailed;
	}

	void CreatePopup(string message, string button_text = "", PopupButtonCallback onClickCallback = NULL, string button_text_2 = "", PopupButtonCallback onClickCallback_2 = NULL )
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

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == MouseState.LEFT)
		{
			if (w == m_Play)
			{
				if ( b_MatchMakingAvailable )
				{
					m_LastFocusedButton = m_Play;
//					ref ClosePopupButtonCallback closecallback = new ClosePopupButtonCallback( this );
//					CreatePopup("Currently match making", "Close", closecallback, "", NULL);
					//--- A press is a FRESH search, so the attempt budget restarts here rather than
					//--- inside StartMatchMaking - which is also the automatic re-queue's entry
					//--- point, and must keep counting.
					m_MatchMakingTryCount = 0;
					StartMatchMaking();

					return true;
				} else {
					ConnectFromCLI();

					return true;
				}
			}
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

	// Avoid showing the tooltip
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w == m_Play)
		{
			if (IsFocusable(w))
			{
				ColorHighlight(w);
				return true;
			}
		}

		return super.OnMouseEnter(w, x, y);
	}

	/**
	 *  Connect straight to whatever -connect/-port/-password the client was launched with. This is
	 *  the non-matchmaking half of the Play button, factored out so -br-autoconnect goes through the
	 *  exact same call the button does rather than a parallel copy of it.
	 */
	void ConnectFromCLI()
	{
		string connect_address;
		string connect_port;
		string connect_password;
		GetCLIParam("connect", connect_address);
		GetCLIParam("port", connect_port);
		GetCLIParam("password", connect_password);

		g_Game.ConnectFromServerBrowser( connect_address, connect_port.ToInt(), connect_password );
	}

#ifdef DIAG
	/**
	 *  -br-autoconnect presses Play for you, so a local test run goes from launch to in-game with no
	 *  input. Diag-only on purpose: a retail client that reconnected on its own could never reach
	 *  its own main menu.
	 *
	 *  The latch is the whole point, and it is what the TODO this replaces warned about. Init() runs
	 *  again every time the menu is rebuilt - including the return from a disconnect - so firing on
	 *  each Init() would bounce the client back into the server the instant it left, with no way out
	 *  but killing the process. Static, so it survives the menu object and means "once per client
	 *  process" rather than "once per menu instance".
	 */
	void TryAutoConnect()
	{
		if (s_AutoConnectDone)
			return;

		if (!IsCLIParam("br-autoconnect"))
			return;

		//--- Matchmaking owns the button when it is available, and it resolves its own address, so
		//--- there is nothing here to skip past - let the normal path run.
		if (b_MatchMakingAvailable)
			return;

		//--- Nothing to connect TO. The flag can outlive the -connect it was paired with, in a
		//--- ClientLaunchParams line shared with LaunchOffline.bat, and firing anyway would hand
		//--- ConnectFromServerBrowser an empty address instead of leaving the menu alone.
		string autoconnect_address;
		if (!GetCLIParam("connect", autoconnect_address) || autoconnect_address == "")
		{
			BattleRoyaleUtils.Warn("[AutoConnect] -br-autoconnect set but no -connect address; staying on the menu");
			return;
		}

		s_AutoConnectDone = true;

		BattleRoyaleUtils.Info("[AutoConnect] -br-autoconnect set, connecting without the Play button");

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "ConnectFromCLI", BR_AUTOCONNECT_DELAY_MS, false);
	}
#endif

	void StartMatchMaking()
	{
		m_MatchMakingTryCount = m_MatchMakingTryCount + 1;

		// Change the play button text
		m_MatchMakingState = BattleRoyaleMatchMakingState.Searching;
		m_PlayButtonLabel.SetText("#STR_BR_MM_SEARCHING");
		m_Play.SetFlags(WidgetFlags.IGNOREPOINTER);
		ColorDisable(m_Play);

		// MatchMaking Webhook
		MatchMakingWebhook matchMakingWebhook = new MatchMakingWebhook();
		array<string> server_info = matchMakingWebhook.searchGame( modList );

		// TODO: Enable when find a way to re-enable the button
//		m_Play.SetFlags(WidgetFlags.IGNOREPOINTER);
//		ColorDisable(m_Play);

		// Connect to the server
		if( server_info.Count() == 3 )
		{
			// Disable the button state
			m_MatchMakingState = BattleRoyaleMatchMakingState.Connecting;
			m_Play.SetFlags(WidgetFlags.IGNOREPOINTER);
			ColorDisable(m_Play);

			// Format : [IP, Port, Password]
			string ip = server_info[0];
			int port = server_info[1].ToInt();
			string password = server_info[2];

			// If password is null, set it to empty
			if ( password == "null" )
			{
				password = "";
			}

			g_Game.ConnectFromServerBrowser( ip, port, password );

			// Reset the button state
			m_MatchMakingState = BattleRoyaleMatchMakingState.None;
			m_Play.ClearFlags(WidgetFlags.IGNOREPOINTER);
			ColorNormal(m_Play);

			if (IsMatchMakingAvailable())
			{
				m_PlayButtonLabel.SetText("#STR_BR_MM_MATCHMAKING");
			}
			else
			{
				m_PlayButtonLabel.SetText("#main_menu_play");
			}
		}
		else if ( server_info.Count() == 1 )
		{
			//--- "Come back later." Bounded, because it used to re-queue forever: nothing ever
			//--- compared m_MatchMakingTryCount against anything, so a matchmaker that is up but
			//--- never allocating kept the Play button disabled for as long as the client ran.
			if ( m_MatchMakingTryCount >= BR_MATCHMAKING_MAX_ATTEMPTS )
			{
				FailMatchMaking("no server allocated after " + m_MatchMakingTryCount + " attempts");
				return;
			}

			int sleep = server_info[0].ToInt();
			if( sleep < 1000 )
				sleep = 10000;  // If the sleep is anomalously low, set it to 10 seconds

			// Sleep before retrying
			BattleRoyaleUtils.Trace("Sleeping for " + sleep + " milliseconds before retrying");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "StartMatchMaking", sleep, false);
		}
		else
		{
			//--- Count() == 0 is the network failure / empty body case, and it matched NEITHER
			//--- branch above - so the state stayed Searching with Play permanently
			//--- IGNOREPOINTER-disabled and no retry queued. Any other count is a response shape
			//--- this client does not understand, which is the same dead end.
			FailMatchMaking("matchmaker returned " + server_info.Count() + " field(s)");
		}
	}

	/**
	 *  Every dead end in StartMatchMaking funnels through here: hand the Play button back and say so.
	 *
	 *  The point is that the player is never left looking at a disabled button with no way forward.
	 *  BattleRoyaleMatchMakingState.Failed has existed in the enum since the beginning and was never
	 *  reached; Update() paints the label only while Searching or Connecting, so this text sticks.
	 */
	void FailMatchMaking(string reason)
	{
		BattleRoyaleUtils.Warn("[MatchMaking] giving up: " + reason);

		m_MatchMakingState = BattleRoyaleMatchMakingState.Failed;
		//--- The next press is a fresh search with a fresh budget.
		m_MatchMakingTryCount = 0;

		m_Play.ClearFlags(WidgetFlags.IGNOREPOINTER);
		ColorNormal(m_Play);
		m_PlayButtonLabel.SetText("#STR_BR_MM_FAILED");
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		if (m_MatchMakingState == BattleRoyaleMatchMakingState.Searching)
		{
			m_PlayButtonLabel.SetText("#STR_BR_MM_SEARCHING " + m_MatchMakingTryCount);
		}
		else if (m_MatchMakingState == BattleRoyaleMatchMakingState.Connecting)
		{
			m_PlayButtonLabel.SetText("#STR_BR_MM_CONNECTING");
		}
	}

	void ColorDisable(Widget w)
	{
		#ifdef PLATFORM_WINDOWS
		SetFocus(null);
		#endif

		if (w)
		{
			ButtonWidget button = ButtonWidget.Cast(w);
			if (button)
			{
				button.SetTextColor(ColorManager.COLOR_DISABLED_TEXT);
			}
		}
	}

	override void ColorNormal(Widget w)
	{
		if (!w)
			return;

		int color_pnl = ARGB(0, 0, 0, 0);
		int color_lbl = ARGB(255, 255, 255, 255);
		int color_img = ARGB(255, 255, 255, 255);

		ButtonSetColor(w, color_pnl);
		ButtonSetTextColor(w, color_lbl);
		ImagenSetColor(w, color_img);
	}
};
