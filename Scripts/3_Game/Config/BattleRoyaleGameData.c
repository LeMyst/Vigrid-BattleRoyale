class BattleRoyaleGameData extends BattleRoyaleDataBase
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

    bool spawn_in_villages = true;

    ref array<string> player_starting_items = {
        "TrackSuitJacket_Red",
        "TrackSuitPants_Red",
        "JoggingShoes_Red",
        "MedicalBandage"
    };

    string mission = "BattleRoyale.ChernarusPlusGloom";

    bool use_spectate_whitelist = true;
    bool auto_spectate_mode = true;
    ref array<string> allowed_spectate_steamid64 = {
        "123456789123456789" // Dummy SteamID64
    };

    int num_vehicles = 1000; // TODO: Move this to world config
    int vehicle_ticktime_ms = 1000;
    float vehicle_spawn_radius = 500.0;

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
}
