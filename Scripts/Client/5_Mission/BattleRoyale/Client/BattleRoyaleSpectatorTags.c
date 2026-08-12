#ifndef SERVER
/**
 *  Vigrid Battle Royale - floating player tags for the admin spectator.
 *
 *  A pure renderer, like BattleRoyaleSpeakingList: everything it draws arrives from the server on
 *  the SetAdminPlayerList RPC and is parked on BattleRoyaleRPC. Nothing is computed here beyond
 *  screen geometry.
 *
 *  WHY THE POSITIONS COME FROM THE SERVER AND NOT FROM ClientData.m_PlayerBaseList. The network
 *  bubble stays on the connection's own entity - measured, see BattleRoyaleSpectatorCamera - so a
 *  client-side enumeration only ever sees players within ~1 km of the admin's body. The server push
 *  has no such limit, which is why a tag appears for a player on the far side of the map even
 *  though their character does not render there. That asymmetry is deliberate and is the whole
 *  reason for the RPC: names and health work everywhere, models only where the engine replicates
 *  them. A live entity IS preferred when there is one, because it is per-frame rather than 2 Hz.
 *
 *  The projection, the pooling and the edge clamp are all VigridPartyNametags' (Party/Scripts/
 *  5_Mission/Client/), reimplemented here rather than called: Party must not depend on this mod and
 *  this mod must not require Party to be present.
 */
class BattleRoyaleSpectatorTags
{
    protected Widget m_Root;
    protected bool m_RootFailed;

    protected ref array<Widget> m_Tags;
    protected ref array<TextWidget> m_TagNames;
    protected ref array<TextWidget> m_TagInfos;
    protected ref array<Widget> m_TagHealthBacks;
    protected ref array<Widget> m_TagHealthFills;

    void BattleRoyaleSpectatorTags()
    {
        m_Tags = new array<Widget>();
        m_TagNames = new array<TextWidget>();
        m_TagInfos = new array<TextWidget>();
        m_TagHealthBacks = new array<Widget>();
        m_TagHealthFills = new array<Widget>();
        m_RootFailed = false;
    }

    void ~BattleRoyaleSpectatorTags()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    /**
     *  Created on the first Update, not in the constructor - the same timing rule the party name
     *  tags follow. The constructor runs inside MissionGameplay.OnInit, and this mod only ever
     *  builds widgets after super.OnInit() has returned.
     */
    protected bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        //--- Literal path, matching every other layout load in this mod (LeaderboardMenu.c:56 and
        //--- the rest). Unlike the Extra/ addons this one has no PREFIX constant - it is the host
        //--- mod, so there is nothing to keep portable.
        m_Root = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/spectator_tags.layout");

        if (!m_Root)
        {
            //--- Latched: retrying every frame would spam the log for the whole session.
            //---
            //--- WARN, NOT ERROR - corrected after the fact. BattleRoyaleUtils.Error raises a VM
            //--- exception and unwinds the stack, and this is reached from BattleRoyaleClient.Update,
            //--- so a missing layout would have taken the client's whole per-frame update down every
            //--- frame instead of costing one overlay. Same correction as BattleRoyaleLobbyTags.
            m_RootFailed = true;
            BattleRoyaleUtils.Warn("[Spectate] Could not create spectator_tags.layout - admin tags disabled");
            return false;
        }

        m_Root.Show(false);
        BattleRoyaleUtils.Debug("[Spectate] Admin tag layout ready");
        return true;
    }

    protected void EnsureCapacity(int wanted)
    {
        if (!m_Root)
            return;

        while (m_Tags.Count() < wanted)
        {
            Widget tag = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/spectator_tag.layout", m_Root);
            if (!tag)
                return;

            m_Tags.Insert(tag);
            m_TagNames.Insert(TextWidget.Cast(tag.FindAnyWidget("TagName")));
            m_TagInfos.Insert(TextWidget.Cast(tag.FindAnyWidget("TagInfo")));
            m_TagHealthBacks.Insert(tag.FindAnyWidget("TagHealthBack"));
            m_TagHealthFills.Insert(tag.FindAnyWidget("TagHealthFill"));
        }
    }

    protected void HideAll()
    {
        int count = m_Tags.Count();
        for (int i = 0; i < count; i++)
        {
            m_Tags.Get(i).Show(false);
        }

        if (m_Root)
            m_Root.Show(false);
    }

    /**
     *  Called every frame from BattleRoyaleClient.UpdateSpectate while an admin is spectating.
     *
     *  `self_pos` is the camera, not a body: distances on this overlay are "how far from what I am
     *  looking at", which is the only meaningful origin for a flying camera.
     */
    void Update(bool active, vector self_pos, string highlight_uid)
    {
        if (!EnsureRoot())
            return;

        if (!active)
        {
            HideAll();
            return;
        }

        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if (!br_rpc)
        {
            HideAll();
            return;
        }

        int count = br_rpc.admin_uids.Count();
        if (count == 0)
        {
            HideAll();
            return;
        }

        EnsureCapacity(count);
        m_Root.Show(true);

        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);
        if (parent_w <= 0 || parent_h <= 0)
            return;

        //--- Interpolation fraction between the last two server pushes, so a tag on a player outside
        //--- the network bubble glides instead of stepping twice a second. Computed once for the
        //--- whole frame: every row shares the same pair of timestamps.
        float lerp_t = 1.0;
        bool can_lerp = false;
        if (br_rpc.admin_prev_positions.Count() == count && br_rpc.admin_prev_recv_ms > 0)
        {
            float span = br_rpc.admin_recv_ms - br_rpc.admin_prev_recv_ms;
            if (span > 0)
            {
                can_lerp = true;
                lerp_t = Math.Clamp((GetGame().GetTime() - br_rpc.admin_recv_ms) / span, 0, 1);
            }
        }

        int rendered = 0;
        for (int i = 0; i < count; i++)
        {
            if (i >= m_Tags.Count())
                break;

            RenderSlot(i, br_rpc, self_pos, highlight_uid, parent_w, parent_h, can_lerp, lerp_t);
            rendered = i + 1;
        }

        //--- Surplus pooled widgets from a larger earlier roster. Hidden rather than unlinked: these
        //--- are free-positioned in a plain panel, not laid out by a spacer, so a hidden one occupies
        //--- nothing. (The Unlink rule applies under a WrapSpacer, which this is not.)
        for (int j = rendered; j < m_Tags.Count(); j++)
        {
            m_Tags.Get(j).Show(false);
        }
    }

    protected void RenderSlot(int slot, BattleRoyaleRPC br_rpc, vector self_pos, string highlight_uid, float parent_w, float parent_h, bool can_lerp, float lerp_t)
    {
        Widget tag = m_Tags.Get(slot);
        if (!tag)
            return;

        string uid = br_rpc.admin_uids.Get(slot);

        vector body_pos = ResolveBodyPos(br_rpc, slot, uid, can_lerp, lerp_t);
        if (body_pos == vector.Zero)
        {
            tag.Show(false);
            return;
        }

        //--- Measured with vector.Distance, never from the projection's z. That component is the
        //--- depth along the view axis, not the camera distance the engine comment claims
        //--- (game.c:966) - it shrinks as the target moves off screen centre, so a readout taken
        //--- from it changes when the camera turns and nobody has moved.
        float distance = vector.Distance(self_pos, body_pos);

        vector screen_pos = GetGame().GetScreenPosRelative(ResolveAnchorPos(uid, body_pos));
        bool on_screen = IsOnScreen(screen_pos);

        //--- Shown before being measured: a widget that has never been displayed reports zero size,
        //--- and every offset below is derived from that size.
        tag.Show(true);

        float tag_w;
        float tag_h;
        tag.GetScreenSize(tag_w, tag_h);
        if (tag_w <= 0)
            tag_w = BR_SPECTATE_TAG_SIZE_W;
        if (tag_h <= 0)
            tag_h = BR_SPECTATE_TAG_SIZE_H;

        float px;
        float py;

        if (on_screen)
        {
            //--- SetPos anchors by the top-left corner, so without these offsets the tag hangs a full
            //--- width right and a full height below the character.
            px = (screen_pos[0] * parent_w) - (tag_w * 0.5);
            py = (screen_pos[1] * parent_h) - tag_h - BR_SPECTATE_TAG_GAP_PX;
        }
        else
        {
            //--- Bearing from world-space yaw rather than a clamped projection: GetScreenPosRelative
            //--- mirrors x/y behind the camera, so clamping it puts the tag on the wrong side.
            vector clamped = EdgeClampedPos(body_pos, parent_w, parent_h, tag_w, tag_h);
            px = clamped[0];
            py = clamped[1];
        }

        tag.SetPos(px, py);

        //--- No crosshair fade here, unlike the party tags. An admin has no weapon and nothing to
        //--- shoot past; hiding the tag they are looking straight at would defeat the overlay.
        float alpha = 1.0;
        if (!on_screen)
            alpha = BR_SPECTATE_TAG_OFFSCREEN_ALPHA;

        tag.SetAlpha(alpha);

        //--- Nearer tags draw over farther ones.
        tag.SetSort(Math.Round(10000 - Math.Clamp(distance, 0, 9999)));

        ApplyRow(slot, br_rpc, uid, distance, highlight_uid);
    }

    //! Text, health bar and colour for one row.
    protected void ApplyRow(int slot, BattleRoyaleRPC br_rpc, string uid, float distance, string highlight_uid)
    {
        TextWidget name_widget = m_TagNames.Get(slot);
        if (name_widget)
        {
            name_widget.SetText(br_rpc.admin_names.Get(slot));

            //--- The player the camera is following, marked so cycling has visible feedback even in
            //--- FREE mode where the camera is not pointed at them.
            if (uid == highlight_uid && highlight_uid != "")
                name_widget.SetColor(BR_SPECTATE_TAG_TARGET_COLOUR);
            else
                name_widget.SetColor(SlotColour(br_rpc.admin_slots.Get(slot)));
        }

        TextWidget info_widget = m_TagInfos.Get(slot);
        if (info_widget)
        {
            string info = FormatDistance(distance);

            int kills = br_rpc.admin_kills.Get(slot);
            if (kills > 0)
                info = info + "  " + kills.ToString() + "K";

            info_widget.SetText(info);
        }

        //--- The bar is a fill inside a dark backing, so it has to be resized rather than tinted -
        //--- 1 1 is full health and 0 1 is none. Sized in the parent's own normalised units, which
        //--- is what the layout declares it in.
        Widget fill = m_TagHealthFills.Get(slot);
        if (fill)
        {
            float health = Math.Clamp(br_rpc.admin_healths.Get(slot), 0, 1);
            fill.SetSize(health, 1.0);
            fill.SetColor(HealthColour(health));
        }
    }

    /**
     *  Where a player actually is, this frame.
     *
     *  A live entity wins when there is one: it is per-frame and exact, where the server push is
     *  2 Hz. Outside the network bubble there is no entity at all - which for a flying admin is most
     *  of the map - and the interpolated push is all there is. Same two-source shape as
     *  VigridPartyAPI.ResolveBodyPos, for the same reason.
     */
    protected vector ResolveBodyPos(BattleRoyaleRPC br_rpc, int slot, string uid, bool can_lerp, float lerp_t)
    {
        PlayerBase entity = FindLocalPlayer(uid);
        if (entity)
            return entity.GetPosition();

        if (slot >= br_rpc.admin_positions.Count())
            return vector.Zero;

        vector current = br_rpc.admin_positions.Get(slot);
        if (!can_lerp)
            return current;

        vector previous = br_rpc.admin_prev_positions.Get(slot);

        return vector.Lerp(previous, current, lerp_t);
    }

    //! Head height when the entity is present, an estimate when it is not.
    protected vector ResolveAnchorPos(string uid, vector body_pos)
    {
        PlayerBase entity = FindLocalPlayer(uid);
        if (entity)
        {
            //--- GetBoneIndexByName, not GetBoneIndex - the latter resolves a proxy SELECTION, which
            //--- is a different thing entirely and returns nonsense here.
            int head = entity.GetBoneIndexByName("Head");
            if (head != -1)
                return entity.GetBonePositionWS(head) + Vector(0, BR_SPECTATE_TAG_HEAD_OFFSET, 0);
        }

        return body_pos + Vector(0, BR_SPECTATE_TAG_HEIGHT_OFFSET, 0);
    }

    //! The entity for a uid inside the local network bubble, or NULL. NULL is normal, not an error.
    protected PlayerBase FindLocalPlayer(string uid)
    {
        if (uid == "")
            return NULL;
        if (!ClientData.m_PlayerBaseList)
            return NULL;

        int count = ClientData.m_PlayerBaseList.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = PlayerBase.Cast(ClientData.m_PlayerBaseList.Get(i));
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;
            if (candidate.GetIdentity().GetPlainId() != uid)
                continue;

            return candidate;
        }

        return NULL;
    }

    //--------------------------------------------------------------------------------------------
    //--- Screen geometry. Reimplemented from VigridPartyScreen rather than called, so that neither
    //--- addon depends on the other - the same rule that made the Map addon carry its own copy.
    //--------------------------------------------------------------------------------------------

    //! z is depth along the view axis, not distance. Its sign is the one thing it is good for.
    protected bool IsOnScreen(vector screen_pos)
    {
        if (screen_pos[2] <= 0)
            return false;
        if (screen_pos[0] < 0 || screen_pos[0] > 1)
            return false;
        if (screen_pos[1] < 0 || screen_pos[1] > 1)
            return false;

        return true;
    }

    //! Top-left position for a widget standing in for something off-screen, clamped to an ellipse.
    protected vector EdgeClampedPos(vector world_pos, float parent_w, float parent_h, float w, float h)
    {
        vector camera_pos = GetGame().GetCurrentCameraPosition();
        vector to_target = vector.Direction(camera_pos, world_pos);
        to_target[1] = 0;

        float camera_yaw = GetGame().GetCurrentCameraDirection().VectorToAngles()[0];
        float target_yaw = to_target.VectorToAngles()[0];
        float angle = Math.NormalizeAngle(target_yaw - camera_yaw);

        float radians = angle * Math.DEG2RAD;
        float rx = (parent_w * 0.5) - BR_SPECTATE_TAG_EDGE_MARGIN;
        float ry = (parent_h * 0.5) - BR_SPECTATE_TAG_EDGE_MARGIN;

        float px = (parent_w * 0.5) + (rx * Math.Sin(radians)) - (w * 0.5);
        float py = (parent_h * 0.5) - (ry * Math.Cos(radians)) - (h * 0.5);

        return Vector(px, py, 0);
    }

    //! "123m" below a kilometre, "1.4km" above. Math.Round returns a float, so the tenths are
    //! carried as an int and the point inserted by hand.
    protected string FormatDistance(float metres)
    {
        if (metres < 1000)
            return Math.Round(metres).ToString() + "m";

        int tenths = Math.Round(metres / 100.0);
        int whole = tenths / 10;
        int fraction = tenths - (whole * 10);

        return whole.ToString() + "." + fraction.ToString() + "km";
    }

    //! Party colour for a slot, or plain white when solo / without the addon.
    protected int SlotColour(int slot)
    {
        if (slot < 0)
            return BR_SPECTATE_TAG_SOLO_COLOUR;

#ifdef VIGRID_PARTY
        return VigridPartyAPI.GetColourForSlot(slot, 1.0);
#else
        return BR_SPECTATE_TAG_SOLO_COLOUR;
#endif
    }

    //! Green through amber to red. A ramp rather than tiers, so a bar that is visibly moving reads
    //! as damage being taken rather than as a state change.
    protected int HealthColour(float health)
    {
        int red = 255;
        int green = 255;

        if (health > 0.5)
            red = Math.Round(Math.Lerp(255, 90, (health - 0.5) * 2));
        else
            green = Math.Round(Math.Lerp(60, 255, health * 2));

        return ARGB(255, red, green, 60);
    }
}
#endif
