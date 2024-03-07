#ifdef SERVER
class BattleRoyaleGameData: BattleRoyaleDataBase
{
    int num_zones = 6;
    int round_duration_minutes = 5; //round length in minutes

    int time_until_teleport_unlock = 10; //seconds before unlock after teleporting & preparing

    ref array<int> zone_notification_minutes = { 1, 2 };
    ref array<int> zone_notification_seconds = { 30, 10 };

    int debug_heal_tick_seconds = 5;
    int zone_damage_tick_seconds = 5;

    float zone_damage_delta = 0.1;
    bool enable_zone_damage = true;

    bool hide_players_endgame = false;

    bool spawn_in_villages = true;
    bool spawn_in_first_zone = true;
    bool show_first_zone_at_start = true;
    ref array<string> avoid_city_spawn = {
        "Camp_Shkolnik",
        "Hill_Zelenayagora",
        "Settlement_Kumyrna",
        "Ruin_Voron",
        "Settlement_Skalisty",
        "Settlement_Novoselki",
        "Settlement_Dubovo",
        "Settlement_Vysotovo"
    };

    ref array<ref BattleRoyaleOverrideSpawnPosition> override_spawn_positions;

    bool artillery_sound = true;

    ref array<string> player_starting_clothes = {
        "TrackSuitJacket_Red",
        "TrackSuitPants_Red",
        "JoggingShoes_Red"
    };

    ref array<string> player_starting_items = {
        "HuntingKnife",
        "BandageDressing",
        "Compass",
        "Battery9V",
        "Battery9V"
    };

    string mission = "BattleRoyale.ChernarusPlus";

    bool use_spectate_whitelist = true;
    bool auto_spectate_mode = true;
    ref array<string> allowed_spectate_steamid64 = {
        "123456789123456789" // Dummy SteamID64
    };

    //--- do not modify
    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "general_settings.json";
    }

    override void Save()
    {
        JsonFileLoader<BattleRoyaleGameData>.JsonSaveFile(GetPath(), this);
    }

    override void Load()
    {
        JsonFileLoader<BattleRoyaleGameData>.JsonLoadFile(GetPath(), this);
    }
};

class BattleRoyaleOverrideSpawnPosition
{
    string CityName;
    vector NewPosition;
};
#endif