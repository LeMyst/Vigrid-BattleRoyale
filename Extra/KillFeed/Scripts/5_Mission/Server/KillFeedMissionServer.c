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

        KillFeedLog.Info("KillFeed " + KILLFEED_VERSION + " ready");
    }
}
#endif
