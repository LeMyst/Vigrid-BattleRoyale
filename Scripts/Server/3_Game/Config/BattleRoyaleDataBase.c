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

	//--- Clamp settings that are internally inconsistent, so a misconfiguration degrades into a
	//--- playable match instead of halting boot. Distinct from Upgrade(), which migrates between
	//--- versions: Upgrade runs inside Load(), before the mission pass, while Validate runs after
	//--- BOTH passes so a mission override is checked too.
	//---
	//--- An implementation MUST NOT call Save(). BattleRoyaleConfig.Load() already re-saves before
	//--- the mission pass, so persisting a clamp here would overwrite the admin's intent in their
	//--- profile JSON permanently - the clamp is meant to be in memory, for this boot only.
	void Validate() {}
};
