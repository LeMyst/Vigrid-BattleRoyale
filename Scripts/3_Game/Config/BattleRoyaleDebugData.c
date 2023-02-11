class BattleRoyaleDebugData extends BattleRoyaleDataBase
{
    vector spawn_point = "14829.2 0 14572.3";
    float radius = 50;
    int minimum_players = 10;
    int use_ready_up = 1;
    float ready_up_percent = 0.8;
    int time_to_start_match_seconds = 30;

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "debug_settings.json";
    }

    override void Save()
    {
        JsonFileLoader<BattleRoyaleDebugData>.JsonSaveFile(GetPath(), this);
    }

    override void Load()
    {
        JsonFileLoader<BattleRoyaleDebugData>.JsonLoadFile(GetPath(), this);

    }
}
