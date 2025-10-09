#ifdef SERVER
class BattleRoyaleDataBase
{
	// Path to the config file from profile folder
	string GetProfilePath()
	{
		return "";
	}

	// Path to the config file from mission folder
	string GetMissionPath()
	{
		return "";
	}

	void Load() {}  // Load the config from the file
	void LoadMission() {}  // Load the config from the mission file
	void Save() {}  // Save the config to the file
	void Upgrade() {}  // Upgrade the config to the latest version if needed
};
