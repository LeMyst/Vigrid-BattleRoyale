#ifndef SERVER
/**
 *  Vigrid Map - client-side preferences.
 *
 *  Separate from the server settings on purpose, and it has to be: the settings class is
 *  #ifdef SERVER, so a client cannot read it at all, and $profile: on a client resolves to the
 *  CLIENT's profile directory. A per-player choice therefore has nowhere else to live.
 *
 *  Two preferences, and their defaults differ on purpose: the minimap is OFF (opt-in - it is a
 *  200 px window nobody asked for) and the compass is ON (a thin strip answering a question the HUD
 *  could not answer at all). Each pairs with a harder server-side switch - minimap_allowed and
 *  compass_allowed - and effective visibility is `allowed && enabled`, so an admin can take either
 *  away entirely and a player can only ever opt further out, never in.
 *
 *  WRITING JSON TO $profile: FROM CLIENT SCRIPT HAS NO PRECEDENT IN THIS REPO. Every other
 *  JsonFileLoader write here is behind #ifdef SERVER, and the two client-side reads target packed
 *  mod-dir assets. So this degrades rather than insists: if the file cannot be written the value
 *  still works for the session, it simply does not survive a restart, and the failure is logged
 *  once rather than every time. That is a mild loss - the server re-pushes minimap_allowed every
 *  session anyway, and an unwritable profile just means the player re-presses the key next session.
 */
class VigridMapPrefsData
{
    int version = 1;

    //--- OFF by default: the minimap is opt-in, so a player who never presses the toggle never gets
    //--- a 200 px overlay they did not ask for. The admin gate (minimap_allowed) ships ON, so the
    //--- key works out of the box - "available but not shown" rather than "unavailable".
    //---
    //--- Flipping this initialiser only changes what a player with NO saved prefs file gets. Anyone
    //--- who already toggled it has `minimap_enabled` written in their map_client.json and keeps
    //--- their choice, which is the point of a default.
    bool minimap_enabled = false;

    //--- ON by default, unlike the minimap above, and the asymmetry is deliberate. The minimap is a
    //--- 200 px window onto another view; the compass is a thin strip that answers one question the
    //--- HUD could not answer at all, so the useful default is "there". The key toggles it away.
    //---
    //--- An existing player's map_client.json has no `compass_enabled` key, so it deserializes to
    //--- this initialiser and they get the compass on their next session - which is the intent.
    bool compass_enabled = true;

    void Upgrade()
    {
        //--- No migrations yet. Bump `version` and add a branch here when a field changes meaning;
        //--- adding a new field needs nothing, since a missing key keeps its initialiser.
        version = 1;
    }
}

class VigridMapPrefs
{
    private static ref VigridMapPrefsData m_Data;

    //--- Latched so a profile directory that cannot be written does not produce one log line per
    //--- toggle for the rest of the session.
    private static bool m_SaveFailed;

    private static VigridMapPrefsData Data()
    {
        if (!m_Data)
            Load();

        return m_Data;
    }

    private static void Load()
    {
        m_Data = new VigridMapPrefsData();

        if (!FileExist(VIGRID_MAP_PREFS_FILE))
        {
            VigridMapLog.Debug("No client prefs file, using defaults");
            //--- Written straight back out, so the file exists to be hand-edited and so a broken
            //--- profile path is discovered now rather than on the first toggle.
            Save();
            return;
        }

        string error_message;
        if (!JsonFileLoader<VigridMapPrefsData>.LoadFile(VIGRID_MAP_PREFS_FILE, m_Data, error_message))
        {
            //--- Warn, not ErrorEx: on this codebase a fatal here would take the client down over a
            //--- preference. A corrupt file just means defaults for the session.
            VigridMapLog.Warn("Could not read client prefs: " + error_message);
            m_Data = new VigridMapPrefsData();
            return;
        }

        m_Data.Upgrade();

        VigridMapLog.Debug("Client prefs loaded (minimap_enabled=" + m_Data.minimap_enabled + ")");
    }

    private static void Save()
    {
        if (!m_Data)
            return;

        if (!FileExist(VIGRID_MAP_SETTINGS_FOLDER))
            MakeDirectory(VIGRID_MAP_SETTINGS_FOLDER);

        string error_message;
        bool ok = JsonFileLoader<VigridMapPrefsData>.SaveFile(VIGRID_MAP_PREFS_FILE, m_Data, error_message);

        //--- Checked on disk as well as by return value: writing to a client $profile: path is
        //--- untested on this codebase, and a loader that reports success without producing a file
        //--- would otherwise look like it worked.
        if (ok && FileExist(VIGRID_MAP_PREFS_FILE))
            return;
        if (m_SaveFailed)
            return;

        m_SaveFailed = true;
        VigridMapLog.Warn("Could not write " + VIGRID_MAP_PREFS_FILE + " (" + error_message + ") - the minimap preference will work this session but not survive a restart");
    }

    static bool IsMinimapEnabled()
    {
        return Data().minimap_enabled;
    }

    static void SetMinimapEnabled(bool enabled)
    {
        Data().minimap_enabled = enabled;
        Save();
    }

    //! Returns the new state, so a caller can report it without asking again.
    static bool ToggleMinimap()
    {
        SetMinimapEnabled(!IsMinimapEnabled());
        return IsMinimapEnabled();
    }

    static bool IsCompassEnabled()
    {
        return Data().compass_enabled;
    }

    static void SetCompassEnabled(bool enabled)
    {
        Data().compass_enabled = enabled;
        Save();
    }

    static bool ToggleCompass()
    {
        SetCompassEnabled(!IsCompassEnabled());
        return IsCompassEnabled();
    }
}
#endif
