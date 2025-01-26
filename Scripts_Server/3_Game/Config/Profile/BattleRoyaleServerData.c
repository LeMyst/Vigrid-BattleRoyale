#ifdef SERVER
class BattleRoyaleServerData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

	// Enable Vigrid API support
	bool enable_vigrid_api = false;

	// JWT Token for the Vigrid API webhook
    string webhook_jwt_token = "changeme";

    // Force getting a match UUID from the webhook, otherwise restart the server
    bool force_match_uuid = false;

    // If the server should warn the players if no UUID is received
    bool warning_no_uuid = false;

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "server_settings.json";
    }

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleServerData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleServerData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
