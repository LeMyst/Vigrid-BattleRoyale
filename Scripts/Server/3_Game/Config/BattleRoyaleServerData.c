#ifdef SERVER
class BattleRoyaleServerData: BattleRoyaleDataBase
{
	int version = 6;  // Config version

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
    // Only has an effect when enable_vigrid_api is true
    bool warning_no_uuid = true;

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

	// Write $profile:Vigrid-BattleRoyale\server_lock.txt: "1" when the match countdown starts, "0" at boot.
	// Nothing reads it unless the server runs the dzhooks native hook library with its scriptlock or
	// adminbypass module pointed at that file (DZHOOK_LOCKFILE / DZHOOK_ADMINBYPASS_FILE), in which case
	// the server locks itself natively - no RCon, no remote API. Harmless without the hook.
	// Only the FILE. With the hook's serverlock module (on by default) the mod uses its natives
	// instead and only ever writes "0" at boot, to clear a stale "1"; this switch does not gate the natives.
	// See Docs/native-hooks.md.
	bool write_lock_file = true;

	// Look the Steam persona name up for players who never set a name in the launcher.
	// PlayerIdentity cannot be renamed, so this only changes what the mod itself displays
	// (party HUD, nametags, kill feed, leaderboard, win screen); vanilla chat is unaffected.
	bool enable_steam_name_lookup = false;

	// Steam Web API key, from https://steamcommunity.com/dev/apikey
	// Mandatory if enable_steam_name_lookup is true. Never logged.
	string steam_web_api_key = "";

	// Names considered "the player never picked one". Matched case-insensitively against the
	// unprocessed nick, so the engine's " (2)" duplicate suffix does not need listing.
	ref array<string> placeholder_player_names = {"Survivor", "Player"};

	// Persist resolved names to steam_names.json. Worth leaving on: the server process restarts
	// between matches, so without it every match re-queries every returning player.
	bool cache_steam_names = true;

	// How long a cached name stays trusted. Past this, the next time that player connects with a
	// placeholder name their persona is looked up again - the cached name is still applied
	// immediately, so nobody waits on the request. 0 or less = never expires.
	// This only ever costs a lookup on a connect that was going to use the cache anyway: a player
	// who set a name of their own in the launcher never triggers one.
	int steam_name_cache_max_age_hours = 168;  // 7 days

	// Start the next match in the same server process instead of restarting it. Needs the dzhooks
	// native hook library with its ce_refill module (DZHOOK_CE_REFILL=1, serverkit 7c13ada or later):
	// the map loot is expired with GetCEApi().TimeShift and refilled by the hook while players wait in
	// the lobby. Without the module, or when anything goes wrong, the server restarts as before.
	// Also raise CleanupLifetimeLimit to 5000 in the mission's db/globals.xml, or the old loot takes
	// over an hour to drain. See Docs/in-place-reset.md.
	bool in_place_reset = false;

	// What happens to the players still connected when a match ends (in-place reset only).
	// false: they stay connected and get a fresh body in the lobby for the next match.
	// true:  everyone is disconnected and the server reopens the lobby; a full server lets players
	//        waiting for a slot try again.
	bool in_place_reset_kick_all = false;

	// What happens to a player who dies (in-place reset only; ignored with in_place_reset_kick_all).
	// false: as before, the death screen sends them back to the main menu, which keeps the player
	//        count, and the server's load, down while the match runs.
	// true:  the death screen (or the spectator camera) waits for the next match instead, and the
	//        player gets a fresh lobby body when the match ends.
	bool in_place_reset_keep_dead_players = false;

	// Restart the server process anyway after this many in-place matches. A full restart reloads the
	// settings files and clears anything an in-place reset cannot. 0 = never.
	int in_place_reset_max_matches = 10;

	// Seconds between expiring the old loot and refilling the map. The drain halves what is left
	// about every 50 s (~93% gone after ~190 s with CleanupLifetimeLimit 5000). The refill also runs,
	// at the latest, when the start countdown begins.
	int in_place_reset_drain_seconds = 200;

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "server_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyaleServerData>.LoadFile(GetProfilePath(), this, errorMessage))
			{
				ErrorEx(errorMessage);
				m_ProfileLoadFailed = true;
				return;  // a half-parsed old version would make Upgrade() Save() over the file
			}
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

	override void Upgrade()
	{
		if (version < 2)
		{
			// warning_no_uuid was never read before v2, the warning was always shown.
			// Enable it so the behaviour is unchanged for existing servers.
			warning_no_uuid = true;

			version = 2;
			Save();  // Save the upgraded config
		}

		if (version < 3)
		{
			// The Steam-name keys were INTRODUCED in v3, so an older file does not carry them - but
			// unlike the scalar keys, the array field initialiser does NOT survive deserialization of
			// an existing file: it comes back empty, which would mean "no name is ever a placeholder"
			// and silently disable the feature on every server that already had a server_settings.json.
			// Same trap, same fix as BattleRoyaleGameData.Upgrade(): refill only when it came back
			// genuinely empty, so an admin who deliberately cleared the list keeps their choice.
			if (!placeholder_player_names || placeholder_player_names.Count() == 0)
			{
				placeholder_player_names = new array<string>();
				placeholder_player_names.Insert("Survivor");
				placeholder_player_names.Insert("Player");
			}

			version = 3;
			Save();  // Save the upgraded config
		}

		if (version < 4)
		{
			// steam_name_cache_max_age_hours was introduced in v4. It is a scalar, so unlike the
			// array above its field initialiser DOES survive deserialization of an existing file -
			// there is nothing to refill here. The bump exists only so the Save() below writes the
			// new key into server_settings.json, where an admin can find it.
			version = 4;
			Save();  // Save the upgraded config
		}

		if (version < 5)
		{
			// write_lock_file was introduced in v5. A scalar, so its initialiser survives deserialization;
			// the bump only makes the Save() below write the new key where an admin can find it.
			version = 5;
			Save();  // Save the upgraded config
		}

		if (version < 6)
		{
			// The five in_place_reset* keys were introduced in v6. All scalars, so their initialisers
			// survive deserialization; the bump only makes the Save() below write them where an admin
			// can find them. in_place_reset defaults to false, so nothing changes until it is set.
			version = 6;
			Save();  // Save the upgraded config
		}
	}
};
