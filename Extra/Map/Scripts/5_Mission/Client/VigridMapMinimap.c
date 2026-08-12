#ifndef SERVER
/**
 *  Vigrid Map - the HUD minimap.
 *
 *  The widget block is salvaged from the mod's own long-disabled BR_MINIMAP layout, which was the
 *  only part of that attempt worth keeping: 200x200 in the bottom-right, a canvas child and a
 *  rotating heading arrow, geometry somebody already tuned. The tick is written fresh - the old
 *  script does not compile and is not being repaired.
 *
 *  It deliberately does NOT reuse the fullscreen map's probe/dirty/watchdog gate. That gate exists
 *  to avoid repainting when nothing moved, and on a minimap something always has: it follows the
 *  camera, so the transform changes almost every tick. The 10 Hz timer IS the gate here, and at
 *  200 px across nobody can see the difference between that and frame rate.
 *
 *  Visibility is the AND of three switches that are not equivalent. VIGRID_MAP_MINIMAP is the
 *  BUILD's, declared in Extra/Map/config.cpp and wrapping this whole file - comment it out and there
 *  is no minimap in the PBO at all. The server's minimap_allowed is an admin decision arriving over
 *  VM_Settings, and VigridMapPrefs.IsMinimapEnabled is the player's, persisted locally. Each one can
 *  only ever opt further out than the one above it.
 *
 *  Deliberately NOT behind the define: VigridMapPrefs, VigridMapRPC.minimap_allowed and
 *  VigridMapData.minimap_allowed. A client build flag must not change the wire format or the shape
 *  of the server's settings file, so those stay and simply go unread. The N keybind stays in
 *  Data/Inputs.xml too - XML cannot be conditional - so it still shows up under Options > Controls
 *  in a minimap-less build, bound to nothing.
 */
#ifdef VIGRID_MAP_MINIMAP
class VigridMapMinimap
{
    private Widget m_Root;
    private bool m_RootFailed;

    private MapWidget m_MapWidget;
    private CanvasWidget m_Canvas;

    private int m_LastTickMs;
    private bool m_Shown;

    void ~VigridMapMinimap()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    /**
     *  Built on the first Update rather than in the constructor, the same shape the world markers
     *  and the kill feed both use: the constructor runs inside MissionGameplay.OnInit, and the only
     *  timing proven to work in this mod is after super.OnInit() has returned.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        m_Root = GetGame().GetWorkspace().CreateWidgets(VIGRID_MAP_PREFIX + "GUI/layouts/minimap.layout");

        if (!m_Root)
        {
            //--- Latched: retrying every tick would spam the log for the whole session.
            m_RootFailed = true;
            VigridMapLog.Error("Could not create minimap.layout - minimap disabled");
            return false;
        }

        m_MapWidget = MapWidget.Cast(m_Root.FindAnyWidget("MiniMap"));
        if (!m_MapWidget)
        {
            m_RootFailed = true;
            m_Root.Unlink();
            m_Root = NULL;
            VigridMapLog.Error("minimap.layout has no MiniMap widget - minimap disabled");
            return false;
        }

        //--- Same mechanism as the fullscreen map: a canvas DECLARED as a child of the MapWidget is
        //--- the only overlay that renders. Widgets created from script over a map are positioned
        //--- correctly and never drawn.
        m_Canvas = CanvasWidget.Cast(m_MapWidget.FindAnyWidget("CanvasMiniMap"));
        if (!m_Canvas)
            VigridMapLog.Error("minimap.layout has no CanvasMiniMap - minimap overlay disabled");

        m_Root.Show(false);
        VigridMapLog.Debug("Minimap layout ready");
        return true;
    }

    //! Effective visibility: the admin's switch, the player's switch, and being alive to read it.
    private bool ShouldShow()
    {
        if (!VigridMapRPC.GetInstance().minimap_allowed)
            return false;
        if (!VigridMapPrefs.IsMinimapEnabled())
            return false;

        //--- Hidden behind any full-screen menu, the addon's own map included - a 200 px map over
        //--- the fullscreen one is noise, and the inventory and chat deserve the room.
        UIManager ui = GetGame().GetUIManager();
        if (ui && ui.GetMenu())
            return false;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        return player.IsAlive();
    }

    void Update(float timeslice)
    {
        if (!EnsureRoot())
            return;

        if (!ShouldShow())
        {
            if (m_Shown)
            {
                m_Root.Show(false);
                m_Shown = false;
            }
            return;
        }

        if (!m_Shown)
        {
            m_Root.Show(true);
            m_Shown = true;
        }

        if ((GetGame().GetTime() - m_LastTickMs) < VIGRID_MAP_MINIMAP_TICK_MS)
            return;

        m_LastTickMs = GetGame().GetTime();
        Tick();
    }

    private void Tick()
    {
        //--- Centred on the camera rather than the player: in third person the two differ by a few
        //--- metres, and it is the camera the player is steering by.
        vector camera_pos = GetGame().GetCurrentCameraPosition();

        m_MapWidget.SetMapPos(camera_pos);
        m_MapWidget.SetScale(VIGRID_MAP_MINIMAP_SCALE);

        //--- The fullscreen map needs a deferred re-issue of SetMapPos because the widget ignores it
        //--- until it has been laid out. Here that solves itself: the next tick is 100 ms away and
        //--- lands after layout.

        if (!m_Canvas)
            return;

        //--- A canvas keeps its draw list between frames, so without this every tick stacks another
        //--- copy on the last and the whole thing smears as you walk.
        m_Canvas.Clear();

        DrawZones();
        DrawMarkers();

        //--- Last, so the heading dart sits on top of the zone rings and any marker underneath it.
        //--- The map is centred on the camera, so the player is by definition at camera_pos.
        DrawHeadingArrow(camera_pos);
    }

    /**
     *  The "you are here, facing that way" dart, drawn on the canvas.
     *
     *  The map is north-up and does not rotate, so the glyph carries the heading instead.
     *
     *  THE SOURCE IS THE CAMERA, NOT THE PLAYER, and that is the part worth keeping.
     *  `player.GetYawPitchRoll()[0]` is the animated BODY orientation: in DayZ the legs turn in
     *  discrete steps and the torso leads, so it lags, snaps, and does not come back to where it
     *  started after a full turn. The symptom was exactly "spin a 360 and the arrow doesn't stop in
     *  the same place", which no constant offset or sign flip can explain, since an absolute heading
     *  must repeat every 360 degrees. Camera direction is continuous and is what the player steers
     *  by. (Vanilla's MapMenu uses body yaw and gets away with it because its map is a fullscreen
     *  menu read while standing still. The mod's own long-disabled BR_MINIMAP used the camera, but
     *  paired it with `icon_minus` - a symmetric dash - so it never proved anything either way.)
     *
     *  This replaced a rotated ImageWidget, which went wrong twice for reasons that no longer apply:
     *  `icon_arrow`'s rest angle points DOWN so it needed an unobvious +180, and it is a solid
     *  triangle, which reads ambiguously at 24 px because both ends look like a possible point.
     *  Vanilla's dedicated arrow texture would have been better but does not resolve from a mod PBO.
     *  Drawing it sidesteps all three: no asset, no rest angle, and a notched tail that can only be
     *  read one way round.
     */
    private void DrawHeadingArrow(vector center_pos)
    {
        float heading = Math.NormalizeAngle(GetGame().GetCurrentCameraDirection().VectorToAngles()[0]);

        VigridMapRender.WorldRenderHeadingArrow(m_Canvas, m_MapWidget, center_pos, heading, VIGRID_MAP_MINIMAP_ARROW_PX, VIGRID_MAP_COLOR_SELF, VIGRID_MAP_SELF_LINE_WIDTH);
    }

    private void DrawZones()
    {
        if (VigridMapAPI.HasCurrentZone())
        {
            VigridMapRender.WorldRenderOval(m_Canvas, m_MapWidget, VigridMapAPI.GetCurrentCenter(), VigridMapAPI.GetCurrentRadius(), VigridMapAPI.GetCurrentRadius(), VIGRID_MAP_COLOR_CURRENT_ZONE, VIGRID_MAP_ZONE_LINE_WIDTH);
        }

        if (!VigridMapAPI.HasNextZone())
            return;

        VigridMapRender.WorldRenderOval(m_Canvas, m_MapWidget, VigridMapAPI.GetNextCenter(), VigridMapAPI.GetNextRadius(), VigridMapAPI.GetNextRadius(), VIGRID_MAP_COLOR_NEXT_ZONE, VIGRID_MAP_ZONE_LINE_WIDTH);
        VigridMapRender.WorldRenderDot(m_Canvas, m_MapWidget, VigridMapAPI.GetNextCenter(), VIGRID_MAP_CENTER_DOT_PX, VIGRID_MAP_COLOR_NEXT_ZONE);
    }

    /**
     *  Placed markers only - no teammates and no pings.
     *
     *  Not an oversight: at 200 px across, a triangle, a diamond and a pin within a few pixels of
     *  each other are unreadable, and the teammate information is already on the fullscreen map and
     *  in the party name tags. The glyph helpers are shared, so adding them later is a few lines if
     *  it turns out to be wanted.
     */
    private void DrawMarkers()
    {
        VigridMapClient client = GetClient();
        if (!client)
            return;

        int count = client.GetDrawCount();
        for (int i = 0; i < count; i++)
        {
            int color = VigridMapTeam.GetColorForSlot(client.GetDrawSlot(i), 1.0);
            if (client.IsDrawOwn(i))
                color = VIGRID_MAP_COLOR_OWN_MARKER;

            //--- A plain dot rather than the fullscreen map's ring-and-cross: at this size the ring
            //--- closes up into a blob anyway, and the cross reads as noise.
            VigridMapRender.WorldRenderDot(m_Canvas, m_MapWidget, client.GetDrawPos(i), VIGRID_MAP_MINIMAP_MARKER_PX, color);
        }
    }

    private VigridMapClient GetClient()
    {
        MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
        if (!mission)
            return NULL;

        return mission.GetVigridMapClient();
    }
}
#endif
#endif
