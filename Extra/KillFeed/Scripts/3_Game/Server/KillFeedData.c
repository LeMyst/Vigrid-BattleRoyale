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
    int version = 1;

    bool enabled = true;                  //!< master switch; false stops the server broadcasting
    bool show_distance = true;            //!< include the metre count on gunshot rows
    bool show_environment_deaths = true;  //!< zone, falls, starvation, infected and animals

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
        //--- No migrations yet. When one is needed: add an `if (version == N)` branch that fills
        //--- the new fields, set version to N+1, then Save().
        if (version < 1)
            version = 1;
    }
}
#endif
