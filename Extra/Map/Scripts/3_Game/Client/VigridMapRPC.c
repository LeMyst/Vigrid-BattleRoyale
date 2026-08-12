#ifndef SERVER
/**
 *  Vigrid Map - client RPC receiver.
 *
 *  A pure data bag: handlers store, they never act. That is what lets this class live in 3_Game,
 *  which cannot reference MissionGameplay; the 5_Mission controller polls these fields. Keeping it
 *  that way is the whole reason the split exists.
 *
 *  Handler methods must be named EXACTLY like their registered strings - CF's AddRPC dispatches by
 *  method name, so a rename here silently stops the message arriving.
 */
class VigridMapRPC
{
    private static ref VigridMapRPC m_Instance;

    //--- Settings mirrored from the server. Defaulted permissively so a client that has not yet
    //--- received the push behaves like a normal server rather than a locked-down one.
    bool markers_enabled = VIGRID_MAP_DEF_MARKERS_ENABLED;
    bool minimap_allowed = VIGRID_MAP_DEF_MINIMAP_ALLOWED;
    bool compass_allowed = VIGRID_MAP_DEF_COMPASS_ALLOWED;
    int label_max_length = VIGRID_MAP_DEF_LABEL_MAX;

    //--- The visible marker set: mine plus my teammates'. Parallel arrays, matching how the party
    //--- addon puts its roster on the wire.
    ref array<string> marker_owner_uids = new array<string>();
    ref array<int> marker_owner_slots = new array<int>();
    ref array<vector> marker_positions = new array<vector>();
    ref array<string> marker_labels = new array<string>();

    //--- Monotonic on the server. Used to drop a snapshot that arrives out of order, and as the
    //--- repaint trigger, so renderers never diff positions themselves.
    int set_version = -1;
    int marker_seq = 0;

    void VigridMapRPC()
    {
        GetRPCManager().AddRPC(RPC_VIGRIDMAP_NAMESPACE, VM_RPC_SETTINGS, this);
        GetRPCManager().AddRPC(RPC_VIGRIDMAP_NAMESPACE, VM_RPC_MARKERS, this);
        GetRPCManager().AddRPC(RPC_VIGRIDMAP_NAMESPACE, VM_RPC_REJECTED, this);

        VigridMapLog.Debug("VigridMapRPC registered");
    }

    static VigridMapRPC GetInstance()
    {
        if (!m_Instance)
            m_Instance = new VigridMapRPC();

        return m_Instance;
    }

    /**
     *  The singleton outlives a server change, so anything still held belongs to the previous
     *  session and would render on this one's map. Called from MissionGameplay.OnInit, exactly as
     *  the kill feed and party addons do.
     */
    void Reset()
    {
        markers_enabled = VIGRID_MAP_DEF_MARKERS_ENABLED;
        minimap_allowed = VIGRID_MAP_DEF_MINIMAP_ALLOWED;
        compass_allowed = VIGRID_MAP_DEF_COMPASS_ALLOWED;
        label_max_length = VIGRID_MAP_DEF_LABEL_MAX;

        marker_owner_uids.Clear();
        marker_owner_slots.Clear();
        marker_positions.Clear();
        marker_labels.Clear();

        set_version = -1;
        marker_seq = marker_seq + 1;
    }

    void VM_Settings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param4<bool, bool, bool, int> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        markers_enabled = data.param1;
        minimap_allowed = data.param2;
        compass_allowed = data.param3;
        label_max_length = data.param4;

        VigridMapLog.Debug("VM_Settings enabled=" + markers_enabled + " minimap=" + minimap_allowed + " compass=" + compass_allowed);
    }

    /**
     *  The whole visible set, replacing whatever was held.
     *
     *  A snapshot rather than a delta because the set is tiny - a party of four is four markers -
     *  and a snapshot is idempotent: a dropped or duplicated packet cannot leave the client
     *  disagreeing with the server about what exists.
     */
    void VM_Markers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param5<int, ref array<string>, ref array<int>, ref array<vector>, ref array<string>> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        //--- Guaranteed delivery still does not guarantee ordering against a resync sent moments
        //--- later, so an older snapshot is discarded rather than applied.
        if (data.param1 < set_version)
        {
            VigridMapLog.Trace("Discarding stale marker set " + data.param1 + " < " + set_version);
            return;
        }

        //--- Length disagreement means the payload is malformed; applying it would index one array
        //--- against another's count later. Drop the whole thing.
        int count = data.param2.Count();
        if (data.param3.Count() != count || data.param4.Count() != count || data.param5.Count() != count)
        {
            VigridMapLog.Warn("Marker set arrays disagree on length - discarded");
            return;
        }

        set_version = data.param1;

        marker_owner_uids.Copy(data.param2);
        marker_owner_slots.Copy(data.param3);
        marker_positions.Copy(data.param4);
        marker_labels.Copy(data.param5);

        marker_seq = marker_seq + 1;

        VigridMapLog.Debug("VM_Markers version=" + set_version + " count=" + count);
    }

    //! A bare stringtable key, localised here rather than on the server - the server has no idea
    //! what language this client is running.
    void VM_Rejected(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        VigridMapLog.Debug("VM_Rejected " + data.param1);
    }
}
#endif
