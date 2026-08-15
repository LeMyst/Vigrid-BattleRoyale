#ifndef SERVER
/**
 *  EarPlugs - client-side preference.
 *
 *  One value: the level the player last chose. It is persisted because the reference implementation
 *  did not, and re-plugging your ears after every reconnect is exactly the kind of small friction
 *  that makes a comfort feature not get used.
 *
 *  Same shape as Extra/Map's VigridMapPrefs, which is the proven pattern in this repo for writing
 *  JSON to $profile: from CLIENT script - every other JsonFileLoader write here is behind
 *  #ifdef SERVER. It degrades rather than insists: if the file cannot be written the level still
 *  works for the session, it simply does not survive a restart, and the failure is logged once
 *  rather than on every keypress.
 */
class VigridEarPlugsPrefsData
{
    int version = 1;

    //--- Off by default, and Off is 0, so a missing key, a corrupt file and an unwritable profile
    //--- all degrade to "no earplugs" rather than to "silently deaf with no idea why".
    int level = 0;

    void Upgrade()
    {
        //--- No migrations yet. Bump `version` and add a branch here when a field changes meaning;
        //--- adding a new scalar field needs nothing, since a missing key keeps its initialiser. A
        //--- `ref array` WOULD need a branch - an array initialiser does not survive deserialization
        //--- and would load back empty.
        version = 1;
    }
}

class VigridEarPlugsPrefs
{
    private static ref VigridEarPlugsPrefsData m_Data;

    //--- Latched so a profile directory that cannot be written does not produce one log line per
    //--- keypress for the rest of the session.
    private static bool m_SaveFailed;

    private static VigridEarPlugsPrefsData Data()
    {
        if (!m_Data)
            Load();

        return m_Data;
    }

    private static void Load()
    {
        m_Data = new VigridEarPlugsPrefsData();

        if (!FileExist(VIGRID_EARPLUGS_PREFS_FILE))
        {
            VigridEarPlugsLog.Debug("No client prefs file, starting at Off");
            //--- Written straight back out, so the file exists to be hand-edited and so a broken
            //--- profile path is discovered now rather than on the first keypress.
            Save();
            return;
        }

        string error_message;
        if (!JsonFileLoader<VigridEarPlugsPrefsData>.LoadFile(VIGRID_EARPLUGS_PREFS_FILE, m_Data, error_message))
        {
            //--- Warn, not ErrorEx: a fatal here would take the client down over a comfort setting.
            //--- A corrupt file just means Off for the session.
            VigridEarPlugsLog.Warn("Could not read client prefs: " + error_message);
            m_Data = new VigridEarPlugsPrefsData();
            return;
        }

        m_Data.Upgrade();

        //--- The file is hand-editable and may predate a change in how many levels there are, so a
        //--- stale value must not reach VigridEarPlugsLevels.Factor and mean something unintended.
        m_Data.level = VigridEarPlugsLevels.Clamp(m_Data.level);

        VigridEarPlugsLog.Debug("Client prefs loaded (level=" + VigridEarPlugsLevels.DebugName(m_Data.level) + ")");
    }

    private static void Save()
    {
        if (!m_Data)
            return;

        if (!FileExist(VIGRID_EARPLUGS_SETTINGS_FOLDER))
            MakeDirectory(VIGRID_EARPLUGS_SETTINGS_FOLDER);

        string error_message;
        bool ok = JsonFileLoader<VigridEarPlugsPrefsData>.SaveFile(VIGRID_EARPLUGS_PREFS_FILE, m_Data, error_message);

        //--- Checked on disk as well as by return value: a loader that reports success without
        //--- producing a file would otherwise look like it worked.
        if (ok && FileExist(VIGRID_EARPLUGS_PREFS_FILE))
            return;
        if (m_SaveFailed)
            return;

        m_SaveFailed = true;
        VigridEarPlugsLog.Warn("Could not write " + VIGRID_EARPLUGS_PREFS_FILE + " (" + error_message + ") - the level will work this session but not survive a restart");
    }

    static int GetLevel()
    {
        return Data().level;
    }

    static void SetLevel(int level)
    {
        Data().level = VigridEarPlugsLevels.Clamp(level);
        Save();
    }
}
#endif
