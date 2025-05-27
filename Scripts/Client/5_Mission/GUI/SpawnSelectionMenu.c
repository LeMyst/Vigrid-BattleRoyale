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

	protected ref map<string, vector> m_TeammateSpawnPoints = new map<string, vector>();
	protected ref map<string, int> m_TeammateSpawnPointsColor = new map<string, int>();

	protected CanvasWidget m_HeatMapCanvas;
	protected ref array<vector> m_HeatMapSpawnPoints = new array<vector>();

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
			m_HeatMapCanvas = layoutRoot.FindAnyWidget("CanvasHeatmap");
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
			float time_left = Math.Ceil((i_CountdownEnd - GetGame().GetTime()) / 1000);  // calculate time left in seconds
			if (time_left >= 1)
			{
				StringLocaliser message;
				if (time_left > 1)
				{
					message = new StringLocaliser("STR_BR_TIMER_SPAWN_SELECTION_SECONDS");
				} else {
					message = new StringLocaliser("STR_BR_TIMER_SPAWN_SELECTION_SECOND");
				}
				message.Set(0, time_left);  // replace the first parameter with the time left in seconds
				m_CountdownText.SetText(message.Format());
			} else {
				message = new StringLocaliser("STR_BR_TIME_TO_DEPLOY");
				m_CountdownText.SetText(message.Format());
			}
		}

		if(m_MapWidget && m_SpawnCanvas)
		{
			vector m_edgePos_A, m_edgePos_B, mapPos_edge_A, mapPos_edge_B, mapPos_center;
			float distance_A, distance_B;

			m_SpawnCanvas.Clear();

			float screen_x, screen_y;
			m_MapWidget.GetScreenPos(screen_x, screen_y);

			// Show the teammates zones
			foreach (string playerId, vector spawn_point : m_TeammateSpawnPoints)
			{
				if (playerId)
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

					WorldRenderOval(m_SpawnCanvas, m_MapWidget, spawn_point[0], spawn_point[2], spawn_size, spawn_size, m_TeammateSpawnPointsColor.Get(playerId));
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

				WorldRenderOval(m_SpawnCanvas, m_MapWidget, v_FirstZoneCenter[0], v_FirstZoneCenter[2], f_FirstZoneRadius, f_FirstZoneRadius, ARGB(255, 255, 255, 255));
			}

			// Define constants for heat map
			float heatmap_grid_size = spawn_size * HEATMAP_GRID_SIZE_MULTIPLIER; // Size of each grid cell in the heat map

			// Update the heat map
			if (m_HeatMapCanvas)
			{
				m_HeatMapCanvas.Clear();

				// Declare all variables upfront
				string grid_key;
				int grid_x, grid_z;
				array<string> coords = new array<string>();
				float world_x, world_z;
				float intensity, surroundIntensity;
				int capped_density, r, b, alpha, color, surroundColor;
				int offset, dx, dz;

				// Create a grid data structure to count spawn points in each cell
				ref map<string, int> grid_density = new map<string, int>();

				// Count spawn points in each grid cell
				for (int i = 0; i < m_HeatMapSpawnPoints.Count(); i++)
				{
					vector heatPoint = m_HeatMapSpawnPoints[i];

					// Convert to grid coordinates (snap to grid)
					grid_x = Math.Floor(heatPoint[0] / heatmap_grid_size);
					grid_z = Math.Floor(heatPoint[2] / heatmap_grid_size);

					// Create a unique key for this grid cell
					grid_key = grid_x.ToString() + "," + grid_z.ToString();

					// Increment the density counter for this cell
					if (grid_density.Contains(grid_key))
						grid_density.Set(grid_key, grid_density.Get(grid_key) + 1);
					else
						grid_density.Insert(grid_key, 1);
				}

				// First pass: render degraded effect for surroundings (lowest layers first)
				foreach (string key, int density : grid_density)
				{
					// Skip if density is 0
					if (density <= 0)
						continue;

					// Parse grid coordinates from key
					coords.Clear();
					key.Split(",", coords);

					if (coords.Count() != 2)
						continue;

					grid_x = coords[0].ToInt();
					grid_z = coords[1].ToInt();

					// Convert back to world coordinates for rendering (use cell center)
					world_x = grid_x * heatmap_grid_size + (heatmap_grid_size / 2);
					world_z = grid_z * heatmap_grid_size + (heatmap_grid_size / 2);

					// Cap density at max_density for color calculation
					capped_density = Math.Min(density, HEATMAP_MAX_DENSITY);

					// Calculate intensity factor (0.0 to 1.0)
					intensity = capped_density / (float)HEATMAP_MAX_DENSITY;

					// Add degraded effect to surrounding cells
					for (offset = 2; offset >= 1; offset--)
					{
						// Calculate alpha for surrounding cells (50-100 based on offset)
						alpha = 50 * (3 - offset);

						// Calculate color intensity for surrounding (reduced by distance)
						surroundIntensity = intensity * (1 - (offset * 0.3));

						// Calculate RGB values - blue (low) to red (high) gradient
						r = Math.Round(surroundIntensity * 255);
						b = Math.Round((1 - surroundIntensity) * 255);
						surroundColor = ARGB(alpha, r, 0, b);

						// Render surrounding cells in a ring pattern
						for (dx = -offset; dx <= offset; dx++)
						{
							for (dz = -offset; dz <= offset; dz++)
							{
								// Skip the center and non-edge cells (only draw the ring)
								if ((Math.AbsInt(dx) != offset) && (Math.AbsInt(dz) != offset))
									continue;

								RenderFilledSquare(m_HeatMapCanvas, m_MapWidget, world_x + (dx * heatmap_grid_size), world_z + (dz * heatmap_grid_size), heatmap_grid_size, surroundColor);
							}
						}
					}
				}

				// Second pass: render the primary cells with full intensity
				foreach (string density_key, int density_value : grid_density)
				{
					// Skip if density is 0
					if (density_value <= 0)
						continue;

					// Parse grid coordinates from key
					coords.Clear();
					density_key.Split(",", coords);

					if (coords.Count() != 2)
						continue;

					grid_x = coords[0].ToInt();
					grid_z = coords[1].ToInt();

					// Convert back to world coordinates for rendering (use cell center)
					world_x = grid_x * heatmap_grid_size + (heatmap_grid_size / 2);
					world_z = grid_z * heatmap_grid_size + (heatmap_grid_size / 2);

					// Cap density at max_density for color calculation
					capped_density = Math.Min(density_value, HEATMAP_MAX_DENSITY);

					// Calculate color intensity (0.0 to 1.0)
					intensity = capped_density / (float)HEATMAP_MAX_DENSITY;

					// Calculate RGB values - blue (low) to red (high) gradient
					r = Math.Round(intensity * 255);
					b = Math.Round((1 - intensity) * 255);
					color = ARGB(150, r, 0, b);

					// Draw the primary cell
					RenderFilledSquare(m_HeatMapCanvas, m_MapWidget, world_x, world_z, heatmap_grid_size, color);
				}
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
			// Add a small tolerance (25 meters) to account for precision errors
			if (distance > (f_FirstZoneRadius + 25))
			{
				BattleRoyaleUtils.Trace("SpawnSelectionMenu::SelectSpawnPoint Invalid Position (out of first zone)");
				return;
			}
		}

		BattleRoyaleUtils.Trace(string.Format("SpawnSelectionMenu::SelectSpawnPoint: %1", tempPosition));

		PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
		GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", new Param1<vector>( tempPosition ), true, NULL, player );
	}

	void WorldRenderOval(CanvasWidget canvas, MapWidget world_map, float world_x, float world_z, float radius_x, float radius_z, int color = -1)
	{
		if (!world_map || radius_x <= 0 || radius_z <= 0 || !canvas)
		{
			BattleRoyaleUtils.Trace("WorldRenderOval: Invalid parameters");
			return;
		}

		float screen_x, screen_y;
		world_map.GetScreenPos(screen_x, screen_y);

		// Create the center point in world coordinates
		vector worldCenter = Vector(world_x, 0, world_z);
		vector screenCenter = world_map.MapToScreen(worldCenter);

		// Create edge points in world coordinates for accurate size calculation
		vector worldEdgeX = Vector(world_x + radius_x, 0, world_z);
		vector worldEdgeZ = Vector(world_x, 0, world_z + radius_z);

		// Convert edges to screen coordinates
		vector screenEdgeX = world_map.MapToScreen(worldEdgeX);
		vector screenEdgeZ = world_map.MapToScreen(worldEdgeZ);

		// Calculate width and height in screen pixels
		float screenWidth = vector.Distance(screenCenter, screenEdgeX);
		float screenHeight = vector.Distance(screenCenter, screenEdgeZ);

		// Calculate the center of the oval in screen space
		float cx = screenCenter[0] - screen_x;
		float cy = screenCenter[1] - screen_y;

		// Draw the oval
		int segments = Math.Max(360, Math.Round(Math.Max(screenWidth, screenHeight) / 2));
		float angleIncrement = 360.0 / segments;

		for(int i = 0; i < segments; i++)
		{
			float angle1 = i * angleIncrement;
			float angle2 = (i + 1) * angleIncrement;

			float x1 = cx + (screenWidth * Math.Cos(angle1 * Math.DEG2RAD));
			float y1 = cy + (screenHeight * Math.Sin(angle1 * Math.DEG2RAD));

			float x2 = cx + (screenWidth * Math.Cos(angle2 * Math.DEG2RAD));
			float y2 = cy + (screenHeight * Math.Sin(angle2 * Math.DEG2RAD));

			canvas.DrawLine(x1, y1, x2, y2, 2, color);
		}
	}

	/**
	 * Renders a filled square on the given canvas, using world coordinates for positioning.
	 *
	 * @param canvas      The CanvasWidget instance used for rendering.
	 * @param world_map   The MapWidget instance used to convert world coordinates to screen coordinates.
	 * @param world_x     The X-coordinate of the square's center in world space.
	 * @param world_z     The Z-coordinate of the square's center in world space.
	 * @param size        The size of the square in world units.
	 * @param color       The color of the square, specified as an ARGB integer. Defaults to -1 (white).
	 *
	 * The method calculates the square's position and dimensions in screen space
	 * and renders it as a series of horizontal lines to fill the area.
	 */
	void RenderFilledSquare(CanvasWidget canvas, MapWidget world_map, float world_x, float world_z, float size, int color = -1)
	{
		if (!world_map || size <= 0 || !canvas)
		{
			BattleRoyaleUtils.Trace("RenderFilledSquare: Invalid parameters");
			return;
		}

		float screen_x, screen_y;
		world_map.GetScreenPos(screen_x, screen_y);

		// Create the center point in world coordinates
		vector worldCenter = Vector(world_x, 0, world_z);
		vector screenCenter = world_map.MapToScreen(worldCenter);

		// Create edge points in world coordinates for accurate size calculation
		vector worldEdgeX = Vector(world_x + size, 0, world_z);
		vector worldEdgeZ = Vector(world_x, 0, world_z + size);

		// Convert edges to screen coordinates
		vector screenEdgeX = world_map.MapToScreen(worldEdgeX);
		vector screenEdgeZ = world_map.MapToScreen(worldEdgeZ);

		// Calculate width and height in screen pixels
		float screenWidth = vector.Distance(screenCenter, screenEdgeX);
		float screenHeight = vector.Distance(screenCenter, screenEdgeZ);

		// Calculate the corners of the square in screen space
		float cx = screenCenter[0] - screen_x;
		float cy = screenCenter[1] - screen_y;

		float left = cx - screenWidth/2;
		float right = cx + screenWidth/2;
		float top = cy - screenHeight/2;
		float bottom = cy + screenHeight/2;

		// Draw the filled square using horizontal lines
		for (float y = top; y <= bottom; y += 1)
		{
			canvas.DrawLine(left, y, right, y, 1, color);
		}
	}

	void UpdateHeatMap(array<vector> spawnPoints)
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::UpdateHeatMap");
		m_HeatMapSpawnPoints = spawnPoints;
	}

	vector GetSelectedSpawnPoint()
	{
		return m_SelectedSpawnPoint;
	}

	void SetTeammateSpawnPoint(PlayerBase player, vector pos, int color)
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::SetTeammateSpawnPoint");
		if (player && player.GetIdentity() && player.GetIdentity().GetId())
		{
			string playerId = player.GetIdentity().GetId();

			BattleRoyaleUtils.Trace(string.Format("Player: %1", player.GetIdentity().GetName()));
			if (m_TeammateSpawnPoints.Contains(playerId))
			{
				BattleRoyaleUtils.Trace(string.Format("Player already in map: %1", player.GetIdentity().GetName()));
				m_TeammateSpawnPoints.Set(playerId, pos);
			} else {
				BattleRoyaleUtils.Trace(string.Format("Player not in map: %1", player.GetIdentity().GetName()));
				m_TeammateSpawnPoints.Insert(playerId, pos);
			}
			m_TeammateSpawnPointsColor.Set(playerId, color);
		}
	}
}