#ifndef SERVER

// One precomputed heat map square, in world space. The colour is baked at
// cache-build time so rendering a frame is pure arithmetic plus the fill.
class BRHeatCell
{
	float world_x;
	float world_z;
	int color;

	void BRHeatCell(float x, float z, int c)
	{
		world_x = x;
		world_z = z;
		color = c;
	}
}

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

	// Precomputed heat map squares, in the exact order they must be drawn --
	// the translucent halos blend on top of one another, so order is visible.
	protected ref array<ref BRHeatCell> m_HeatMapCells = new array<ref BRHeatCell>();
	protected float f_CachedGridSize = 0;
	protected bool b_HeatMapCacheDirty = true;

	// To track changes
	protected bool b_RenderDirty = true;
	protected vector br_previous_probe_origin = "0 0 0";
	protected vector br_previous_probe_far = "0 0 0";

	// An Enfusion canvas keeps its draw list until Clear() is called, so a frame
	// where neither the data nor the map transform moved needs no work at all.
	// If that ever stops holding, the map blanks out as soon as it sits still --
	// set this to false to fall back to redrawing every frame.
	protected bool b_SkipStaticRedraw = true;
	protected int i_LastRepaintTime = 0;
	//--- Starts at -1 so the first frame always repaints, even if the hot zones landed
	//--- before this menu was built and the sequence has not moved since.
	protected int i_LastHotZoneSeq = -1;
	protected int i_LastTransformCheck = 0;

	// Countdown text only changes once a second; avoid rebuilding it per frame.
	protected int i_PreviousDisplayedSecond = -1;

	void SpawnSelectionMenu()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::SpawnSelectionMenu");
		g_Game.SetKeyboardHandle(this);
	}

	void ~SpawnSelectionMenu()
	{
		g_Game.SetKeyboardHandle(NULL);

		if (m_Hud)
		{
			m_Hud.ShowHudUI(true);
			m_Hud.ShowQuickbarUI(true);
		}
	}

	override void OnShow()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::OnShow");
		super.OnShow();

		PPEffects.SetBlurMenu(1);
		GetGame().GetInput().ChangeGameFocus(1);
		SetFocus(layoutRoot);

		// Nothing on the canvases survives a hide/show cycle -- force a redraw.
		b_RenderDirty = true;
		i_PreviousDisplayedSecond = -1;
	}

	override void OnHide()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::OnHide");
		super.OnHide();

		PPEffects.SetBlurMenu(0);
		//--- ChangeGameFocus(-1), not ResetGameFocus(): the reset SETS the shared additive counter
		//--- to zero on every device (input.c:22-27), releasing every other holder's acquire rather
		//--- than our own. Latent here rather than live - spawn selection runs with no other menu
		//--- up - but it is the same defect that was reachable through the leaderboard, and one
		//--- copy left behind is how it comes back.
		GetGame().GetInput().ChangeGameFocus(-1);
	}

	override Widget Init()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::Init");
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
			m_HeatMapCanvas = CanvasWidget.Cast(layoutRoot.FindAnyWidget("CanvasHeatmap"));
		}

		layoutRoot.Update();

		return layoutRoot;
	}

	override void Update(float timeslice)
	{
        super.Update(timeslice);

		UpdateCountdownText();

		if (!m_MapWidget || !m_SpawnCanvas)
			return;

		// The MapWidget handles pan and zoom natively, so there is no script
		// event for it. Probe the world->screen transform instead: the map is
		// north-up and unrotated, so two points fully describe it, and they
		// double as the change detector for the whole render.
		vector probe_origin = m_MapWidget.MapToScreen(Vector(0.0, 0.0, 0.0));
		vector probe_far = m_MapWidget.MapToScreen(Vector(HEATMAP_PROBE_DISTANCE, 0.0, HEATMAP_PROBE_DISTANCE));

		if (b_HeatMapCacheDirty)
			RebuildHeatMapCache();

		bool transform_moved = (probe_origin != br_previous_probe_origin) || (probe_far != br_previous_probe_far);

		// Hot zones arrive over their own RPC and can land after this menu is already
		// open, so they need an edge of their own. Without it the circles would not
		// appear until the watchdog fired, which reads as the server being slow.
		BattleRoyaleRPC hot_zone_rpc = BattleRoyaleRPC.GetInstance();
		if (hot_zone_rpc && hot_zone_rpc.hot_zone_seq != i_LastHotZoneSeq)
		{
			i_LastHotZoneSeq = hot_zone_rpc.hot_zone_seq;
			b_RenderDirty = true;
		}

		// Watchdog: repaint periodically even when nothing changed. If a canvas
		// ever stops retaining its draw list between frames the map strobes at
		// 2 Hz -- obvious on the first launch -- rather than silently blanking.
		bool watchdog_due = (GetGame().GetTime() - i_LastRepaintTime) >= HEATMAP_REPAINT_WATCHDOG_MS;

		if (b_SkipStaticRedraw && !b_RenderDirty && !transform_moved && !watchdog_due)
			return;

		br_previous_probe_origin = probe_origin;
		br_previous_probe_far = probe_far;
		b_RenderDirty = false;
		i_LastRepaintTime = GetGame().GetTime();

		float px_per_m_x = (probe_far[0] - probe_origin[0]) / HEATMAP_PROBE_DISTANCE;
		float px_per_m_z = (probe_far[1] - probe_origin[1]) / HEATMAP_PROBE_DISTANCE;

#ifdef DIAG
		VerifyMapTransform(probe_origin, px_per_m_x, px_per_m_z);
#endif

		RenderMarkers();
		RenderHeatMap(probe_origin, px_per_m_x, px_per_m_z);
	}

#ifdef DIAG
	/**
	 * Sanity-checks the hoisted transform against a real MapToScreen for a third
	 * world point, at most once a second.
	 *
	 * The heat map renders from a two-point affine model, which is only valid
	 * because the DayZ map is north-up, unrotated and independently scaled per
	 * axis. If that ever stops holding, this reports it in the .rpt rather than
	 * leaving a subtly misplaced heat map to be spotted by eye.
	 */
	protected void VerifyMapTransform(vector probe_origin, float px_per_m_x, float px_per_m_z)
	{
		if ((GetGame().GetTime() - i_LastTransformCheck) < 1000)
			return;

		i_LastTransformCheck = GetGame().GetTime();

		float check_x = 2500.0;
		float check_z = 7300.0;

		vector actual = m_MapWidget.MapToScreen(Vector(check_x, 0.0, check_z));
		float predicted_x = probe_origin[0] + (check_x * px_per_m_x);
		float predicted_y = probe_origin[1] + (check_z * px_per_m_z);

		float error_x = Math.AbsFloat(actual[0] - predicted_x);
		float error_y = Math.AbsFloat(actual[1] - predicted_y);

		if (error_x > 1.0 || error_y > 1.0)
			BattleRoyaleUtils.Warn(string.Format("SpawnSelectionMenu: map transform is not affine as assumed (error %1, %2 px). Heat map placement will be wrong.", error_x, error_y));
	}
#endif

	/**
	 * Refreshes the countdown label, but only when the displayed second actually
	 * changes -- the original rebuilt a StringLocaliser and re-set the text on
	 * every frame for a value that ticks once a second.
	 */
	protected void UpdateCountdownText()
	{
		if (!m_CountdownText)
			return;

		float time_left = Math.Ceil((i_CountdownEnd - GetGame().GetTime()) / 1000);  // calculate time left in seconds

		// Clamp below zero so the expired "time to deploy" branch settles on one
		// value instead of re-firing as the countdown keeps running negative.
		int displayed_second = Math.Round(time_left);
		if (displayed_second < 0)
			displayed_second = 0;

		if (displayed_second == i_PreviousDisplayedSecond)
			return;

		i_PreviousDisplayedSecond = displayed_second;

		StringLocaliser message;
		if (time_left >= 1)
		{
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

	/**
	 * Draws the teammate spawn markers and the first zone onto the marker canvas.
	 * Only ever a handful of ovals, so this still converts through MapToScreen.
	 */
	protected void RenderMarkers()
	{
		m_SpawnCanvas.Clear();

		// Hot zones first, so the teammate markers and the first-zone ring draw over
		// them. They are the only filled shape on this canvas, and a fill laid over a
		// marker would swallow it.
		RenderHotZones();

		// Show the teammates zones
		foreach (string playerId, vector spawn_point : m_TeammateSpawnPoints)
		{
			if (playerId)
			{
				WorldRenderOval(m_SpawnCanvas, m_MapWidget, spawn_point[0], spawn_point[2], spawn_size, spawn_size, m_TeammateSpawnPointsColor.Get(playerId));
			}
		}

		// Show the first zone
		if(v_FirstZoneCenter != "0 0 0" && f_FirstZoneRadius > 0)
		{
			WorldRenderOval(m_SpawnCanvas, m_MapWidget, v_FirstZoneCenter[0], v_FirstZoneCenter[2], f_FirstZoneRadius, f_FirstZoneRadius, ARGB(255, 255, 255, 255));
		}
	}

	/**
	 * Draws the server's hot zones as filled red discs.
	 *
	 * Filled here and outline-only on the in-game map: this screen is one zoomed-out
	 * decision, where a wash of colour reads at a glance, and it is drawn once per
	 * repaint rather than panned around.
	 *
	 * The pair is already length-matched and sanity-checked server-side in
	 * BattleRoyaleZoneData.Validate(), so this only skips the degenerate cases that
	 * would draw nothing anyway.
	 */
	protected void RenderHotZones()
	{
		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
		if (!br_rpc || !br_rpc.hot_zone_centers || !br_rpc.hot_zone_radii)
			return;

		int count = br_rpc.hot_zone_centers.Count();
		if (br_rpc.hot_zone_radii.Count() < count)
			count = br_rpc.hot_zone_radii.Count();

		for (int i = 0; i < count; i++)
		{
			// One array read per line, never nested inside the call below.
			vector center = br_rpc.hot_zone_centers[i];
			float radius = br_rpc.hot_zone_radii[i];

			if (radius <= 0 || center == vector.Zero)
				continue;

			WorldRenderOval(m_SpawnCanvas, m_MapWidget, center[0], center[2], radius, radius, BR_HOT_ZONE_OUTLINE_COLOR, BR_HOT_ZONE_FILL_COLOR);
		}
	}

	/**
	 * Draws the cached heat map squares using the hoisted world->screen transform.
	 * Per cell this is pure arithmetic plus a single DrawLine -- the original did
	 * three native MapToScreen calls and one DrawLine per screen pixel row.
	 */
	protected void RenderHeatMap(vector probe_origin, float px_per_m_x, float px_per_m_z)
	{
		if (!m_HeatMapCanvas)
			return;

		m_HeatMapCanvas.Clear();

		if (m_HeatMapCells.Count() == 0)
			return;

		float screen_x, screen_y;
		m_MapWidget.GetScreenPos(screen_x, screen_y);

		float canvas_w, canvas_h;
		m_HeatMapCanvas.GetScreenSize(canvas_w, canvas_h);
		bool cull_enabled = (canvas_w > 0 && canvas_h > 0);

		// Cells are axis aligned and all the same size, so the on-screen extent
		// is computed once rather than per cell.
		float half_w = Math.AbsFloat(px_per_m_x * f_CachedGridSize) * 0.5;
		float half_h = Math.AbsFloat(px_per_m_z * f_CachedGridSize) * 0.5;

		if (half_w <= 0 || half_h <= 0)
			return;

		float origin_x = probe_origin[0] - screen_x;
		float origin_y = probe_origin[1] - screen_y;

		for (int i = 0; i < m_HeatMapCells.Count(); i++)
		{
			BRHeatCell cell = m_HeatMapCells[i];

			float cx = origin_x + (cell.world_x * px_per_m_x);
			float cy = origin_y + (cell.world_z * px_per_m_z);

			// Cull anything entirely outside the visible map area. Skipped while
			// the canvas has no measured size yet, so a not-yet-laid-out widget
			// cannot cull the whole heat map away.
			if (cull_enabled)
			{
				if (cx + half_w < 0 || cx - half_w > canvas_w)
					continue;
				if (cy + half_h < 0 || cy - half_h > canvas_h)
					continue;
			}

			RenderFilledRect(m_HeatMapCanvas, cx - half_w, cy - half_h, cx + half_w, cy + half_h, cell.color);
		}
	}

	/**
	 * Fills an axis-aligned screen-space rectangle by stacking horizontal strokes.
	 *
	 * The band height is derived from a row count rather than accumulated, so the
	 * rows tile the rectangle exactly -- no gap, no overshoot, and no drift from
	 * repeated float addition (the original stepped `y += 1` and let its last row
	 * land on an arbitrary sub-pixel offset, which made cell edges shimmer while
	 * panning).
	 *
	 * HEATMAP_FILL_MAX_STROKE caps the stroke width. Capping it keeps the fill
	 * correct whether or not CanvasWidget.DrawLine centres its stroke on the line:
	 * the worst-case misregistration is half the stroke, so a 4 px cap is at most
	 * 2 px out, against half a cell if a single full-height stroke were used and
	 * the stroke turned out not to be centred.
	 */
	protected void RenderFilledRect(CanvasWidget canvas, float left, float top, float right, float bottom, int color)
	{
		float height = bottom - top;

		if (height <= 0 || right <= left)
			return;

		float step = height;
		if (HEATMAP_FILL_MAX_STROKE > 0)
			step = Math.Min(height, HEATMAP_FILL_MAX_STROKE);

		int rows = Math.Ceil(height / step);
		if (rows < 1)
			rows = 1;

		float band = height / rows;

		for (int r = 0; r < rows; r++)
		{
			float y = top + (band * (r + 0.5));
			canvas.DrawLine(left, y, right, y, band, color);
		}
	}

	/**
	 * Rebuilds the cached heat map draw list from the current spawn points.
	 *
	 * Called only when the spawn point set or the grid size changes -- roughly
	 * once per player selection -- rather than every frame. Cells are appended in
	 * exactly the order the renderer must draw them: for each occupied cell the
	 * offset-2 ring then the offset-1 ring, and only afterwards every primary
	 * cell. The squares are translucent and blend onto each other, so changing
	 * the order changes the picture.
	 */
	protected void RebuildHeatMapCache()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::RebuildHeatMapCache");

		b_HeatMapCacheDirty = false;
		b_RenderDirty = true;

		m_HeatMapCells.Clear();
		f_CachedGridSize = spawn_size * HEATMAP_GRID_SIZE_MULTIPLIER; // Size of each grid cell in the heat map

		if (f_CachedGridSize <= 0 || !m_HeatMapSpawnPoints)
			return;

		// Declare all variables upfront
		int grid_x, grid_z, packed_key, existing_density;
		float world_x, world_z;
		float intensity, surroundIntensity;
		int capped_density, r, b, alpha, color, surroundColor;
		int offset, dx, dz;

		// Create a grid data structure to count spawn points in each cell
		ref map<int, int> grid_density = new map<int, int>();

		// Count spawn points in each grid cell
		for (int i = 0; i < m_HeatMapSpawnPoints.Count(); i++)
		{
			vector heatPoint = m_HeatMapSpawnPoints[i];

			// Convert to grid coordinates (snap to grid)
			grid_x = Math.Floor(heatPoint[0] / f_CachedGridSize);
			grid_z = Math.Floor(heatPoint[2] / f_CachedGridSize);

			if (!IsPackableCell(grid_x, grid_z))
				continue;

			// Create a unique key for this grid cell
			packed_key = PackGridKey(grid_x, grid_z);

			// Increment the density counter for this cell (one lookup, not two)
			if (grid_density.Find(packed_key, existing_density))
				grid_density.Set(packed_key, existing_density + 1);
			else
				grid_density.Insert(packed_key, 1);
		}

		// First pass: degraded effect for surroundings (lowest layers first)
		foreach (int key, int density : grid_density)
		{
			// Skip if density is 0
			if (density <= 0)
				continue;

			grid_x = UnpackGridX(key);
			grid_z = UnpackGridZ(key);

			// Convert back to world coordinates for rendering (use cell center)
			world_x = grid_x * f_CachedGridSize + (f_CachedGridSize / 2);
			world_z = grid_z * f_CachedGridSize + (f_CachedGridSize / 2);

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

				// Queue surrounding cells in a ring pattern
				for (dx = -offset; dx <= offset; dx++)
				{
					for (dz = -offset; dz <= offset; dz++)
					{
						// Skip the center and non-edge cells (only draw the ring)
						if ((Math.AbsInt(dx) != offset) && (Math.AbsInt(dz) != offset))
							continue;

						m_HeatMapCells.Insert(new BRHeatCell(world_x + (dx * f_CachedGridSize), world_z + (dz * f_CachedGridSize), surroundColor));
					}
				}
			}
		}

		// Second pass: the primary cells with full intensity
		foreach (int density_key, int density_value : grid_density)
		{
			// Skip if density is 0
			if (density_value <= 0)
				continue;

			grid_x = UnpackGridX(density_key);
			grid_z = UnpackGridZ(density_key);

			// Convert back to world coordinates for rendering (use cell center)
			world_x = grid_x * f_CachedGridSize + (f_CachedGridSize / 2);
			world_z = grid_z * f_CachedGridSize + (f_CachedGridSize / 2);

			// Cap density at max_density for color calculation
			capped_density = Math.Min(density_value, HEATMAP_MAX_DENSITY);

			// Calculate color intensity (0.0 to 1.0)
			intensity = capped_density / (float)HEATMAP_MAX_DENSITY;

			// Calculate RGB values - blue (low) to red (high) gradient
			r = Math.Round(intensity * 255);
			b = Math.Round((1 - intensity) * 255);
			color = ARGB(150, r, 0, b);

			// Queue the primary cell
			m_HeatMapCells.Insert(new BRHeatCell(world_x, world_z, color));
		}
	}

	protected bool IsPackableCell(int grid_x, int grid_z)
	{
		if (grid_x < -HEATMAP_KEY_BIAS || grid_x >= HEATMAP_KEY_BIAS || grid_z < -HEATMAP_KEY_BIAS || grid_z >= HEATMAP_KEY_BIAS)
		{
			BattleRoyaleUtils.Warn(string.Format("SpawnSelectionMenu: heat map cell out of packable range, skipped: %1 %2", grid_x, grid_z));
			return false;
		}
		return true;
	}

	protected int PackGridKey(int grid_x, int grid_z)
	{
		return ((grid_x + HEATMAP_KEY_BIAS) * HEATMAP_KEY_STRIDE) + (grid_z + HEATMAP_KEY_BIAS);
	}

	protected int UnpackGridX(int key)
	{
		return (key / HEATMAP_KEY_STRIDE) - HEATMAP_KEY_BIAS;
	}

	protected int UnpackGridZ(int key)
	{
		return (key % HEATMAP_KEY_STRIDE) - HEATMAP_KEY_BIAS;
	}

	void SetInitialCountdown(int countdown)
	{
		i_CountdownEnd = GetGame().GetTime() + (countdown * 1000);

		// Force the label to repaint on the next frame rather than waiting for the
		// second to roll over, in case the countdown is pushed mid-window.
		i_PreviousDisplayedSecond = -1;
	}

	void SetSpawnSize(float size)
	{
		spawn_size = size;

		// Drives both the marker radius and the heat map grid size, and arrives
		// after the menu is constructed -- invalidate everything.
		b_HeatMapCacheDirty = true;
		b_RenderDirty = true;
	}

	float GetSpawnSize()
	{
		return spawn_size;
	}

	void SetFirstZone(vector pos, float size)
	{
		v_FirstZoneCenter = pos;
		f_FirstZoneRadius = size;

		b_RenderDirty = true;

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

			b_RenderDirty = true;
		}
	}

	// Client mirror of 3_BattleRoyaleSpawnSelection.IsValidSpawnSelection(). Keep the two in step:
	// the server is the authority and silently drops anything it disagrees with, so a point this
	// accepts but the server does not costs the player their pick with no feedback at all.
	bool IsSpawnPositionValid(vector p)
	{
		float half_spawn = GetSpawnSize() / 2;
		int worldSize = GetGame().GetWorld().GetWorldSize();

		if (p[0] < half_spawn || p[2] < half_spawn || p[0] > (worldSize - half_spawn) || p[2] > (worldSize - half_spawn))
			return false;

		if (GetGame().SurfaceIsSea(p[0], p[2]))
			return false;

		// The server rejects ponds as well. Without this the walk below happily stops on a lake and
		// the selection is thrown away server-side.
		if (GetGame().SurfaceIsPond(p[0], p[2]))
			return false;

		if (v_FirstZoneCenter != "0 0 0" && f_FirstZoneRadius > 0)
		{
			// 2D, like the server. vector.Distance would fold in the y that ScreenToMap returns and
			// make the client stricter than the authority for no reason.
			float dx = p[0] - v_FirstZoneCenter[0];
			float dz = p[2] - v_FirstZoneCenter[2];
			// Add a small tolerance (25 meters) to account for precision errors
			if (Math.Sqrt((dx * dx) + (dz * dz)) > (f_FirstZoneRadius + 25))
				return false;
		}

		return true;
	}

	// One extra step inland from a point that just passed validation. A snapped point sitting right
	// on a shoreline leaves the server's GetRandomSafePosition(pos, spawn_selection_radius) ball
	// mostly water, and when that search gives up the player is teleported somewhere random instead
	// of anywhere near what they clicked.
	vector NudgeInland(vector pos, vector step_dir, float remaining)
	{
		if (remaining <= 0)
			return pos;

		float travel = BR_SPAWN_SNAP_STEP;
		if (travel > remaining)
			travel = remaining;

		vector nudged = pos + (step_dir * travel);
		nudged[1] = 0;

		if (IsSpawnPositionValid(nudged))
			return nudged;

		return pos;
	}

	// Resolve a clicked point to something the player can actually spawn on, by walking from the
	// click towards the zone centre and taking the first valid sample. Searching that one direction
	// is enough: for a click outside the circle the nearest in-circle point IS on the line to the
	// centre, and for a click on water that same line heads inland on any sensibly placed zone.
	// Returns vector.Zero only when the whole line is unusable - a zone centred on open water.
	vector FindNearestValidSpawn(vector pos)
	{
		int worldSize = GetGame().GetWorld().GetWorldSize();
		float half_spawn = GetSpawnSize() / 2;

		vector candidate = pos;
		candidate[1] = 0;
		candidate[0] = Math.Clamp(candidate[0], half_spawn, worldSize - half_spawn);
		candidate[2] = Math.Clamp(candidate[2], half_spawn, worldSize - half_spawn);

		// Where to walk towards. Without a usable zone the menu still needs a direction, and the
		// middle of the map is the only landmark it has.
		vector target = Vector(worldSize / 2, 0, worldSize / 2);
		if (v_FirstZoneCenter != "0 0 0" && f_FirstZoneRadius > 0)
		{
			target = Vector(v_FirstZoneCenter[0], 0, v_FirstZoneCenter[2]);

			// A click outside the circle is resolved by this projection alone: drop it onto the
			// boundary, a little inside it, and let the walk below deal with water from there.
			vector from_center = candidate - target;
			float center_distance = from_center.Length();
			if (center_distance > f_FirstZoneRadius)
			{
				float inset_radius = f_FirstZoneRadius - BR_SPAWN_SNAP_INSET;
				if (inset_radius < 0)
					inset_radius = 0;

				candidate = target + (from_center.Normalized() * inset_radius);
				candidate[1] = 0;
				candidate[0] = Math.Clamp(candidate[0], half_spawn, worldSize - half_spawn);
				candidate[2] = Math.Clamp(candidate[2], half_spawn, worldSize - half_spawn);
			}
		}

		// The common case - a click on open ground inside the circle - costs one validity test and
		// no walking.
		if (IsSpawnPositionValid(candidate))
			return candidate;

		vector to_target = target - candidate;
		float remaining = to_target.Length();
		if (remaining <= 0)
			return vector.Zero;

		vector step_dir = to_target.Normalized();

		int steps = Math.Ceil(remaining / BR_SPAWN_SNAP_STEP);
		if (steps > BR_SPAWN_SNAP_MAX_STEPS)
			steps = BR_SPAWN_SNAP_MAX_STEPS;

		for (int i = 1; i <= steps; i++)
		{
			float travelled = i * BR_SPAWN_SNAP_STEP;
			// The last sample is the target itself rather than a point beyond it.
			if (travelled > remaining)
				travelled = remaining;

			vector probe = candidate + (step_dir * travelled);
			probe[1] = 0;

			if (IsSpawnPositionValid(probe))
				return NudgeInland(probe, step_dir, remaining - travelled);
		}

		return vector.Zero;
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

		// Water, out of bounds and outside the first zone are no longer rejected outright - the click
		// is snapped to the nearest point the server will accept. Zero comes back only when no such
		// point exists along the line to the zone centre.
		vector spawnPosition = FindNearestValidSpawn(tempPosition);

		if (spawnPosition == vector.Zero)
		{
			BattleRoyaleUtils.Trace(string.Format("SpawnSelectionMenu::SelectSpawnPoint No valid spawn near %1", tempPosition));
			return;
		}

		BattleRoyaleUtils.Trace(string.Format("SpawnSelectionMenu::SelectSpawnPoint: click %1 -> spawn %2", tempPosition, spawnPosition));

		//--- No target: the server resolves the subject from the RPC sender identity and re-runs the
		//--- checks above. Sending one would be ignored - it was how a client could set another
		//--- player's spawn point.
		GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", new Param1<vector>( spawnPosition ), true );
	}

	void WorldRenderOval(CanvasWidget canvas, MapWidget world_map, float world_x, float world_z, float radius_x, float radius_z, int color = -1, int fill_color = 0)
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

		// Optional fill, drawn under the outline. Horizontal bands of up to
		// HEATMAP_FILL_MAX_STROKE pixels, exactly like RenderFilledRect -- NOT one
		// stroke per screen pixel row, which is what the heat map used to do and
		// costs 2*radius native calls for a shape that needs a few dozen. The
		// half-width is sampled at each band's centre, so the fill is off by at
		// most half a band at the very top and bottom of the ellipse; the outline
		// below is what defines the edge, and the fill is translucent anyway.
		if (fill_color != 0)
		{
			float fill_step = 2 * screenHeight;
			if (HEATMAP_FILL_MAX_STROKE > 0)
				fill_step = Math.Min(fill_step, HEATMAP_FILL_MAX_STROKE);

			int fill_bands = Math.Ceil((2 * screenHeight) / fill_step);
			if (fill_bands < 1)
				fill_bands = 1;

			float fill_band_h = (2 * screenHeight) / fill_bands;

			for (int band = 0; band < fill_bands; band++)
			{
				float band_y = (cy - screenHeight) + (fill_band_h * (band + 0.5));
				float band_dy = (band_y - cy) / screenHeight;
				float band_span = 1 - (band_dy * band_dy);
				if (band_span <= 0)
					continue;

				float band_half_w = screenWidth * Math.Sqrt(band_span);
				canvas.DrawLine(cx - band_half_w, band_y, cx + band_half_w, band_y, fill_band_h, fill_color);
			}
		}

		// Draw the oval. Scale the segment count to the on-screen size instead of
		// forcing a floor of 360 segments regardless of radius. n = PI*sqrt(r)
		// holds the sagitta -- how far the chord strays from the true arc -- at
		// half a pixel for any radius, so this is a visual no-op: a 400 px zone
		// circle gets 63 segments, a 10 px spawn marker gets the 16 floor.
		float maxRadius = Math.Max(screenWidth, screenHeight);
		if (maxRadius < 1)
			maxRadius = 1;

		int segments = Math.Round(Math.PI * Math.Sqrt(maxRadius));
		if (segments < BR_OVAL_MIN_SEGMENTS)
			segments = BR_OVAL_MIN_SEGMENTS;
		if (segments > BR_OVAL_MAX_SEGMENTS)
			segments = BR_OVAL_MAX_SEGMENTS;

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

	void UpdateHeatMap(array<vector> spawnPoints)
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::UpdateHeatMap");
		m_HeatMapSpawnPoints = spawnPoints;
		b_HeatMapCacheDirty = true;
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

			b_RenderDirty = true;
		}
	}
}