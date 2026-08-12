#ifndef SERVER
/**
 *  Vigrid Map - the floating on-screen markers.
 *
 *  Structurally the same layer as the party name tags and pings, and for the same reasons, but it
 *  is a separate layer drawing a separate thing. A party ping is a transient callout - three per
 *  player, thirty seconds, thrown where you are looking. A map marker is a standing plan - one per
 *  player, permanent, placed by clicking the map. Both can be on screen at once, so they must not
 *  look alike: this one keeps a solid backdrop and does not fade out with distance the way a ping
 *  does, because a plan you cannot see is not a plan.
 *
 *  Three projection traps are handled here, all of them already paid for in the party addon:
 *
 *    - GetScreenPosRelative's z is depth along the view axis, NOT camera distance, so distance is
 *      measured separately with vector.Distance;
 *    - behind the camera that projection is mirrored, so the off-screen case computes a bearing
 *      from world-space yaw instead of clamping the projected point;
 *    - SetPos anchors a widget by its top-left corner, so half its size has to come off.
 */
class VigridMapMarkers3D
{
    private Widget m_Root;
    private bool m_RootFailed;
    private ref array<Widget> m_Tags;
    private ref array<TextWidget> m_TagDistances;
    private ref array<ImageWidget> m_TagIcons;

    void VigridMapMarkers3D()
    {
        m_Tags = new array<Widget>();
        m_TagDistances = new array<TextWidget>();
        m_TagIcons = new array<ImageWidget>();
    }

    void ~VigridMapMarkers3D()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    /**
     *  Created on the first Update, not in the constructor. The constructor runs inside
     *  MissionGameplay.OnInit, and the only timing proven to work in this mod is building widgets
     *  after super.OnInit() has returned. Deferring to the first frame costs one boolean test.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        m_Root = GetGame().GetWorkspace().CreateWidgets(VIGRID_MAP_PREFIX + "GUI/layouts/map_markers_3d.layout");

        if (!m_Root)
        {
            //--- Latched: retrying every frame would spam the log for the whole session.
            m_RootFailed = true;
            VigridMapLog.Error("Could not create map_markers_3d.layout - world markers disabled");
            return false;
        }

        m_Root.Show(false);
        VigridMapLog.Debug("World marker layer ready");
        return true;
    }

    private void EnsureCapacity(int wanted)
    {
        if (!m_Root)
            return;

        while (m_Tags.Count() < wanted)
        {
            Widget tag = GetGame().GetWorkspace().CreateWidgets(VIGRID_MAP_PREFIX + "GUI/layouts/map_marker_3d.layout", m_Root);
            if (!tag)
                return;

            m_Tags.Insert(tag);
            m_TagDistances.Insert(TextWidget.Cast(tag.FindAnyWidget("MarkerDistance")));
            m_TagIcons.Insert(ImageWidget.Cast(tag.FindAnyWidget("MarkerIcon")));
        }
    }

    private void HideAll()
    {
        int count = m_Tags.Count();
        for (int i = 0; i < count; i++)
        {
            m_Tags.Get(i).Show(false);
        }

        if (m_Root)
            m_Root.Show(false);
    }

    void Update(float timeslice)
    {
        if (!EnsureRoot())
            return;

        VigridMapClient client = GetClient();
        if (!client || client.GetDrawCount() == 0)
        {
            HideAll();
            return;
        }

        //--- Hidden behind any full-screen menu, including this addon's own map: a marker drawn
        //--- over the map screen would sit on top of the very thing it is meant to annotate.
        if (IsAnyMenuOpen())
        {
            HideAll();
            return;
        }

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.IsAlive())
        {
            HideAll();
            return;
        }

        int wanted = client.GetDrawCount();
        EnsureCapacity(wanted);

        m_Root.Show(true);

        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);

        int drawn = 0;
        for (int i = 0; i < wanted; i++)
        {
            if (drawn >= m_Tags.Count())
                break;

            //--- Party slot colour, so the floating tag and the pin on the map are the same colour
            //--- for the same person.
            int color = VigridMapTeam.GetColorForSlot(client.GetDrawSlot(i), 1.0);
            if (client.IsDrawOwn(i))
                color = VIGRID_MAP_COLOR_OWN_MARKER;

            RenderSlot(drawn, client.GetDrawPos(i), player.GetPosition(), parent_w, parent_h, color);
            drawn = drawn + 1;
        }

        for (int j = drawn; j < m_Tags.Count(); j++)
        {
            m_Tags.Get(j).Show(false);
        }
    }

    private bool IsAnyMenuOpen()
    {
        UIManager ui = GetGame().GetUIManager();
        if (!ui)
            return false;

        return ui.GetMenu() != NULL;
    }

    private VigridMapClient GetClient()
    {
        MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
        if (!mission)
            return NULL;

        return mission.GetVigridMapClient();
    }

    private void RenderSlot(int slot, vector marker_pos, vector player_pos, float parent_w, float parent_h, int color)
    {
        Widget tag = m_Tags.Get(slot);

        //--- A marker is stored flat, at y = 0, because it is placed by clicking a map and the map
        //--- projection has no elevation. That is correct for the map and wrong for the world: at
        //--- 224 m altitude the marker sat 224 m underground, and the distance readout was inflated
        //--- by the same amount. So the height is resolved against the terrain here, at the point
        //--- of drawing, rather than being guessed at placement time - the ground under a map click
        //--- is not necessarily streamed in when the click happens.
        vector ground_pos = marker_pos;
        ground_pos[1] = GetGame().SurfaceY(marker_pos[0], marker_pos[2]);

        //--- Measured from the player rather than read off the projection: GetScreenPosRelative's
        //--- z shrinks as the target moves away from screen centre, so using it would make the
        //--- readout change when the camera turned with nobody moving.
        float distance = vector.Distance(player_pos, ground_pos);

        vector anchor_world = ground_pos + Vector(0, VIGRID_MAP_MARKER_HEIGHT_OFFSET, 0);
        vector screen_pos = GetGame().GetScreenPosRelative(anchor_world);

        bool on_screen = VigridMapRender.IsOnScreen(screen_pos);

        //--- Shown before being measured: a widget that has never been displayed reports a zero
        //--- size, and the centring offsets below are derived from that size.
        tag.Show(true);

        float tag_w;
        float tag_h;
        tag.GetScreenSize(tag_w, tag_h);
        if (tag_w <= 0)
            tag_w = VIGRID_MAP_MARKER_SIZE_W;
        if (tag_h <= 0)
            tag_h = VIGRID_MAP_MARKER_SIZE_H;

        float px;
        float py;
        float center_factor = 1.0;

        if (on_screen)
        {
            float anchor_x = screen_pos[0] * parent_w;
            float anchor_y = screen_pos[1] * parent_h;

            px = anchor_x - (tag_w * 0.5);
            py = anchor_y - tag_h;

            center_factor = VigridMapRender.CrosshairFade(anchor_x, anchor_y, parent_w, parent_h, VIGRID_MAP_MARKER_CENTER_HIDE, VIGRID_MAP_MARKER_CENTER_FADE, VIGRID_MAP_MARKER_CENTER_MIN_ALPHA);
        }
        else
        {
            //--- Ground position, not the flat one: the bearing is taken from world-space yaw, and
            //--- a point 200 m underground gives a different one once the camera pitches.
            vector clamped = VigridMapRender.EdgeClampedPos(ground_pos, parent_w, parent_h, tag_w, tag_h, VIGRID_MAP_MARKER_EDGE_MARGIN);
            px = clamped[0];
            py = clamped[1];
        }

        tag.SetPos(px, py);

        //--- Floors well above zero on purpose. A ping is allowed to fade away with distance
        //--- because another one is a keypress away; a marker is the plan, so it stays legible
        //--- across the map.
        float alpha = Math.Lerp(1.0, VIGRID_MAP_MARKER_MIN_ALPHA, Math.Clamp(distance / VIGRID_MAP_MARKER_FADE_DISTANCE, 0, 1));

        tag.SetAlpha(alpha * center_factor);

        ImageWidget icon = m_TagIcons.Get(slot);
        if (icon)
            icon.SetColor(color);

        TextWidget text = m_TagDistances.Get(slot);
        if (text)
            text.SetText(VigridMapRender.FormatDistance(distance));
    }
}
#endif
