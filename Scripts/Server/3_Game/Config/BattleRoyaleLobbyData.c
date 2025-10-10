#ifdef SERVER
class BattleRoyaleLobbyData: BattleRoyaleDataBase
{
	int version = 2;  // Config version

	// Lobby spawn location are in the spawns settings located in the mission folder.

    int minimum_players = 10;  // Minimum players to start the match
    int use_ready_up = 1;  // Use ready up system (F2 by default)
    float ready_up_percent = 0.8;  // Percentage of players needed to ready up to automatically start the match
    float min_waiting_time = 300.0;  // Minimum waiting time before the match can start
    int time_to_start_match_seconds = 30;  // Time to start the match after the minimum waiting time

	// Autostart settings
	// The autostart system will start the match automatically based on the number of maximum players the server can handle
	// More the time passes, the less players are needed to start the match
	// By example, if the server can handle 100 players and there is 50 players ready, the match will start at 50% of the autostart delay
	// If there is 75 players ready, the match will start at 25% of the autostart delay
	// If there is 100 players ready, the match will start immediately
	// The autostart system will not start the match if the minimum waiting time (min_waiting_time) is not reached
    bool autostart_enabled = true;  // Enable autostart
    float autostart_delay = 750.0;  // Delay before autostart

	// Items given to players when they spawn in the lobby
    ref array<string> player_lobby_items = {
        "TShirt_DBR",
        "Jeans_Black",
        "Sneakers_Black",
        "Apple",
        "Zucchini",
        "Apple"
    };

    // Items that can be quickly accessed in the inventory hotbar
    // The index corresponds to the item in the player_lobby_items array (0 = first item, 1 = second item, etc.)
    ref array<int> player_lobby_items_shortcut = {
		4,  // Zucchini
	};

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "lobby_settings.json";
    }

    override string GetMissionPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "lobby_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyaleLobbyData>.LoadFile(GetProfilePath(), this, errorMessage))
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
			if (!JsonFileLoader<BattleRoyaleLobbyData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);
		}
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyaleLobbyData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}

	override void Upgrade()
	{
		if (version < 2)
		{
			// Add default items to the lobby items
			player_lobby_items.Insert("TShirt_DBR");
			player_lobby_items.Insert("Jeans_Black");
			player_lobby_items.Insert("Sneakers_Black");
			player_lobby_items.Insert("Apple");
			player_lobby_items.Insert("Zucchini");
			player_lobby_items.Insert("Apple");

			// Set default shortcut to the Zucchini
			player_lobby_items_shortcut.Insert(4);  // Zucchini

			version = 2;
			Save();
		}
	}
};
