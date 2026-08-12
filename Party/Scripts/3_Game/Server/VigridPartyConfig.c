#ifdef SERVER
/**
 *  Vigrid Party - settings singleton. Creates the profile folder, loads party_settings.json and
 *  writes it straight back so a fresh server ends up with a documented file it can edit.
 */
class VigridPartyConfig
{
    private static ref VigridPartyConfig m_Instance;

    private ref VigridPartyData m_Settings;
    private bool m_HasLoaded;

    void VigridPartyConfig()
    {
        m_Settings = new VigridPartyData();
        m_HasLoaded = false;
    }

    static VigridPartyConfig GetConfig()
    {
        if (!m_Instance)
        {
            m_Instance = new VigridPartyConfig();
            m_Instance.Load();
        }

        return m_Instance;
    }

    void Load()
    {
        if (m_HasLoaded)
            return;

        m_HasLoaded = true;

        if (!FileExist(VIGRID_PARTY_SETTINGS_FOLDER))
        {
            VigridPartyLog.Info("Creating settings folder " + VIGRID_PARTY_SETTINGS_FOLDER);
            MakeDirectory(VIGRID_PARTY_SETTINGS_FOLDER);
        }

        m_Settings.Load();

        //--- Re-save unconditionally: creates the file on a fresh server, and on an existing one
        //--- writes back any field added since it was last generated.
        m_Settings.Save();

        VigridPartyLog.Info("Settings loaded (enabled=" + m_Settings.enabled + " max_party_size=" + m_Settings.max_party_size + ")");
    }

    VigridPartyData GetSettings()
    {
        if (!m_HasLoaded)
            Load();

        return m_Settings;
    }
}
#endif
