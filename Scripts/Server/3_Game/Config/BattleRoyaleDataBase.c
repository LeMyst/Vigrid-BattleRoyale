#ifdef SERVER
class BattleRoyaleDataBase
{
	// Path to the config file
    string GetPath()
    {
        return "";
    }

    void Load() {}  // Load the config from the file
    void Save() {}  // Save the config to the file
    void Upgrade() {}  // Upgrade the config to the latest version if needed
};
#endif