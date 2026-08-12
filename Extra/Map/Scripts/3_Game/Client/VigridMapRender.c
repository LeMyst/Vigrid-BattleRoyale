#ifndef SERVER
/**
 *  Vigrid Map - drawing and screen-space geometry. Pure static functions, no state.
 *
 *  Lives in 3_Game because MapWidget and CanvasWidget are 3_Game classes and both consumers - the
 *  fullscreen menu and the minimap, each in 5_Mission - need every function here. It references
 *  nothing above 3_Game.
 *
 *  Two things a CanvasWidget cannot do, which shape everything below:
 *
 *    - it has no circle primitive, so a zone ring is a fan of DrawLine chords;
 *    - it has no fill primitive, so a filled rectangle is a stack of DrawLine strokes.
 *
 *  MapToScreen returns ABSOLUTE screen pixels while DrawLine takes canvas-local ones, so every
 *  world point is converted through WorldToCanvas, which subtracts the map widget's own screen
 *  position. Getting that wrong offsets the entire drawing by the map's position on screen, which
 *  looks like a projection bug and is not one.
 */
class VigridMapRender
{
    /**
     *  World position -> canvas-local pixels for a canvas that covers the map widget.
     *
     *  The subtraction is against the MAP widget, not the canvas, because the canvas is a child
     *  filling the map exactly. If the canvas is ever inset from its parent this becomes wrong and
     *  should subtract the canvas's own screen position instead.
     */
    static void WorldToCanvas(MapWidget map_widget, vector world_pos, out float cx, out float cy)
    {
        cx = 0;
        cy = 0;
        if (!map_widget)
            return;

        float screen_x;
        float screen_y;
        map_widget.GetScreenPos(screen_x, screen_y);

        vector projected = map_widget.MapToScreen(world_pos);

        cx = projected[0] - screen_x;
        cy = projected[1] - screen_y;
    }

    /**
     *  Pixels-per-metre along each axis, probed from the widget rather than derived from GetScale.
     *
     *  There is no pan/zoom script event on MapWidget, so the transform has to be sampled. Two
     *  points fully describe it because the map is north-up and unrotated. Callers keep the two
     *  probe results from the previous frame and compare: a change means the player panned or
     *  zoomed and the canvas needs repainting. That makes one pair of native calls serve as both
     *  the projection basis and the dirty check.
     */
    static void ProbeTransform(MapWidget map_widget, out vector probe_origin, out vector probe_far, out float px_per_m_x, out float px_per_m_z)
    {
        probe_origin = vector.Zero;
        probe_far = vector.Zero;
        px_per_m_x = 0;
        px_per_m_z = 0;
        if (!map_widget)
            return;

        probe_origin = map_widget.MapToScreen(Vector(0.0, 0.0, 0.0));
        probe_far = map_widget.MapToScreen(Vector(VIGRID_MAP_PROBE_DISTANCE, 0.0, VIGRID_MAP_PROBE_DISTANCE));

        px_per_m_x = (probe_far[0] - probe_origin[0]) / VIGRID_MAP_PROBE_DISTANCE;
        px_per_m_z = (probe_far[1] - probe_origin[1]) / VIGRID_MAP_PROBE_DISTANCE;
    }

    /**
     *  A world-space circle, drawn as a fan of chords onto a canvas covering the map.
     *
     *  The radius is measured in pixels by projecting two edge points rather than by scaling the
     *  world radius, so it stays correct whatever the map's scale is doing.
     *
     *  Segment count is n = PI*sqrt(r), which holds the sagitta - how far a chord strays from the
     *  true arc - at about half a pixel for any radius. That makes it a visual no-op against a
     *  fixed count while costing far less on small circles: a 400 px zone ring gets 63 segments, a
     *  10 px dot gets the 16 floor.
     */
    static void WorldRenderOval(CanvasWidget canvas, MapWidget map_widget, vector center, float radius_x, float radius_z, int color, float width)
    {
        if (!canvas || !map_widget)
            return;
        if (radius_x <= 0 || radius_z <= 0)
            return;

        float cx;
        float cy;
        WorldToCanvas(map_widget, Vector(center[0], 0, center[2]), cx, cy);

        float edge_x_cx;
        float edge_x_cy;
        WorldToCanvas(map_widget, Vector(center[0] + radius_x, 0, center[2]), edge_x_cx, edge_x_cy);

        float edge_z_cx;
        float edge_z_cy;
        WorldToCanvas(map_widget, Vector(center[0], 0, center[2] + radius_z), edge_z_cx, edge_z_cy);

        float pixel_radius_x = Math.Sqrt(((edge_x_cx - cx) * (edge_x_cx - cx)) + ((edge_x_cy - cy) * (edge_x_cy - cy)));
        float pixel_radius_y = Math.Sqrt(((edge_z_cx - cx) * (edge_z_cx - cx)) + ((edge_z_cy - cy) * (edge_z_cy - cy)));

        float max_radius = Math.Max(pixel_radius_x, pixel_radius_y);
        if (max_radius < 1)
            max_radius = 1;

        int segments = Math.Round(Math.PI * Math.Sqrt(max_radius));
        if (segments < VIGRID_MAP_OVAL_MIN_SEGMENTS)
            segments = VIGRID_MAP_OVAL_MIN_SEGMENTS;
        if (segments > VIGRID_MAP_OVAL_MAX_SEGMENTS)
            segments = VIGRID_MAP_OVAL_MAX_SEGMENTS;

        float angle_increment = 360.0 / segments;

        for (int i = 0; i < segments; i++)
        {
            float angle1 = i * angle_increment;
            float angle2 = (i + 1) * angle_increment;

            float x1 = cx + (pixel_radius_x * Math.Cos(angle1 * Math.DEG2RAD));
            float y1 = cy + (pixel_radius_y * Math.Sin(angle1 * Math.DEG2RAD));
            float x2 = cx + (pixel_radius_x * Math.Cos(angle2 * Math.DEG2RAD));
            float y2 = cy + (pixel_radius_y * Math.Sin(angle2 * Math.DEG2RAD));

            canvas.DrawLine(x1, y1, x2, y2, width, color);
        }
    }

    /**
     *  A filled rectangle, as a stack of horizontal strokes.
     *
     *  The band height is derived from the stroke width rather than advancing by one pixel per
     *  line: stepping by 1 while drawing a wider stroke redraws the same pixels many times over,
     *  and stepping by 1 with a 1 px stroke leaves hairline gaps when the rectangle's height is not
     *  an integer.
     */
    static void RenderFilledRect(CanvasWidget canvas, float x1, float y1, float x2, float y2, int color)
    {
        if (!canvas)
            return;

        float top = Math.Min(y1, y2);
        float bottom = Math.Max(y1, y2);
        float height = bottom - top;
        if (height <= 0)
            return;

        float stroke = 2.0;
        int bands = Math.Ceil(height / stroke);
        if (bands < 1)
            bands = 1;

        for (int i = 0; i < bands; i++)
        {
            float y = top + (i * stroke) + (stroke * 0.5);
            if (y > bottom)
                y = bottom;

            canvas.DrawLine(x1, y, x2, y, stroke, color);
        }
    }

    /**
     *  A dot centred on a world position - what marks a zone centre.
     */
    static void WorldRenderDot(CanvasWidget canvas, MapWidget map_widget, vector world_pos, float size_px, int color)
    {
        if (!canvas || !map_widget)
            return;

        float cx;
        float cy;
        WorldToCanvas(map_widget, Vector(world_pos[0], 0, world_pos[2]), cx, cy);

        float half = size_px * 0.5;
        RenderFilledRect(canvas, cx - half, cy - half, cx + half, cy + half, color);
    }

    //--- The three live-position glyphs. See VigridMapConstants for why these particular shapes:
    //--- the circle is already spent on zone rings and on the ring-and-cross marker, so a teammate
    //--- and a ping have to be straight-edged and have to differ from each other in more than one
    //--- way at once.
    //---
    //--- All three are sized in screen pixels and drawn with plain arithmetic after a single
    //--- projection, so they cost one MapToScreen each however far the map is zoomed out. None of
    //--- them culls: the MapWidget carries `clipchildren 1`, and the worst case across a full party
    //--- plus its pings is under a hundred strokes. That is deliberately unlike WorldRenderDashedLine
    //--- below, where clipping is load-bearing because one unclipped line can be 50 000 px long.

    /**
     *  A hollow triangle, apex up - a teammate.
     *
     *  Centred on the world point rather than sitting above it: this marks where somebody IS, so the
     *  centroid is the honest anchor. Drawn from an inscribed circle so the apparent size matches the
     *  diamond and the marker ring at the same size_px.
     */
    static void WorldRenderTriangle(CanvasWidget canvas, MapWidget map_widget, vector world_pos, float size_px, int color, float width)
    {
        if (!canvas || !map_widget)
            return;

        float cx;
        float cy;
        WorldToCanvas(map_widget, Vector(world_pos[0], 0, world_pos[2]), cx, cy);

        float r = size_px * 0.5;

        //--- Apex straight up, the other two at +/-120 degrees. Screen y grows downward, so the apex
        //--- takes the negative offset.
        float apex_x = cx;
        float apex_y = cy - r;

        float left_x = cx - (r * 0.8660254);   // cos(30)
        float left_y = cy + (r * 0.5);         // sin(30)

        float right_x = cx + (r * 0.8660254);
        float right_y = left_y;

        canvas.DrawLine(apex_x, apex_y, right_x, right_y, width, color);
        canvas.DrawLine(right_x, right_y, left_x, left_y, width, color);
        canvas.DrawLine(left_x, left_y, apex_x, apex_y, width, color);
    }

    /**
     *  A diamond - a party ping. Four strokes, one more vertex than the teammate triangle, and drawn
     *  lighter and more transparent so a transient callout never outweighs a live player.
     */
    static void WorldRenderDiamond(CanvasWidget canvas, MapWidget map_widget, vector world_pos, float size_px, int color, float width)
    {
        if (!canvas || !map_widget)
            return;

        float cx;
        float cy;
        WorldToCanvas(map_widget, Vector(world_pos[0], 0, world_pos[2]), cx, cy);

        float r = size_px * 0.5;

        canvas.DrawLine(cx, cy - r, cx + r, cy, width, color);
        canvas.DrawLine(cx + r, cy, cx, cy + r, width, color);
        canvas.DrawLine(cx, cy + r, cx - r, cy, width, color);
        canvas.DrawLine(cx - r, cy, cx, cy - r, width, color);
    }

    /**
     *  A heading dart - the local player on the MINIMAP, pointing where the camera looks.
     *
     *  Four strokes: tip, two swept-back corners, and a notch in the tail. The notch is the whole
     *  point. A plain isoceles triangle is genuinely ambiguous at this size - both ends read as a
     *  possible point, which is exactly the complaint that produced this glyph - whereas a concave
     *  tail can only be read one way round. It is the same reason every GPS cursor is a dart rather
     *  than a triangle.
     *
     *  Drawn rather than rotated as an image on purpose. The ImageWidget route needs the icon's rest
     *  angle to be known (`icon_arrow` points DOWN, which cost two wrong attempts) and vanilla's own
     *  arrow texture will not resolve from a mod PBO at all. Here the vertices come straight from the
     *  heading, so there is no hidden constant to get wrong and no asset to fail to load.
     *
     *  `heading_deg` is a compass bearing: 0 = north, increasing clockwise. Screen y grows downward,
     *  hence sin for x and MINUS cos for y.
     *
     *  The fullscreen map deliberately does NOT use this - see WorldRenderCross below.
     */
    static void WorldRenderHeadingArrow(CanvasWidget canvas, MapWidget map_widget, vector world_pos, float heading_deg, float size_px, int color, float width)
    {
        if (!canvas || !map_widget)
            return;

        float cx;
        float cy;
        WorldToCanvas(map_widget, Vector(world_pos[0], 0, world_pos[2]), cx, cy);

        float r = size_px * 0.5;

        //--- Corners swept back 145 degrees from the tip: wide enough to read as an arrowhead, narrow
        //--- enough that the tip stays the obvious extreme.
        float rad_tip = heading_deg * Math.DEG2RAD;
        float rad_left = (heading_deg + 145.0) * Math.DEG2RAD;
        float rad_right = (heading_deg - 145.0) * Math.DEG2RAD;
        float rad_tail = (heading_deg + 180.0) * Math.DEG2RAD;

        float tip_x = cx + (r * Math.Sin(rad_tip));
        float tip_y = cy - (r * Math.Cos(rad_tip));

        float left_x = cx + (r * Math.Sin(rad_left));
        float left_y = cy - (r * Math.Cos(rad_left));

        float right_x = cx + (r * Math.Sin(rad_right));
        float right_y = cy - (r * Math.Cos(rad_right));

        //--- The notch sits well inside the corners; too shallow and it reads as a straight back.
        float tail_x = cx + (r * 0.45 * Math.Sin(rad_tail));
        float tail_y = cy - (r * 0.45 * Math.Cos(rad_tail));

        canvas.DrawLine(tip_x, tip_y, right_x, right_y, width, color);
        canvas.DrawLine(right_x, right_y, tail_x, tail_y, width, color);
        canvas.DrawLine(tail_x, tail_y, left_x, left_y, width, color);
        canvas.DrawLine(left_x, left_y, tip_x, tip_y, width, color);
    }

    /**
     *  An axis-aligned plus - the local player.
     *
     *  Two strokes and no enclosed area, which is what makes it unmistakable against every other
     *  glyph on the map at any size. Not rotated to heading: the fullscreen map is north-up and a
     *  rotating "you" is harder to find than a fixed one.
     */
    static void WorldRenderCross(CanvasWidget canvas, MapWidget map_widget, vector world_pos, float size_px, int color, float width)
    {
        if (!canvas || !map_widget)
            return;

        float cx;
        float cy;
        WorldToCanvas(map_widget, Vector(world_pos[0], 0, world_pos[2]), cx, cy);

        float r = size_px * 0.5;

        canvas.DrawLine(cx - r, cy, cx + r, cy, width, color);
        canvas.DrawLine(cx, cy - r, cx, cy + r, width, color);
    }

    /**
     *  Liang-Barsky: clip the segment (x1,y1)-(x2,y2) to an axis-aligned rectangle, returning the
     *  surviving parameter range along the segment. False means the segment misses the rectangle
     *  entirely.
     *
     *  Written with four explicit edge tests rather than a loop over an array of coefficients: the
     *  loop form needs a per-call allocation or a shared scratch array, and neither is worth it for
     *  four iterations.
     */
    static bool ClipSegment(float x1, float y1, float x2, float y2, float min_x, float min_y, float max_x, float max_y, out float t_start, out float t_end)
    {
        t_start = 0.0;
        t_end = 1.0;

        float dx = x2 - x1;
        float dy = y2 - y1;

        float p_val;
        float q_val;
        float r_val;

        //--- Left edge.
        p_val = -dx;
        q_val = x1 - min_x;
        if (p_val == 0)
        {
            if (q_val < 0)
                return false;
        }
        else
        {
            r_val = q_val / p_val;
            if (p_val < 0)
            {
                if (r_val > t_end)
                    return false;
                if (r_val > t_start)
                    t_start = r_val;
            }
            else
            {
                if (r_val < t_start)
                    return false;
                if (r_val < t_end)
                    t_end = r_val;
            }
        }

        //--- Right edge.
        p_val = dx;
        q_val = max_x - x1;
        if (p_val == 0)
        {
            if (q_val < 0)
                return false;
        }
        else
        {
            r_val = q_val / p_val;
            if (p_val < 0)
            {
                if (r_val > t_end)
                    return false;
                if (r_val > t_start)
                    t_start = r_val;
            }
            else
            {
                if (r_val < t_start)
                    return false;
                if (r_val < t_end)
                    t_end = r_val;
            }
        }

        //--- Top edge.
        p_val = -dy;
        q_val = y1 - min_y;
        if (p_val == 0)
        {
            if (q_val < 0)
                return false;
        }
        else
        {
            r_val = q_val / p_val;
            if (p_val < 0)
            {
                if (r_val > t_end)
                    return false;
                if (r_val > t_start)
                    t_start = r_val;
            }
            else
            {
                if (r_val < t_start)
                    return false;
                if (r_val < t_end)
                    t_end = r_val;
            }
        }

        //--- Bottom edge.
        p_val = dy;
        q_val = max_y - y1;
        if (p_val == 0)
        {
            if (q_val < 0)
                return false;
        }
        else
        {
            r_val = q_val / p_val;
            if (p_val < 0)
            {
                if (r_val > t_end)
                    return false;
                if (r_val > t_start)
                    t_start = r_val;
            }
            else
            {
                if (r_val < t_start)
                    return false;
                if (r_val < t_end)
                    t_end = r_val;
            }
        }

        return true;
    }

    /**
     *  A dashed line between two world positions.
     *
     *  Clipped to the canvas BEFORE being dashed, and this is not an optimisation detail - it is
     *  what makes the feature viable. At maximum zoom-in a 5 km line projects to roughly 50 000
     *  pixels, which at a 20 px period is 2 500 DrawLine calls per repaint for a line that is
     *  almost entirely off-screen. Clipping first also keeps the dash period constant at every
     *  zoom, which capping the segment count would not: a cap would silently stretch the dashes.
     */
    static void WorldRenderDashedLine(CanvasWidget canvas, MapWidget map_widget, vector from_pos, vector to_pos, float canvas_w, float canvas_h, int color)
    {
        if (!canvas || !map_widget)
            return;
        if (canvas_w <= 0 || canvas_h <= 0)
            return;

        float x1;
        float y1;
        WorldToCanvas(map_widget, Vector(from_pos[0], 0, from_pos[2]), x1, y1);

        float x2;
        float y2;
        WorldToCanvas(map_widget, Vector(to_pos[0], 0, to_pos[2]), x2, y2);

        float t_start;
        float t_end;
        if (!ClipSegment(x1, y1, x2, y2, 0, 0, canvas_w, canvas_h, t_start, t_end))
            return;

        float full_dx = x2 - x1;
        float full_dy = y2 - y1;

        float clipped_x1 = x1 + (full_dx * t_start);
        float clipped_y1 = y1 + (full_dy * t_start);
        float clipped_x2 = x1 + (full_dx * t_end);
        float clipped_y2 = y1 + (full_dy * t_end);

        float dx = clipped_x2 - clipped_x1;
        float dy = clipped_y2 - clipped_y1;
        float length = Math.Sqrt((dx * dx) + (dy * dy));
        if (length <= 0)
            return;

        float ux = dx / length;
        float uy = dy / length;

        float period = VIGRID_MAP_DASH_ON_PX + VIGRID_MAP_DASH_OFF_PX;
        if (period <= 0)
            return;

        int dashes = Math.Ceil(length / period);
        if (dashes > VIGRID_MAP_DASH_MAX_SEGMENTS)
            dashes = VIGRID_MAP_DASH_MAX_SEGMENTS;

        for (int i = 0; i < dashes; i++)
        {
            float a = i * period;
            float b = a + VIGRID_MAP_DASH_ON_PX;
            if (b > length)
                b = length;
            if (b <= a)
                break;

            canvas.DrawLine(clipped_x1 + (ux * a), clipped_y1 + (uy * a), clipped_x1 + (ux * b), clipped_y1 + (uy * b), VIGRID_MAP_DASH_WIDTH, color);
        }
    }

    /**
     *  Is a projected point in front of the camera and inside the viewport?
     *
     *  z is the depth along the view axis, NOT the camera distance the engine comment claims
     *  (game.c:966) - it shrinks as the target moves away from screen centre. Testing its sign is
     *  the one thing it is actually good for.
     *
     *  Copied from VigridPartyScreen.IsOnScreen rather than called: this addon must build with
     *  Party disabled.
     */
    static bool IsOnScreen(vector screen_pos)
    {
        if (screen_pos[2] <= 0)
            return false;
        if (screen_pos[0] < 0 || screen_pos[0] > 1)
            return false;
        if (screen_pos[1] < 0 || screen_pos[1] > 1)
            return false;

        return true;
    }

    /**
     *  Top-left position for a widget of size (w, h) standing in for something off-screen, clamped
     *  to an ellipse inset from the edges by `margin`.
     *
     *  This deliberately does NOT reuse the projection: GetScreenPosRelative mirrors x/y for
     *  anything behind the camera, so a clamped projected point would sit on the wrong side of the
     *  screen. The bearing is computed from world-space yaw instead.
     *
     *  Copied from VigridPartyScreen.EdgeClampedPos.
     */
    static vector EdgeClampedPos(vector world_pos, float parent_w, float parent_h, float w, float h, float margin)
    {
        vector camera_pos = GetGame().GetCurrentCameraPosition();
        vector to_target = vector.Direction(camera_pos, world_pos);
        to_target[1] = 0;

        float camera_yaw = GetGame().GetCurrentCameraDirection().VectorToAngles()[0];
        float target_yaw = to_target.VectorToAngles()[0];
        float angle = Math.NormalizeAngle(target_yaw - camera_yaw);

        float radians = angle * Math.DEG2RAD;
        float rx = (parent_w * 0.5) - margin;
        float ry = (parent_h * 0.5) - margin;

        float px = (parent_w * 0.5) + (rx * Math.Sin(radians)) - (w * 0.5);
        float py = (parent_h * 0.5) - (ry * Math.Cos(radians)) - (h * 0.5);

        return Vector(px, py, 0);
    }

    /**
     *  Opacity multiplier for a widget sitting near the crosshair, so it cannot hide an enemy
     *  standing between the player and whatever it marks.
     *
     *  Worked in pixels rather than normalised coordinates: on a 16:9 screen the normalised form
     *  would give an elliptical dead zone, wider than it is tall.
     *
     *  Copied from VigridPartyScreen.CrosshairFade, and called with the same fractions, so a map
     *  marker and a party ping fade on the same curve.
     */
    static float CrosshairFade(float anchor_x, float anchor_y, float parent_w, float parent_h, float hide_fraction, float fade_fraction, float min_alpha)
    {
        float dx = anchor_x - (parent_w * 0.5);
        float dy = anchor_y - (parent_h * 0.5);
        float from_center = Math.Sqrt((dx * dx) + (dy * dy));

        float hide_radius = parent_h * hide_fraction;
        float fade_radius = parent_h * fade_fraction;
        if (fade_radius <= hide_radius)
            return 1.0;

        float ramp = Math.Clamp((from_center - hide_radius) / (fade_radius - hide_radius), 0, 1);

        return Math.Lerp(min_alpha, 1.0, ramp);
    }

    /**
     *  "123m" below a kilometre, "1.4km" above it.
     *
     *  Math.Round returns a float (enmath.c:413), so the tenths are carried as an int and the
     *  decimal point is inserted by hand rather than trusting a float-to-string conversion to pick
     *  a sensible number of digits.
     *
     *  Copied from VigridPartyScreen.FormatDistance.
     */
    static string FormatDistance(float metres)
    {
        if (metres < VIGRID_MAP_KM_THRESHOLD)
            return Math.Round(metres).ToString() + "m";

        int tenths = Math.Round(metres / 100.0);
        int whole = tenths / 10;
        int fraction = tenths - (whole * 10);

        return whole.ToString() + "." + fraction.ToString() + "km";
    }
}
#endif
