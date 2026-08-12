#ifndef SERVER
/**
 *  Vigrid Party - client-side RPC receiver.
 *
 *  A bag of plain fields, exactly like BattleRoyaleRPC: handlers store, they never act. The
 *  5_Mission controller polls these every frame. Keeping it that way is what allows this class to
 *  live in 3_Game and reference nothing above it.
 *
 *  The *_seq counters are how the controller notices a change without diffing whole arrays: it
 *  keeps its own last-seen value and compares.
 */
class VigridPartyRPC
{
    private static ref VigridPartyRPC m_Instance;

    //--- Settings, mirrored from the server. Defaults apply until the first VP_Settings arrives.
    bool enabled = VIGRID_PARTY_DEF_ENABLED;
    int max_party_size = VIGRID_PARTY_DEF_MAX_SIZE;
    int invite_ttl_seconds = VIGRID_PARTY_DEF_INVITE_TTL;
    float nametag_max_distance = VIGRID_PARTY_DEF_NAMETAG_MAX_DIST;
    float nametag_min_alpha = VIGRID_PARTY_DEF_NAMETAG_MIN_ALPHA;
    bool locked = false;

    //--- Roster. version 0 with an empty uid list means "not in a party".
    int roster_version = 0;
    string party_id = "";
    int self_index = -1;
    int leader_index = -1;
    ref array<string> roster_uids = new array<string>();
    ref array<string> roster_names = new array<string>();
    int roster_seq = 0;

    //--- Live member state, parallel to roster_uids. Only valid while state_version matches
    //--- roster_version.
    int state_version = 0;
    ref array<vector> state_positions = new array<vector>();

    //--- Vanilla EStatLevels, 0 (GREAT) to 4 (CRITICAL), decided server-side by the same
    //--- PlayerBase calls that drive the player's own HUD badge. Not a percentage - do not
    //--- compare these against health values.
    ref array<int> state_health_level = new array<int>();
    ref array<int> state_blood_level = new array<int>();
    ref array<int> state_flags = new array<int>();
    int state_recv_ms = 0;
    int state_prev_recv_ms = 0;
    ref array<vector> state_prev_positions = new array<vector>();

    //--- Ping settings, mirrored from the server on VP_PingSettings. Only the two the client
    //--- actually consumes are sent: this one gates the keybind and hides the layer, and the
    //--- cooldown lets a mashed key be refused without generating traffic.
    bool ping_enabled = VIGRID_PARTY_DEF_PING_ENABLED;
    int ping_cooldown_ms = VIGRID_PARTY_DEF_PING_COOLDOWN_MS;

    //--- The party's world markers. Parallel arrays, but self-describing: every entry names its
    //--- owner, so unlike state_* they never have to line up with the roster.
    ref array<string> ping_owner_uids = new array<string>();
    ref array<vector> ping_positions = new array<vector>();
    ref array<int> ping_expire_ms = new array<int>(); //!< local clock, absolute; 0 = never
    int ping_recv_ms = 0;

    //--- Pending invitation, if any.
    string invite_id = "";
    string invite_inviter_uid = "";
    string invite_inviter_name = "";
    int invite_expires_ms = 0;
    int invite_seq = 0;

    //--- Last player list reply.
    ref array<string> list_uids = new array<string>();
    ref array<string> list_names = new array<string>();
    ref array<int> list_flags = new array<int>();
    int list_seq = 0;

    //--- Notifications the controller has not surfaced yet.
    ref array<string> pending_notifications = new array<string>();

#ifdef DIAG_DEVELOPER
    //--- Set while the diag menu is driving a fabricated party. See DebugSuppressesPush.
    bool debug_fake_session = false;
#endif

    void VigridPartyRPC()
    {
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_SETTINGS, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_LOCKED, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_ROSTER, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_TEAMSTATE, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_INVITE_RECEIVED, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_INVITE_CANCELLED, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PLAYERLIST, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_NOTIFY, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PING_SETTINGS, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PING_SET, this);
    }

    static VigridPartyRPC GetInstance()
    {
        if (!m_Instance)
            m_Instance = new VigridPartyRPC();

        return m_Instance;
    }

    /**
     *  Wipe everything that describes the current session. Called when the mission starts, because
     *  the singleton outlives a server change and stale roster data would otherwise render on the
     *  next server's HUD.
     */
    void Reset()
    {
        enabled = VIGRID_PARTY_DEF_ENABLED;
        max_party_size = VIGRID_PARTY_DEF_MAX_SIZE;
        invite_ttl_seconds = VIGRID_PARTY_DEF_INVITE_TTL;
        nametag_max_distance = VIGRID_PARTY_DEF_NAMETAG_MAX_DIST;
        nametag_min_alpha = VIGRID_PARTY_DEF_NAMETAG_MIN_ALPHA;
        locked = false;

        roster_version = 0;
        party_id = "";
        self_index = -1;
        leader_index = -1;
        roster_uids.Clear();
        roster_names.Clear();
        roster_seq = 0;

        state_version = 0;
        state_positions.Clear();
        state_health_level.Clear();
        state_blood_level.Clear();
        state_flags.Clear();
        state_prev_positions.Clear();
        state_recv_ms = 0;
        state_prev_recv_ms = 0;

        ping_enabled = VIGRID_PARTY_DEF_PING_ENABLED;
        ping_cooldown_ms = VIGRID_PARTY_DEF_PING_COOLDOWN_MS;
        ping_owner_uids.Clear();
        ping_positions.Clear();
        ping_expire_ms.Clear();
        ping_recv_ms = 0;

        ClearInvite();
        invite_seq = 0;

        list_uids.Clear();
        list_names.Clear();
        list_flags.Clear();
        list_seq = 0;

        pending_notifications.Clear();

#ifdef DIAG_DEVELOPER
        debug_fake_session = false;
#endif
    }

    /**
     *  Whether an incoming server push should be discarded rather than applied.
     *
     *  True only while the diag menu is driving a fabricated party. The menu polls the server for
     *  the online player list every three seconds and VP_PlayerList clears the list arrays
     *  unconditionally, so without this a fabricated list survives at most one poll on a live
     *  server - and a real VP_Roster would do the same to a fabricated roster.
     *
     *  Guarded inside the body rather than at every call site: the handlers must read identically
     *  in both builds, and a release build compiles this down to a constant false.
     *
     *  Deliberately NOT consulted by VP_Settings, VP_PingSettings or VP_Notify - none of them
     *  carries roster or list state, and keeping VP_Settings live is what makes max_party_size
     *  truthful while faking, so "party full hides the Invite button" stays reachable.
     */
    bool DebugSuppressesPush()
    {
#ifdef DIAG_DEVELOPER
        if (debug_fake_session)
            return true;
#endif

        return false;
    }

    bool HasParty()
    {
        return roster_uids.Count() > 1;
    }

    void ClearInvite()
    {
        invite_id = "";
        invite_inviter_uid = "";
        invite_inviter_name = "";
        invite_expires_ms = 0;
    }

    bool HasInvite()
    {
        return invite_id != "";
    }

    // ---------------------------------------------------------------- handlers

    void VP_Settings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param5<bool, int, int, float, float> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        enabled = data.param1;
        max_party_size = data.param2;
        invite_ttl_seconds = data.param3;
        nametag_max_distance = data.param4;
        nametag_min_alpha = data.param5;

        VigridPartyLog.Debug("VP_Settings enabled=" + enabled + " max=" + max_party_size);
    }

    void VP_Locked(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<bool> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;
        if (DebugSuppressesPush())
            return;

        locked = data.param1;
    }

    void VP_Roster(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param7<int, string, int, int, array<string>, array<string>, bool> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;
        if (DebugSuppressesPush())
            return;

        roster_version = data.param1;
        party_id = data.param2;
        self_index = data.param3;
        leader_index = data.param4;

        roster_uids.Clear();
        roster_names.Clear();

        if (data.param5)
            roster_uids.Copy(data.param5);
        if (data.param6)
            roster_names.Copy(data.param6);

        locked = data.param7;

        //--- Member state belongs to the previous roster; drop it rather than let the renderer
        //--- index a stale array against a new roster for one interval.
        state_version = 0;
        state_positions.Clear();
        state_health_level.Clear();
        state_blood_level.Clear();
        state_flags.Clear();
        state_prev_positions.Clear();

        roster_seq = roster_seq + 1;

        VigridPartyLog.Debug("VP_Roster v" + roster_version + " members=" + roster_uids.Count());
    }

    void VP_TeamState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param5<int, array<vector>, array<int>, array<int>, array<int>> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        if (DebugSuppressesPush())
            return;

        //--- Sent unguaranteed and possibly in flight across a roster change, so anything built
        //--- against a different roster version is discarded rather than mis-indexed.
        if (data.param1 != roster_version)
            return;
        if (!data.param2)
            return;
        if (data.param2.Count() != roster_uids.Count())
            return;

        //--- Keep the previous sample so distant members can be interpolated between pushes
        //--- instead of teleporting once per interval.
        state_prev_positions.Clear();
        state_prev_positions.Copy(state_positions);
        state_prev_recv_ms = state_recv_ms;

        state_version = data.param1;
        state_positions.Copy(data.param2);
        state_health_level.Copy(data.param3);
        state_blood_level.Copy(data.param4);
        state_flags.Copy(data.param5);
        state_recv_ms = GetGame().GetTime();

        if (state_prev_positions.Count() != state_positions.Count())
        {
            state_prev_positions.Clear();
            state_prev_positions.Copy(state_positions);
            state_prev_recv_ms = state_recv_ms;
        }
    }

    void VP_InviteReceived(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param4<string, string, string, int> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        if (DebugSuppressesPush())
            return;

        invite_id = data.param1;
        invite_inviter_uid = data.param2;
        invite_inviter_name = data.param3;
        invite_expires_ms = GetGame().GetTime() + data.param4 * 1000;
        invite_seq = invite_seq + 1;
    }

    void VP_InviteCancelled(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;
        if (data.param1 != invite_id)
            return;
        if (DebugSuppressesPush())
            return;

        ClearInvite();
        invite_seq = invite_seq + 1;
    }

    void VP_PlayerList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param3<array<string>, array<string>, array<int>> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        if (DebugSuppressesPush())
            return;

        list_uids.Clear();
        list_names.Clear();
        list_flags.Clear();

        if (data.param1)
            list_uids.Copy(data.param1);
        if (data.param2)
            list_names.Copy(data.param2);
        if (data.param3)
            list_flags.Copy(data.param3);

        list_seq = list_seq + 1;
    }

    void VP_PingSettings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param2<bool, int> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        ping_enabled = data.param1;
        ping_cooldown_ms = data.param2;

        VigridPartyLog.Debug("VP_PingSettings enabled=" + ping_enabled + " cooldown=" + ping_cooldown_ms);
    }

    /**
     *  The party's whole ping set, replacing whatever was held. Idempotent by design, so there is
     *  nothing to reconcile and no sequence number to track.
     */
    void VP_PingSet(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param3<array<string>, array<vector>, array<int>> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        if (DebugSuppressesPush())
            return;

        //--- Cleared first: an empty set is how the server says "you have no markers", and it
        //--- arrives as three empty arrays rather than as a distinct message.
        ping_owner_uids.Clear();
        ping_positions.Clear();
        ping_expire_ms.Clear();
        ping_recv_ms = GetGame().GetTime();

        if (!data.param1)
            return;
        if (!data.param2)
            return;
        if (!data.param3)
            return;

        //--- The renderer indexes all three with one counter, so a length disagreement is dropped
        //--- rather than half-applied. Same guard VP_TeamState uses.
        if (data.param1.Count() != data.param2.Count())
            return;
        if (data.param1.Count() != data.param3.Count())
            return;

        ping_owner_uids.Copy(data.param1);
        ping_positions.Copy(data.param2);

        //--- Milliseconds remaining arrive, not an absolute time: the server's clock and ours are
        //--- unrelated. Converted against the local clock here, exactly as VP_InviteReceived does.
        //--- 0 travels as "never expires".
        int count = data.param3.Count();
        for (int i = 0; i < count; i++)
        {
            int remaining = data.param3.Get(i);
            if (remaining <= 0)
                ping_expire_ms.Insert(0);
            else
                ping_expire_ms.Insert(ping_recv_ms + remaining);
        }

        VigridPartyLog.Debug("VP_PingSet count=" + ping_owner_uids.Count());
    }

    void VP_Notify(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param3<string, string, string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        //--- An argument can itself be a stringtable key - VIGRID_PARTY_UNKNOWN_NAME_KEY, when the
        //--- server has no name for the member a message is about. Resolve those before handing
        //--- them over: whether StringLocaliser recurses into its own arguments is not something
        //--- worth depending on. Guarded on the leading '#', so a real player name is never touched.
        string arg1 = data.param2;
        string arg2 = data.param3;
        if (arg1.IndexOf("#") == 0)
            arg1 = Widget.TranslateString(arg1);
        if (arg2.IndexOf("#") == 0)
            arg2 = Widget.TranslateString(arg2);

        //--- The server sends a bare stringtable key; localisation happens here.
        StringLocaliser message = new StringLocaliser(data.param1, arg1, arg2);
        pending_notifications.Insert(message.Format());
    }
}
#endif
