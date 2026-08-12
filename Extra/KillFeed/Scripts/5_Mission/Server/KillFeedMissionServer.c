#ifdef SERVER
/**
 *  KillFeed - server boot.
 *
 *  Exists so the settings file materialises when the server starts rather than when the first
 *  player dies. An admin looking for killfeed_settings.json should find it on a fresh server, not
 *  have to get someone killed first.
 */
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();

        //--- Touching the singleton is what creates the profile folder and writes the file.
        KillFeedConfig.GetConfig();

        KillFeedSuppress.Apply();

        //--- Re-apply once the mission has settled. Another mod's settings may finish loading after
        //--- this point, which would restore the switch we just cleared; nobody can die in the
        //--- meantime, so a single late pass is enough.
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "SuppressOtherKillFeeds", 10000, false);

        KillFeedLog.Info("KillFeed " + KILLFEED_VERSION + " ready");
    }

    //! Second pass, see OnInit. Safe to run when nothing needs suppressing - it no-ops.
    void SuppressOtherKillFeeds()
    {
        KillFeedSuppress.Apply();
    }
}
#endif
