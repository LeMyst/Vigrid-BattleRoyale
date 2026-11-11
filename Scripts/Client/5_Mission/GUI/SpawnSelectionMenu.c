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
	//--- Not ref: the mission owns its HUD, and this menu must not keep it alive past the mission.
	protected IngameHud m_Hud;
	protected ref BRMapHandler m_MapMenuHandler;

	protected MapWidget m_MapWidget;

	//--- Timer: the number and its label are centred together by LayoutTimer, the bar drains.
	protected Widget m_TimerInner;
	protected TextWidget m_TimerNumber;
	protected TextWidget m_TimerLabel;
	protected Widget m_TimerBar;
	protected int i_CountdownTotalMs;
	protected int i_TimerLayoutFrames;

	//--- "Your pick" card.
	protected TextWidget m_PickName;
	protected TextWidget m_PickCoords;

	//--- Party member name tags, placed beside their markers.
	protected ref array<Widget> m_MateTags = new array<Widget>();
	protected ref array<TextWidget> m_MateTagTexts = new array<TextWidget>();

	//--- Marker colours follow the role, not the server's per-slot colour: you, or a party member.
	static const int BR_SPAWN_SELF_COLOR = 0xFFD99E29;
	static const int BR_SPAWN_MATE_COLOR = 0xFF3DB9A6;
	static const int BR_SPAWN_INK_COLOR = 0xFF111417;
	static const int BR_SPAWN_ZONE_COLOR = 0xFF6E9BF5;

	static const int BR_SPAWN_MATE_TAGS = 6;				//!< MateTag0..5 in the layout
	static const float BR_SPAWN_TIMER_INNER_W = 598.0;		//!< TimerInner's declared width
	static const int BR_OVAL_DASHED_MIN_SEGMENTS = 64;

	//--- Other players' picks, one hue so it never reads as a hot zone (red) or the play area (blue).
	static const int BR_HEAT_R = 176;
	static const int BR_HEAT_G = 124;
	static const int BR_HEAT_B = 240;

	//--- Named places for the pick card, read once per world from CfgWorlds.
	protected static ref array<string> s_PlaceNames;
	protected static ref array<vector> s_PlacePos;
	protected static ref array<float> s_PlaceRadius;
	protected static string s_PlaceWorld;

	protected CanvasWidget m_SpawnCanvas;

	protected int i_CountdownEnd;

	protected float spawn_size = 50.0;

	protected vector v_FirstZoneCenter = "0 0 0";
	protected float f_FirstZoneRadius = 0;

	//--- Every confirmed pick this client is told about, its own included (m_SelfId).
	protected ref map<string, vector> m_TeammateSpawnPoints = new map<string, vector>();
	protected ref map<string, string> m_TeammateNames = new map<string, string>();
	protected string m_SelfId = "";

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

	protected ref BattleRoyaleMenuInputLock m_InputLock;

	void SpawnSelectionMenu()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::SpawnSelectionMenu");
		m_InputLock = new BattleRoyaleMenuInputLock();
	}

	void ~SpawnSelectionMenu()
	{
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
		//--- No ChangeGameFocus(1) here, and none of the matching -1 in OnHide. super.OnShow ->
		//--- LockControls already takes focus PER DEVICE, as every vanilla menu does; the extra
		//--- ALL-DEVICE call froze the whole input layer (see VigridPartyMenu.OnShow, where it broke
		//--- P and Esc). Removing it does not make this menu dismissable - nothing here polls
		//--- UAUIBack - and does not free the player: the movement lock is the server's SetInput(true)
		//--- from 3_BattleRoyaleSpawnSelection.DisableInput, plus LockControls itself.
		SetFocus(layoutRoot);

		//--- Neither of those stops the CAMERA: mouse look kept turning the player behind the map.
		//--- This freezes it, and keeps push-to-talk so a party can still coordinate its drop. See
		//--- BattleRoyaleMenuInputLock.
		m_InputLock.Take();

		// Nothing on the canvases survives a hide/show cycle -- force a redraw.
		b_RenderDirty = true;
		i_PreviousDisplayedSecond = -1;
	}

	override void OnHide()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::OnHide");
		super.OnHide();

		PPEffects.SetBlurMenu(0);
		//--- super.OnHide -> UnlockControls releases exactly what LockControls took. NEVER add a
		//--- ResetGameFocus() here: it SETS the shared additive counter to zero on every device
		//--- (input.c:22-27), releasing every other holder's acquire rather than our own.
		m_InputLock.Release();
	}

	override Widget Init()
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::Init");
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/spawn_selection.layout");
		m_Hud = IngameHud.Cast(GetGame().GetMission().GetHud());

		m_MapWidget = MapWidget.Cast(layoutRoot.FindAnyWidget("SpawnMap"));

		m_TimerInner = layoutRoot.FindAnyWidget("TimerInner");
		m_TimerNumber = TextWidget.Cast(layoutRoot.FindAnyWidget("TimerNumber"));
		m_TimerLabel = TextWidget.Cast(layoutRoot.FindAnyWidget("TimerLabel"));
		m_TimerBar = layoutRoot.FindAnyWidget("TimerBar");
		m_PickName = TextWidget.Cast(layoutRoot.FindAnyWidget("PickName"));
		m_PickCoords = TextWidget.Cast(layoutRoot.FindAnyWidget("PickCoords"));

		for (int tag_index = 0; tag_index < BR_SPAWN_MATE_TAGS; tag_index++)
		{
			Widget mate_tag = layoutRoot.FindAnyWidget("MateTag" + tag_index);
			if (!mate_tag)
				break;

			m_MateTags.Insert(mate_tag);
			m_MateTagTexts.Insert(TextWidget.Cast(layoutRoot.FindAnyWidget("MateTag" + tag_index + "Text")));
		}

		BuildPlaceCache();

		if (m_MapWidget)
		{
			//--- No map centring here: Init runs inside EnterScriptedMenu, before SetFirstZone.
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
		UpdateTimerBar();

		//--- GetTextSize lags a SetText, so the centring runs for a few frames after each change.
		if (i_TimerLayoutFrames > 0)
		{
			i_TimerLayoutFrames--;
			LayoutTimer();
		}

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
	 * Refreshes the number and its label, but only when the displayed second changes.
	 */
	protected void UpdateCountdownText()
	{
		if (!m_TimerNumber || !m_TimerLabel)
			return;

		//--- 1000.0, not 1000: int division truncates before Ceil sees it, losing up to 999 ms.
		float time_left = Math.Ceil((i_CountdownEnd - GetGame().GetTime()) / 1000.0);

		// Clamp below zero so the expired "time to deploy" branch settles on one
		// value instead of re-firing as the countdown keeps running negative.
		int displayed_second = Math.Round(time_left);
		if (displayed_second < 0)
			displayed_second = 0;

		if (displayed_second == i_PreviousDisplayedSecond)
			return;

		i_PreviousDisplayedSecond = displayed_second;
		i_TimerLayoutFrames = 4;

		if (time_left >= 1)
		{
			m_TimerNumber.Show(true);
			m_TimerNumber.SetText(displayed_second.ToString());

			if (time_left > 1)
				m_TimerLabel.SetText("#STR_BR_TIMER_SPAWN_SELECTION_SECONDS");
			else
				m_TimerLabel.SetText("#STR_BR_TIMER_SPAWN_SELECTION_SECOND");
		}
		else
		{
			m_TimerNumber.Show(false);
			m_TimerLabel.SetText("#STR_BR_TIME_TO_DEPLOY");
		}
	}

	//! The bar under the timer, as the share of the window still left. Every frame: it is one SetSize.
	protected void UpdateTimerBar()
	{
		if (!m_TimerBar || !m_TimerInner)
			return;

		float inner_w, inner_h;
		m_TimerInner.GetScreenSize(inner_w, inner_h);
		if (inner_w <= 0)
			return;

		float frac = 0;
		if (i_CountdownTotalMs > 0)
			frac = Math.Clamp((i_CountdownEnd - GetGame().GetTime()) / (float)i_CountdownTotalMs, 0, 1);

		float s = inner_w / BR_SPAWN_TIMER_INNER_W;
		m_TimerBar.SetSize(inner_w * frac, 6 * s);
	}

	//! Centre the number and the label as one group: their widths change with the second and the language.
	protected void LayoutTimer()
	{
		if (!m_TimerInner || !m_TimerNumber || !m_TimerLabel)
			return;

		float inner_w, inner_h;
		m_TimerInner.GetScreenSize(inner_w, inner_h);
		if (inner_w <= 0)
			return;

		float s = inner_w / BR_SPAWN_TIMER_INNER_W;

		int number_w = 0;
		int number_h = 0;
		int label_w = 0;
		int label_h = 0;
		m_TimerNumber.GetTextSize(number_w, number_h);
		m_TimerLabel.GetTextSize(label_w, label_h);

		float gap = 18 * s;
		if (!m_TimerNumber.IsVisible() || number_w <= 0)
		{
			number_w = 0;
			gap = 0;
		}

		float x = (inner_w - (number_w + gap + label_w)) / 2;
		if (x < 0)
			x = 0;

		m_TimerNumber.SetPos(x, 8 * s);
		m_TimerNumber.SetSize(number_w + 2, 72 * s);
		m_TimerLabel.SetPos(x + number_w + gap, 8 * s);
		m_TimerLabel.SetSize(label_w + 2, 72 * s);
	}

	/**
	 * Draws the hot zones, the play area and every pick onto the marker canvas, and places the party
	 * name tags. Only ever a handful of shapes, so this still converts through MapToScreen.
	 */
	protected void RenderMarkers()
	{
		m_SpawnCanvas.Clear();

		// Hot zones first, so the markers and the play-area ring draw over them. They are the only
		// filled world shape on this canvas, and a fill laid over a marker would swallow it.
		RenderHotZones();

		if(v_FirstZoneCenter != "0 0 0" && f_FirstZoneRadius > 0)
			WorldRenderOval(m_SpawnCanvas, m_MapWidget, v_FirstZoneCenter[0], v_FirstZoneCenter[2], f_FirstZoneRadius, f_FirstZoneRadius, BR_SPAWN_ZONE_COLOR, 0, true, 3);

		float root_w, root_h;
		layoutRoot.GetScreenSize(root_w, root_h);
		float s = 1;
		if (root_w > 0)
			s = root_w / 1920.0;

		float map_x, map_y;
		m_MapWidget.GetScreenPos(map_x, map_y);

		int used_tags = 0;
		foreach (string playerId, vector spawn_point : m_TeammateSpawnPoints)
		{
			if (!playerId)
				continue;

			bool is_self = (playerId == m_SelfId);
			int color = BR_SPAWN_MATE_COLOR;
			if (is_self)
				color = BR_SPAWN_SELF_COLOR;

			// The area the server scatters the spawn over, then a fixed-size marker that stays
			// visible at any zoom.
			WorldRenderOval(m_SpawnCanvas, m_MapWidget, spawn_point[0], spawn_point[2], spawn_size, spawn_size, color);

			vector marker_screen = m_MapWidget.MapToScreen(Vector(spawn_point[0], 0, spawn_point[2]));
			float cx = marker_screen[0] - map_x;
			float cy = marker_screen[1] - map_y;

			if (is_self)
			{
				RenderFilledDisc(m_SpawnCanvas, cx, cy, 12 * s, BR_SPAWN_INK_COLOR);
				RenderFilledDisc(m_SpawnCanvas, cx, cy, 9 * s, color);
				RenderFilledDisc(m_SpawnCanvas, cx, cy, 3 * s, BR_SPAWN_INK_COLOR);
				continue;
			}

			RenderFilledRect(m_SpawnCanvas, cx - (9 * s), cy - (9 * s), cx + (9 * s), cy + (9 * s), BR_SPAWN_INK_COLOR);
			RenderFilledRect(m_SpawnCanvas, cx - (6 * s), cy - (6 * s), cx + (6 * s), cy + (6 * s), color);

			if (used_tags < m_MateTags.Count())
			{
				PlaceMateTag(used_tags, m_TeammateNames.Get(playerId), cx + (15 * s), cy - (15 * s), s);
				used_tags++;
			}
		}

		for (int i = used_tags; i < m_MateTags.Count(); i++)
		{
			Widget unused_tag = m_MateTags[i];
			if (unused_tag)
				unused_tag.Show(false);
		}
	}

	//! A party member's name beside their marker, in SpawnMap's own coordinates (the tags are its
	//! children). Sized from the text, which lags a SetText by a frame - the repaint watchdog
	//! corrects the first width.
	protected void PlaceMateTag(int slot, string mate_name, float x, float y, float s)
	{
		Widget mate_tag = m_MateTags[slot];
		TextWidget mate_text = m_MateTagTexts[slot];
		if (!mate_tag || !mate_text)
			return;

		mate_text.SetText(mate_name);

		int text_w = 0;
		int text_h = 0;
		mate_text.GetTextSize(text_w, text_h);

		float tag_w = 160 * s;
		if (text_w > 0)
			tag_w = text_w + (18 * s);

		mate_tag.SetPos(x, y);
		mate_tag.SetSize(tag_w, 30 * s);
		mate_text.SetPos(9 * s, 0);
		mate_text.SetSize(text_w + 2, 30 * s);
		mate_tag.Show(true);
	}

	//! A screen-space filled disc, in bands like RenderFilledRect.
	protected void RenderFilledDisc(CanvasWidget canvas, float cx, float cy, float radius, int color)
	{
		if (radius <= 0)
			return;

		float height = 2 * radius;
		float step = height;
		if (HEATMAP_FILL_MAX_STROKE > 0)
			step = Math.Min(height, HEATMAP_FILL_MAX_STROKE);

		int rows = Math.Ceil(height / step);
		if (rows < 1)
			rows = 1;

		float band = height / rows;
		for (int r = 0; r < rows; r++)
		{
			float y = (cy - radius) + (band * (r + 0.5));
			float dy = (y - cy) / radius;
			float span = 1 - (dy * dy);
			if (span <= 0)
				continue;

			float half_w = radius * Math.Sqrt(span);
			canvas.DrawLine(cx - half_w, y, cx + half_w, y, band, color);
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
		int capped_density, alpha, color, surroundColor;
		int offset, dx, dz;

		// Create a grid data structure to count spawn points in each cell
		map<int, int> grid_density = new map<int, int>();

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
				// Density fades with distance from the cell, and the ring further out is fainter still.
				surroundIntensity = intensity * (1 - (offset * 0.3));
				alpha = Math.Round((25 + (75 * surroundIntensity)) * (3 - offset) / 2.0);
				surroundColor = ARGB(alpha, BR_HEAT_R, BR_HEAT_G, BR_HEAT_B);

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

			// One hue, as the legend's ramp: alpha carries the density.
			alpha = 60 + Math.Round(intensity * 140);
			color = ARGB(alpha, BR_HEAT_R, BR_HEAT_G, BR_HEAT_B);

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
		i_CountdownTotalMs = countdown * 1000;

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

		//--- Offline there is no server to answer with ShowSpawnPoint, so place the marker here.
		if (!GetGame().IsMultiplayer())
		{
			string self_id = m_SelfId;
			if (self_id == "")
				self_id = "local";
			string self_name = "You";
			if (m_TeammateNames.Contains(self_id))
				self_name = m_TeammateNames.Get(self_id);
			SetSpawnMarker(self_id, self_name, spawnPosition, true);
			return;
		}

		//--- No target: the server resolves the subject from the RPC sender identity and re-runs the
		//--- checks above. Sending one would be ignored - it was how a client could set another
		//--- player's spawn point.
		GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", new Param1<vector>( spawnPosition ), true );
	}

	void WorldRenderOval(CanvasWidget canvas, MapWidget world_map, float world_x, float world_z, float radius_x, float radius_z, int color = -1, int fill_color = 0, bool dashed = false, float line_width = 2)
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

		//--- Dashed: every other segment, with enough of them that a dash stays short.
		if (dashed && segments < BR_OVAL_DASHED_MIN_SEGMENTS)
			segments = BR_OVAL_DASHED_MIN_SEGMENTS;

		float angleIncrement = 360.0 / segments;

		for(int i = 0; i < segments; i++)
		{
			if (dashed && (i % 2) == 1)
				continue;

			float angle1 = i * angleIncrement;
			float angle2 = (i + 1) * angleIncrement;

			float x1 = cx + (screenWidth * Math.Cos(angle1 * Math.DEG2RAD));
			float y1 = cy + (screenHeight * Math.Sin(angle1 * Math.DEG2RAD));

			float x2 = cx + (screenWidth * Math.Cos(angle2 * Math.DEG2RAD));
			float y2 = cy + (screenHeight * Math.Sin(angle2 * Math.DEG2RAD));

			canvas.DrawLine(x1, y1, x2, y2, line_width, color);
		}
	}

	void UpdateHeatMap(array<vector> spawnPoints)
	{
		BattleRoyaleUtils.Trace("SpawnSelectionMenu::UpdateHeatMap");
		m_HeatMapSpawnPoints = spawnPoints;
		b_HeatMapCacheDirty = true;
	}

	//! The server's ShowSpawnPoint. Its colour is the party slot's; this screen colours by role instead.
	void SetTeammateSpawnPoint(PlayerBase player, vector pos, int color)
	{
		if (!player || !player.GetIdentity() || !player.GetIdentity().GetId())
			return;

		bool is_self = (player == GetGame().GetPlayer());
		SetSpawnMarker(player.GetIdentity().GetId(), player.GetIdentity().GetName(), pos, is_self);
	}

	void SetSpawnMarker(string id, string display_name, vector pos, bool is_self)
	{
		BattleRoyaleUtils.Trace(string.Format("SpawnSelectionMenu::SetSpawnMarker %1 self=%2 %3", display_name, is_self, pos));

		m_TeammateSpawnPoints.Set(id, pos);
		m_TeammateNames.Set(id, display_name);

		if (is_self)
		{
			m_SelfId = id;
			UpdatePickCard(pos);
		}

		b_RenderDirty = true;
	}

	protected void UpdatePickCard(vector pos)
	{
		if (m_PickName)
			m_PickName.SetText(DescribePlace(pos));

		if (m_PickCoords)
		{
			int grid_x = Math.Round(pos[0]);
			int grid_z = Math.Round(pos[2]);
			m_PickCoords.SetText(string.Format("%1 / %2", grid_x, grid_z));
		}
	}

	//! The nearest named place whose reach covers pos, or "open ground".
	string DescribePlace(vector pos)
	{
		BuildPlaceCache();

		string best = "#STR_BR_SPAWN_OPEN_GROUND";
		float best_ratio = 1;

		for (int i = 0; i < s_PlaceNames.Count(); i++)
		{
			vector place = s_PlacePos[i];
			float reach = s_PlaceRadius[i];

			float dx = pos[0] - place[0];
			float dz = pos[2] - place[2];
			float ratio = Math.Sqrt((dx * dx) + (dz * dz)) / reach;

			if (ratio < best_ratio)
			{
				best_ratio = ratio;
				best = s_PlaceNames[i];
			}
		}

		return best;
	}

	protected static void BuildPlaceCache()
	{
		string world = GetGame().GetWorldName();
		if (s_PlaceNames && s_PlaceWorld == world)
			return;

		s_PlaceWorld = world;
		s_PlaceNames = new array<string>();
		s_PlacePos = new array<vector>();
		s_PlaceRadius = new array<float>();

		string cfg = "CfgWorlds " + world + " Names";
		int total = GetGame().ConfigGetChildrenCount(cfg);
		for (int i = 0; i < total; i++)
		{
			string child;
			GetGame().ConfigGetChildName(cfg, i, child);

			string path = cfg + " " + child;
			float reach = PlaceReach(GetGame().ConfigGetTextOut(path + " type"));
			if (reach <= 0)
				continue;

			TFloatArray raw = {};
			GetGame().ConfigGetFloatArray(path + " position", raw);
			if (raw.Count() < 2)
				continue;

			string place_name = GetGame().ConfigGetTextOut(path + " name");
			if (place_name == "")
				continue;

			//--- A stringtable reference in config is "$STR_..."; widgets want "#STR_...".
			if (place_name.IndexOf("$") == 0)
				place_name = Widget.TranslateString("#" + place_name.Substring(1, place_name.Length() - 1));

			s_PlaceNames.Insert(place_name);
			s_PlacePos.Insert(Vector(raw.Get(0), 0, raw.Get(1)));
			s_PlaceRadius.Insert(reach);
		}

		BattleRoyaleUtils.Trace(string.Format("SpawnSelectionMenu: %1 named places on %2", s_PlaceNames.Count(), world));
	}

	//! How far a place's name carries, by CfgWorlds type. 0 = never named (hills, viewpoints, offices).
	protected static float PlaceReach(string place_type)
	{
		if (place_type == "Capital")
			return 900;
		if (place_type == "City")
			return 700;
		if (place_type == "Village")
			return 450;
		if (place_type == "Local" || place_type == "Camp" || place_type == "Ruin" || place_type == "Marine" || place_type == "RailroadStation")
			return 300;
		return 0;
	}
}
