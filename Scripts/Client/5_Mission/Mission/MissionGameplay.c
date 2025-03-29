#ifndef SERVER
modded class MissionGameplay
{
	protected Widget m_BattleRoyaleHudRootWidget;
	protected ref BattleRoyaleHud m_BattleRoyaleHud;

#ifdef BR_MINIMAP
	protected ref MapWidget m_MiniMap;
	protected ref CanvasWidget m_MiniMapCanvas;
	protected ref ImageWidget m_UserMarkerImageWidget;
	protected float f_MiniMapScale = 0.1;
	protected bool b_MiniMapShow = true;

	vector spawn_point;
	float radius;
#endif

#ifdef SPECTATOR
	protected bool is_spectator;
#endif

	void MissionGameplay()
	{
		Print("MissionGameplay::MissionGameplay");
		m_BattleRoyaleHudRootWidget = null;
		m_BattleRoyale = null;

#ifdef SPECTATOR
		is_spectator = false;
#endif

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
#ifdef BR_MINIMAP
			m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/br_hud_minimap.layout");
			m_MiniMap = MapWidget.Cast(m_BattleRoyaleHudRootWidget.FindAnyWidget("MiniMap"));
			m_MiniMapCanvas = CanvasWidget.Cast(m_BattleRoyaleHudRootWidget.FindAnyWidget("CanvasMiniMap"));
			m_UserMarkerImageWidget = ImageWidget.Cast( m_BattleRoyaleHudRootWidget.FindAnyWidget( "UserMarker" ) );
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName( this, "UpdateMiniMap", 200, true );

			float canvas_width;
			float canvas_height;
			m_MiniMapCanvas.GetSize(canvas_width, canvas_height);
			BattleRoyaleUtils.Trace(canvas_width);
			BattleRoyaleUtils.Trace(canvas_height);
#else
			m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/br_hud.layout");
#endif

			m_BattleRoyaleHud = new BattleRoyaleHud( m_BattleRoyaleHudRootWidget );
			m_BattleRoyaleHud.ShowHud( true );
			BattleRoyaleUtils.Trace("HUD Initialized");
		}
	}

#ifdef SPECTATOR
	bool IsInSpectator()
	{
		return is_spectator;
	}

	void InitSpectator()
	{
		BattleRoyaleUtils.Trace("Initializing Spectator HUD");
		m_BattleRoyaleHud.InitSpectator();

		is_spectator = true;

		HideHud();
	}
#endif

	void UpdateKillCount(int count)
	{
		m_BattleRoyaleHud.ShowKillCount( count > 0 );
		m_BattleRoyaleHud.SetKillCount( count );
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

	void UpdateZoneDistance(bool isInsideZone, float distExt, float distInt, float angle)
	{
		m_BattleRoyaleHud.ShowDistance(true);
		//m_BattleRoyaleHud.ShowDistance( distance > 0 );
		m_BattleRoyaleHud.SetDistance( isInsideZone, distExt, distInt, angle );
	}

	void HideDistance()
	{
		m_BattleRoyaleHud.ShowDistance( false );
	}

#ifdef BR_MINIMAP
	void UpdateMiniMap()
	{
		UpdateMiniMap(GetGame().GetCurrentCameraPosition());
	}

	void UpdateMiniMap(vector pos)
	{
		//m_BattleRoyaleHudRootWidget.Show( true );

		if( b_MiniMapShow )
		{
			m_MiniMap.Show( true )
		}
		else
		{
			m_MiniMap.Show( false );
			return;
		}

		if(!m_MiniMap)
			return;

		PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
		if (!player)
			return;

		m_MiniMap.SetMapPos( Vector(pos[0], 0, pos[2]) );
		m_MiniMap.SetScale( f_MiniMapScale );

		m_MiniMap.ClearUserMarks();
		//m_MiniMap.AddUserMark(pos, "", ARGB(192,0,0,255), "\\dz\\gear\\navigation\\data\\map_bush_ca.paa");

		BattleRoyaleClient brc = BattleRoyaleClient.Cast( m_BattleRoyale );
		if(brc && brc.GetNextArea())
			m_MiniMap.AddUserMark(brc.GetNextArea().GetCenter(), "", ARGB(255,255,0,0), "\\dz\\gear\\navigation\\data\\map_bush_ca.paa");

		if(m_UserMarkerImageWidget)
		{
			//vector rot = player.GetYawPitchRoll();
			//float angle = Math.Round(rot[0]);
			//if (angle < 0)
			//{
			//    angle = 360 + angle;
			//}
			//m_UserMarkerImageWidget.SetRotation(0, 0, angle);


			//float camera_angle = GetGame().GetCurrentCameraDirection().VectorToAngles()[0];
			//camera_angle = Math.NormalizeAngle(camera_angle);

			m_UserMarkerImageWidget.SetRotation( 0, 0, Math.Round( Math.NormalizeAngle( GetGame().GetCurrentCameraDirection().VectorToAngles()[0] ) ), true );
		}

		//! TODO: Display Party stuff here

		/*
		ref map<string, vector> positions = party_manager.GetPositions();
		foreach(string player_id, vector position: positions)
		{
			m_MiniMap.AddUserMark(position, "", ARGB(192,0,0,255), "\\dz\\gear\\navigation\\data\\map_bush_ca.paa");
		}
		*/

		// TODO: Temporary placeholders
		UpdateMiniMapCanvas();
	}

	void UpdateMiniMapCanvas()
	{
		if (m_MiniMapCanvas && spawn_point && radius)
		{
			BattleRoyaleUtils.Trace("Update MiniMap Canvas!");
			m_MiniMapCanvas.Clear();

			vector m_edgePos_A = spawn_point;
			m_edgePos_A[0] = m_edgePos_A[0] + radius;

			vector m_edgePos_B = spawn_point;
			m_edgePos_B[2] = m_edgePos_B[2] + radius;

			vector mapPos_edge_A = m_MiniMap.MapToScreen(m_edgePos_A);
			vector mapPos_edge_B = m_MiniMap.MapToScreen(m_edgePos_B);
			vector mapPos_center = m_MiniMap.MapToScreen(spawn_point);

			float distance_A = vector.Distance(mapPos_center, mapPos_edge_A);
			float distance_B = vector.Distance(mapPos_center, mapPos_edge_B);

			float canvas_width;
			float canvas_height;
			m_MiniMapCanvas.GetSize(canvas_width, canvas_height);
			float center_x = canvas_width / 2.0;
			float center_y = canvas_height / 2.0;

			RenderOval(mapPos_center[0], mapPos_center[1], distance_A, distance_B);
		}

		/*float width;
		float height;
		m_MiniMapCanvas.GetSize(width, height);
		Print(string.Format("GetSize %1 %2", width, height))
		m_MiniMapCanvas.GetScreenPos(width, height);
		Print(string.Format("GetScreenPos %1 %2", width, height))
		m_MiniMapCanvas.GetScreenSize(width, height);
		Print(string.Format("GetScreenSize %1 %2", width, height))
		m_MiniMapCanvas.DrawLine(0, 0, width, height, 2, ARGB(255, 0, 0, 255));*/
	}

	//TODO: temporary place
	void RenderOval(float cx, float cy, float a, float b)
	{
		BattleRoyaleUtils.Trace("RenderOval "+cx+" "+cy+" "+a+" "+b);
		for(int i = 0; i < 360; i++)
		{
			float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
			float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

			float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
			float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

			m_MiniMapCanvas.DrawLine(x1, y1, x2, y2, 2, ARGB(255, 0, 0, 255));
		}
	}
#endif

#ifndef SERVER
	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);

		// test if a menu is open
		if( IsInputExcludeActive("menu") )
			return;

		// test if menu is open, return if it is
		if( m_UIManager.FindMenu(MENU_CHAT_INPUT) )
			return;

		if (key == KeyCode.KC_B)
		{
				GetGame().Chat("Pressing Test Key", "colorFriendly");
				PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );

				Basic_Zone zone = Basic_Zone.Cast( GetGame().CreateObjectEx( "Basic_Zone", player.GetPosition(), ECE_LOCAL|ECE_PLACE_ON_SURFACE ) );

				if ( zone )
				{
					float scale = Math.RandomFloatInclusive( 0.5, 10.0 );
					BattleRoyaleUtils.Trace("Zone created '" + zone.GetZoneName() + "' on position " + player.GetPosition() + " with scale " + scale);
					zone.SetScale( scale );
					zone.Update();
				}

//				GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "CreateZone", new Param1< vector >( player.GetPosition() ), true, player.GetIdentity() );

//				GetGame().Chat("Current open menu: " + GetUIManager().GetMenu(), "colorFriendly");
//				if(!GetUIManager().IsMenuOpen(MENU_SPAWN_SELECTION)) {
//					UIScriptedMenu menu = GetUIManager().EnterScriptedMenu(MENU_SPAWN_SELECTION, GetUIManager().GetMenu());
//					GetGame().GetMission().AddActiveInputExcludes({"map"});
//					GetGame().GetMission().AddActiveInputRestriction(EInputRestrictors.MAP);
//					if (menu) {
//						GetGame().Chat("Menu opened", "colorFriendly");
//					} else {
//						GetGame().Chat("Menu not opened", "colorFriendly");
//					}
//				} else {
//					GetGame().Chat("Menu already open", "colorFriendly");
//				}
		}
	}
#endif

	override void OnUpdate( float timeslice )
	{
		super.OnUpdate( timeslice ); //no more using fade out because it causes way to much compatibility issues, instead we'll use widgets

		m_BattleRoyale.Update( timeslice ); //send tick to br client

#ifdef SPECTATOR
		if ( is_spectator )
			m_BattleRoyaleHud.Update( timeslice ); //this is really only used for spectator HUD updates
#endif

		if (GetUApi() && !m_UIManager.IsMenuOpen(MENU_CHAT_INPUT)) {
			if (GetUApi().GetInputByID(UADayZBRReadyUp).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).ReadyUp();
			}
			if (GetUApi().GetInputByID(UADayZBRUnstuck).LocalPress()) {
				BattleRoyaleClient.Cast( m_BattleRoyale ).Unstuck();
			}
#ifdef DIAG
			// Debug key
			if (GetUApi().GetInputByID(UADayZBRDebug).LocalPress()) {
				SpawnSelectionMenu m = SpawnSelectionMenu.Cast(GetUIManager().EnterScriptedMenu(MENU_SPAWN_SELECTION, GetUIManager().GetMenu()));
				m.SetInitialCountdown(45);
				m.SetSpawnSize(50);
				m.SetFirstZone(Vector(6000, 0, 7777), 1500);
			}
#endif
#ifdef BR_MINIMAP
			if (GetUApi().GetInputByID(UADayZBRToggleMiniMap).LocalPress()) {
				b_MiniMapShow = !b_MiniMapShow;
			}

			if (GetUApi().GetInputByID(UADayZBRMiniMapZoomPlus).LocalPress()) {
				if(f_MiniMapScale > 0.1)
					f_MiniMapScale = f_MiniMapScale - 0.05;
			}

			if (GetUApi().GetInputByID(UADayZBRMiniMapZoomMinus).LocalPress()) {
				if(f_MiniMapScale < 1.0)
					f_MiniMapScale = f_MiniMapScale + 0.05;
			}
#endif
		}

#ifdef SPECTATOR
		if ( is_spectator )
		{
			HideHud();
		}
#endif
	}

#ifdef SPECTATOR
	void HideHud()
	{
		IngameHud hud = IngameHud.Cast( GetHud() );
		if ( hud )
		{
			hud.BR_HIDE();
		}
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
			//ANVICaptchaMenu.Cast(GetUIManager().EnterScriptedMenu(MENU_ANVI_CAPTCHA, GetUIManager().GetMenu()));
			SpawnSelectionMenu m = SpawnSelectionMenu.Cast(GetUIManager().EnterScriptedMenu(MENU_SPAWN_SELECTION, GetUIManager().GetMenu()));
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
