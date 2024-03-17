#ifndef SERVER

class LocalPlayer
{
	private Object m_PlayerVehicle;
	private int m_UniqueID;

	void LocalPlayer(ref Object playerVeh, int PID) {
		m_PlayerVehicle = playerVeh;
		m_UniqueID      = PID;
	}

	void ~LocalPlayer() {
	}

	int GetUniqueID() {
		return m_UniqueID;
	}

	ref Object GetPlayerVehicle() {
		return m_PlayerVehicle;
	}

	void SetPlayerVehicle(ref Object vehObj) {
		m_PlayerVehicle = vehObj;
	}
}

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

	private ref array<ref LocalPlayer> m_LocalPlayers;

    void MissionGameplay()
    {
		Print("MissionGameplay::MissionGameplay");
		m_BattleRoyaleHudRootWidget = null;
		m_BattleRoyale = null;
#ifdef SPECTATOR
		is_spectator = false;
#endif

		GetRPCManager().AddRPC( "RPC_MissionGameplay", "InitESPBox", this );
    }

	void ~MissionGameplay()
	{
		if (m_BattleRoyaleHudRootWidget)
		{
			m_BattleRoyaleHudRootWidget.Unlink();
		}

		m_BattleRoyale = null;
	}

	override void OnInit()
	{
		BattleRoyaleUtils.Trace("MissionGameplay::OnInit");
		super.OnInit();

		m_BattleRoyale = new BattleRoyaleClient;

		Print("OnInit()");
		GetGame().Chat( "ESP ON!", "colorImportant" );
		UpdateESP();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.UpdateESP, 3700, true);

		InitBRhud();
	}

	BattleRoyaleClient GetBattleRoyale()
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

//	override void Expansion_OnUpdate(float timeslice, PlayerBase player, bool isAliveConscious, Input input, bool inputIsFocused, UIScriptedMenu menu, ExpansionScriptViewMenuBase viewMenu)
//	{
//		super.Expansion_OnUpdate(timeslice, player, isAliveConscious, input, inputIsFocused, menu, viewMenu);
//
//		auto generalSettings = GetExpansionSettings().GetGeneral(false);
//
//		if (isAliveConscious)
//		{
//			if (!menu && !inputIsFocused)
//			{
//				//! Toggle Earplugs
//				if (input.LocalPress( "UADayZBRToggleEarplugsLegacy", false )  && !viewMenu)
//				{
//					if (generalSettings.EnableEarPlugs)
//						m_Hud.Expansion_ToggleEarplugs();
//				}
//			}
//		}
//	}

    void UpdateESP()
	{
        Print("UpdateESP()");
		GetGame().Chat( "UpdateESP()", "colorImportant" );
		m_LocalPlayers = new array<ref LocalPlayer>;
		ref array<Object> objects = new array<Object>;
		GetGame().GetObjectsAtPosition3D(GetGame().GetPlayer().GetPosition(), 1100, objects, NULL);
		for (int i = 0; i < objects.Count(); ++i)
		{
			Object obj;
			PlayerBase playerFound;
			obj = Object.Cast( objects.Get(i) );

			if (GetGame().ObjectIsKindOf(obj, "SurvivorBase"))
			{
				if (Class.CastTo(playerFound, obj))
				{
					if (obj != GetGame().GetPlayer() && obj.IsAlive())
					{
						int PID = Math.RandomIntInclusive(0, 99999999999);
						ref LocalPlayer m_PlayerCache = new LocalPlayer(obj, PID);
						m_LocalPlayers.Insert(m_PlayerCache);

						ref Param2<PlayerBase, int> param = new Param2<PlayerBase, int>(playerFound, PID);
						GetGame().Chat( "send GetDataFromServer()", "colorImportant" );
       					GetRPCManager().SendRPC( "RPC_MissionServer", "GetDataFromServer", param, true);
					}
				}
			}
		}
	}

	void InitESPBox( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
        Print("InitESPBox()");
		GetGame().Chat( "InitESPBox()", "colorImportant" );
		Param2<string, int> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
			if (m_LocalPlayers != NULL)
			{
				for (int i = 0; i < m_LocalPlayers.Count(); ++i)
				{
					ref LocalPlayer m_Player = m_LocalPlayers.Get(i);
					if (m_Player.GetUniqueID() == data.param2)
					{
        				Print("Create new ESPBox for :" + data.param1);
        				GetGame().Chat( "Create new ESPBox for :" + data.param1, "colorImportant" );
						ref ESPBox DisplayBox = new ESPBox(m_Player.GetPlayerVehicle(), data.param1);
					}
				}
			}
        }
	}
}
#endif
