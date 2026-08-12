#ifdef SERVER
/**
 *  Vigrid Map - server lifecycle and the client->server commands.
 *
 *  Commands never carry a subject: the actor is resolved from the RPC sender identity and every
 *  check is re-run here. A client is free to send anything, so nothing it sends about *who* it is
 *  may be believed.
 *
 *  Handler methods must be named EXACTLY like their registered strings - CF dispatches by method
 *  name.
 */
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();

        //--- Touch the config so the settings file materialises on a fresh server rather than on
        //--- the first placement.
        VigridMapConfig.GetConfig();

        GetRPCManager().AddRPC(RPC_VIGRIDMAP_SERVER_NAMESPACE, VM_RPC_PLACE, this);
        GetRPCManager().AddRPC(RPC_VIGRIDMAP_SERVER_NAMESPACE, VM_RPC_REMOVE, this);
        GetRPCManager().AddRPC(RPC_VIGRIDMAP_SERVER_NAMESPACE, VM_RPC_REQUEST_SYNC, this);

        VigridMapLog.Debug("MissionServer::OnInit done");
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        VigridMapMarkerStore.GetInstance().Update();
    }

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);

        VigridMapMarkerStore store = VigridMapMarkerStore.GetInstance();
        store.SendSettings(identity);
        store.PushTo(player);
    }

    override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
    {
        //--- PlayerDisconnected is handed the hashed GetId(), not the SteamID64 the store is keyed
        //--- by, so the plain id is taken from the identity while it is still available.
        if (identity)
            VigridMapMarkerStore.GetInstance().OnPlayerDisconnected(identity.GetPlainId());

        super.PlayerDisconnected(player, identity, uid);
    }

    /**
     *  Tell the sender their request was refused.
     *
     *  Sent for EVERY refusal, including the ones with nothing to say - `key` is then empty and the
     *  client shows no message. The signal is not the text, it is that an answer came at all: the
     *  client draws the request optimistically, so silence reads as acceptance until the prediction
     *  times out two seconds later and the marker jumps back with no explanation.
     *
     *  A CORRECTIVE SNAPSHOT WAS TRIED HERE FIRST AND CANNOT WORK. Pushing the authoritative set back
     *  looks like the more honest answer, but a refusal does not bump m_SetVersion, so the push
     *  carries the version the client already holds and is indistinguishable from the five-second
     *  resync. The client's prediction test is deliberately content-based - that is what stops an
     *  unrelated resync retiring a move early - so it would read the corrective push as "still
     *  waiting" and hold the prediction anyway. The refusal has to be its own message.
     */
    private void RejectRequest(PlayerIdentity sender, string key)
    {
        GetRPCManager().SendRPC(RPC_VIGRIDMAP_NAMESPACE, VM_RPC_REJECTED, new Param1<string>(key), true, sender);
    }

    //! Place or move the sender's marker.
    void VM_Place(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param2<vector, string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        VigridMapMarkerStore store = VigridMapMarkerStore.GetInstance();
        if (!store.IsActive())
        {
            //--- The one refusal with a reason worth reading.
            RejectRequest(sender, "STR_MAP_MARKERS_OFF");
            return;
        }

        //--- Re-resolved from the identity rather than trusted from the payload, and re-checked
        //--- even though the client checks too: a modified client is not bound by our UI.
        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        if (!player.IsAlive())
        {
            RejectRequest(sender, "");
            return;
        }

        //--- False is the cooldown or a position outside the world. Both are deliberately silent to
        //--- the player, but neither may be silent to the client.
        if (!store.Place(sender, player, data.param1, data.param2))
            RejectRequest(sender, "");
    }

    //! Clear the sender's own marker. Carries no id: there is only ever one candidate, and letting
    //! a client name a marker would let it delete someone else's.
    void VM_Remove(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        //--- False means there was nothing to remove, so Remove pushed nothing and the client is
        //--- left predicting a deletion that will never be confirmed.
        if (!VigridMapMarkerStore.GetInstance().Remove(sender, player))
            RejectRequest(sender, "");
    }

    //! Answered with settings AND the marker snapshot, so a client that loaded late catches up in
    //! one round trip instead of waiting for the resync tick.
    void VM_RequestSync(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        VigridMapMarkerStore store = VigridMapMarkerStore.GetInstance();
        store.SendSettings(sender);
        store.PushTo(player);
    }

    private PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
    {
        if (!identity)
            return NULL;

        string uid = identity.GetPlainId();

        array<Man> men = new array<Man>();
        GetGame().GetPlayers(men);

        int count = men.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = PlayerBase.Cast(men.Get(i));
            if (!candidate || !candidate.GetIdentity())
                continue;
            if (candidate.GetIdentity().GetPlainId() != uid)
                continue;

            return candidate;
        }

        return NULL;
    }
}
#endif
