#ifdef SERVER
class BattleRoyaleDebugData: BattleRoyaleDataBase
{
    vector spawn_point = "14829.2 0 14572.3";
    float radius = 50;
    int minimum_players = 10;
    int use_ready_up = 1;
    float ready_up_percent = 0.8;
    float min_waiting_time = 300.0;
    int time_to_start_match_seconds = 30;

    ref array<string> allowed_outside_spawn = {
        "123456789123456789" // Dummy SteamID64
    };

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "debug_settings.json";
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleDebugData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleDebugData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
