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
			{
				ErrorEx(errorMessage);
				m_ProfileLoadFailed = true;
				return;  // a half-parsed old version would make Upgrade() Save() over the file
			}
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
			//--- Restored if the mission pass empties it - see BattleRoyaleDataBase.CopyStrings.
			ref array<string> keptAvoid = CopyStrings(avoid_city_spawn);

			if (!JsonFileLoader<BattleRoyaleSpawnsData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);

			if (!avoid_city_spawn || avoid_city_spawn.Count() == 0)
				avoid_city_spawn = keptAvoid;
		}
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyaleSpawnsData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}

	//--- The default spawn_point is a ChernarusPlus coordinate, used as-is on every other map. Warned,
	//--- never moved: the lobby is the admin's choice. Must not Save() - see BattleRoyaleDataBase.Validate().
	override void Validate()
	{
		if (!GetGame() || !GetGame().GetWorld())
			return;

		int world_size = GetGame().GetWorld().GetWorldSize();
		float x = spawn_point[0];
		float z = spawn_point[2];

		if (world_size > 0 && (x < 0 || z < 0 || x > world_size || z > world_size))
		{
			BattleRoyaleUtils.Warn(string.Format("[Spawns] lobby spawn_point %1 is outside this %2 m world - set spawn_point in spawns_settings.json for this map", spawn_point, world_size));
			return;
		}

		if (GetGame().SurfaceIsSea(x, z))
			BattleRoyaleUtils.Warn(string.Format("[Spawns] lobby spawn_point %1 is in the sea - set spawn_point in spawns_settings.json for this map", spawn_point));
		else if (GetGame().SurfaceIsPond(x, z))
			BattleRoyaleUtils.Warn(string.Format("[Spawns] lobby spawn_point %1 is in a pond - set spawn_point in spawns_settings.json for this map", spawn_point));
	}
};
#endif
