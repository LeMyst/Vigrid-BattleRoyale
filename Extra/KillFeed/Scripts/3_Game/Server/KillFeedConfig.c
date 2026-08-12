#ifdef SERVER
/**
 *  KillFeed - settings singleton. Creates the profile folder, loads killfeed_settings.json and
 *  writes it straight back so a fresh server ends up with a file it can edit.
 */
class KillFeedConfig
{
    private static ref KillFeedConfig m_Instance;

    private ref KillFeedData m_Settings;
    private bool m_HasLoaded;

    void KillFeedConfig()
    {
        m_Settings = new KillFeedData();
        m_HasLoaded = false;
    }

    static KillFeedConfig GetConfig()
    {
        if (!m_Instance)
        {
            m_Instance = new KillFeedConfig();
            m_Instance.Load();
        }

        return m_Instance;
    }

    void Load()
    {
        if (m_HasLoaded)
            return;

        m_HasLoaded = true;

        if (!FileExist(KILLFEED_SETTINGS_FOLDER))
        {
            KillFeedLog.Info("Creating settings folder " + KILLFEED_SETTINGS_FOLDER);
            MakeDirectory(KILLFEED_SETTINGS_FOLDER);
        }

        m_Settings.Load();

        //--- Re-save unconditionally: creates the file on a fresh server, and on an existing one
        //--- writes back any field added since it was last generated.
        m_Settings.Save();

        KillFeedLog.Info("Settings loaded (enabled=" + m_Settings.enabled + " show_distance=" + m_Settings.show_distance + ")");
    }

    KillFeedData GetSettings()
    {
        if (!m_HasLoaded)
            Load();

        return m_Settings;
    }
}
#endif
