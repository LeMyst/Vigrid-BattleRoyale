class BattleRoyaleGameData extends BattleRoyaleDataBase
{
    int num_zones = 7;
    int round_duration_minutes = 8; //round length in minutes


    int time_until_teleport_unlock = 10; //seconds before unlock after teleporting & preparing

    ref array<int> zone_notification_minutes = { 1, 2 };
    ref array<int> zone_notification_seconds = { 30, 10 };

    int debug_heal_tick_seconds = 5;
    int zone_damage_tick_seconds = 5;

    float zone_damage_delta = 0.1;


    bool enable_zone_damage = true;


    ref array<string> player_starting_items = {
        "TrackSuitJacket_Red",
        "TrackSuitPants_Red",
        "JoggingShoes_Red",
        "MedicalBandage" //zombies at start can be brutal. this will help
    };


    bool use_spectate_whitelist = true;
    ref array<string> allowed_spectate_steamid64 = {
        "76561198277370562" //kegan's steam id :)
    };


    int num_vehicles = 1000;
    int vehicle_ticktime_ms = 1000;
    float vehicle_spawn_radius = 10.0;

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