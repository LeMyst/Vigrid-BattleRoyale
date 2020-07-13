class BattleRoyaleGameData extends BattleRoyaleDataBase
{
    int num_zones = 7;
    
    int round_duration_minutes = 5; //5 minute rounds
    int time_until_teleport_unlock = 10; //seconds before unlock after teleporting & preparing

    ref array<int> zone_notification_minutes = { 1, 2 };
    ref array<int> zone_notification_seconds = { 30, 10 };

    debug_heal_tick_seconds = 5;
    zone_damage_tick_seconds = 5;


    ref array<string> player_starting_items = {
        "TrackSuitJacket_Red",
        "TrackSuitPants_Red",
        "JoggingShoes_Red"
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
}