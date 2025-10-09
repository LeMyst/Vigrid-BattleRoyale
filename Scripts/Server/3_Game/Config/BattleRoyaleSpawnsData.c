#ifdef SERVER
class BattleRoyaleSpawnsData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

    vector spawn_point = "14829.2 0 14572.3";  // Lobby spawn point, change it related to the map
    float radius = 50;  // Lobby spawn point radius

    bool spawn_in_villages = true;  // Spawn players and groups in villages
    bool spawn_in_first_zone = true;  // Spawn players and groups in the first zone, otherwise they will spawn all around the map
    int extra_spawn_radius = 250;  // Extra radius around the first zone for spawning, allow player to spawn outside the first zone

	// Cities to avoid for spawning
	// Most of the time, you want to avoid cities because they have no loot
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

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "spawns_settings.json";
    }

    override string GetMissionPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "spawns_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyaleSpawnsData>.LoadFile(GetProfilePath(), this, errorMessage))
				ErrorEx(errorMessage);
		}

		// Run the upgrade function here to avoid overrides from mission folder
		Upgrade();
	}

	override void LoadMission()
	{
		string errorMessage;
		// Override from mission folder
		if (FileExist(GetMissionPath()))
		{
			if (!JsonFileLoader<BattleRoyaleSpawnsData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);
		}
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyaleSpawnsData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}
};
