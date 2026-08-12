#ifndef SERVER
/**
 *  Vigrid Map - the HUD compass strip.
 *
 *  A 620x40 band across the top of the screen: cardinal letters and numeric degrees sliding under a
 *  fixed centre cursor, with the exact bearing read out below it, and carets in the bottom lane for
 *  the next zone, party pings and teammates.
 *
 *  THE SOURCE IS THE CAMERA, NOT THE PLAYER. `player.GetYawPitchRoll()[0]` is the animated BODY
 *  orientation: the legs turn in discrete steps and the torso leads, so it lags, snaps, and does not
 *  come back to where it started after a full turn. That was diagnosed and fixed once already on the
 *  minimap's heading dart (VigridMapMinimap.DrawHeadingArrow) and the same line is used here. The
 *  regression test is the same too: spin a 360 and the strip must return to the same reading.
 *
 *  DRAWN WITH POOLED WIDGETS, NOT A CANVAS, and there was no choice. CanvasWidget has no text
 *  primitive, so degree numbers and cardinal letters cannot be strokes; and every CanvasWidget in
 *  this repo is declared as a child of a MapWidget, so a bare HUD-level canvas has no precedent to
 *  lean on. The pooled-widget shape is proven twice over here - VigridMapMarkers3D and
 *  VigridPartyNametags both SetPos a pool under a full-screen frame root every frame.
 *
 *  TWO POOLS, AND THEY ARE NOT THE SAME KIND. The entry pool is indexed BY BEARING: entry i is
 *  permanently the i*15-degree mark, so its tick height and its label are set once at creation and
 *  only position, alpha and visibility move per frame. That is what keeps a stringtable lookup out
 *  of the render loop. The caret pool is an ordinary slot pool, because its contents change as
 *  people move, ping and die.
 *
 *  TICKED EVERY FRAME, deliberately unlike the minimap. The minimap gates itself to 10 Hz on the
 *  argument that at 200 px across nobody can see the difference - true there, false here. A strip
 *  sliding continuously under a fixed cursor is exactly where a 10 Hz update reads as stutter.
 *
 *  Visibility is the AND of three switches that are not equivalent, the same shape as the minimap's:
 *  VIGRID_MAP_COMPASS is the BUILD's, declared in Extra/Map/config.cpp and wrapping this whole file;
 *  compass_allowed is the admin's, arriving over VM_Settings; VigridMapPrefs.IsCompassEnabled is the
 *  player's, persisted locally. Each can only opt further out than the one above it. Note the player
 *  default differs from the minimap's - the compass ships ON.
 *
 *  Deliberately NOT behind the define, for the same reasons the minimap's counterparts are not:
 *  VigridMapPrefs.compass_enabled, VigridMapRPC.compass_allowed and VigridMapData.compass_allowed. A
 *  client build flag must not change the wire format or the shape of the server's settings file. The
 *  K keybind stays in Data/Inputs.xml too, since XML cannot be conditional.
 */
#ifdef VIGRID_MAP_COMPASS
class VigridMapCompass
{
    private Widget m_Root;
    private bool m_RootFailed;

    private Widget m_Backdrop;
    private Widget m_Cursor;
    private TextWidget m_Readout;

    //--- Entry pool, indexed by bearing. m_EntryWeights is the per-entry alpha multiplier that sets
    //--- an unlabelled tick back from a labelled one; it is parallel to m_Entries and fixed for the
    //--- life of the pool.
    private ref array<Widget> m_Entries;
    private ref array<float> m_EntryWeights;

    //--- Cached children and per-entry geometry, so ApplyScale can re-apply them without another
    //--- FindAnyWidget sweep or a second pass over the bearing arithmetic.
    private ref array<Widget> m_EntryTicks;
    private ref array<TextWidget> m_EntryLabels;
    private ref array<float> m_EntryTickHeights;

    //--- Caret pool. m_CaretBars caches the child cast so FindAnyWidget stays out of the render loop.
    private ref array<Widget> m_Carets;
    private ref array<Widget> m_CaretBars;

    //--- This frame's caret set. Members rather than locals so the render loop allocates nothing.
    private ref array<float> m_CaretBearings;
    private ref array<float> m_CaretWidths;
    private ref array<float> m_CaretHeights;
    private ref array<int> m_CaretColors;

    private bool m_Shown;

    //--- Last whole degree pushed to the readout, so SetText runs on a change rather than 60 times a
    //--- second. -1 is "nothing pushed yet" and cannot collide with a real bearing.
    private int m_LastReadout;

    /**
     *  Pixels per reference unit. EVERY constant in the VIGRID_MAP_COMPASS_* block is expressed
     *  against a 1920-wide screen and multiplied by this before it reaches SetPos or SetSize.
     *
     *  This is not a nicety. SetPos and SetSize take REAL screen pixels, so a strip authored as a
     *  flat 620 px is 48% of a 1280-wide window and 24% of a 2560-wide one - it shrinks into the
     *  corner as the resolution goes up, while the text does not, because a widget's font size is
     *  scaled by the viewport for you. That mismatch is what collided the bearing readout with the
     *  labels at fullscreen: the lanes stayed 42 px tall while the glyphs in them grew.
     */
    private float m_Scale;

    void VigridMapCompass()
    {
        m_Entries = new array<Widget>();
        m_EntryWeights = new array<float>();
        m_EntryTicks = new array<Widget>();
        m_EntryLabels = new array<TextWidget>();
        m_EntryTickHeights = new array<float>();
        m_Carets = new array<Widget>();
        m_CaretBars = new array<Widget>();
        m_CaretBearings = new array<float>();
        m_CaretWidths = new array<float>();
        m_CaretHeights = new array<float>();
        m_CaretColors = new array<int>();
        m_LastReadout = -1;
    }

    void ~VigridMapCompass()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    /**
     *  Built on the first Update rather than in the constructor, the same shape the minimap, the
     *  world markers and the kill feed all use: the constructor runs inside MissionGameplay.OnInit,
     *  and the only timing proven to work in this mod is after super.OnInit() has returned.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        m_Root = GetGame().GetWorkspace().CreateWidgets(VIGRID_MAP_PREFIX + "GUI/layouts/compass.layout");

        if (!m_Root)
        {
            //--- Latched: retrying every frame would spam the log for the whole session.
            m_RootFailed = true;
            VigridMapLog.Error("Could not create compass.layout - compass disabled");
            return false;
        }

        //--- None of these is fatal: the ticks are the compass, and each of the three is furniture
        //--- around them. VigridMapLog.Error would be, in any case - it routes through Error2, which
        //--- raises a VM exception rather than printing a line.
        m_Backdrop = m_Root.FindAnyWidget("CompassBackdrop");
        if (!m_Backdrop)
            VigridMapLog.Warn("compass.layout has no CompassBackdrop - strip will have no backing");

        m_Cursor = m_Root.FindAnyWidget("CompassCursor");
        if (!m_Cursor)
            VigridMapLog.Warn("compass.layout has no CompassCursor - no reading edge");

        m_Readout = TextWidget.Cast(m_Root.FindAnyWidget("CompassReadout"));
        if (!m_Readout)
            VigridMapLog.Warn("compass.layout has no CompassReadout - bearing readout disabled");

        BuildEntries();

        m_Root.Show(false);
        VigridMapLog.Debug("Compass layout ready (" + m_Entries.Count() + " entries)");
        return true;
    }

    /**
     *  The 24 fixed marks, one per 15 degrees, created once and never rebuilt.
     *
     *  Three weights of mark, and the arithmetic order matters: 45 is tested BEFORE 30, because 90,
     *  180 and 270 satisfy both and must come out as named directions rather than numbers.
     */
    private void BuildEntries()
    {
        for (int i = 0; i < VIGRID_MAP_COMPASS_ENTRY_COUNT; i++)
        {
            Widget entry = GetGame().GetWorkspace().CreateWidgets(VIGRID_MAP_PREFIX + "GUI/layouts/compass_entry.layout", m_Root);
            if (!entry)
            {
                VigridMapLog.Error("Could not create compass_entry.layout - compass ticks incomplete");
                return;
            }

            int bearing = i * VIGRID_MAP_COMPASS_STEP_DEG;

            float weight = VIGRID_MAP_COMPASS_MINOR_ALPHA;
            float tick_height = VIGRID_MAP_COMPASS_TICK_MINOR_H;
            string label_text = "";
            string label_widget = "";

            //--- 90 is tested BEFORE 45, and 45 before 30, because every 90 also satisfies the other
            //--- two - so the widest test has to come first or N/E/S/W would come out at the
            //--- diagonals' size.
            if ((bearing % 90) == 0)
            {
                weight = 1.0;
                tick_height = VIGRID_MAP_COMPASS_TICK_MAJOR_H;
                label_text = CardinalKey(bearing);
                label_widget = "EntryLabelCardinal";
            }
            else if ((bearing % 45) == 0)
            {
                weight = 1.0;
                tick_height = VIGRID_MAP_COMPASS_TICK_MAJOR_H;
                label_text = CardinalKey(bearing);
                label_widget = "EntryLabelOrdinal";
            }
            else if ((bearing % 30) == 0)
            {
                weight = 1.0;
                tick_height = VIGRID_MAP_COMPASS_TICK_MEDIUM_H;
                label_text = Pad3(bearing);
                label_widget = "EntryLabelNumeric";
            }

            //--- ONE of the three label widgets is kept and the other two are hidden for good. The
            //--- size tier IS the widget, because each declares its own font face and a face is the
            //--- only thing that actually sets glyph size - see the header of compass_entry.layout.
            TextWidget label = PickLabel(entry, label_widget);

            //--- Set once. A '#'-prefixed key costs a stringtable lookup, and the label for a given
            //--- bearing can never change.
            if (label)
                label.SetText(label_text);

            entry.Show(false);

            m_Entries.Insert(entry);
            m_EntryWeights.Insert(weight);
            m_EntryTicks.Insert(entry.FindAnyWidget("EntryTick"));
            m_EntryLabels.Insert(label);
            m_EntryTickHeights.Insert(tick_height);
        }
    }

    /**
     *  Show the label widget for this entry's size tier, hide the other two, and return the one kept.
     *
     *  `wanted` is empty for the unlabelled 15-degree ticks, which hides all three and returns NULL.
     */
    private TextWidget PickLabel(Widget entry, string wanted)
    {
        TextWidget kept = NULL;

        for (int i = 0; i < 3; i++)
        {
            string name = "EntryLabelCardinal";
            if (i == 1)
                name = "EntryLabelOrdinal";
            if (i == 2)
                name = "EntryLabelNumeric";

            TextWidget candidate = TextWidget.Cast(entry.FindAnyWidget(name));
            if (!candidate)
                continue;

            if (name == wanted)
            {
                candidate.Show(true);
                kept = candidate;
                continue;
            }

            candidate.Show(false);
        }

        return kept;
    }

    /**
     *  Re-apply every size to the current viewport.
     *
     *  Called from Tick when m_Scale moves, which is on the first frame and then only on a resolution
     *  or window change - so this is not per-frame work.
     *
     *  NONE of the layouts' own numbers is trusted, at any scale. MEASURED: a widget's DECLARED
     *  position and size are scaled by viewport/1920 (0.667 on a 1280-wide client) while SetPos and
     *  SetSize take real screen pixels, so the 48-unit-wide root of compass_entry.layout actually
     *  rendered 32 px and its tick sat at 15.3 rather than the declared 23. Setting all of it from
     *  script makes the declared units irrelevant and the arithmetic exact.
     */
    private void ApplyScale(float scale)
    {
        float entry_w = VIGRID_MAP_COMPASS_ENTRY_W * scale;
        float tick_w = VIGRID_MAP_COMPASS_TICK_W * scale;

        int count = m_Entries.Count();
        for (int i = 0; i < count; i++)
        {
            Widget entry = m_Entries.Get(i);
            entry.SetSize(entry_w, VIGRID_MAP_COMPASS_ENTRY_H * scale);

            Widget tick = m_EntryTicks.Get(i);
            if (tick)
            {
                float tick_height = m_EntryTickHeights.Get(i);
                tick.SetPos((entry_w - tick_w) * 0.5, 0);
                tick.SetSize(tick_w, tick_height * scale);
            }

            //--- Only the BOX is sized here. The glyph size is the widget's declared font face and
            //--- the engine scales that by the viewport itself, which is why this scales the lanes by
            //--- the same viewport/1920 factor - the two then stay in proportion. Calling
            //--- SetTextExactSize would be pointless: it was measured to do nothing at all.
            TextWidget label = m_EntryLabels.Get(i);
            if (label)
            {
                label.SetPos(0, VIGRID_MAP_COMPASS_LABEL_Y * scale);
                label.SetSize(entry_w, VIGRID_MAP_COMPASS_LABEL_H * scale);
            }
        }

        int caret_count = m_Carets.Count();
        for (int j = 0; j < caret_count; j++)
        {
            Widget caret = m_Carets.Get(j);
            caret.SetSize(VIGRID_MAP_COMPASS_CARET_W * scale, VIGRID_MAP_COMPASS_CARET_H * scale);
        }

        VigridMapLog.Debug("Compass scaled to " + scale);
    }

    //! Effective visibility: the admin's switch, the player's switch, and being alive to read it.
    private bool ShouldShow()
    {
        if (!VigridMapRPC.GetInstance().compass_allowed)
            return false;
        if (!VigridMapPrefs.IsCompassEnabled())
            return false;

        //--- Hidden behind any full-screen menu, the addon's own map included. Matches the minimap
        //--- and the world markers rather than naming menu ids, which drifted.
        UIManager ui = GetGame().GetUIManager();
        if (ui && ui.GetMenu())
            return false;

        //--- Note the consequence, shared with the minimap: a spectator has no compass, because
        //--- GetGame().GetPlayer() keeps returning their corpse and IsAlive() is false for it.
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

        Tick();
    }

    private void Tick()
    {
        float heading = Math.NormalizeAngle(GetGame().GetCurrentCameraDirection().VectorToAngles()[0]);

        //--- MEASURED, not assumed. Everything below is placed in the pixels the ROOT reports, which
        //--- is the only space this repo has proven that script-positioned children actually live in.
        //--- Deriving the centre from the layout's declared 620 instead is what put the whole strip
        //--- 95 px off centre - see the header of compass.layout.
        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);
        if (parent_w <= 0)
            return;

        //--- Everything below is authored against a 1920-wide screen and scaled to the real one, so
        //--- the strip occupies the same fraction of the display at any resolution. Re-sizing is
        //--- edge-triggered: this is the first frame, or the window changed.
        float scale = parent_w / VIGRID_MAP_COMPASS_REFERENCE_W;
        if (Math.AbsFloat(scale - m_Scale) > 0.001)
        {
            m_Scale = scale;
            ApplyScale(scale);
        }

        float half_window = VIGRID_MAP_COMPASS_WINDOW_DEG * 0.5;
        float px_per_degree = (VIGRID_MAP_COMPASS_WIDTH * scale) / VIGRID_MAP_COMPASS_WINDOW_DEG;
        float center_x = parent_w * 0.5;

        LayoutFurniture(center_x, scale);

        RenderEntries(heading, half_window, px_per_degree, center_x, scale);
        RenderCarets(heading, half_window, px_per_degree, center_x, scale);
        RenderReadout(heading);
    }

    /**
     *  Backdrop, cursor and readout, placed around the centre.
     *
     *  Done per frame rather than once, because the root's screen size changes when the window is
     *  resized and there is no resize event to hook - the same reason the fullscreen map re-clamps
     *  its zoom every frame. Three SetPos calls is not worth an edge test.
     */
    private void LayoutFurniture(float center_x, float scale)
    {
        if (m_Backdrop)
        {
            float backdrop_w = VIGRID_MAP_COMPASS_WIDTH * scale;
            m_Backdrop.SetSize(backdrop_w, VIGRID_MAP_COMPASS_HEIGHT * scale);
            m_Backdrop.SetPos(center_x - (backdrop_w * 0.5), VIGRID_MAP_COMPASS_TOP * scale);
        }

        if (m_Cursor)
        {
            float cursor_w = VIGRID_MAP_COMPASS_CURSOR_W * scale;
            m_Cursor.SetSize(cursor_w, VIGRID_MAP_COMPASS_CURSOR_H * scale);
            m_Cursor.SetPos(center_x - (cursor_w * 0.5), VIGRID_MAP_COMPASS_TOP * scale);
        }

        if (m_Readout)
        {
            float readout_w = VIGRID_MAP_COMPASS_READOUT_W * scale;
            m_Readout.SetSize(readout_w, VIGRID_MAP_COMPASS_READOUT_H * scale);
            m_Readout.SetPos(center_x - (readout_w * 0.5), (VIGRID_MAP_COMPASS_TOP + VIGRID_MAP_COMPASS_HEIGHT + VIGRID_MAP_COMPASS_READOUT_GAP) * scale);
        }
    }

    private void RenderEntries(float heading, float half_window, float px_per_degree, float center_x, float scale)
    {
        float half_entry = VIGRID_MAP_COMPASS_ENTRY_W * scale * 0.5;
        float entry_y = VIGRID_MAP_COMPASS_TOP * scale;

        int count = m_Entries.Count();
        for (int i = 0; i < count; i++)
        {
            float bearing = i * VIGRID_MAP_COMPASS_STEP_DEG;
            float delta = SignedDelta(bearing, heading);
            float fade = EdgeFade(delta, half_window);

            Widget entry = m_Entries.Get(i);

            if (fade <= 0)
            {
                entry.Show(false);
                continue;
            }

            //--- SetPos anchors by the top-left corner, so half the entry's width comes off to put
            //--- the TICK - which is centred inside it - on the bearing.
            entry.Show(true);
            entry.SetPos(center_x + (delta * px_per_degree) - half_entry, entry_y);

            float weight = m_EntryWeights.Get(i);
            entry.SetAlpha(fade * weight);
        }
    }

    private void RenderCarets(float heading, float half_window, float px_per_degree, float center_x, float scale)
    {
        float caret_w = VIGRID_MAP_COMPASS_CARET_W * scale;
        float caret_y = (VIGRID_MAP_COMPASS_TOP + VIGRID_MAP_COMPASS_CARET_LANE_Y) * scale;

        CollectCarets();

        int wanted = m_CaretBearings.Count();
        if (wanted > VIGRID_MAP_COMPASS_MAX_CARETS)
            wanted = VIGRID_MAP_COMPASS_MAX_CARETS;

        EnsureCaretCapacity(wanted);

        int drawn = 0;
        for (int i = 0; i < wanted; i++)
        {
            if (drawn >= m_Carets.Count())
                break;

            float bearing = m_CaretBearings.Get(i);
            float delta = SignedDelta(bearing, heading);
            float fade = EdgeFade(delta, half_window);
            if (fade <= 0)
                continue;

            float bar_width = m_CaretWidths.Get(i);
            float bar_height = m_CaretHeights.Get(i);
            int bar_color = m_CaretColors.Get(i);

            Widget caret = m_Carets.Get(drawn);
            caret.Show(true);
            caret.SetPos(center_x + (delta * px_per_degree) - (caret_w * 0.5), caret_y);

            //--- The root carries the edge fade, the bar carries the owner's colour. They cannot be
            //--- the same widget: a widget's colour alpha and its widget alpha are one value, so
            //--- SetColor and SetAlpha on one panel would overwrite each other.
            caret.SetAlpha(fade);

            Widget bar = m_CaretBars.Get(drawn);
            if (bar)
            {
                float scaled_bar_w = bar_width * scale;
                bar.SetSize(scaled_bar_w, bar_height * scale);
                bar.SetPos((caret_w - scaled_bar_w) * 0.5, 0);
                bar.SetColor(bar_color);
            }

            drawn = drawn + 1;
        }

        int pool = m_Carets.Count();
        for (int j = drawn; j < pool; j++)
        {
            m_Carets.Get(j).Show(false);
        }
    }

    /**
     *  This frame's carets, in draw priority order: the next zone first, then teammates, then pings.
     *  The order is what MAX_CARETS truncates against, so the zone can never be crowded out by a
     *  party spamming markers.
     *
     *  Every Party read goes through VigridMapTeam, whose bodies are this addon's only
     *  #ifdef VIGRID_PARTY code. With party.pbo absent all of them answer empty and the strip simply
     *  carries the zone caret.
     */
    private void CollectCarets()
    {
        m_CaretBearings.Clear();
        m_CaretWidths.Clear();
        m_CaretHeights.Clear();
        m_CaretColors.Clear();

        //--- The camera, not the player: in third person the two differ by a few metres, and it is
        //--- the camera the strip is calibrated to.
        vector eye = GetGame().GetCurrentCameraPosition();

        if (VigridMapAPI.HasNextZone())
        {
            vector zone_center = VigridMapAPI.GetNextCenter();
            AddCaret(eye, zone_center, VIGRID_MAP_COMPASS_CARET_ZONE_W, VIGRID_MAP_COMPASS_CARET_FULL_H, VIGRID_MAP_COLOR_ZONE_LINE);
        }

        if (VigridMapTeam.IsAvailable())
        {
            float team_alpha = 1.0;
            if (VigridMapTeam.IsStale())
                team_alpha = VIGRID_MAP_TEAM_STALE_ALPHA;

            int self_index = VigridMapTeam.GetSelfIndex();
            int member_count = VigridMapTeam.GetCount();

            for (int i = 0; i < member_count; i++)
            {
                if (i == self_index)
                    continue;
                if (!VigridMapTeam.IsSlotVisible(i))
                    continue;

                //--- vector.Zero is the documented "no data" answer, not a real position at the map
                //--- origin - a caret for it would sit at a fixed bearing and never move.
                vector member_pos = VigridMapTeam.GetSlotPos(i);
                if (member_pos == vector.Zero)
                    continue;

                int member_color = VigridMapTeam.GetSlotColor(i, team_alpha);
                AddCaret(eye, member_pos, VIGRID_MAP_COMPASS_CARET_TEAM_W, VIGRID_MAP_COMPASS_CARET_FULL_H, member_color);
            }
        }

        int ping_count = VigridMapTeam.GetPingCount();
        for (int j = 0; j < ping_count; j++)
        {
            vector ping_pos = VigridMapTeam.GetPingPos(j);
            if (ping_pos == vector.Zero)
                continue;

            //--- Half height and 0.75 alpha. A ping and a teammate necessarily share the owner's slot
            //--- colour, so those two are the only things telling them apart - the same pair of axes
            //--- separating the map's diamond from its triangle. Do not collapse either without
            //--- giving the ping a different silhouette.
            int ping_color = VigridMapTeam.GetPingColor(j, VIGRID_MAP_PING_ALPHA);
            AddCaret(eye, ping_pos, VIGRID_MAP_COMPASS_CARET_TEAM_W, VIGRID_MAP_COMPASS_CARET_PING_H, ping_color);
        }
    }

    /**
     *  Bearing of a world point from the camera, flattened to the horizontal plane.
     *
     *  Same form as VigridMapRender.EdgeClampedPos, and flattening y is not optional: without it a
     *  target far above or below reports a yaw pulled towards the vertical, so a teammate at the
     *  bottom of a hill would slide along the strip as the camera pitched.
     */
    private void AddCaret(vector eye, vector target, float width, float height, int color)
    {
        vector to_target = vector.Direction(eye, target);
        to_target[1] = 0;

        float bearing = Math.NormalizeAngle(to_target.VectorToAngles()[0]);

        m_CaretBearings.Insert(bearing);
        m_CaretWidths.Insert(width);
        m_CaretHeights.Insert(height);
        m_CaretColors.Insert(color);
    }

    private void EnsureCaretCapacity(int wanted)
    {
        if (!m_Root)
            return;

        while (m_Carets.Count() < wanted)
        {
            Widget caret = GetGame().GetWorkspace().CreateWidgets(VIGRID_MAP_PREFIX + "GUI/layouts/compass_caret.layout", m_Root);
            if (!caret)
                return;

            //--- Explicit for the same reason as the entry pool: declared units are viewport-scaled,
            //--- SetSize is real pixels, and the centring arithmetic assumes the latter. Uses the
            //--- current scale rather than waiting for the next ApplyScale, since a caret can be
            //--- created at any time and ApplyScale only fires when the viewport changes.
            caret.SetSize(VIGRID_MAP_COMPASS_CARET_W * m_Scale, VIGRID_MAP_COMPASS_CARET_H * m_Scale);

            m_Carets.Insert(caret);
            m_CaretBars.Insert(caret.FindAnyWidget("CaretBar"));
        }
    }

    private void RenderReadout(float heading)
    {
        if (!m_Readout)
            return;

        int degrees = Math.Round(heading);

        //--- 360 and 0 are the same bearing, and NormalizeAngle can round up into the former.
        if (degrees >= 360)
            degrees = 0;

        if (degrees == m_LastReadout)
            return;

        m_LastReadout = degrees;
        m_Readout.SetText(Pad3(degrees) + "°");
    }

    /**
     *  Signed offset of `bearing` from `heading`, in the range -180..180.
     *
     *  Math.NormalizeAngle returns 0..360 (enmath.c:144), which is the wrong half of the job: due
     *  north while facing 350 must read +10, not +370. Math.DiffAngle looks like the ready-made
     *  answer and is not taken, because it has ZERO call sites anywhere in P:\scripts - the same
     *  shape as OverrideAimChangeX/Y, which compiled, ran, and silently did nothing.
     */
    private float SignedDelta(float bearing, float heading)
    {
        float delta = Math.NormalizeAngle(bearing - heading);
        if (delta > 180.0)
            delta = delta - 360.0;

        return delta;
    }

    /**
     *  Opacity for something `delta` degrees off centre: 1 across the middle of the window, ramping
     *  to 0 at its edge.
     *
     *  THIS IS WHAT KEEPS THE STRIP INSIDE ITS BACKDROP, not the container's clipchildren. Clipping
     *  of absolutely-positioned children is unproven on this codebase, so the fade is the mechanism
     *  and the clip is belt and braces - if the clip turns out to do nothing, nothing looks wrong.
     */
    private float EdgeFade(float delta, float half_window)
    {
        float distance = Math.AbsFloat(delta);
        if (distance >= half_window)
            return 0;

        float fade_start = half_window - VIGRID_MAP_COMPASS_FADE_DEG;
        if (distance <= fade_start)
            return 1.0;

        return (half_window - distance) / VIGRID_MAP_COMPASS_FADE_DEG;
    }

    //! Stringtable key for a named direction, held with its leading '#' so the widget localises it.
    private string CardinalKey(int bearing)
    {
        if (bearing == 0)
            return STR_VIGRID_MAP_COMPASS_N;
        if (bearing == 45)
            return STR_VIGRID_MAP_COMPASS_NE;
        if (bearing == 90)
            return STR_VIGRID_MAP_COMPASS_E;
        if (bearing == 135)
            return STR_VIGRID_MAP_COMPASS_SE;
        if (bearing == 180)
            return STR_VIGRID_MAP_COMPASS_S;
        if (bearing == 225)
            return STR_VIGRID_MAP_COMPASS_SW;
        if (bearing == 270)
            return STR_VIGRID_MAP_COMPASS_W;
        if (bearing == 315)
            return STR_VIGRID_MAP_COMPASS_NW;

        return "";
    }

    /**
     *  Three digits, zero padded - "007", "047", "312".
     *
     *  Fixed width on purpose: an unpadded readout changes length as you turn, and a centred label
     *  that changes length jitters sideways under a cursor that does not.
     */
    private string Pad3(int degrees)
    {
        string text = degrees.ToString();

        if (degrees < 10)
            return "00" + text;
        if (degrees < 100)
            return "0" + text;

        return text;
    }
}
#endif
#endif
