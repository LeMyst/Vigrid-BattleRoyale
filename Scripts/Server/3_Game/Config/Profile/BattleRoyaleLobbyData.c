#ifdef SERVER
class BattleRoyaleLobbyData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

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

    ref array<string> allowed_outside_lobby = {
        "123456789123456789" // Dummy SteamID64
    };

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "lobby_settings.json";
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleLobbyData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleLobbyData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
