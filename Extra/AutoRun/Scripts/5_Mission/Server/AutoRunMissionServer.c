#ifdef SERVER
/**
 *  Auto-Run - server lifecycle and the one client->server command.
 *
 *  The command carries no subject: the actor is resolved from the RPC sender identity, never from
 *  anything the client says about who it is. Its single value is clamped, because a modified client
 *  is not bound by our UI.
 *
 *  Handler methods must be named EXACTLY like their registered strings - CF dispatches by method
 *  name.
 */
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();

        GetRPCManager().AddRPC(RPC_VIGRIDAUTORUN_SERVER_NAMESPACE, VA_RPC_SET_SPEED, this);

        VigridAutoRunLog.Debug("MissionServer::OnInit done");
    }

    /**
     *  Release the hold the moment the client goes, not when the body is finally removed.
     *
     *  A player who disconnects mid auto-run leaves a combat-logout body behind, and the override on
     *  it does not care that nobody is driving any more - the body would keep sprinting for the
     *  whole logout timer.
     */
    override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
    {
        if (identity)
            VigridAutoRunState.Clear(player, identity.GetPlainId());

        super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
    }

    /**
     *  Hold, change or release the sender's movement speed.
     *
     *  The client applies the same override locally; this is the server's copy of it. Vanilla's own
     *  RPC_DAYZPLAYER_DEBUGSERVERWALK (dayzplayerimplement.c:3723-3733) is the precedent - a
     *  client->server message whose whole body is OverrideMovementSpeed on the server's player.
     */
    void VA_SetSpeed(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<int> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        PlayerBase player = VigridAutoRun_FindPlayer(sender);
        if (!player)
            return;

        int speed = data.param1;
        if (speed < VIGRID_AUTORUN_SPEED_OFF)
            speed = VIGRID_AUTORUN_SPEED_OFF;
        if (speed > VIGRID_AUTORUN_SPEED_SPRINT)
            speed = VIGRID_AUTORUN_SPEED_SPRINT;

        //--- Re-checked here even though the client checks too, for the same reason every other
        //--- server handler in this repo does: a modified client is not bound by our UI. A dead or
        //--- unconscious player is released rather than refused, so a stale hold cannot survive.
        if (speed != VIGRID_AUTORUN_SPEED_OFF)
        {
            if (!player.IsAlive())
                speed = VIGRID_AUTORUN_SPEED_OFF;
            else if (player.IsUnconscious())
                speed = VIGRID_AUTORUN_SPEED_OFF;
        }

        VigridAutoRunState.Apply(player, sender.GetPlainId(), speed);
    }

    /**
     *  Resolve the sender's own player object.
     *
     *  ⚠️ VANILLA MissionServer HAS NO SUCH HELPER - assuming it did cost one build. Extra/Map has an
     *  identical private one, and that is precisely why this one is prefixed: two `modded class
     *  MissionServer` blocks in different PBOs both declaring `GetPlayerByIdentity` would be
     *  redeclaring the same method on the same class.
     */
    private PlayerBase VigridAutoRun_FindPlayer(PlayerIdentity identity)
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
}
#endif
