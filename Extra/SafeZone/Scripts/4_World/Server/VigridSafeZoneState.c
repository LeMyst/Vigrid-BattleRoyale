#ifdef SERVER
/**
 *  SafeZone - the authoritative flag, server side.
 *
 *  The truce is global rather than geographic: a host game that has a lobby phase already knows
 *  when peace applies, and asking it is cheaper and exact where polling positions against circle
 *  geometry is neither. That is the whole reason this addon is a fraction of the size of
 *  Expansion's zone system - there is no zone module, no actor list and no per-tick point-in-shape
 *  test to run.
 *
 *  The flag is mirrored onto every player as a netsync bool rather than broadcast over an RPC. The
 *  engine then replays it to a client that joins later for free, which is exactly the case an RPC
 *  broadcast would miss: someone connecting into an already-running lobby has to be disarmed too.
 */
class VigridSafeZoneState
{
    //--- Defaults to off, so dropping this PBO on a server that never calls the API changes nothing.
    private static bool s_Active;

    static bool IsActive()
    {
        return s_Active;
    }

    static void SetActive(bool active)
    {
        if (s_Active == active)
            return;

        s_Active = active;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players[i]);
            if (player)
                player.VigridSafeZone_Apply(active);
        }

        VigridSafeZoneLog.Info("Truce active=" + active + " applied to " + players.Count() + " player(s)");
    }
}
#endif
