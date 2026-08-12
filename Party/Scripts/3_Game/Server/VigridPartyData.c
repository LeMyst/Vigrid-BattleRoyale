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
    int version = 2;

    bool enabled = VIGRID_PARTY_DEF_ENABLED;
    int max_party_size = VIGRID_PARTY_DEF_MAX_SIZE;
    int invite_ttl_seconds = VIGRID_PARTY_DEF_INVITE_TTL;
    int party_ttl_hours = VIGRID_PARTY_DEF_PARTY_TTL_HOURS;
    int state_push_interval_ms = VIGRID_PARTY_DEF_STATE_INTERVAL_MS;

    float nametag_max_distance = VIGRID_PARTY_DEF_NAMETAG_MAX_DIST; //!< 0 = unlimited
    float nametag_min_alpha = VIGRID_PARTY_DEF_NAMETAG_MIN_ALPHA;

    bool show_hud_panel = true;
    bool leader_transfer_on_disconnect = true;

    bool ping_enabled = VIGRID_PARTY_DEF_PING_ENABLED;
    int ping_max_per_player = VIGRID_PARTY_DEF_PING_MAX;
    int ping_ttl_seconds = VIGRID_PARTY_DEF_PING_TTL_SECONDS; //!< 0 = never expires
    int ping_cooldown_ms = VIGRID_PARTY_DEF_PING_COOLDOWN_MS;

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
        if (version < 1)
            version = 1;

        //--- 1 -> 2 added the four ping_* fields, and the migration is intentionally empty: a
        //--- member absent from the JSON keeps the initialiser it is declared with above, which is
        //--- already the intended default. Load()'s unconditional re-save is what writes the new
        //--- keys into an existing server's file. Only a migration that has to *derive* a value
        //--- from an old one needs code here.
        if (version < 2)
            version = 2;
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

        //--- Floors at 1, not 0: ping_enabled is the off switch, and a cap of 0 would present as a
        //--- keybind that silently does nothing.
        if (ping_max_per_player < 1)
            ping_max_per_player = 1;
        if (ping_max_per_player > 10)
            ping_max_per_player = 10;

        //--- 0 is legal and means "never expires" - the behaviour Carim always had.
        if (ping_ttl_seconds < 0)
            ping_ttl_seconds = 0;
        if (ping_ttl_seconds > 3600)
            ping_ttl_seconds = 3600;

        if (ping_cooldown_ms < 0)
            ping_cooldown_ms = 0;
        if (ping_cooldown_ms > 60000)
            ping_cooldown_ms = 60000;
    }
}
#endif
