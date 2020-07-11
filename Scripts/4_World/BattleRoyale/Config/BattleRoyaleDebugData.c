class BattleRoyaleDebugData extends BattleRoyaleDataBase
{

    vector spawn_point = "14829.2 72.3148 14572.3";
    float radius = 50;
    int minimum_players = 10;


    override string GetPath()
    {
        return BATTLEROYALE_FOLDER + "debug_settings.json";
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