#ifndef SERVER
modded class MainMenu
{
	protected ImageWidget m_Logo;
	protected ref array<ref ModInfo> modList = new array<ref ModInfo>();
	protected TextWidget m_PlayButtonLabel;
	protected TextWidget m_PlayStatus;

	//--- -connect / -port / -password, with "-connect=ip:port" split. Empty address = no target.
	protected string m_ConnectAddress;
	protected string m_ConnectPort;
	protected string m_ConnectPassword;

	//--- All matchmaking runs from Update() off these: no call queue, so nothing outlives the menu.
	protected BattleRoyaleMatchMakingState m_MatchMakingState = BattleRoyaleMatchMakingState.None;
	protected bool m_MatchMakingAvailable;
	protected ref MatchMakingRequest m_MMRequest;
	protected float m_MMRequestSentAt;
	protected float m_MMSearchStartedAt;
	protected float m_MMNextAttemptAt;
	protected float m_MMFallbackAt;
	protected int m_MMAttempt;

	//--- What was last painted on the Play label, so the searching clock writes once a second.
	protected int m_PlayLabelSecond = -1;

#ifdef DIAG
	//--- one-shot for the whole client process, not per menu instance - see TryAutoConnect.
	protected static bool s_AutoConnectDone;
	protected float m_AutoConnectAt;
#endif

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/MainMenu/main_menu.layout");

		m_Play						= layoutRoot.FindAnyWidget("play");
		m_PlayButtonLabel			= TextWidget.Cast(m_Play.FindAnyWidget("play_label"));
		m_PlayStatus				= TextWidget.Cast(layoutRoot.FindAnyWidget("play_status"));
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
		//--- No GetOnModMenuVisibilityChanged subscription: it only ever re-hid the DLC frame through
		//--- ShowDlcFrame, which 1.30 marks [Obsolete("No replacement")], and our LoadMods() deletes
		//--- the mods list whose visibility it reports. BR_OverrideUI() hides the frame once and
		//--- nothing shows it again.

		g_Game.SetLoadState(DayZLoadState.MAIN_MENU_CONTROLLER_SELECT);

		// Get mod list
		GetGame().GetModInfos(modList);

		m_Logo = ImageWidget.Cast( layoutRoot.FindAnyWidget( "dayz_logo" ) );
		if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
			BattleRoyaleUtils.Warn("MainMenu: failed to load the logo image");

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
			//--- Expansion segment is optional - same reasoning as InGameMenu.SetGameVersion().
			string version_line = "Client #main_menu_version" + " " + version;
#ifdef DZ_Expansion_Core
			version_line = version_line + " | " + GetDayZGame().GetExpansionClientVersion();
#endif
			version_line = version_line + " | " + GetDayZGame().GetBattleRoyaleClientVersion();
			m_Version.SetText( version_line );
		}
		else
		{
			m_Version.SetText( "DayZ SA #main_menu_version" + " " + version );
		}

		if(OnStart())
		{
			BattleRoyaleUtils.Trace("MainMenu: OnStart ok");
		}
		else
		{
			BattleRoyaleUtils.Warn("MainMenu: OnStart failed");
		}

		BR_OverrideUI();

		//--- The button is usable at once as PLAY; MATCHMAKING replaces it only if the API says so.
		ReadConnectTarget();
		SetMatchMakingState(BattleRoyaleMatchMakingState.None);
		StartAvailabilityCheck();

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

	void ReadConnectTarget()
	{
		m_ConnectAddress = "";
		m_ConnectPort = "";
		m_ConnectPassword = "";
		GetCLIParam("connect", m_ConnectAddress);
		GetCLIParam("port", m_ConnectPort);
		GetCLIParam("password", m_ConnectPassword);

		if (m_ConnectAddress.Contains(":"))
		{
			array<string> connect_parts = new array<string>;
			m_ConnectAddress.Split(":", connect_parts);
			if (connect_parts.Count() >= 2)
			{
				m_ConnectAddress = connect_parts[0];
				m_ConnectPort = connect_parts[1];
			}
		}

		m_ConnectAddress = m_ConnectAddress.Trim();
		if (m_ConnectPort == "")
			m_ConnectPort = "2302";
	}

	bool HasConnectTarget()
	{
		return m_ConnectAddress != "";
	}

	//! "ip:port" of the -connect target: the site matches within that server's shard.
	string GetConnectServer()
	{
		return m_ConnectAddress + ":" + m_ConnectPort;
	}

	//! Matchmaking is keyed on the -connect target, so with none there is nothing to ask.
	void StartAvailabilityCheck()
	{
		if (!HasConnectTarget())
		{
			BattleRoyaleUtils.Info("[MatchMaking] no -connect target: PLAY opens the server browser");
			return;
		}

		m_MMRequest = MatchMakingWebhook.CheckAvailability(GetConnectServer());
		if (!m_MMRequest)
			return;

		m_MMRequestSentAt = GetGame().GetTickTime();
		SetMatchMakingState(BattleRoyaleMatchMakingState.Checking);
	}

	//--- FilterDlcs and ShowDlcFrame used to be overridden here. Both are [Obsolete("No replacement")]
	//--- in 1.30, vanilla has not called FilterDlcs since at least 1.29 (it calls FilterDlcsEx), and
	//--- an override of an obsolete method stops compiling the day Bohemia deletes it.

#ifdef ENABLE_CAROUSEL
	//--- 1.30 moved vanilla's news carousel into RefreshNewsCarousel() and now calls it from OnShow()
	//--- and OnDLCChange() too, not just from Init(). This menu builds its own layout, which has no
	//--- "carousel_Frame", so m_NewsCarouselFrame is NULL here - and NewsCarousel hands that NULL to
	//--- CreateWidgets as the parent, which puts Bohemia's news panel at the workspace ROOT, over the
	//--- Battle Royale menu. This menu has no carousel. The guard is vanilla's own (mainmenu.c:1-3).
	override protected void RefreshNewsCarousel()
	{
		BattleRoyaleUtils.Debug("MainMenu: vanilla news carousel suppressed");
	}
#endif

	//--- Vanilla's Play() joins the last session or the "best" server; ours is OnPlayPressed().
	override void Play()
	{

	}

	bool OnStart(bool force_restart = false)
	{
		BattleRoyaleUtils.Trace("MainMenu.OnStart");
		BiosUserManager p_UserManager = GetGame().GetUserManager();
		if(!p_UserManager)
		{
			BattleRoyaleUtils.Warn("MainMenu.OnStart: no BiosUserManager");
			return false;
		}

		BiosUser p_User = p_UserManager.GetTitleInitiator();
		if(!p_User)
		{
			BattleRoyaleUtils.Warn("MainMenu.OnStart: no title-initiator BiosUser");
			return false;
		}

		BattleRoyaleUtils.Trace("MainMenu.OnStart: user " + p_User.GetName());

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
				m_LastFocusedButton = m_Play;
				OnPlayPressed();
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
	 *  What the Play button does, in every state. -br-autoconnect calls this too, so a diag run
	 *  goes through the exact path a click does.
	 */
	void OnPlayPressed()
	{
		switch (m_MatchMakingState)
		{
			case BattleRoyaleMatchMakingState.Available:
				StartSearch();
				break;

			case BattleRoyaleMatchMakingState.Searching:
				CancelSearch();
				break;

			case BattleRoyaleMatchMakingState.Fallback:
				//--- The reason is on screen already; a click skips the wait.
				ConnectFromCLI();
				break;

			case BattleRoyaleMatchMakingState.Connecting:
				break;

			default:
				//--- None, or Checking: an unanswered check must not hold the button hostage.
				m_MMRequest = NULL;
				if (HasConnectTarget())
				{
					ConnectFromCLI();
				}
				else
				{
					OpenMenuServerBrowser();
				}
				break;
		}
	}

	//! Connect straight to the -connect target: plain PLAY, and the matchmaking fallback.
	void ConnectFromCLI()
	{
		if (!HasConnectTarget())
		{
			BattleRoyaleUtils.Warn("[MatchMaking] no -connect target to join");
			SetMatchMakingState(BattleRoyaleMatchMakingState.None);
			return;
		}

		BattleRoyaleUtils.Info("[MatchMaking] joining " + m_ConnectAddress + ":" + m_ConnectPort);
		SetMatchMakingState(BattleRoyaleMatchMakingState.Connecting);
		g_Game.ConnectFromServerBrowser( m_ConnectAddress, m_ConnectPort.ToInt(), m_ConnectPassword );
	}

#ifdef DIAG
	/**
	 *  -br-autoconnect presses Play for you, so a local test run goes from launch to in-game with no
	 *  input. Diag-only on purpose: a retail client that reconnected on its own could never reach
	 *  its own main menu.
	 *
	 *  The latch is the whole point. Init() runs again every time the menu is rebuilt - including the
	 *  return from a disconnect - so firing on each Init() would bounce the client back into the
	 *  server the instant it left. Static, so it means "once per client process".
	 *
	 *  The press waits for the availability check, so it searches when matchmaking is up and joins
	 *  -connect directly when it is not - the same choice a player's click would get.
	 */
	void TryAutoConnect()
	{
		if (s_AutoConnectDone)
			return;

		if (!IsCLIParam("br-autoconnect"))
			return;

		//--- Nothing to connect TO: the flag can outlive the -connect it was paired with.
		if (!HasConnectTarget())
		{
			BattleRoyaleUtils.Warn("[AutoConnect] -br-autoconnect set but no -connect address; staying on the menu");
			return;
		}

		m_AutoConnectAt = GetGame().GetTickTime() + BR_AUTOCONNECT_DELAY_MS / 1000.0;
	}

	void UpdateAutoConnect()
	{
		if (m_AutoConnectAt <= 0 || s_AutoConnectDone)
			return;

		if (GetGame().GetTickTime() < m_AutoConnectAt)
			return;

		if (m_MatchMakingState == BattleRoyaleMatchMakingState.Checking)
			return;

		s_AutoConnectDone = true;
		m_AutoConnectAt = 0;
		BattleRoyaleUtils.Info("[AutoConnect] -br-autoconnect set, pressing Play");
		OnPlayPressed();
	}
#endif

	void StartSearch()
	{
		BattleRoyaleUtils.Info("[MatchMaking] search started");
		m_MMSearchStartedAt = GetGame().GetTickTime();
		m_MMAttempt = 0;
		m_PlayLabelSecond = -1;
		SetMatchMakingState(BattleRoyaleMatchMakingState.Searching);
		SendSearch();
	}

	void SendSearch()
	{
		m_MMAttempt = m_MMAttempt + 1;
		m_MMNextAttemptAt = 0;
		m_MMRequest = MatchMakingWebhook.Search(GetConnectServer(), modList);
		m_MMRequestSentAt = GetGame().GetTickTime();

		if (!m_MMRequest)
			StartFallback("#STR_BR_MM_UNAVAILABLE", "no REST context");
	}

	void CancelSearch()
	{
		BattleRoyaleUtils.Info("[MatchMaking] search cancelled after " + m_MMAttempt + " attempt(s)");
		//--- A late reply lands in a request nobody holds any more, and is ignored.
		m_MMRequest = NULL;
		SetMatchMakingState(BattleRoyaleMatchMakingState.Available);
	}

	/**
	 *  Matchmaking gave up: say why, then join the -connect server the client was launched for. The
	 *  pause is only so the reason is readable before the connect screen replaces the menu.
	 */
	void StartFallback(string reason_key, string log_reason)
	{
		m_MMRequest = NULL;

		if (!HasConnectTarget())
		{
			BattleRoyaleUtils.Warn("[MatchMaking] giving up (" + log_reason + "), and no -connect to fall back to");
			SetMatchMakingState(BattleRoyaleMatchMakingState.Available);
			SetPlayStatus(reason_key);
			return;
		}

		BattleRoyaleUtils.Warn("[MatchMaking] giving up (" + log_reason + "), joining -connect directly");
		m_MMFallbackAt = GetGame().GetTickTime() + BR_MATCHMAKING_FALLBACK_DELAY_MS / 1000.0;
		SetMatchMakingState(BattleRoyaleMatchMakingState.Fallback);
		SetPlayStatus(reason_key);
	}

	//! The request's outcome, with the menu's own timeout applied on top of the engine's.
	int PollRequest()
	{
		if (!m_MMRequest)
			return MatchMakingRequestStatus.MM_ERROR;

		int status = m_MMRequest.GetStatus();
		if (status == MatchMakingRequestStatus.MM_PENDING)
		{
			if (GetGame().GetTickTime() - m_MMRequestSentAt > BR_MATCHMAKING_TIMEOUT_S)
				return MatchMakingRequestStatus.MM_TIMEOUT;
		}

		return status;
	}

	void UpdateAvailabilityCheck()
	{
		int status = PollRequest();
		if (status == MatchMakingRequestStatus.MM_PENDING)
			return;

		m_MatchMakingAvailable = false;
		if (status == MatchMakingRequestStatus.MM_SUCCESS)
			m_MatchMakingAvailable = MatchMakingWebhook.ParseAvailable(m_MMRequest.GetData());

		m_MMRequest = NULL;
		BattleRoyaleUtils.Info("[MatchMaking] available: " + m_MatchMakingAvailable + " (status " + status + ")");

		if (m_MatchMakingAvailable)
		{
			SetMatchMakingState(BattleRoyaleMatchMakingState.Available);
		}
		else
		{
			SetMatchMakingState(BattleRoyaleMatchMakingState.None);
		}
	}

	void UpdateSearch()
	{
		float now = GetGame().GetTickTime();

		if (m_MMRequest)
		{
			int status = PollRequest();
			if (status == MatchMakingRequestStatus.MM_PENDING)
			{
				PaintSearchClock(now);
				return;
			}

			if (status != MatchMakingRequestStatus.MM_SUCCESS)
			{
				StartFallback("#STR_BR_MM_UNAVAILABLE", "attempt " + m_MMAttempt + " status " + status);
				return;
			}

			string reply = m_MMRequest.GetData();
			m_MMRequest = NULL;

			string ip;
			int port;
			string password;
			if (MatchMakingWebhook.ParseServer(reply, ip, port, password))
			{
				BattleRoyaleUtils.Info("[MatchMaking] attempt " + m_MMAttempt + " found " + ip + ":" + port);
				SetMatchMakingState(BattleRoyaleMatchMakingState.Connecting);
				g_Game.ConnectFromServerBrowser(ip, port, password);
				return;
			}

			int retry_ms = MatchMakingWebhook.ParseRetryMs(reply);
			float retry_s = retry_ms / 1000.0;
			if (now + retry_s - m_MMSearchStartedAt > BR_MATCHMAKING_SEARCH_BUDGET_S)
			{
				StartFallback("#STR_BR_MM_NO_MATCH", "no match after " + m_MMAttempt + " attempt(s)");
				return;
			}

			BattleRoyaleUtils.Debug("[MatchMaking] attempt " + m_MMAttempt + " no match, retrying in " + retry_ms + " ms");
			m_MMNextAttemptAt = now + retry_s;
		}
		else if (now >= m_MMNextAttemptAt)
		{
			SendSearch();
			if (m_MatchMakingState != BattleRoyaleMatchMakingState.Searching)
				return;
		}

		PaintSearchClock(now);
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		switch (m_MatchMakingState)
		{
			case BattleRoyaleMatchMakingState.Checking:
				UpdateAvailabilityCheck();
				break;

			case BattleRoyaleMatchMakingState.Searching:
				UpdateSearch();
				break;

			case BattleRoyaleMatchMakingState.Fallback:
				if (GetGame().GetTickTime() >= m_MMFallbackAt)
					ConnectFromCLI();
				break;
		}

#ifdef DIAG
		UpdateAutoConnect();
#endif
	}

	override void OnShow()
	{
		super.OnShow();

		//--- Back from a connect that did not stick (refused, kicked, cancelled): hand the button back.
		if (m_MatchMakingState == BattleRoyaleMatchMakingState.Connecting || m_MatchMakingState == BattleRoyaleMatchMakingState.Fallback)
		{
			if (m_MatchMakingAvailable)
			{
				SetMatchMakingState(BattleRoyaleMatchMakingState.Available);
			}
			else
			{
				SetMatchMakingState(BattleRoyaleMatchMakingState.None);
			}
		}
	}

	override void OnHide()
	{
		super.OnHide();

		//--- A match found while the player is in Options must not yank them out of it.
		if (m_MatchMakingState == BattleRoyaleMatchMakingState.Searching)
			CancelSearch();
	}

	//! Sets the state and repaints the button and its status line to match it.
	void SetMatchMakingState(BattleRoyaleMatchMakingState state)
	{
		m_MatchMakingState = state;
		m_PlayLabelSecond = -1;

		bool clickable = true;
		switch (state)
		{
			case BattleRoyaleMatchMakingState.Available:
				m_PlayButtonLabel.SetText("#STR_BR_MM_MATCHMAKING");
				SetPlayStatus("");
				break;

			case BattleRoyaleMatchMakingState.Searching:
				PaintSearchClock(GetGame().GetTickTime());
				SetPlayStatus("#STR_BR_MM_CANCEL_HINT");
				break;

			case BattleRoyaleMatchMakingState.Connecting:
				m_PlayButtonLabel.SetText("#STR_BR_MM_CONNECTING");
				SetPlayStatus("");
				clickable = false;
				break;

			case BattleRoyaleMatchMakingState.Fallback:
				//--- The caller sets the status line: it carries the reason.
				m_PlayButtonLabel.SetText("#STR_BR_MM_CONNECTING");
				break;

			default:
				m_PlayButtonLabel.SetText("#main_menu_play");
				SetPlayStatus("");
				break;
		}

		if (clickable)
		{
			m_Play.ClearFlags(WidgetFlags.IGNOREPOINTER);
			ColorNormal(m_Play);
		}
		else
		{
			m_Play.SetFlags(WidgetFlags.IGNOREPOINTER);
			ColorDisable(m_Play);
		}
	}

	//! "SEARCHING... 1:07", written only when the second changes.
	void PaintSearchClock(float now)
	{
		int elapsed = now - m_MMSearchStartedAt;
		if (elapsed < 0)
			elapsed = 0;

		if (elapsed == m_PlayLabelSecond)
			return;

		m_PlayLabelSecond = elapsed;

		int minutes = elapsed / 60;
		int seconds = elapsed % 60;
		string clock = minutes.ToString() + ":" + seconds.ToStringLen(2);
		m_PlayButtonLabel.SetText("#STR_BR_MM_SEARCHING " + clock);
	}

	//! The small line under the Play label. Empty hides it.
	void SetPlayStatus(string text)
	{
		if (!m_PlayStatus)
			return;

		m_PlayStatus.SetText(text);
		m_PlayStatus.Show(text != "");
	}

	void ColorDisable(Widget w)
	{
		#ifdef PLATFORM_WINDOWS
		SetFocus(null);
		#endif

		//--- ButtonSetTextColor reaches the "play_label" child; the ButtonWidget's own text is unused.
		ButtonSetColor(w, ARGB(0, 0, 0, 0));
		ButtonSetTextColor(w, ColorManager.COLOR_DISABLED_TEXT);
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
#endif
