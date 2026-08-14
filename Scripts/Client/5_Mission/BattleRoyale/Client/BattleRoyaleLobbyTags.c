#ifndef SERVER
/**
 *  Vigrid Battle Royale - a name over every non-teammate's head while the lobby is running.
 *
 *  WHAT THIS REPLACED. The mod used to re-enable vanilla's own player tag here (IngameHud's
 *  RefreshPlayerTags / ShowPlayerTag, which ship #ifdef PLATFORM_PS4 and are dead on PC otherwise).
 *  That tag names ONE player - whoever a 25 m raycast down the camera axis happens to hit - and
 *  draws the name at a fixed spot next to the crosshair rather than over the character. In a lobby
 *  where the question is "who is here", pointing at people one at a time is the wrong shape.
 *
 *  NAMES COME FROM THE SERVER, KEYED BY NETWORK ID - and the reason recorded here was WRONG.
 *
 *  ⚠️ RETRACTED 2026-08-12. This transport replaced a version that enumerated
 *  ClientData.m_PlayerBaseList and read each entity's identity, on the theory that
 *  PlayerBase.GetIdentity() is not populated CLIENT-side for a REMOTE player. THAT THEORY IS FALSE.
 *  Measured immediately afterwards by the funnel below: `noidentity=0`, sustained over every sample.
 *  Remote identities are populated and the original version worked - the bug report that triggered
 *  the rewrite turned out to be about a different overlay entirely.
 *
 *  The rewrite was therefore unnecessary, not incorrect: Object.GetNetworkID and
 *  GetGame().GetObjectByNetworkId do work on both sides, and this version is measured good. It is
 *  kept only because it is the one that shipped and was verified. IF YOU ARE SIMPLIFYING THIS FILE,
 *  the client-side version is strictly smaller - it needs no SetLobbyNames RPC, no 1 Hz per-player
 *  push and no arrays on BattleRoyaleRPC - and it is the one to go back to. Do not preserve the
 *  server push out of the belief that identities are unavailable; they are.
 *
 *  POSITIONS ARE STILL LOCAL, and that part of the original reasoning stands: everyone in the lobby
 *  is in one clearing and therefore inside the bubble, so the live entity is exact and per-frame
 *  where a push would be 1 Hz. The server sends who and what to call them; the client works out
 *  where. `noentity` in the diagnostic below is what would disprove that.
 *
 *  PARTY MEMBERS ARE EXCLUDED SERVER-SIDE, before the packet is built - they already carry the
 *  party's own coloured tags, and two labels over one character is exactly the stacking that had to
 *  be fixed for the admin overlay. Doing it there rather than here keeps party composition off the
 *  wire and means this file names no Party symbol at all.
 *
 *  NO OCCLUSION TEST. Vanilla's tag needed line of sight because it was one raycast; matching that
 *  here would be one raycast per player per frame, sixty of them in a full lobby, to enforce a rule
 *  that buys nothing in a safe zone where nobody can shoot. A name is visible through a wall in the
 *  lobby. If that ever needs changing, note it is a cost decision and not an oversight.
 *
 *  The projection and pooling are BattleRoyaleSpectatorTags', minus the edge clamp: an off-screen
 *  tag is simply dropped. The admin overlay clamps because an admin wants to know somebody is behind
 *  them; a player in the lobby does not need an arrow pointing at everyone they cannot see.
 */
class BattleRoyaleLobbyTags
{
    protected Widget m_Root;
    protected bool m_RootFailed;

    protected ref array<Widget> m_Tags;
    protected ref array<TextWidget> m_TagNames;

    //--- Throttle for the funnel diagnostic.
    protected int m_NextDiagMs;

    void BattleRoyaleLobbyTags()
    {
        m_Tags = new array<Widget>();
        m_TagNames = new array<TextWidget>();
        m_RootFailed = false;
    }

    void ~BattleRoyaleLobbyTags()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    //! Created on the first Update, not in the constructor - the constructor runs inside
    //! MissionGameplay.OnInit, and this mod only ever builds widgets after super.OnInit() returned.
    protected bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        m_Root = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/lobby_tags.layout");

        if (!m_Root)
        {
            //--- Latched: retrying every frame would spam the log for the whole lobby.
            //---
            //--- WARN, NOT ERROR. BattleRoyaleUtils.Error raises a VM exception and unwinds the
            //--- stack, and this runs from BattleRoyaleClient.Update - so a missing layout would
            //--- take the client's whole per-frame update down, every frame, rather than costing
            //--- one cosmetic overlay. Nothing here is unrecoverable.
            m_RootFailed = true;
            BattleRoyaleUtils.Warn("[Lobby] Could not create lobby_tags.layout - lobby name tags disabled");
            return false;
        }

        m_Root.Show(false);
        BattleRoyaleUtils.Debug("[Lobby] Name tag layout ready");
        return true;
    }

    protected void EnsureCapacity(int wanted)
    {
        if (!m_Root)
            return;

        while (m_Tags.Count() < wanted)
        {
            Widget tag = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/lobby_tag.layout", m_Root);
            if (!tag)
                return;

            m_Tags.Insert(tag);
            m_TagNames.Insert(TextWidget.Cast(tag.FindAnyWidget("TagName")));
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
     *  Called every frame from BattleRoyaleClient.Update.
     *
     *  Distances are measured from the local player's BODY, feet to feet - so BR_LOBBY_TAG_MAX_DISTANCE_M
     *  means that many metres between two characters, whatever the camera is doing.
     *
     *  REVERSED 2026-08-14, and the reasoning it replaced was not wrong so much as outgrown. This
     *  measured from the CAMERA, on the grounds that a third-person player then gets the same answer
     *  as what they are looking at rather than one offset by the boom arm. That holds - but the boom
     *  is ~2 m, which was noise against the old 80 m cap and is 20% of the 10 m one. A "10 m" rule
     *  that is really 8 m in third person and 10 m in first is not a rule anybody can reason about.
     *  The trade accepted in exchange: in third person a tag can now appear on somebody fractionally
     *  behind the camera. Do not re-derive the camera version from the shape of the fallback below.
     */
    void Update(bool active)
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

        m_Root.Show(true);

        float parent_w;
        float parent_h;
        m_Root.GetScreenSize(parent_w, parent_h);
        if (parent_w <= 0 || parent_h <= 0)
            return;

        //--- Body first, camera only as a fallback for the frames before the local player exists -
        //--- this runs every frame, including during load-in. NOT the spectator case: UpdateLobbyTags
        //--- gates on !IsSpectating(), so the "GetPlayer() keeps returning the corpse" trap is out of
        //--- reach here.
        vector reference_pos = GetGame().GetCurrentCameraPosition();

        PlayerBase local_player = PlayerBase.Cast(GetGame().GetPlayer());
        if (local_player)
            reference_pos = local_player.GetPosition();

        int used = 0;
        int rows = br_rpc.lobby_names.Count();

        //--- Funnel counters, for the one diagnostic line below. Cheap, and the alternative is
        //--- guessing which gate ate the roster - which is exactly what cost the first attempt.
        int no_entity = 0;
        int no_identity = 0;
        int too_far = 0;
        int off_screen = 0;

        for (int i = 0; i < rows; i++)
        {
            if (used >= BR_LOBBY_TAG_MAX_ROWS)
                break;
            if (i >= br_rpc.lobby_net_low.Count())
                break;
            if (i >= br_rpc.lobby_net_high.Count())
                break;

            //--- Each read on its own line before the call that consumes them: this codebase has a
            //--- measured defect where a container read nested in a call argument returns the wrong
            //--- element.
            int net_low = br_rpc.lobby_net_low.Get(i);
            int net_high = br_rpc.lobby_net_high.Get(i);

            PlayerBase other = PlayerBase.Cast(GetGame().GetObjectByNetworkId(net_low, net_high));
            if (!other)
            {
                //--- Normal, not an error: the subject is outside this client's network bubble. In
                //--- the lobby everybody is in one clearing, so it should be rare - a persistently
                //--- high count here means the assumption behind sending no positions is wrong.
                no_entity++;
                continue;
            }

            if (!other.IsAlive())
                continue;

            //--- COUNTED, NOT USED AS A GATE. This is the measurement that settles why the first
            //--- attempt drew nothing: it enumerated ClientData.m_PlayerBaseList and required
            //--- GetIdentity() to match a uid, and every consumer of that idiom in this repo hides
            //--- its failure behind a server-pushed fallback. If this stays at zero the identity is
            //--- populated after all and something else was wrong; if it equals the row count, it
            //--- was exactly the cause.
            if (!other.GetIdentity())
                no_identity++;

            float distance = vector.Distance(reference_pos, other.GetPosition());
            if (distance > BR_LOBBY_TAG_MAX_DISTANCE_M)
            {
                too_far++;
                continue;
            }

            vector screen_pos = GetGame().GetScreenPosRelative(AnchorPos(other));
            if (!IsOnScreen(screen_pos))
            {
                off_screen++;
                continue;
            }

            EnsureCapacity(used + 1);
            if (used >= m_Tags.Count())
                break;

            string display_name = br_rpc.lobby_names.Get(i);

            RenderSlot(used, display_name, screen_pos, distance, parent_w, parent_h);
            used++;
        }

        ReportFunnel(rows, no_entity, no_identity, too_far, off_screen, used);

        //--- Surplus pooled widgets from a busier frame. Hidden rather than unlinked: these are
        //--- free-positioned in a plain panel, not laid out by a spacer, so a hidden one occupies
        //--- nothing. (The Unlink rule applies under a WrapSpacer, which this is not.)
        for (int j = used; j < m_Tags.Count(); j++)
        {
            m_Tags.Get(j).Show(false);
        }
    }

    /**
     *  One throttled line saying where the roster went.
     *
     *  Kept rather than deleted once it worked. The first version of this overlay drew nothing at
     *  all and there were five plausible reasons why - and only a funnel distinguishes them. Two
     *  seconds apart, and only while the overlay is active, so it costs one comparison per frame for
     *  the rest of the match.
     */
    protected void ReportFunnel(int rows, int no_entity, int no_identity, int too_far, int off_screen, int drawn)
    {
        int now = GetGame().GetTime();
        if (now < m_NextDiagMs)
            return;

        m_NextDiagMs = now + BR_LOBBY_TAG_DIAG_MS;

        BattleRoyaleUtils.Debug("[Lobby] tags rows=" + rows + " noentity=" + no_entity + " noidentity=" + no_identity + " far=" + too_far + " offscreen=" + off_screen + " drawn=" + drawn);
    }

    protected void RenderSlot(int slot, string display_name, vector screen_pos, float distance, float parent_w, float parent_h)
    {
        Widget tag = m_Tags.Get(slot);
        if (!tag)
            return;

        //--- Shown before being measured: a widget that has never been displayed reports zero size,
        //--- and both offsets below are derived from that size.
        tag.Show(true);

        float tag_w;
        float tag_h;
        tag.GetScreenSize(tag_w, tag_h);
        if (tag_w <= 0)
            tag_w = BR_LOBBY_TAG_SIZE_W;
        if (tag_h <= 0)
            tag_h = BR_LOBBY_TAG_SIZE_H;

        //--- SetPos anchors by the top-left corner, so without these offsets the tag hangs a full
        //--- width right and a full height below the head.
        float px = (screen_pos[0] * parent_w) - (tag_w * 0.5);
        float py = (screen_pos[1] * parent_h) - tag_h - BR_LOBBY_TAG_GAP_PX;

        tag.SetPos(px, py);
        tag.SetAlpha(FadeAlpha(distance));

        //--- Nearer tags draw over farther ones.
        tag.SetSort(Math.Round(10000 - Math.Clamp(distance, 0, 9999)));

        TextWidget name_widget = m_TagNames.Get(slot);
        if (name_widget)
        {
            name_widget.SetText(display_name);
            name_widget.SetColor(BR_LOBBY_TAG_COLOUR);
        }
    }

    //! Full strength up to the fade distance, then linearly out to nothing at the cap - so a tag
    //! thins away as its owner walks off rather than blinking out at a hard edge.
    protected float FadeAlpha(float distance)
    {
        if (distance <= BR_LOBBY_TAG_FADE_START_M)
            return 1.0;

        float span = BR_LOBBY_TAG_MAX_DISTANCE_M - BR_LOBBY_TAG_FADE_START_M;
        if (span <= 0)
            return 1.0;

        return Math.Clamp(1.0 - ((distance - BR_LOBBY_TAG_FADE_START_M) / span), 0, 1);
    }

    //! Head height, falling back to a fixed offset from the feet when the bone will not resolve.
    protected vector AnchorPos(PlayerBase other)
    {
        //--- GetBoneIndexByName, not GetBoneIndex - the latter resolves a proxy SELECTION, which is
        //--- a different thing entirely and returns nonsense here.
        int head = other.GetBoneIndexByName("Head");
        if (head != -1)
            return other.GetBonePositionWS(head) + Vector(0, BR_LOBBY_TAG_HEAD_OFFSET, 0);

        return other.GetPosition() + Vector(0, BR_LOBBY_TAG_HEIGHT_OFFSET, 0);
    }

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
}
#endif
