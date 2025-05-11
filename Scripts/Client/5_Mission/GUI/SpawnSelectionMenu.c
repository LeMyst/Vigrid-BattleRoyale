#ifndef SERVER
class SpawnSelectionMenu extends UIScriptedMenu
{
	protected ref IngameHud m_Hud;
	protected ref BRMapHandler m_MapMenuHandler;
	protected ref MapWidget m_MapWidgetInstance;

	protected MapWidget m_MapWidget;
	protected TextWidget m_CountdownText;
	protected ButtonWidget m_ConfirmButton;

	protected vector m_SelectedSpawnPoint;

	protected CanvasWidget m_SpawnCanvas;

	protected int i_CountdownEnd;

	protected float spawn_size = 50.0;

	protected vector v_FirstZoneCenter = "0 0 0";
	protected float f_FirstZoneRadius = 0;

	protected ref map<PlayerBase, vector> m_TeammateSpawnPoints = new map<PlayerBase, vector>();
	protected ref map<PlayerBase, int> m_TeammateSpawnPointsColor = new map<PlayerBase, int>();

	void SpawnSelectionMenu()
	{
		GetGame().Chat("SpawnSelectionMenu::SpawnSelectionMenu", "colorFriendly");
		g_Game.SetKeyboardHandle(this);
	}

	void ~SpawnSelectionMenu()
	{
		g_Game.SetKeyboardHandle(NULL);
	}

	override void OnShow()
	{
		GetGame().Chat("SpawnSelectionMenu::OnShow", "colorFriendly");
		super.OnShow();

		PPEffects.SetBlurMenu(1);
		GetGame().GetInput().ChangeGameFocus(1);
		SetFocus(layoutRoot);
	}

	override void OnHide()
	{
		GetGame().Chat("SpawnSelectionMenu::OnHide", "colorFriendly");
		super.OnHide();

		PPEffects.SetBlurMenu(0);
		GetGame().GetInput().ResetGameFocus();
	}

	override Widget Init()
	{
		GetGame().Chat("SpawnSelectionMenu::Init", "colorFriendly");
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/spawn_selection.layout");
		m_Hud = IngameHud.Cast(GetGame().GetMission().GetHud());

		m_MapWidget = MapWidget.Cast(layoutRoot.FindAnyWidget("SpawnMap"));

		m_CountdownText = TextWidget.Cast(layoutRoot.FindAnyWidget("CountdownText"));

		m_ConfirmButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ConfirmButton"));

		if (m_MapWidget)
		{
			vector tempPosition = GetGame().ConfigGetVector(string.Format("CfgWorlds %1 centerPosition", GetGame().GetWorldName()));
			float scale = 0.33;
			vector mapPosition = Vector(tempPosition[0], tempPosition[1], tempPosition[2]);

			if (v_FirstZoneCenter != "0 0 0")
			{
				m_MapWidget.SetMapPos(v_FirstZoneCenter);
				m_MapWidget.SetScale(0.10);
			}

			m_MapMenuHandler = new BRMapHandler(m_MapWidget);

			if (m_Hud)
			{
				m_Hud.ShowHudUI(false);
				m_Hud.ShowQuickbarUI(false);
			}

			m_SpawnCanvas = CanvasWidget.Cast(m_MapWidget.FindAnyWidget("CanvasSpawnMap"));
		}

		layoutRoot.Update();

		return layoutRoot;
	}

	override void Update(float timeslice)
	{
        super.Update(timeslice);

		// Find CountdownText widget
		if (m_CountdownText)
		{
			int time_left = i_CountdownEnd - GetGame().GetTime();
			if (time_left >= 1000)
			{
				StringLocaliser message;
				if (time_left >= 1500)
				{
					message = new StringLocaliser("STR_BR_TIMER_SPAWN_SELECTION_SECONDS");
				} else {
					message = new StringLocaliser("STR_BR_TIMER_SPAWN_SELECTION_SECOND");
				}
				message.Set(0, time_left / 1000);
				m_CountdownText.SetText(message.Format());
			} else {
				message = new StringLocaliser("STR_BR_TIME_TO_DEPLOY");
				m_CountdownText.SetText(message.Format());
			}
		}

		if(m_SpawnCanvas)
		{
			vector m_edgePos_A, m_edgePos_B, mapPos_edge_A, mapPos_edge_B, mapPos_center;
			float distance_A, distance_B;

			m_SpawnCanvas.Clear();

			vector map_pos = m_MapWidget.MapToScreen(m_SelectedSpawnPoint);

			float screen_x, screen_y;
			m_MapWidget.GetScreenPos(screen_x, screen_y);

			// Selected spawn zone
			m_edgePos_A = m_SelectedSpawnPoint;
			m_edgePos_A[0] = m_edgePos_A[0] + spawn_size;

			m_edgePos_B = m_SelectedSpawnPoint;
			m_edgePos_B[2] = m_edgePos_B[2] + spawn_size;

			mapPos_edge_A = m_MapWidget.MapToScreen(m_edgePos_A);
			mapPos_edge_B = m_MapWidget.MapToScreen(m_edgePos_B);
			mapPos_center = m_MapWidget.MapToScreen(m_SelectedSpawnPoint);

			distance_A = vector.Distance(mapPos_center,mapPos_edge_A);
			distance_B = vector.Distance(mapPos_center,mapPos_edge_B);

			RenderOval(map_pos[0] - screen_x, map_pos[1] - screen_y, distance_A, distance_B);

			// Show the teammates zones
			foreach (PlayerBase player, vector spawn_point : m_TeammateSpawnPoints)
			{
				if (player)
				{
					m_edgePos_A = spawn_point;
					m_edgePos_A[0] = m_edgePos_A[0] + spawn_size;

					m_edgePos_B = spawn_point;
					m_edgePos_B[2] = m_edgePos_B[2] + spawn_size;

					mapPos_edge_A = m_MapWidget.MapToScreen(m_edgePos_A);
					mapPos_edge_B = m_MapWidget.MapToScreen(m_edgePos_B);
					mapPos_center = m_MapWidget.MapToScreen(spawn_point);

					distance_A = vector.Distance(mapPos_center,mapPos_edge_A);
					distance_B = vector.Distance(mapPos_center,mapPos_edge_B);

					// Generate a random color for the player if it doesn't exist
					if(!m_TeammateSpawnPointsColor.Contains(player))
					{
						m_TeammateSpawnPointsColor.Set(player, ARGB(255, Math.RandomIntInclusive(0, 255), Math.RandomIntInclusive(0, 255), Math.RandomIntInclusive(0, 255)));
					}

					RenderOval(mapPos_center[0] - screen_x, mapPos_center[1] - screen_y, distance_A, distance_B, m_TeammateSpawnPointsColor.Get(player));
				}
			}

			// Show the first zone
			if(v_FirstZoneCenter != "0 0 0" && f_FirstZoneRadius > 0)
			{
				m_edgePos_A = v_FirstZoneCenter;
				m_edgePos_A[0] = m_edgePos_A[0] + f_FirstZoneRadius;

				m_edgePos_B = v_FirstZoneCenter;
				m_edgePos_B[2] = m_edgePos_B[2] + f_FirstZoneRadius;

				mapPos_edge_A = m_MapWidget.MapToScreen(m_edgePos_A);
				mapPos_edge_B = m_MapWidget.MapToScreen(m_edgePos_B);
				mapPos_center = m_MapWidget.MapToScreen(v_FirstZoneCenter);

				distance_A = vector.Distance(mapPos_center,mapPos_edge_A);
				distance_B = vector.Distance(mapPos_center,mapPos_edge_B);

				RenderOval(mapPos_center[0] - screen_x, mapPos_center[1] - screen_y, distance_A, distance_B, ARGB(255, 255, 255, 255));
			}
		}
	}

	void SetInitialCountdown(int countdown)
	{
		i_CountdownEnd = GetGame().GetTime() + (countdown * 1000);
	}

	void SetSpawnSize(float size)
	{
		spawn_size = size;
	}

	float GetSpawnSize()
	{
		return spawn_size;
	}

	void SetFirstZone(vector pos, float size)
	{
		v_FirstZoneCenter = pos;
		f_FirstZoneRadius = size;

		if (m_MapWidget)
		{
			BattleRoyaleUtils.Trace("SpawnSelectionMenu::SetFirstZone");

			m_MapWidget.SetMapPos(v_FirstZoneCenter);
			m_MapWidget.SetScale(0.33);

			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DelayedSetMapPos, 100, false, v_FirstZoneCenter);
		}
	}

	void DelayedSetMapPos(vector pos)
	{
		if (m_MapWidget)
		{
			BattleRoyaleUtils.Trace("SpawnSelectionMenu::DelayedSetMapPos");
			m_MapWidget.SetMapPos(pos);
			m_MapWidget.SetScale(0.33);
		}
	}

	void SelectSpawnPoint(vector pos)
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::SelectSpawnPoint");

		// Check if some time is left
		if (GetGame().GetTime() > i_CountdownEnd)
		{
			BattleRoyaleUtils.Trace("SpawnSelectionMenu::SelectSpawnPoint Time expired");
			return;
		}

		vector tempPosition = m_MapWidget.ScreenToMap(pos);

		// Check if the position is valid, e.g. within the world bounds
		int worldSize = GetGame().GetWorld().GetWorldSize();  // Get the world size
		if (tempPosition[0] < (GetSpawnSize()/2) || tempPosition[2] < (GetSpawnSize()/2) || tempPosition[0] > (worldSize-(GetSpawnSize()/2)) || tempPosition[2] > (worldSize-(GetSpawnSize()/2)))
		{
			BattleRoyaleUtils.Trace("SpawnSelectionMenu::SelectSpawnPoint Invalid Position (out of world bounds)");
			BattleRoyaleUtils.Trace(string.Format("X: %1 Y: %2", (GetSpawnSize()/2), worldSize-(GetSpawnSize()/2)));
			return;
		}

		// Check if the position is valid, e.g. not in sea
		if(GetGame().SurfaceIsSea(tempPosition[0], tempPosition[2]))
		{
			BattleRoyaleUtils.Trace("SpawnSelectionMenu::SelectSpawnPoint Invalid Position (in sea)");
			return;
		}

		// Check if the position is valid, e.g. within the first zone
		if (v_FirstZoneCenter != "0 0 0" && f_FirstZoneRadius > 0)
		{
			float distance = vector.Distance(tempPosition, v_FirstZoneCenter);
			if (distance > f_FirstZoneRadius)
			{
				BattleRoyaleUtils.Trace("SpawnSelectionMenu::SelectSpawnPoint Invalid Position (out of first zone)");
				return;
			}
		}

		m_SelectedSpawnPoint = tempPosition;

//		m_MapWidget.ClearUserMarks();
//		int color = ARGB(255, 255, 0, 255);
//		m_MapWidget.AddUserMark(m_SelectedSpawnPoint, "", color, "\\DZ\\gear\\navigation\\data\\map_viewpoint_ca.paa");

		BattleRoyaleUtils.Trace(string.Format("SpawnSelectionMenu::SelectSpawnPoint: %1", m_SelectedSpawnPoint));

		PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
		GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", new Param1<vector>( m_SelectedSpawnPoint ), true, NULL, player );
	}

	void RenderOval(float cx, float cy, float a, float b, int color = -1)
	{
		for(int i = 0; i < 360; i++)
		{
			float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
			float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

			float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
			float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

			if (m_SpawnCanvas)
			{
				m_SpawnCanvas.DrawLine(x1, y1, x2, y2, 2, color);
			} else {
				BattleRoyaleUtils.Trace("m_SpawnCanvas is NULL");
			}
		}
	}

	vector GetSelectedSpawnPoint()
	{
		return m_SelectedSpawnPoint;
	}

	void SetTeammateSpawnPoint(PlayerBase player, vector pos)
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::SetTeammateSpawnPoint");
		if (player)
		{
			BattleRoyaleUtils.Trace(string.Format("Player: %1", player.GetIdentity().GetName()));
			if (m_TeammateSpawnPoints.Contains(player))
			{
				BattleRoyaleUtils.Trace(string.Format("Player already in map: %1", player.GetIdentity().GetName()));
				m_TeammateSpawnPoints.Set(player, pos);
			} else {
				BattleRoyaleUtils.Trace(string.Format("Player not in map: %1", player.GetIdentity().GetName()));
				m_TeammateSpawnPoints.Insert(player, pos);
			}
		}
	}
}