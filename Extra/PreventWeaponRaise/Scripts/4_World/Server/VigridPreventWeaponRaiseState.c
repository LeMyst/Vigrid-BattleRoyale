#ifdef SERVER
/**
 *  PreventWeaponRaise - the authoritative switch, server side.
 *
 *  The suppression itself is a client-side concern: vanilla's whole CheckLiftWeapon body sits inside
 *  an INSTANCETYPE_CLIENT branch, so the client is the only side that ever decides to lift. But the
 *  switch that turns the suppression off is a serverDZ.cfg key, and ServerConfigGetInt is a
 *  server-only API - on a client there is no server config to parse and it answers 0 for every key.
 *  So the server reads it and mirrors the answer down to each player as a netsync bool; see
 *  PlayerBase.c.
 *
 *  Read once per process rather than per connect. The file is parsed at boot and never changes
 *  under a running server, so re-asking is pure waste - and the original version of this feature
 *  asked once per frame, from inside the command handler.
 */
class VigridPreventWeaponRaiseState
{
    private static bool s_Resolved;
    private static bool s_Disabled;

    /**
     *  Is the addon switched off by the admin?
     *
     *  An absent key reads as 0, which is indistinguishable from an explicit 0 - both mean "leave the
     *  addon active". That is the safe default: every server that has never heard of this key keeps
     *  the behaviour it has today.
     */
    static bool IsDisabled()
    {
        if (!s_Resolved)
        {
            s_Disabled = GetGame().ServerConfigGetInt("BRDisablePreventWeaponRaise") == 1;
            s_Resolved = true;
        }

        return s_Disabled;
    }
}
#endif
