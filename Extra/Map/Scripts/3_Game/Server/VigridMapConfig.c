#ifdef SERVER
/**
 *  Vigrid Map - settings singleton. Creates the profile folder, loads map_settings.json and writes
 *  it straight back so a fresh server ends up with a file it can edit.
 */
class VigridMapConfig
{
    private static ref VigridMapConfig m_Instance;

    private ref VigridMapData m_Settings;
    private bool m_HasLoaded;

    void VigridMapConfig()
    {
        m_Settings = new VigridMapData();
        m_HasLoaded = false;
    }

    static VigridMapConfig GetConfig()
    {
        if (!m_Instance)
        {
            m_Instance = new VigridMapConfig();
            m_Instance.Load();
        }

        return m_Instance;
    }

    void Load()
    {
        if (m_HasLoaded)
            return;

        m_HasLoaded = true;

        if (!FileExist(VIGRID_MAP_SETTINGS_FOLDER))
        {
            VigridMapLog.Info("Creating settings folder " + VIGRID_MAP_SETTINGS_FOLDER);
            MakeDirectory(VIGRID_MAP_SETTINGS_FOLDER);
        }

        m_Settings.Load();

        //--- Re-save unconditionally: creates the file on a fresh server, and on an existing one
        //--- writes back any field added since it was last generated.
        m_Settings.Save();

        VigridMapLog.Info("Settings loaded (enabled=" + m_Settings.enabled + " minimap_allowed=" + m_Settings.minimap_allowed + " compass_allowed=" + m_Settings.compass_allowed + ")");
    }

    VigridMapData GetSettings()
    {
        if (!m_HasLoaded)
            Load();

        return m_Settings;
    }
}
#endif
