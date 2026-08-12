#ifdef SERVER
/**
 *  KillFeed - server settings, persisted at $profile:KillFeed\killfeed_settings.json.
 *
 *  Deliberately not built on the host mod's config base class: the discipline rule keeps this
 *  addon free of BattleRoyale* symbols, so it carries its own tiny load/save/upgrade. The shape
 *  mirrors the Battle Royale config classes on purpose, including the "load then immediately
 *  re-save" trick that makes fields added in a later version materialise in an existing server's
 *  JSON on next boot.
 */
class KillFeedData
{
    int version = 2;

    bool enabled = true;                  //!< master switch; false stops the server broadcasting
    bool show_distance = true;            //!< include the metre count on gunshot rows
    bool show_environment_deaths = true;  //!< zone, falls, starvation, infected and animals

    //--- Turn off other mods' kill feeds so a death is not announced twice. Only mods that are
    //--- actually loaded are touched; see KillFeedSuppress.
    bool suppress_other_killfeeds = true;

    string GetPath()
    {
        return KILLFEED_SETTINGS_FILE;
    }

    void Load()
    {
        string error_message;

        if (FileExist(GetPath()))
        {
            if (!JsonFileLoader<KillFeedData>.LoadFile(GetPath(), this, error_message))
                ErrorEx(error_message);
        }

        Upgrade();
    }

    void Save()
    {
        string error_message;
        if (!JsonFileLoader<KillFeedData>.SaveFile(GetPath(), this, error_message))
            ErrorEx(error_message);
    }

    void Upgrade()
    {
        if (version < 2)
        {
            //--- New in v2. Default it on: a server running this addon alongside another kill feed
            //--- was announcing every death twice, which is the reason the option exists.
            suppress_other_killfeeds = true;

            version = 2;
            Save();
        }
    }
}
#endif
