#ifndef SERVER
/**
 *  Vigrid Party - world markers ("pings").
 *
 *  Same pooled-widget shape as VigridPartyNametags, and the same geometry through VigridPartyScreen,
 *  but the two cannot sensibly be one class: the slot source is the ping array rather than the
 *  roster, and the world position is fixed rather than read from a live entity and interpolated.
 *  The pure geometry functions are shared; the rest is not.
 *
 *  A marker carries no owner name - just a coloured icon and a distance. Who placed it is read from
 *  the colour, which is keyed to their party slot, because a marker is a glance-at-it hint and a
 *  second line of text is a thing to read.
 *
 *  Markers fade near the crosshair like name tags do, so one never sits on top of what the player
 *  is aiming at. The floor (VIGRID_PARTY_PING_CENTER_MIN_ALPHA) is higher than the tags' because a
 *  marker is placed by looking straight at it, and it should dim rather than disappear at the
 *  moment it appears.
 */
class VigridPartyPings
{
    private Widget m_Root;
    private bool m_RootFailed;
    private ref array<Widget> m_Markers;
    private ref array<ImageWidget> m_MarkerIcons;
    private ref array<TextWidget> m_MarkerDistances;

    void VigridPartyPings()
    {
        m_Markers = new array<Widget>();
        m_MarkerIcons = new array<ImageWidget>();
        m_MarkerDistances = new array<TextWidget>();
    }

    void ~VigridPartyPings()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    /**
     *  Created on the first Update, not in the constructor - which runs inside
     *  MissionGameplay.OnInit, before the only timing this mod has ever built widgets at
     *  successfully (after super.OnInit() has returned).
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        VigridPartyLog.Debug("Creating party ping layout");
        m_Root = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_pings.layout");

        if (!m_Root)
        {
            //--- Latch it: retrying every frame would spam the log for the whole session.
            m_RootFailed = true;
            VigridPartyLog.Error("Could not create party_pings.layout - pings disabled");
            return false;
        }

        m_Root.Show(false);
        VigridPartyLog.Debug("Party ping layout ready");
        return true;
    }

    private void EnsureCapacity(int wanted)
    {
        if (!m_Root)
            return;

        while (m_Markers.Count() < wanted)
        {
            Widget marker = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_ping.layout", m_Root);
            if (!marker)
                return;

            m_Markers.Insert(marker);
            m_MarkerIcons.Insert(ImageWidget.Cast(marker.FindAnyWidget("PingIcon")));
            m_MarkerDistances.Insert(TextWidget.Cast(marker.FindAnyWidget("PingDistance")));
        }
    }

    private void HideAll()
    {
        int count = m_Markers.Count();
        for (int i = 0; i < count; i++)
        {
            m_Markers.Get(i).Show(false);
        }

        if (m_Root)
            m_Root.Show(false);
    }

    void Update()
    {
        if (!EnsureRoot())
            return;

        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (!rpc.enabled || !rpc.ping_enabled || !rpc.HasParty())
        {
            HideAll();
            return;
        }

        //--- The receiver only ever fills the three arrays together, but they are indexed with one
        //--- counter below, so the shortest of them is what this loop trusts.
        int ping_count = rpc.ping_owner_uids.Count();
        if (rpc.ping_positions.Count() < ping_count)
            ping_count = rpc.ping_positions.Count();
        if (rpc.ping_expire_ms.Count() < ping_count)
            ping_count = rpc.ping_expire_ms.Count();
        if (ping_count > VIGRID_PARTY_PING_MAX_RENDERED)
            ping_count = VIGRID_PARTY_PING_MAX_RENDERED;

        if (ping_count == 0)
        {
            HideAll();
            return;
        }

        EnsureCapacity(ping_count);

        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);

        //--- Measured from the player rather than the camera, so the metres on a marker match the
        //--- metres on a name tag. In third person the camera sits a couple of metres back.
        vector self_pos = GetGame().GetCurrentCameraPosition();
        PlayerBase local_player = PlayerBase.Cast(GetGame().GetPlayer());
        if (local_player)
            self_pos = local_player.GetPosition();

        int now_ms = GetGame().GetTime();

        m_Root.Show(true);

        int slot = 0;
        for (int i = 0; i < ping_count; i++)
        {
            if (slot >= m_Markers.Count())
                break;

            //--- The server sweep is authoritative but only runs at 1 Hz, so expiry is honoured
            //--- locally to the frame and the sweep tidies up behind it. Without this a marker
            //--- lingers for up to a second past the lifetime the admin configured.
            int expire_ms = rpc.ping_expire_ms.Get(i);
            if (expire_ms > 0 && now_ms >= expire_ms)
            {
                m_Markers.Get(slot).Show(false);
                slot = slot + 1;
                continue;
            }

            //--- Roster position of whoever placed it; -1 when they are not on the roster yet.
            RenderSlot(slot, rpc.ping_positions.Get(i), rpc.roster_uids.Find(rpc.ping_owner_uids.Get(i)), parent_w, parent_h, self_pos);
            slot = slot + 1;
        }

        //--- Any pooled widget beyond the current set stays hidden.
        for (int j = slot; j < m_Markers.Count(); j++)
        {
            m_Markers.Get(j).Show(false);
        }
    }

    /**
     *  Colour for the member in party slot `owner_slot`, at opacity `alpha` (0..1).
     *
     *  The values moved to VigridPartyPalette (3_Game) once the map addon needed them too - a
     *  4_World API cannot reach a 5_Mission class. This stays as a delegate rather than having the
     *  two call sites below reach across directly, so the diff that moved them is provably
     *  behaviour-neutral inside this file.
     */
    private int ColourForSlot(int owner_slot, float alpha)
    {
        return VigridPartyPalette.ColourForSlot(owner_slot, alpha);
    }

    private void RenderSlot(int slot, vector world_pos, int owner_slot, float parent_w, float parent_h, vector self_pos)
    {
        Widget marker = m_Markers.Get(slot);

        if (world_pos == vector.Zero)
        {
            marker.Show(false);
            return;
        }

        float distance = vector.Distance(self_pos, world_pos);

        //--- Shown before being measured: a widget that has never been displayed can report a zero
        //--- size, and the offsets below are derived from that size.
        marker.Show(true);

        float marker_w;
        float marker_h;
        marker.GetScreenSize(marker_w, marker_h);
        if (marker_w <= 0)
            marker_w = VIGRID_PARTY_PING_SIZE_W;
        if (marker_h <= 0)
            marker_h = VIGRID_PARTY_PING_SIZE_H;

        vector screen_pos = GetGame().GetScreenPosRelative(world_pos + Vector(0, VIGRID_PARTY_PING_HEIGHT_OFFSET, 0));

        float px;
        float py;
        float center_factor = 1.0;

        if (VigridPartyScreen.IsOnScreen(screen_pos))
        {
            float anchor_x = screen_pos[0] * parent_w;
            float anchor_y = screen_pos[1] * parent_h;

            //--- SetPos anchors by the top-left corner, so the marker is shifted by its own size to
            //--- sit above the pinged point rather than hanging below and to the right of it.
            px = anchor_x - (marker_w * 0.5);
            py = anchor_y - marker_h;

            //--- Faded near the crosshair, exactly like a name tag: a marker is most likely to be
            //--- in the way precisely when the player is aiming through it. It never reaches zero,
            //--- so the marker you just placed is dimmed rather than lost.
            center_factor = VigridPartyScreen.CrosshairFade(anchor_x, anchor_y, parent_w, parent_h, VIGRID_PARTY_PING_CENTER_HIDE, VIGRID_PARTY_PING_CENTER_FADE, VIGRID_PARTY_PING_CENTER_MIN_ALPHA);
        }
        else
        {
            vector clamped = VigridPartyScreen.EdgeClampedPos(world_pos, parent_w, parent_h, marker_w, marker_h, VIGRID_PARTY_PING_EDGE_MARGIN);
            px = clamped[0];
            py = clamped[1];
        }

        marker.SetPos(px, py);

        //--- Never full opacity, then faded again with distance so a marker across the map does not
        //--- compete with one in the room, and once more by proximity to the crosshair.
        float alpha = Math.Lerp(1.0, VIGRID_PARTY_PING_MIN_ALPHA, Math.Clamp(distance / VIGRID_PARTY_PING_FADE_DISTANCE, 0, 1));
        float final_alpha = VIGRID_PARTY_PING_BASE_ALPHA * alpha * center_factor;

        //--- Drives the distance text, which inherits it, along with its outline and shadow - the
        //--- icon does NOT inherit it and is faded through its own colour below.
        marker.SetAlpha(final_alpha);

        //--- Nearer markers draw on top of farther ones.
        marker.SetSort(Math.Round(10000 - Math.Clamp(distance, 0, 9999)));

        //--- The distance is tinted too, not just the icon. With no owner name on the marker the
        //--- colour is the only thing saying who placed it, so it needs more than 14 px to read.
        //--- Full opacity here on purpose: the text inherits the widget alpha set above, so baking
        //--- it in again would fade it twice as fast as the icon.
        m_MarkerIcons.Get(slot).SetColor(ColourForSlot(owner_slot, final_alpha));
        m_MarkerDistances.Get(slot).SetColor(ColourForSlot(owner_slot, 1.0));
        m_MarkerDistances.Get(slot).SetText(VigridPartyScreen.FormatDistance(distance));
    }
}
#endif
