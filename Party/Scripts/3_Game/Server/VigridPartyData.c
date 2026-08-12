#ifdef SERVER
/**
 *  Vigrid Party - server settings, persisted at $profile:Vigrid-Party\party_settings.json.
 *
 *  Deliberately not built on BattleRoyaleDataBase: the discipline rule keeps this addon free of
 *  BattleRoyale* symbols, so it carries its own tiny load/save/upgrade instead. The shape mirrors
 *  the Battle Royale config classes on purpose, including the "load then immediately re-save" trick
 *  that makes fields added in a later version materialise in an existing server's JSON on next boot.
 */
class VigridPartyData
{
    int version = 1;

    bool enabled = VIGRID_PARTY_DEF_ENABLED;
    int max_party_size = VIGRID_PARTY_DEF_MAX_SIZE;
    int invite_ttl_seconds = VIGRID_PARTY_DEF_INVITE_TTL;
    int party_ttl_hours = VIGRID_PARTY_DEF_PARTY_TTL_HOURS;
    int state_push_interval_ms = VIGRID_PARTY_DEF_STATE_INTERVAL_MS;

    float nametag_max_distance = VIGRID_PARTY_DEF_NAMETAG_MAX_DIST; //!< 0 = unlimited
    float nametag_min_alpha = VIGRID_PARTY_DEF_NAMETAG_MIN_ALPHA;

    bool show_hud_panel = true;
    bool leader_transfer_on_disconnect = true;

    string GetPath()
    {
        return VIGRID_PARTY_SETTINGS_FILE;
    }

    void Load()
    {
        string error_message;

        if (FileExist(GetPath()))
        {
            if (!JsonFileLoader<VigridPartyData>.LoadFile(GetPath(), this, error_message))
                ErrorEx(error_message);
        }

        Upgrade();
        Clamp();
    }

    void Save()
    {
        string error_message;
        if (!JsonFileLoader<VigridPartyData>.SaveFile(GetPath(), this, error_message))
            ErrorEx(error_message);
    }

    void Upgrade()
    {
        //--- No migrations yet. When one is needed: bump the constant, add an `if (version == N)`
        //--- branch that fills the new fields, set version to N+1, then Save().
        if (version < 1)
            version = 1;
    }

    /**
     *  A hand-edited JSON is the norm on a live server, so treat every value as hostile. Without
     *  this, max_party_size = 0 silently makes every invite fail and a 0 ms push interval turns
     *  the state channel into a per-frame broadcast.
     */
    void Clamp()
    {
        if (max_party_size < 2)
            max_party_size = 2;
        if (max_party_size > 16)
            max_party_size = 16;

        if (invite_ttl_seconds < 5)
            invite_ttl_seconds = 5;
        if (invite_ttl_seconds > 600)
            invite_ttl_seconds = 600;

        if (party_ttl_hours < 1)
            party_ttl_hours = 1;

        if (state_push_interval_ms < 100)
            state_push_interval_ms = 100;
        if (state_push_interval_ms > 5000)
            state_push_interval_ms = 5000;

        if (nametag_max_distance < 0)
            nametag_max_distance = 0;

        if (nametag_min_alpha < 0)
            nametag_min_alpha = 0;
        if (nametag_min_alpha > 1)
            nametag_min_alpha = 1;
    }
}
#endif
