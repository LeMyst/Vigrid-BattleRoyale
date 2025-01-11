#ifdef SERVER
class BattleRoyaleSpawnsData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

    bool spawn_in_villages = true;  // Spawn players and groups in villages
    bool spawn_in_first_zone = true;  // Spawn players and groups in the first zone, otherwise they will spawn all around the map
    int extra_spawn_radius = 250;  // Extra radius around the first zone for spawning

	// Cities to avoid for spawning
	// Most of the time, you want to avoid cities with no loot
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

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleSpawnsData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleSpawnsData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
