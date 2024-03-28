#ifdef SERVER
class BattleRoyaleZoneData: BattleRoyaleDataBase
{
    int shrink_type = 3; // Exp/Lin/Static/Const

    // constant
    float constant_scale = 0.65;

    // exponential
    float shrink_base = 2.718281828459; // ~ e
    float shrink_exponent = 3.0;
    bool end_in_villages = true;

    bool use_dynamic_zones = true;
    int min_zone_num = 4;

    //ref array<float> static_sizes = { 6000, 5000, 4000, 3000, 2000, 1000, 100 }; //note, this needs as many entries as Num_Rounds in GameSettings
    //ref array<float> static_sizes = { 4500, 3375, 2250, 1125, 562, 140, 35 };
    ref array<float> static_sizes = { 35, 140, 562, 1125, 2250, 3375, 4500 };
    //ref array<int> static_timers = { 75, 100, 132, 177, 236, 315, 420 };
    ref array<int> static_timers = { 155, 260, 307, 495, 495, 495, 495 };
    ref array<int> min_players = { 10, 10, 10, 11, 22, 33, 44 };

    ref array<string> end_avoid_type = {"DeerStand", "FeedShack", "Marine"};
    ref array<string> end_avoid_city = {"Camp_Shkolnik", "Ruin_Voron", "Settlement_Skalisty"};  // , "Local_Drakon", "Local_Otmel", "Ruin_Storozh", "Local_MB_PrisonIsland"

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "zone_settings.json";
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleZoneData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleZoneData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
