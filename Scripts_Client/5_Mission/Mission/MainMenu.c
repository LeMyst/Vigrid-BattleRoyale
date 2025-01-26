#ifndef SERVER
modded class MainMenu
{
	protected ImageWidget m_Logo;
	protected ref array<ref ModInfo> modList = new array<ref ModInfo>();
	protected TextWidget m_PlayButtonLabel;
	protected bool b_MatchMakingAvailable;

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
		ref array<string> funny_strings = {
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

		BR_OverrideUI();

		CheckMatchMakingAvailability();

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

	void CheckMatchMakingAvailability()
	{
		// Change the play button text
		m_PlayButtonLabel.SetText("LOGIN...");

		// MatchMaking Webhook
		MatchMakingWebhook matchMakingWebhook = new MatchMakingWebhook();
		// Check if the matchmaking is available by checking if the IP and port is part of Vigrid hive
		// Get server host and port from CLI arguments
		string m_ConnectAddress;
		string m_ConnectPort;
		GetCLIParam("connect", m_ConnectAddress);
		GetCLIParam("port", m_ConnectPort);
		b_MatchMakingAvailable = matchMakingWebhook.isMatchMakingAvailable( m_ConnectAddress, m_ConnectPort );
		BattleRoyaleUtils.Trace("MatchMaking Available: " + b_MatchMakingAvailable);

		if( !b_MatchMakingAvailable )
		{
			m_PlayButtonLabel.SetText("#main_menu_play")
		}
		else
		{
			m_PlayButtonLabel.SetText("MATCHMAKING");
		}
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

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == MouseState.LEFT)
		{
			if (w == m_Play)
			{
				if ( b_MatchMakingAvailable )
				{
					m_LastFocusedButton = m_Play;
					StartMatchMaking();

					return true;
				} else {
					string m_ConnectAddress;
					string m_ConnectPort;
					string m_ConnectPassword;
					GetCLIParam("connect", m_ConnectAddress);
					GetCLIParam("port", m_ConnectPort);
					GetCLIParam("password", m_ConnectPassword);

					g_Game.ConnectFromServerBrowser( m_ConnectAddress, m_ConnectPort.ToInt(), m_ConnectPassword );

					return true;
				}
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

	void StartMatchMaking()
	{
		// MatchMaking Webhook
		MatchMakingWebhook matchMakingWebhook = new MatchMakingWebhook();
		array<string> server_info = matchMakingWebhook.searchGame( modList );

		// Connect to the server
		if( server_info.Count() > 0 )
		{
			// Format : [IP, Port, Password]
			g_Game.ConnectFromServerBrowser( server_info[0], server_info[1].ToInt(), server_info[2] );
		}
	}
};
