#ifndef SERVER
/**
 *  Vigrid Map - client controller.
 *
 *  Owns the client's view of the marker set and the on-screen marker layer. The fullscreen menu is
 *  deliberately NOT owned here: it is a UIScriptedMenu with its own lifetime, created and destroyed
 *  by the UI manager, and it reads its data from this object. That split is what lets a marker
 *  survive closing the map.
 *
 *  Placement always round-trips to the server, even for a solo player. The alternative - keep it
 *  local until you join a party - means two code paths that can disagree, and the client cannot
 *  reliably tell whether it is in a party anyway: the roster arrives asynchronously and the party
 *  addon may not be installed at all. So the server is always the authority.
 *
 *  The latency that costs is bought back with an optimistic echo: the click draws a pending marker
 *  immediately, and the pending state is dropped as soon as the server's snapshot arrives, or after
 *  a short timeout if it never does. What you see is therefore instant, but what persists is only
 *  ever what the server agreed to.
 */
class VigridMapClient
{
    //--- Drawn immediately on click, before the server has answered.
    protected bool m_PendingActive;
    protected vector m_PendingPos;
    protected int m_PendingSinceMs;

    //--- Bumped whenever the visible set changes, so renderers repaint on the edge rather than
    //--- diffing positions every frame.
    protected int m_MarkerSeq;
    protected int m_LastRpcSeq;

    //--- The floating world markers. Owned here rather than by the menu because they are visible
    //--- while the map is closed - that is the whole point of them.
    protected ref VigridMapMarkers3D m_Markers3D;

    void VigridMapClient()
    {
        m_PendingActive = false;
        m_PendingPos = vector.Zero;
        m_MarkerSeq = 0;
        m_LastRpcSeq = -1;

        m_Markers3D = new VigridMapMarkers3D();

        //--- Ask for settings and the current set. Covers a client that finished loading after the
        //--- server had already pushed on connect.
        //---
        //--- Skipped in single player, where there is no counterpart: the server handlers are
        //--- #ifdef SERVER and so are compiled out of an offline client entirely, and CF warns
        //--- loudly about sending to an RPC that "does not seem to exist".
        if (GetGame().IsMultiplayer())
            GetRPCManager().SendRPC(RPC_VIGRIDMAP_SERVER_NAMESPACE, VM_RPC_REQUEST_SYNC, NULL, true);

        VigridMapLog.Debug("VigridMapClient created");
    }

    /**
     *  Ask the server to place or move our marker, and draw it straight away.
     */
    void PlaceMarker(vector pos)
    {
        //--- The map projection has no meaningful elevation - ScreenToMap's y is not a height - so
        //--- it is flattened once, here, rather than at every consumer.
        vector clean_pos = Vector(pos[0], 0, pos[2]);

        m_PendingActive = true;
        m_PendingPos = clean_pos;
        m_PendingSinceMs = GetGame().GetTime();
        m_MarkerSeq = m_MarkerSeq + 1;

        if (GetGame().IsMultiplayer())
            GetRPCManager().SendRPC(RPC_VIGRIDMAP_SERVER_NAMESPACE, VM_RPC_PLACE, new Param2<vector, string>(clean_pos, ""), true);

        VigridMapLog.Debug("Marker requested at " + clean_pos);
    }

    void ClearMarker()
    {
        m_PendingActive = false;
        m_PendingPos = vector.Zero;
        m_MarkerSeq = m_MarkerSeq + 1;

        if (GetGame().IsMultiplayer())
            GetRPCManager().SendRPC(RPC_VIGRIDMAP_SERVER_NAMESPACE, VM_RPC_REMOVE, NULL, true);

        VigridMapLog.Debug("Marker removal requested");
    }

    protected string GetSelfUid()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !player.GetIdentity())
            return "";

        return player.GetIdentity().GetPlainId();
    }

    //! Index of the local player's marker in the server set, or -1.
    protected int GetOwnIndex()
    {
        string self_uid = GetSelfUid();
        if (self_uid == "")
            return -1;

        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        int count = rpc.marker_owner_uids.Count();

        for (int i = 0; i < count; i++)
        {
            if (rpc.marker_owner_uids.Get(i) == self_uid)
                return i;
        }

        return -1;
    }

    //! Does the local player have a marker to draw - confirmed or still pending?
    bool HasMarker()
    {
        if (GetOwnIndex() != -1)
            return true;

        return m_PendingActive;
    }

    vector GetMarkerPos()
    {
        int index = GetOwnIndex();
        if (index != -1)
            return VigridMapRPC.GetInstance().marker_positions.Get(index);

        return m_PendingPos;
    }

    string GetMarkerLabel()
    {
        int index = GetOwnIndex();
        if (index != -1)
            return VigridMapRPC.GetInstance().marker_labels.Get(index);

        return "";
    }

    //! Every marker the server says we may see - ours plus our teammates'.
    int GetVisibleCount()
    {
        return VigridMapRPC.GetInstance().marker_owner_uids.Count();
    }

    vector GetVisiblePos(int index)
    {
        return VigridMapRPC.GetInstance().marker_positions.Get(index);
    }

    string GetVisibleLabel(int index)
    {
        return VigridMapRPC.GetInstance().marker_labels.Get(index);
    }

    bool IsVisibleOwn(int index)
    {
        return VigridMapRPC.GetInstance().marker_owner_uids.Get(index) == GetSelfUid();
    }

    /**
     *  The party slot of whoever placed marker `index`, or -1 for solo.
     *
     *  Recorded server-side at placement, which is what makes it stable: it survives the placer
     *  disconnecting, and it is the same number on every client regardless of the order their
     *  rosters arrived in. Resolve it through VigridMapTeam.GetColorForSlot, never through a roster
     *  lookup.
     */
    int GetVisibleSlot(int index)
    {
        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        if (index < 0)
            return -1;
        if (index >= rpc.marker_owner_slots.Count())
            return -1;

        return rpc.marker_owner_slots.Get(index);
    }

    int GetMarkerSeq()
    {
        return m_MarkerSeq;
    }

    /**
     *  The merged draw list: everything the server says we may see, plus our own pending marker if
     *  the server has not confirmed it yet.
     *
     *  Both renderers go through this rather than through the two sources separately, so the map
     *  and the world markers can never show different sets. The pending entry is appended last and
     *  only exists while unconfirmed, so indices are stable within a frame.
     */
    int GetDrawCount()
    {
        int count = GetVisibleCount();

        if (m_PendingActive && GetOwnIndex() == -1)
            count = count + 1;

        return count;
    }

    protected bool IsPendingIndex(int index)
    {
        return index >= GetVisibleCount();
    }

    vector GetDrawPos(int index)
    {
        if (IsPendingIndex(index))
            return m_PendingPos;

        return GetVisiblePos(index);
    }

    string GetDrawLabel(int index)
    {
        if (IsPendingIndex(index))
            return "";

        return GetVisibleLabel(index);
    }

    bool IsDrawOwn(int index)
    {
        if (IsPendingIndex(index))
            return true;

        return IsVisibleOwn(index);
    }

    //! -1 for the pending entry: it is drawn in the own-marker colour anyway, so its slot is moot
    //! until the server confirms it and hands back the real one.
    int GetDrawSlot(int index)
    {
        if (IsPendingIndex(index))
            return -1;

        return GetVisibleSlot(index);
    }

    /**
     *  Ticked every frame from MissionGameplay.OnUpdate. The world markers follow the camera, so
     *  they genuinely need every frame rather than a timer - anything slower visibly lags a turn.
     */
    void Update(float timeslice)
    {
        ResolvePending();

        if (m_Markers3D)
            m_Markers3D.Update(timeslice);
    }

    /**
     *  Retire the optimistic marker.
     *
     *  Cleared as soon as the server's set contains one of ours, whatever its position - the server
     *  may legitimately have moved it, by clamping to the world bounds, and in that case its answer
     *  is the truth and the pending copy must stop competing with it.
     *
     *  The timeout is the other exit: a refused placement produces no snapshot at all, so without
     *  it the pending marker would sit there looking accepted for ever.
     */
    protected void ResolvePending()
    {
        if (!m_PendingActive)
            return;

        //--- Single player has no server to echo, so the client is the authority and the marker is
        //--- simply kept. Without this it would time out and vanish two seconds after every click,
        //--- which makes the map impossible to try offline.
        if (!GetGame().IsMultiplayer())
            return;

        VigridMapRPC rpc = VigridMapRPC.GetInstance();

        if (rpc.marker_seq != m_LastRpcSeq)
        {
            m_LastRpcSeq = rpc.marker_seq;

            if (GetOwnIndex() != -1)
            {
                m_PendingActive = false;
                m_MarkerSeq = m_MarkerSeq + 1;
                return;
            }
        }

        if ((GetGame().GetTime() - m_PendingSinceMs) < VIGRID_MAP_PENDING_TTL_MS)
            return;

        VigridMapLog.Debug("Pending marker timed out without a server echo");
        m_PendingActive = false;
        m_MarkerSeq = m_MarkerSeq + 1;
    }
}
#endif
