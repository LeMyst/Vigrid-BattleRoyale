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
 *  The latency that costs is bought back with an optimistic echo: the click draws the result
 *  immediately, and the prediction is dropped as soon as the server's snapshot agrees with it, or
 *  after a short timeout if it never does. What you see is therefore instant, but what persists is
 *  only ever what the server agreed to.
 *
 *  THE PREDICTION COVERS ALL THREE INTERACTIONS, and originally it covered only one. It used to be a
 *  single bool that appended a marker when the player had none, which meant the very first click of
 *  a session was instant and every click after it waited for the wire - a MOVE redrew the old
 *  position until the echo landed, and a right-click kept drawing a marker the player had already
 *  deleted. Both now predict, which is what m_PendingIntent is for: the store keys one marker per
 *  owner, so a second click is a move and the prediction has to OVERRIDE the confirmed entry rather
 *  than sit beside it, and a removal has to SUPPRESS one.
 */
class VigridMapClient
{
    //--- What we have asked the server for and are drawing ahead of the answer: one of the
    //--- VIGRID_MAP_PENDING_* values. m_PendingPos is meaningful only for PLACE.
    protected int m_PendingIntent;
    protected vector m_PendingPos;
    protected int m_PendingSinceMs;

    //--- Bumped whenever the visible set changes, so renderers repaint on the edge rather than
    //--- diffing positions every frame.
    protected int m_MarkerSeq;
    protected int m_LastRpcSeq;

    //--- Last refusal consumed, so one refusal cancels one prediction. Independent of the menu's own
    //--- watcher on the same counter - the menu shows the message, this cancels the drawing.
    protected int m_LastRejectSeq;

    //--- The floating world markers. Owned here rather than by the menu because they are visible
    //--- while the map is closed - that is the whole point of them.
    protected ref VigridMapMarkers3D m_Markers3D;

    void VigridMapClient()
    {
        m_PendingIntent = VIGRID_MAP_PENDING_NONE;
        m_PendingPos = vector.Zero;
        m_MarkerSeq = 0;
        m_LastRpcSeq = -1;
        m_LastRejectSeq = -1;

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

        m_PendingIntent = VIGRID_MAP_PENDING_PLACE;
        m_PendingPos = clean_pos;
        m_PendingSinceMs = GetGame().GetTime();
        m_MarkerSeq = m_MarkerSeq + 1;

        if (GetGame().IsMultiplayer())
            GetRPCManager().SendRPC(RPC_VIGRIDMAP_SERVER_NAMESPACE, VM_RPC_PLACE, new Param2<vector, string>(clean_pos, ""), true);

        VigridMapLog.Debug("Marker requested at " + clean_pos);
    }

    /**
     *  Ask the server to drop our marker, and stop drawing it straight away.
     *
     *  The intent is recorded rather than the prediction simply cleared. Clearing was right only
     *  while the prediction could not outlive an unconfirmed placement: with a CONFIRMED marker in
     *  the set there is something to hide, and nothing else hides it.
     */
    void ClearMarker()
    {
        m_PendingIntent = VIGRID_MAP_PENDING_REMOVE;
        m_PendingPos = vector.Zero;
        m_PendingSinceMs = GetGame().GetTime();
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
        if (m_PendingIntent == VIGRID_MAP_PENDING_PLACE)
            return true;
        if (m_PendingIntent == VIGRID_MAP_PENDING_REMOVE)
            return false;

        return GetOwnIndex() != -1;
    }

    vector GetMarkerPos()
    {
        if (m_PendingIntent == VIGRID_MAP_PENDING_PLACE)
            return m_PendingPos;

        int index = GetOwnIndex();
        if (index != -1)
            return GetVisiblePos(index);

        return vector.Zero;
    }

    string GetMarkerLabel()
    {
        int index = GetOwnIndex();
        if (index != -1)
            return GetVisibleLabel(index);

        return "";
    }

    //! Every marker the server says we may see - ours plus our teammates'.
    int GetVisibleCount()
    {
        return VigridMapRPC.GetInstance().marker_owner_uids.Count();
    }

    /**
     *  The three raw readers below are all bounds-checked, unlike the position and label readers they
     *  replaced. The draw list no longer maps a draw slot to the same array index - a suppressed
     *  removal shifts everything past it - so an off-by-one here would be an out-of-range read rather
     *  than a visibly wrong marker.
     */
    vector GetVisiblePos(int index)
    {
        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        if (index < 0)
            return vector.Zero;
        if (index >= rpc.marker_positions.Count())
            return vector.Zero;

        return rpc.marker_positions.Get(index);
    }

    string GetVisibleLabel(int index)
    {
        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        if (index < 0)
            return "";
        if (index >= rpc.marker_labels.Count())
            return "";

        return rpc.marker_labels.Get(index);
    }

    bool IsVisibleOwn(int index)
    {
        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        if (index < 0)
            return false;
        if (index >= rpc.marker_owner_uids.Count())
            return false;

        return rpc.marker_owner_uids.Get(index) == GetSelfUid();
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
     *  The merged draw list: what the player should be looking at right now, which is the server's
     *  set with our own outstanding request already applied to it.
     *
     *  All three renderers - the fullscreen map, the minimap and the floating world markers - go
     *  through this rather than through the two sources separately, so they can never show different
     *  sets. It is also why none of them needed changing when the prediction was extended to moves
     *  and removals.
     */
    int GetDrawCount()
    {
        int count = GetVisibleCount();
        int own = GetOwnIndex();

        //--- Nothing of ours in the set yet, so the prediction is an extra entry.
        if (m_PendingIntent == VIGRID_MAP_PENDING_PLACE && own == -1)
            return count + 1;

        //--- Ours is still in the set, so the prediction takes one away. A REMOVE with no confirmed
        //--- entry has nothing to hide and leaves the count alone, which is also what keeps this
        //--- from going negative.
        if (m_PendingIntent == VIGRID_MAP_PENDING_REMOVE && own != -1)
            return count - 1;

        return count;
    }

    /**
     *  Map a draw slot onto the server set, or -1 when the slot is our own prediction.
     *
     *  The one place the index arithmetic lives. Every Get*Draw* accessor goes through it, so the
     *  three cases below are stated once instead of once per accessor.
     */
    protected int ResolveDrawIndex(int index)
    {
        int own = GetOwnIndex();

        if (m_PendingIntent == VIGRID_MAP_PENDING_REMOVE)
        {
            //--- Our entry is hidden, so every slot at or past it addresses the one after it.
            if (own != -1 && index >= own)
                return index + 1;

            return index;
        }

        if (m_PendingIntent == VIGRID_MAP_PENDING_PLACE)
        {
            //--- Moving: our confirmed entry keeps its slot and its colour, and only its POSITION
            //--- comes from the prediction. Reporting it as an extra entry instead would draw the
            //--- marker twice, at both ends of the move, until the echo landed.
            if (own != -1 && index == own)
                return -1;

            //--- Placing: the prediction is appended after the server's set.
            if (own == -1 && index >= GetVisibleCount())
                return -1;
        }

        return index;
    }

    vector GetDrawPos(int index)
    {
        //--- Resolved into a local before it is used as a subscript, deliberately. An array read
        //--- sharing an expression with a call has been measured to read the wrong array entirely.
        int source = ResolveDrawIndex(index);
        if (source == -1)
            return m_PendingPos;

        return GetVisiblePos(source);
    }

    string GetDrawLabel(int index)
    {
        int source = ResolveDrawIndex(index);
        if (source == -1)
            return "";

        return GetVisibleLabel(source);
    }

    bool IsDrawOwn(int index)
    {
        int source = ResolveDrawIndex(index);
        if (source == -1)
            return true;

        return IsVisibleOwn(source);
    }

    /**
     *  The party slot behind draw slot `index`.
     *
     *  A prediction that is MOVING a confirmed marker borrows that marker's slot, because the server
     *  has already told us what it is and the marker has not changed hands. Only a placement the
     *  server has never seen answers -1, and that one is drawn in the own-marker colour anyway.
     */
    int GetDrawSlot(int index)
    {
        int source = ResolveDrawIndex(index);
        if (source != -1)
            return GetVisibleSlot(source);

        int own = GetOwnIndex();
        if (own != -1)
            return GetVisibleSlot(own);

        return -1;
    }

    /**
     *  Ticked every frame from MissionGameplay.OnUpdate. The world markers follow the camera, so
     *  they genuinely need every frame rather than a timer - anything slower visibly lags a turn.
     */
    void Update(float timeslice)
    {
        TrackSnapshot();
        ResolveRejection();
        ResolvePending();

        if (m_Markers3D)
            m_Markers3D.Update(timeslice);
    }

    /**
     *  Turn an incoming snapshot into a repaint edge.
     *
     *  UNCONDITIONAL, and that is the point. This used to live inside ResolvePending, below its
     *  early return for "nothing pending", so a snapshot that arrived while we had no outstanding
     *  request raised no edge at all - and the fullscreen map only repaints on an edge. Everything
     *  in that category fell back on the one-second repaint watchdog: a teammate's marker appearing,
     *  and the confirmation of our own REMOVE, which clears the prediction before the echo lands and
     *  so is never pending when its own snapshot arrives. That second was much larger than the round
     *  trip it was mistaken for. The minimap and the world markers never showed it because they
     *  repaint on a clock and every frame respectively - the map being visibly last of the three is
     *  the fingerprint.
     */
    protected void TrackSnapshot()
    {
        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        if (rpc.marker_seq == m_LastRpcSeq)
            return;

        m_LastRpcSeq = rpc.marker_seq;
        m_MarkerSeq = m_MarkerSeq + 1;
    }

    /**
     *  Stop predicting something the server has explicitly refused.
     *
     *  Without this a refusal is indistinguishable from a slow answer, and the prediction sits there
     *  looking accepted until the TTL. The menu watches the same counter independently to show the
     *  message; this half only stops the drawing.
     *
     *  A refusal carries no request id, so on a link slow enough for one to arrive AFTER the player
     *  has clicked again, it cancels the newer prediction instead of the one it belongs to. That is
     *  accepted deliberately: the cost is that the marker shows its confirmed position for the rest
     *  of one round trip - exactly what it did before any of this existed - against a two-second
     *  stale lie if the refusal were ignored. Refusals are also rare now that the click debounce is
     *  longer than the server's place cooldown. Adding an id would fix it and is not worth a wire
     *  change for a flicker.
     */
    protected void ResolveRejection()
    {
        VigridMapRPC rpc = VigridMapRPC.GetInstance();
        if (rpc.rejected_seq == m_LastRejectSeq)
            return;

        m_LastRejectSeq = rpc.rejected_seq;

        if (m_PendingIntent == VIGRID_MAP_PENDING_NONE)
            return;

        VigridMapLog.Debug("Prediction dropped - server refused: " + rpc.rejected_key);

        m_PendingIntent = VIGRID_MAP_PENDING_NONE;
        m_MarkerSeq = m_MarkerSeq + 1;
    }

    /**
     *  Retire the prediction once the server agrees with it.
     *
     *  The test is what the set CONTAINS, not merely that a snapshot arrived. The old test - any
     *  snapshot in which we own a marker - was correct only for a first placement, and both of the
     *  cases added since break it: on a move we already own one, at the OLD position, so an
     *  unrelated bump (the five-second resync, or a teammate placing) would have retired the
     *  prediction and rubber-banded the marker back until the real echo arrived; on a removal the
     *  test is exactly inverted, since owning a marker is the state we are waiting to leave.
     *
     *  The timeout is the other exit, and is now only reached when a request goes unanswered
     *  entirely - every refusal the server can reach answers with a corrective snapshot or a
     *  VM_Rejected.
     */
    protected void ResolvePending()
    {
        if (m_PendingIntent == VIGRID_MAP_PENDING_NONE)
            return;

        //--- Single player has no server to echo, so the client is the authority and the prediction
        //--- is simply kept. Without this it would time out and vanish two seconds after every
        //--- click, which makes the map impossible to try offline.
        if (!GetGame().IsMultiplayer())
            return;

        if (IsPendingSatisfied())
        {
            m_PendingIntent = VIGRID_MAP_PENDING_NONE;
            m_MarkerSeq = m_MarkerSeq + 1;
            return;
        }

        if ((GetGame().GetTime() - m_PendingSinceMs) < VIGRID_MAP_PENDING_TTL_MS)
            return;

        VigridMapLog.Debug("Pending marker timed out without a server echo");
        m_PendingIntent = VIGRID_MAP_PENDING_NONE;
        m_MarkerSeq = m_MarkerSeq + 1;
    }

    //! Has the server's set caught up with what we predicted?
    protected bool IsPendingSatisfied()
    {
        int own = GetOwnIndex();

        if (m_PendingIntent == VIGRID_MAP_PENDING_REMOVE)
            return own == -1;

        if (own == -1)
            return false;

        //--- A tolerance rather than equality only because the position makes a float round trip -
        //--- the server does not adjust it, it refuses a position it will not take.
        vector confirmed = GetVisiblePos(own);
        return vector.Distance(confirmed, m_PendingPos) < VIGRID_MAP_PENDING_MATCH_EPSILON_M;
    }
}
#endif
