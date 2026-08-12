#ifndef SERVER
/**
 *  Vigrid Party - floating teammate name tags.
 *
 *  Position comes from one of two sources depending on distance:
 *
 *    - Teammate inside the network bubble: read the live entity's bones, exactly as vanilla
 *      IngameHud.ShowPlayerTag does. Perfectly smooth, no interpolation needed.
 *    - Teammate outside it: there is no entity at all client-side, so fall back to the position the
 *      server pushes on VP_TeamState and interpolate between the last two samples.
 *
 *  Two distinct world positions are tracked and must not be confused:
 *
 *    - the *body* position, at ground level, which is what distance is measured to;
 *    - the *anchor* position, at head height, which is what gets projected to the screen.
 *
 *  Off-screen teammates are clamped to the screen edge by VigridPartyScreen.EdgeClampedPos, which
 *  the ping renderer shares - see there for why that path cannot just clamp the projection.
 *
 *  Widgets are pooled: created once, then shown/hidden and rebound. Nothing is allocated per frame.
 */
class VigridPartyNametags
{
    private Widget m_Root;
    private bool m_RootFailed;
    private ref array<Widget> m_Tags;
    private ref array<TextWidget> m_TagNames;
    private ref array<TextWidget> m_TagDistances;

    void VigridPartyNametags()
    {
        m_Tags = new array<Widget>();
        m_TagNames = new array<TextWidget>();
        m_TagDistances = new array<TextWidget>();
    }

    /**
     *  Created on the first Update, not in the constructor. The constructor runs inside
     *  MissionGameplay.OnInit; the Battle Royale mod only ever builds widgets after super.OnInit()
     *  has returned, and that is the only timing proven to work here.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        VigridPartyLog.Debug("Creating party nametag layout");
        m_Root = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_nametags.layout");

        if (!m_Root)
        {
            //--- Latch it: retrying every frame would spam the log for the whole session.
            m_RootFailed = true;
            VigridPartyLog.Error("Could not create party_nametags.layout - name tags disabled");
            return false;
        }

        m_Root.Show(false);
        VigridPartyLog.Debug("Party nametag layout ready");
        return true;
    }

    void ~VigridPartyNametags()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    private void EnsureCapacity(int wanted)
    {
        if (!m_Root)
            return;

        while (m_Tags.Count() < wanted)
        {
            Widget tag = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_nametag.layout", m_Root);
            if (!tag)
                return;

            m_Tags.Insert(tag);
            m_TagNames.Insert(TextWidget.Cast(tag.FindAnyWidget("TagName")));
            m_TagDistances.Insert(TextWidget.Cast(tag.FindAnyWidget("TagDistance")));
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

        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (!rpc.enabled || !rpc.HasParty())
        {
            HideAll();
            return;
        }

        int member_count = rpc.roster_uids.Count();

        //--- One widget per member, minus ourselves.
        EnsureCapacity(member_count);

        //--- Anything older than this is treated as unreliable and dimmed, then hidden.
        //---
        //--- These two expressions, and the flag test in the loop below, duplicate what
        //--- VigridPartyAPI.IsStateStale() and IsMemberVisible() now compute. That is deliberate and
        //--- not an oversight: position resolution moved to the API because two consumers must not
        //--- disagree about where a teammate is, whereas this is presentation for one renderer.
        //--- Keeping it here is what made the move a two-identifier diff instead of a rewrite of the
        //--- visibility logic.
        int now_ms = GetGame().GetTime();
        int age_ms = now_ms - rpc.state_recv_ms;
        bool have_state = rpc.state_version == rpc.roster_version;
        bool stale = age_ms > (3 * VIGRID_PARTY_DEF_STATE_INTERVAL_MS);

        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);

        //--- Distance is measured from the player, not from the camera, so that the metres shown on
        //--- a tag are the same metres shown on that teammate's HUD row. In third person the camera
        //--- sits a couple of metres back, which would otherwise make the two readouts disagree.
        vector self_pos = GetGame().GetCurrentCameraPosition();
        PlayerBase local_player = PlayerBase.Cast(GetGame().GetPlayer());
        if (local_player)
            self_pos = local_player.GetPosition();

        //--- Same override as the HUD panel, and for the same reason - see VigridPartyClientAPI.
        //--- It matters MORE here than there: this origin does not just print a number, it drives
        //--- nametag_max_distance culling, the alpha fade and the sort order. Measured from a stale
        //--- body, a teammate standing right next to what this client is actually watching gets
        //--- faded out or culled entirely, and the tag simply is not there to question.
        string viewpoint_uid = "";
        if (VigridPartyClientAPI.HasHudViewpoint())
        {
            self_pos = VigridPartyClientAPI.GetHudViewpointPos();
            viewpoint_uid = VigridPartyClientAPI.GetHudViewpointUid();
        }

        m_Root.Show(true);

        int slot = 0;
        for (int i = 0; i < member_count; i++)
        {
            if (i == rpc.self_index)
                continue;
            if (slot >= m_Tags.Count())
                break;

            bool visible = have_state;
            if (age_ms > VIGRID_PARTY_STALE_HIDE_MS)
                visible = false;

            //--- Only draw over teammates who are connected and alive, so a tag never floats over
            //--- a corpse or a logged-out player.
            if (visible && i < rpc.state_flags.Count())
            {
                int member_flags = rpc.state_flags.Get(i);
                //--- Compared against 0 rather than negated: `!` on an int result is not
                //--- something EnfusionScript can be relied on to convert.
                if ((member_flags & VIGRID_PARTY_FLAG_ONLINE) == 0)
                    visible = false;
                if ((member_flags & VIGRID_PARTY_FLAG_ALIVE) == 0)
                    visible = false;
            }

            if (!visible)
            {
                m_Tags.Get(slot).Show(false);
                slot = slot + 1;
                continue;
            }

            PlayerBase entity = VigridPartyAPI.FindLocalPlayer(rpc.roster_uids.Get(i));
            bool is_viewpoint = viewpoint_uid != "" && rpc.roster_uids.Get(i) == viewpoint_uid;
            RenderSlot(slot, entity, VigridPartyAPI.ResolveBodyPos(rpc, i, entity), VigridPartyAPI.GetMemberName(i), parent_w, parent_h, stale, rpc, self_pos, is_viewpoint);
            slot = slot + 1;
        }

        //--- Any pooled widget beyond the current party size stays hidden.
        for (int j = slot; j < m_Tags.Count(); j++)
        {
            m_Tags.Get(j).Show(false);
        }
    }

    /**
     *  Head-height world position - what gets projected to the screen.
     *
     *  GetBoneIndexByName is the call vanilla uses to resolve a bone by name (miscgameplayfunctions.c:719),
     *  as opposed to GetBoneIndex, which resolves a named proxy selection (entity.c:36).
     */
    private vector ResolveAnchorPos(PlayerBase entity, vector body_pos)
    {
        if (entity)
        {
            int head = entity.GetBoneIndexByName("Head");
            if (head != -1)
                return entity.GetBonePositionWS(head) + Vector(0, VIGRID_PARTY_TAG_HEAD_OFFSET, 0);
        }

        return body_pos + Vector(0, VIGRID_PARTY_TAG_HEIGHT_OFFSET, 0);
    }

    /**
     *  Opacity multiplier for a tag sitting near the crosshair, so it cannot hide an enemy standing
     *  between the player and a teammate. Shared with the ping renderer; the floor is deliberately
     *  non-zero so a distant teammate is not lost at the moment the player looks for them.
     */
    private float GetCrosshairFade(float anchor_x, float anchor_y, float parent_w, float parent_h)
    {
        return VigridPartyScreen.CrosshairFade(anchor_x, anchor_y, parent_w, parent_h, VIGRID_PARTY_TAG_CENTER_HIDE, VIGRID_PARTY_TAG_CENTER_FADE, VIGRID_PARTY_TAG_CENTER_MIN_ALPHA);
    }

    private void RenderSlot(int slot, PlayerBase entity, vector body_pos, string name, float parent_w, float parent_h, bool stale, VigridPartyRPC rpc, vector self_pos, bool is_viewpoint)
    {
        Widget tag = m_Tags.Get(slot);

        if (body_pos == vector.Zero)
        {
            tag.Show(false);
            return;
        }

        //--- Measured here rather than read off the projection. GetScreenPosRelative's z component
        //--- is the depth along the view axis, not the camera distance the engine comment claims
        //--- (game.c:966): it shrinks as the target moves away from screen centre, so the readout
        //--- changed when the camera turned even with nobody moving. Vanilla makes the same split -
        //--- ingamehud.c:1066 computes the distance itself and uses z only for the test below.
        float distance = vector.Distance(self_pos, body_pos);

        if (rpc.nametag_max_distance > 0 && distance > rpc.nametag_max_distance)
        {
            tag.Show(false);
            return;
        }

        vector screen_pos = GetGame().GetScreenPosRelative(ResolveAnchorPos(entity, body_pos));
        bool on_screen = VigridPartyScreen.IsOnScreen(screen_pos);

        //--- Shown before being measured: a widget that has never been displayed can report a zero
        //--- size, and the offsets below are derived from that size.
        tag.Show(true);

        float tag_w;
        float tag_h;
        tag.GetScreenSize(tag_w, tag_h);
        if (tag_w <= 0)
            tag_w = VIGRID_PARTY_TAG_SIZE_W;
        if (tag_h <= 0)
            tag_h = VIGRID_PARTY_TAG_SIZE_H;

        float px;
        float py;
        float center_factor = 1.0;

        if (on_screen)
        {
            float anchor_x = screen_pos[0] * parent_w;
            float anchor_y = screen_pos[1] * parent_h;

            //--- SetPos anchors a widget by its top-left corner. Without these offsets the tag hung
            //--- a full width to the right and a full height below the character, which read as
            //--- being stuck to their side once they were far enough away to be small on screen.
            px = anchor_x - (tag_w * 0.5);
            py = anchor_y - tag_h - VIGRID_PARTY_TAG_GAP_PX;

            center_factor = GetCrosshairFade(anchor_x, anchor_y, parent_w, parent_h);
        }
        else
        {
            //--- Bearing from world-space yaw, because the projection is mirrored behind the
            //--- camera and cannot simply be clamped. Shared with the ping renderer.
            vector clamped = VigridPartyScreen.EdgeClampedPos(body_pos, parent_w, parent_h, tag_w, tag_h, VIGRID_PARTY_TAG_EDGE_MARGIN);
            px = clamped[0];
            py = clamped[1];
        }

        tag.SetPos(px, py);

        //--- Fade with distance so nearby teammates read strongest.
        float fade_over = rpc.nametag_max_distance;
        if (fade_over <= 0)
            fade_over = VIGRID_PARTY_TAG_FADE_DISTANCE;

        float alpha = Math.Lerp(1.0, rpc.nametag_min_alpha, Math.Clamp(distance / fade_over, 0, 1));
        if (stale)
            alpha = alpha * 0.5;

        tag.SetAlpha(alpha * center_factor);

        //--- Nearer tags draw on top of farther ones.
        tag.SetSort(Math.Round(10000 - Math.Clamp(distance, 0, 9999)));

        m_TagNames.Get(slot).SetText(name);

        //--- Blank on the viewpoint's own tag, matching the HUD panel: they ARE the origin, so the
        //--- number is zero by construction and "0m" floating over the person being watched reads
        //--- as a broken readout. The name still shows, which is the useful half.
        if (is_viewpoint)
            m_TagDistances.Get(slot).SetText("");
        else
            m_TagDistances.Get(slot).SetText(Math.Round(distance).ToString() + "m");
    }
}
#endif
