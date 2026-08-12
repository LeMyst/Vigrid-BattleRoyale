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
    ref array<int> state_health = new array<int>();
    ref array<int> state_blood = new array<int>();
    ref array<int> state_flags = new array<int>();
    int state_recv_ms = 0;
    int state_prev_recv_ms = 0;
    ref array<vector> state_prev_positions = new array<vector>();

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
        state_health.Clear();
        state_blood.Clear();
        state_flags.Clear();
        state_prev_positions.Clear();
        state_recv_ms = 0;
        state_prev_recv_ms = 0;

        ClearInvite();
        invite_seq = 0;

        list_uids.Clear();
        list_names.Clear();
        list_flags.Clear();
        list_seq = 0;

        pending_notifications.Clear();
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

        locked = data.param1;
    }

    void VP_Roster(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param7<int, string, int, int, array<string>, array<string>, bool> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
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
        state_health.Clear();
        state_blood.Clear();
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
        state_health.Copy(data.param3);
        state_blood.Copy(data.param4);
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

    void VP_Notify(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param3<string, string, string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Client)
            return;

        //--- The server sends a bare stringtable key; localisation happens here.
        StringLocaliser message = new StringLocaliser(data.param1, data.param2, data.param3);
        pending_notifications.Insert(message.Format());
    }
}
#endif
