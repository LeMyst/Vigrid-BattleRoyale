class BattleRoyaleGameData extends BattleRoyaleDataBase
{
    //todo: add json config values here
    int num_zones = 7;
    
    int round_duration_minutes = 5; //5 minute rounds

    ref array<int> zone_notification_minutes = { 1, 2 }
    ref array<int> zone_notification_seconds = { 30, 10 }

    //--- do not modify    
    override string GetPath()
    {
        return BATTLEROYALE_FOLDER + "general_settings.json";
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