#ifndef SERVER
/**
 *  Vigrid Party - shared screen-space geometry. Pure functions, no state.
 *
 *  Both floating-widget layers (name tags and pings) project a world position onto the screen and
 *  clamp it to the edge when it falls outside. That clamp is subtle enough - see EdgeClampedPos -
 *  that two copies of it would drift apart the first time somebody tuned one of them, so it lives
 *  here and both renderers call it.
 */
class VigridPartyScreen
{
    /**
     *  Is a projected point in front of the camera and inside the viewport?
     *
     *  z is the depth along the view axis, NOT the camera distance the engine comment claims
     *  (game.c:966) - it shrinks as the target moves away from screen centre. Testing its sign is
     *  the one thing it is actually good for.
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
     *  Returns the position instead of filling out-parameters: two values would need `out`, which
     *  is a parameter-direction keyword in EnfusionScript and cannot be used freely.
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

        //--- Centred on the clamped point: there is nothing out here to sit above. The widget
        //--- sliding around the edge is what conveys direction; there is no arrow.
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
     *  `min_alpha` floors the result rather than letting it reach zero, so looking straight at
     *  something does not make it vanish entirely.
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
     */
    static string FormatDistance(float metres)
    {
        if (metres < VIGRID_PARTY_PING_KM_THRESHOLD)
            return Math.Round(metres).ToString() + "m";

        int tenths = Math.Round(metres / 100.0);
        int whole = tenths / 10;
        int fraction = tenths - (whole * 10);

        return whole.ToString() + "." + fraction.ToString() + "km";
    }
}
#endif
