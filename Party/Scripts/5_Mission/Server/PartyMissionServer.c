#ifdef SERVER
/**
 *  Vigrid Party - server lifecycle.
 *
 *  A second `modded class MissionServer` alongside the Battle Royale one. Both chain through
 *  super, so the order the engine applies them in does not matter - but every override here must
 *  keep calling super or it will silently cut the other mod's hook out of the chain.
 */
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();

        VigridPartyManager.CreateInstance();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        VigridPartyManager party_manager = VigridPartyManager.GetInstance();
        if (party_manager)
            party_manager.Update();
    }

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);

        VigridPartyManager party_manager = VigridPartyManager.GetInstance();
        if (party_manager)
            party_manager.OnPlayerConnected(identity);
    }

    /**
     *  `uid` here is identity.GetId() (hashed), not the SteamID64 Party keys on, and `identity`
     *  can be null on the delayed-logout path - so hand both to the manager and let it resolve.
     */
    override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
    {
        VigridPartyManager party_manager = VigridPartyManager.GetInstance();
        if (party_manager)
            party_manager.OnPlayerDisconnected(identity, uid);

        super.PlayerDisconnected(player, identity, uid);
    }
}
#endif
