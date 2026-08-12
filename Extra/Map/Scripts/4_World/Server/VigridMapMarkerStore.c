#ifdef SERVER
/**
 *  Vigrid Map - the authoritative marker set.
 *
 *  One marker per owner, keyed by SteamID64, so placing again moves yours rather than adding a
 *  second. That single rule removes a surprising amount of machinery: there is no cap to enforce,
 *  no FIFO eviction, and removal needs no hit test because there is only ever one candidate.
 *
 *  Markers never expire. There is no sweep tick here and no expiry on the wire - that is the line
 *  between a marker and a party ping, and it is why this class is not the ping store copied.
 *
 *  Ownership is always taken from the RPC sender's identity and never from the payload. A client
 *  that could name its own owner_uid could place, move and delete other players' markers.
 */
class VigridMapMarkerStore
{
    private static ref VigridMapMarkerStore m_Instance;

    private ref map<string, ref VigridMapMarker> m_Markers;
    private ref map<string, int> m_LastPlaceMs;

    //--- Monotonic, sent with every snapshot so a client can discard one that arrives out of order.
    private int m_SetVersion;

    //--- Runtime switch on top of the `enabled` setting, for a host that wants markers off during
    //--- part of a match. Not persisted.
    private bool m_Active;

    private int m_LastResyncMs;

    void VigridMapMarkerStore()
    {
        m_Markers = new map<string, ref VigridMapMarker>();
        m_LastPlaceMs = new map<string, int>();
        m_SetVersion = 0;
        m_Active = true;
        m_LastResyncMs = 0;
    }

    static VigridMapMarkerStore GetInstance()
    {
        if (!m_Instance)
            m_Instance = new VigridMapMarkerStore();

        return m_Instance;
    }

    void SetActive(bool active)
    {
        if (m_Active == active)
            return;

        m_Active = active;
        VigridMapLog.Info("Markers active: " + active);
    }

    bool IsActive()
    {
        if (!m_Active)
            return false;

        return VigridMapConfig.GetConfig().GetSettings().enabled;
    }

    void ClearAll()
    {
        if (m_Markers.Count() == 0)
            return;

        m_Markers.Clear();
        m_SetVersion = m_SetVersion + 1;

        VigridMapLog.Info("All markers cleared");
        PushToAll();
    }

    void OnPlayerDisconnected(string uid)
    {
        if (!VigridMapConfig.GetConfig().GetSettings().clear_markers_on_disconnect)
            return;
        if (!m_Markers.Contains(uid))
            return;

        m_Markers.Remove(uid);
        m_SetVersion = m_SetVersion + 1;
        PushToAll();
    }

    /**
     *  Place or move `uid`'s marker. Returns false when the request was refused, in which case the
     *  caller has already been told why.
     */
    bool Place(PlayerIdentity sender, PlayerBase player, vector pos, string label)
    {
        string uid = sender.GetPlainId();

        int now_ms = GetGame().GetTime();
        if (m_LastPlaceMs.Contains(uid) && (now_ms - m_LastPlaceMs.Get(uid)) < VIGRID_MAP_PLACE_COOLDOWN_MS)
        {
            //--- Dropped silently. A message per rejected click would itself become the spam the
            //--- cooldown exists to stop.
            VigridMapLog.Trace("Place from " + uid + " dropped by cooldown");
            return false;
        }

        //--- The map projection has no meaningful elevation, so height is discarded rather than
        //--- trusted. Everything downstream measures in 2D anyway.
        vector clean_pos = Vector(pos[0], 0, pos[2]);

        float world_size = GetGame().GetWorld().GetWorldSize();
        if (clean_pos[0] < 0 || clean_pos[0] > world_size)
            return false;
        if (clean_pos[2] < 0 || clean_pos[2] > world_size)
            return false;

        m_LastPlaceMs.Set(uid, now_ms);

        //--- Recorded now, at placement, rather than resolved when the marker is drawn: this is the
        //--- number that lets the marker keep its owner's colour after they disconnect.
        int slot = VigridMapTeam.GetMemberSlot(player);

        string clean_label = Sanitise(label);

        //--- Held in a ref local before being handed over: Set() releases whatever was there first,
        //--- so the replacement has to already exist.
        ref VigridMapMarker marker = new VigridMapMarker(uid, slot, clean_pos, clean_label, now_ms);
        m_Markers.Set(uid, marker);

        m_SetVersion = m_SetVersion + 1;

        VigridMapLog.Debug("Marker placed by " + uid + " at " + clean_pos);
        PushToPlayerAndTeam(player);
        return true;
    }

    bool Remove(PlayerIdentity sender, PlayerBase player)
    {
        string uid = sender.GetPlainId();
        if (!m_Markers.Contains(uid))
            return false;

        m_Markers.Remove(uid);
        m_SetVersion = m_SetVersion + 1;

        VigridMapLog.Debug("Marker removed by " + uid);
        PushToPlayerAndTeam(player);
        return true;
    }

    /**
     *  Strip control characters and truncate. Done here rather than trusted from the client,
     *  because the label is rendered into a widget on every teammate's screen.
     */
    private string Sanitise(string label)
    {
        int limit = VigridMapConfig.GetConfig().GetSettings().label_max_length;
        if (limit < 0)
            limit = 0;

        string result = "";
        int count = label.Length();

        for (int i = 0; i < count; i++)
        {
            if (result.Length() >= limit)
                break;

            string ch = label.Get(i);
            if (ch == "\n" || ch == "\r" || ch == "\t")
                continue;

            result = result + ch;
        }

        return result;
    }

    //! Every connected player, as PlayerBase. The party API takes the population explicitly so it
    //! never has to know anything about match state.
    private array<PlayerBase> GetPopulation()
    {
        array<PlayerBase> population = new array<PlayerBase>();

        array<Man> men = new array<Man>();
        GetGame().GetPlayers(men);

        int count = men.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase player = PlayerBase.Cast(men.Get(i));
            if (player)
                population.Insert(player);
        }

        return population;
    }

    /**
     *  Send `player` the markers they are allowed to see: their own, plus their teammates'.
     *
     *  With the party addon absent this degrades to "mine only", which is exactly right - there is
     *  no team to share with.
     */
    void PushTo(PlayerBase player)
    {
        if (!player)
            return;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return;

        array<string> uids = new array<string>();
        array<int> slots = new array<int>();
        array<vector> positions = new array<vector>();
        array<string> labels = new array<string>();

        AppendMarkerFor(player.GetIdentity().GetPlainId(), uids, slots, positions, labels);

        //--- Empty, never null, when Party is absent - so this loop simply does nothing and the
        //--- player sees only their own marker. No guard needed.
        array<PlayerBase> mates = VigridMapTeam.GetTeammates(player, GetPopulation());
        int mate_count = mates.Count();
        for (int i = 0; i < mate_count; i++)
        {
            PlayerBase mate = mates.Get(i);
            if (!mate || !mate.GetIdentity())
                continue;

            AppendMarkerFor(mate.GetIdentity().GetPlainId(), uids, slots, positions, labels);
        }

        GetRPCManager().SendRPC(RPC_VIGRIDMAP_NAMESPACE, VM_RPC_MARKERS, new Param5<int, ref array<string>, ref array<int>, ref array<vector>, ref array<string>>(m_SetVersion, uids, slots, positions, labels), true, identity);
    }

    private void AppendMarkerFor(string uid, array<string> uids, array<int> slots, array<vector> positions, array<string> labels)
    {
        if (uid == "")
            return;
        if (!m_Markers.Contains(uid))
            return;

        VigridMapMarker marker = m_Markers.Get(uid);
        if (!marker)
            return;

        uids.Insert(marker.owner_uid);
        slots.Insert(marker.owner_slot);
        positions.Insert(marker.pos);
        labels.Insert(marker.label);
    }

    //! Push to one player and everyone who can see their markers - the minimum set affected by a
    //! change that player made.
    void PushToPlayerAndTeam(PlayerBase player)
    {
        if (!player)
            return;

        PushTo(player);

        array<PlayerBase> mates = VigridMapTeam.GetTeammates(player, GetPopulation());
        int count = mates.Count();
        for (int i = 0; i < count; i++)
        {
            PushTo(mates.Get(i));
        }
    }

    void PushToAll()
    {
        array<PlayerBase> population = GetPopulation();
        int count = population.Count();
        for (int i = 0; i < count; i++)
        {
            PushTo(population.Get(i));
        }
    }

    void SendSettings(PlayerIdentity identity)
    {
        if (!identity)
            return;

        VigridMapData settings = VigridMapConfig.GetConfig().GetSettings();

        GetRPCManager().SendRPC(RPC_VIGRIDMAP_NAMESPACE, VM_RPC_SETTINGS, new Param3<bool, bool, int>(IsActive(), settings.minimap_allowed, settings.label_max_length), true, identity);
    }

    /**
     *  Slow resync, ticked from MissionServer.OnUpdate.
     *
     *  This is what makes "joined a party mid-match" work without the party addon having to notify
     *  this one, which would be a dependency in the wrong direction. Party membership can change
     *  without any marker changing, and the visible set is derived from membership, so it has to be
     *  recomputed on a timer rather than only on mutation.
     */
    void Update()
    {
        int now_ms = GetGame().GetTime();
        if ((now_ms - m_LastResyncMs) < VIGRID_MAP_RESYNC_INTERVAL_MS)
            return;

        m_LastResyncMs = now_ms;

        if (m_Markers.Count() == 0)
            return;

        PushToAll();
    }
}
#endif
