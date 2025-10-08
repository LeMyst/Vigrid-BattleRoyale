#ifdef SERVER
class BattleRoyaleServerData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

	// Enable Vigrid API support
	bool enable_vigrid_api = false;

	// Server password (to pass it to the Vigrid API)
	// Must be the same as the one in the serverDZ.cfg
	string server_password = "changeme";

	// JWT Token for the Vigrid API webhook
    string webhook_jwt_token = "changeme";

    // Force getting a match UUID from the webhook, otherwise restart the server
    bool force_match_uuid = false;

    // If the server should warn the players if no UUID is received
    bool warning_no_uuid = false;

	// Autolock the server when a match starts
	// Gonna make a POST request to the autolock URL at the start of the match
	// Alternative to using the Vigrid API to lock the server
	// Autolock URL MUST BE in the format http(s)://<autolock_url>/autolock/<autolock_ip>/<autolock_port> and the payload must be a JSON object with the rcon_password field
	// Example: { "rcon_password": "mypassword" }
	bool use_autolock = false;

	string autolock_url = "https://api.vigrid.ovh/";  // Autolock API URL (mandatory if use_autolock is true)
	string autolock_ip = "";  // Server RCon IP (mandatory if use_autolock is true)
	int autolock_port = 2305;  // Server RCon Port (mandatory if use_autolock is true)
	string autolock_rcon_password = "";  // Server RCon Password (mandatory if use_autolock is true)

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "server_settings.json";
    }

    override string GetMissionPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "server_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyaleServerData>.LoadFile(GetProfilePath(), this, errorMessage))
				ErrorEx(errorMessage);
		}

		// Run the upgrade function here to avoid overrides from mission folder
		Upgrade();

		// No load from mission folder for this one
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyaleServerData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}
};
