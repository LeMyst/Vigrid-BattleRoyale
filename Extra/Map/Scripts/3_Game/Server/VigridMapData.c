#ifdef SERVER
/**
 *  Vigrid Map - server settings, persisted at $profile:Vigrid-Map\map_settings.json.
 *
 *  Deliberately not built on the host mod's config base class: the discipline rule keeps this addon
 *  free of BattleRoyale* symbols, so it carries its own tiny load/save/upgrade. The shape mirrors
 *  the other addons' config classes on purpose, including the "load then immediately re-save" trick
 *  that makes fields added in a later version materialise in an existing server's JSON on next boot.
 *
 *  Note what is NOT here: any notion of expiry. Markers are permanent by design - that is the line
 *  between them and party pings, which do have a TTL.
 */
class VigridMapData
{
    int version = 1;

    bool enabled = true;                    //!< master switch for marker placement
    bool minimap_allowed = true;            //!< may clients show the HUD minimap at all
    int label_max_length = 32;              //!< marker labels are truncated to this server-side

    //--- A teammate logging out should not erase the objective the squad is walking to, so their
    //--- marker outlives them by default. Admins who disagree can flip this.
    bool clear_markers_on_disconnect = false;

    string GetPath()
    {
        return VIGRID_MAP_SETTINGS_FILE;
    }

    void Load()
    {
        string error_message;

        if (FileExist(GetPath()))
        {
            if (!JsonFileLoader<VigridMapData>.LoadFile(GetPath(), this, error_message))
                ErrorEx(error_message);
        }

        Upgrade();
    }

    void Save()
    {
        string error_message;
        if (!JsonFileLoader<VigridMapData>.SaveFile(GetPath(), this, error_message))
            ErrorEx(error_message);
    }

    //! Nothing to migrate yet - version 1 is the first shipped shape. The branch is here so the
    //! first field added later has an obvious home.
    void Upgrade()
    {
    }
}
#endif
