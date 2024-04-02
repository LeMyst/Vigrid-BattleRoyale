#ifdef SERVER
class BattleRoyaleSpawnsData: BattleRoyaleDataBase
{
    bool spawn_in_villages = true;
    bool spawn_in_first_zone = true;
    int extra_spawn_radius = 250;

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

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "spawns_settings.json";
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleSpawnsData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleSpawnsData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
