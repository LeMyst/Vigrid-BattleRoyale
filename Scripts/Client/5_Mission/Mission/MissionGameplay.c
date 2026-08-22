#ifndef SERVER
modded class MissionGameplay
{
	protected Widget m_BattleRoyaleHudRootWidget;
	protected ref BattleRoyaleHud m_BattleRoyaleHud;

	void MissionGameplay()
	{
		Print("MissionGameplay::MissionGameplay");
		m_BattleRoyaleHudRootWidget = null;
		m_BattleRoyale = null;

		// Add RPCs
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnSelection", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "HideSpawnSelection", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnPoint", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateHeatMap", this );
	}

	void ~MissionGameplay()
	{
		if (m_BattleRoyaleHudRootWidget)
		{
			m_BattleRoyaleHudRootWidget.Unlink();
		}

		m_BattleRoyale = null;

		// Remove RPCs
		GetRPCManager().RemoveRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnSelection" );
		GetRPCManager().RemoveRPC( RPC_DAYZBR_NAMESPACE, "HideSpawnSelection" );
		GetRPCManager().RemoveRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnPoint" );
		GetRPCManager().RemoveRPC( RPC_DAYZBR_NAMESPACE, "UpdateHeatMap" );
	}

	override void OnInit()
	{
		BattleRoyaleUtils.Trace("MissionGameplay::OnInit");
		super.OnInit();

		m_BattleRoyale = new BattleRoyaleClient;

		InitBRhud();
	}

	override BattleRoyaleClient GetBattleRoyale()
	{
		if ( !m_BattleRoyale )
		{
			m_BattleRoyale = new BattleRoyaleClient;
		}

		return m_BattleRoyale;
	}

	void InitBRhud()
	{
		BattleRoyaleUtils.Trace("Initializing BattleRoyale HUD");
		if(!m_BattleRoyaleHudRootWidget)
		{
			m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/br_hud.layout");

			m_BattleRoyaleHud = new BattleRoyaleHud( m_BattleRoyaleHudRootWidget );
			m_BattleRoyaleHud.ShowHud( true );
			BattleRoyaleUtils.Trace("HUD Initialized");
		}
	}

	void UpdateKillCount(int count)
	{
		m_BattleRoyaleHud.ShowKillCount( count > 0 );
		m_BattleRoyaleHud.SetKillCount( count );
	}

	//! How many people are currently spectating this player (#285). Zero hides the row rather than
	//! printing "0", exactly like the kill count above - and zero is a real pushed value, sent the
	//! moment the last watcher leaves, so this is also how the row goes away again.
	void UpdateAudienceCount(int count)
	{
		m_BattleRoyaleHud.ShowAudienceCount( count > 0 );
		m_BattleRoyaleHud.SetAudienceCount( count );
	}

	void HideCountdownTimer()
	{
		m_BattleRoyaleHud.ShowCountdown( false );
	}

	void UpdateCountdownTimer(int seconds)
	{
		m_BattleRoyaleHud.ShowCountdown( true );
		m_BattleRoyaleHud.SetCountdown( seconds );
	}

	void UpdatePlayerCount(int nb_players, int nb_groups)
	{
		//BattleRoyaleUtils.Trace(string.Format("UpdatePlayerCount: %1 %2", nb_players, nb_groups));
		m_BattleRoyaleHud.ShowCount( true );
		m_BattleRoyaleHud.SetCount( nb_players, nb_groups );
	}

	void UpdateZoneDistance(bool isInsideZone, float distExt, float distInt, float angle, int secondsToZone)
	{
		m_BattleRoyaleHud.ShowDistance(true);
		//m_BattleRoyaleHud.ShowDistance( distance > 0 );
		m_BattleRoyaleHud.SetDistance( isInsideZone, distExt, distInt, angle, secondsToZone );
	}

	void HideDistance()
	{
		m_BattleRoyaleHud.ShowDistance( false );
	}

	/**
	 *  Make sure the death screen never outlives the mission.
	 *
	 *  Vanilla's OnMissionFinish does call DestroyAllMenus(), but the death screen was observed still
	 *  on screen back at the main menu, so it is closed explicitly first rather than relying on the
	 *  order of that teardown.
	 */
	override void OnMissionFinish()
	{
		//--- The static reference rather than FindMenu, which returns NULL whenever the engine's
		//--- current-menu pointer has been nulled - precisely the case where the screen is still on
		//--- the workspace and most needs closing.
		DeathScreenMenu death_menu = DeathScreenMenu.GetInstance();
		if( death_menu )
		{
			BattleRoyaleUtils.Trace("[Spectate] closing death screen on mission finish");
			death_menu.Close();
		}

		super.OnMissionFinish();
	}

	/**
	 *  Show or hide the VANILLA vitals hud - health, hunger, thirst, stamina, badges, quickbar,
	 *  crosshair, stance icons.
	 *
	 *  Vanilla only ever calls m_HudRootWidget.Show(true), and only behind
	 *  "player && m_LifeState == EPlayerStates.ALIVE && !player.IsUnconscious()"
	 *  (missiongameplay.c:483), so nothing puts the hud away on death and a spectator would
	 *  watch the whole match behind their own corpse's zeroed vitals. That same gate is why nothing
	 *  undoes this from under us - but the load-bearing half is m_LifeState, NOT the null test.
	 *  GetGame().GetPlayer() keeps returning the CORPSE while spectating (measured), so "player" is
	 *  perfectly non-null here; it is simply never ALIVE again.
	 *
	 *  Hides "HudPanel" rather than m_HudRootWidget itself, because the CHAT FRAME is a sibling
	 *  under that same root - m_Chat.Init(m_HudRootWidget.FindAnyWidget("ChatFrameWidget")),
	 *  missiongameplay.c:129 - so hiding the root took chat away from the spectator too. "HudPanel"
	 *  is exactly what vanilla hands to m_Hud.Init (missiongameplay.c:133), i.e. the vitals tree and
	 *  nothing else. Falls back to the whole root if that widget ever moves or is renamed.
	 *
	 *  The BR hud is a separate widget tree and is untouched either way.
	 */
	void SetVanillaHudVisible(bool visible)
	{
		if( !m_HudRootWidget )
			return;

		Widget hud_panel = m_HudRootWidget.FindAnyWidget( "HudPanel" );
		if( hud_panel )
		{
			hud_panel.Show( visible );
			return;
		}

		m_HudRootWidget.Show( visible );
	}

	override void OnUpdate( float timeslice )
	{
		super.OnUpdate( timeslice ); //no more using fade out because it causes way to much compatibility issues, instead we'll use widgets

		m_BattleRoyale.Update( timeslice ); //send tick to br client

		if (GetUApi() && !m_UIManager.IsMenuOpen(MENU_CHAT_INPUT)) {
			if (GetUApi().GetInputByID(UADayZBRReadyUp).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).ReadyUp();
			}
			if (GetUApi().GetInputByID(UADayZBRUnstuck).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).Unstuck();
			}
			//--- Admin spectate. Discrete edges only - the free camera's continuous WASD/mouse axes
			//--- are read in BattleRoyaleSpectatorCamera.UpdateFreeCamera, which is where the frame
			//--- time they need lives.
			//---
			//--- No client-side admin check gates these. Every one is refused server-side by
			//--- AdminEligibility, so the worst a non-admin achieves by pressing F3 is one ignored
			//--- packet and a Warn in the server log. The RPC is only sent at all when the client
			//--- believes it is an admin, which keeps the ordinary player's keypress entirely local.
			if (GetUApi().GetInputByID(UADayZBRAdminSpectate).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).AdminSpectateToggle();
			}
			if (GetUApi().GetInputByID(UADayZBRSpectateMode).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).AdminSpectateCycleMode();
			}
			if (GetUApi().GetInputByID(UADayZBRSpectateNext).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).AdminSpectateCycle( 1 );
			}
			if (GetUApi().GetInputByID(UADayZBRSpectatePrev).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).AdminSpectateCycle( -1 );
			}
			//--- The one admin key with no server half at all: the skeleton overlay is COT's own
			//--- client-side renderer, so there is nothing to authorise on the wire. COT applies its
			//--- own ESP.View permission check inside the call.
			if (GetUApi().GetInputByID(UADayZBRSpectateSkeleton).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).AdminSpectateToggleSkeleton();
			}
			//--- Toggle, so the same key closes the board again. Answerable in any state, though it
			//--- is mostly a lobby feature.
			//---
			//--- OPENING is gated on nothing else being open, and the parent is NULL - the same two
			//--- rules VigridPartyMenu and VigridMapMenu already follow. Without the gate F4 stacked
			//--- the board over the death screen, the map, the inventory and the Esc menu; passing
			//--- GetMenu() as the parent then made whatever was underneath the board's parent and
			//--- handed focus back to it on close. The gate is deliberately "any menu at all" rather
			//--- than a list of ids, because a list is what drifted last time.
			//---
			//--- Closing is NOT gated: while the board is open GetMenu() returns it, so testing the
			//--- gate first would make the key one-way.
			if (GetUApi().GetInputByID(UADayZBRLeaderboard).LocalPress()) {
				if (m_UIManager.IsMenuOpen(MENU_BR_LEADERBOARD)) {
					m_UIManager.CloseMenu(MENU_BR_LEADERBOARD);
				} else if (!m_UIManager.GetMenu()) {
					GetUIManager().EnterScriptedMenu(MENU_BR_LEADERBOARD, NULL);
				}
			}
		}
	}

#ifdef DIAG
	/**
	 *  Open spawn selection with plausible made-up data, with no server involved.
	 *
	 *  Reached from the diag menu's "Open Spawn Menu" entry. It is the same
	 *  opening sequence ShowSpawnSelection uses - parentless, on a cleared stack, for the reason
	 *  spelled out there - so what is exercised is the real menu, not a lookalike.
	 */
	void BR_DiagOpenSpawnSelection()
	{
		GetUIManager().CloseAll();

		SpawnSelectionMenu diag_menu = SpawnSelectionMenu.Cast(GetUIManager().EnterScriptedMenu(MENU_SPAWN_SELECTION, NULL));
		if (!diag_menu)
		{
			BattleRoyaleUtils.Warn("BR_DiagOpenSpawnSelection: could not open the spawn selection menu");
			return;
		}

		diag_menu.SetInitialCountdown(45);
		diag_menu.SetSpawnSize(50);
		diag_menu.SetFirstZone(Vector(6000, 0, 7777), 1500);
	}
#endif

	void ShowSpawnSelection(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param4<int, float, vector, float> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SHOWSPAWNSELECTION RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("ShowSpawnSelection: %1 %2 %3 %4", data.param1, data.param2, data.param3, data.param4));

			//--- Close whatever the player happens to have open first - the escape menu above all.
			//---
			//--- This used to pass GetMenu() as the PARENT of the spawn map. GetMenu() is whatever is
			//--- currently open, so a player sitting in the escape menu when spawn selection began got
			//--- the map opened as a *child* of it: the escape menu stayed underneath as the parent and
			//--- could not be dismissed, which left them unable to use the map or talk to their party
			//--- for the whole 30 seconds. Opening it parentless, on a cleared stack, is unconditional.
			GetUIManager().CloseAll();

			SpawnSelectionMenu m = SpawnSelectionMenu.Cast(GetUIManager().EnterScriptedMenu(MENU_SPAWN_SELECTION, NULL));
			if (!m)
			{
				//--- Warn, not Error: Error raises a VM exception, and a player without the map is
				//--- still better off than a dead client - the server assigns them a spawn anyway.
				BattleRoyaleUtils.Warn("ShowSpawnSelection: could not open the spawn selection menu");
				return;
			}

			m.SetInitialCountdown(data.param1);
			m.SetSpawnSize(data.param2);
			m.SetFirstZone(data.param3, data.param4);
		}
	}

	void HideSpawnSelection(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("HideSpawnSelection");
			SpawnSelectionMenu m = SpawnSelectionMenu.Cast(GetUIManager().FindMenu(MENU_SPAWN_SELECTION));
			if (m)
			{
				m.Close();
			}
		}
	}

	void ShowSpawnPoint(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<PlayerBase, vector, int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SHOWSPAWNPOINT RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("ShowSpawnPoint: %1 %2 %3", data.param1, data.param2, data.param3));
			SpawnSelectionMenu m = SpawnSelectionMenu.Cast(GetUIManager().FindMenu(MENU_SPAWN_SELECTION));
			if (m)
			{
				m.SetTeammateSpawnPoint(data.param1, data.param2, data.param3);
			}
		}
	}

	void UpdateHeatMap(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<array<vector>> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ UPDATEHEATMAP RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("UpdateHeatMap");
			SpawnSelectionMenu m = SpawnSelectionMenu.Cast(GetUIManager().FindMenu(MENU_SPAWN_SELECTION));
			if (m)
			{
				m.UpdateHeatMap(data.param1);
			}
		}
	}
}
#endif
