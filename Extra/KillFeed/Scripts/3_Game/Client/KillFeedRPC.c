#ifndef SERVER
/**
 *  KillFeed - client-side RPC receiver.
 *
 *  A queue of plain data, exactly like BattleRoyaleRPC: the handler stores, it never acts. The
 *  5_Mission UI drains `pending` every frame. Keeping it that way is what allows this class to
 *  live in 3_Game, which cannot reference anything above it - and MissionGameplay is 5_Mission.
 */
class KillFeedRPC
{
    private static ref KillFeedRPC m_Instance;

    //--- Entries received but not yet handed to the UI. Drained, not polled: a feed is a stream of
    //--- events, so nothing here is a "current value" the UI could diff against.
    ref array<ref KillFeedEntry> pending = new array<ref KillFeedEntry>();

    void KillFeedRPC()
    {
        GetRPCManager().AddRPC(RPC_KILLFEED_NAMESPACE, KF_RPC_ENTRY, this);
        KillFeedLog.Debug("Client RPC registered");
    }

    static KillFeedRPC GetInstance()
    {
        if (!m_Instance)
            m_Instance = new KillFeedRPC();

        return m_Instance;
    }

    /**
     *  Drop anything not yet shown. Called when the mission starts, because the singleton outlives
     *  a server change and a stale kill would otherwise pop up on the next server's HUD.
     */
    void Reset()
    {
        pending.Clear();
    }

    //! Handler name must match KF_RPC_ENTRY exactly - CF's AddRPC dispatches by method name.
    void KF_Entry(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param6<string, string, string, string, int, int> data;
        if (!ctx.Read(data))
        {
            Error("FAILED TO READ KF_ENTRY RPC");
            return;
        }

        if (type != CallType.Client)
            return;

        KillFeedLog.Trace(string.Format("KF_Entry: %1 -> %2 (%3) cause=%6 dist=%5 att=%4",
            data.param1, data.param2, data.param3, data.param4, data.param5, data.param6));

        pending.Insert(new KillFeedEntry(data.param1, data.param2, data.param3, data.param4, data.param5, data.param6));
    }
}
#endif
