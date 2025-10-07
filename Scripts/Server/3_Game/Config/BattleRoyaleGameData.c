#ifdef SERVER
class BattleRoyaleGameData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

	// Allowed admins - Are immune to kick and can go outside the play area
	ref array<string> admins_steamid64 = {
		"123456789123456789" // Dummy SteamID64
	};

    int num_zones = 6;  // number of zones
    int round_duration_minutes = 5;  // round length in minutes

	bool enable_spawn_selection_menu = true;  // show spawn selection menu (0 = no, 1 = yes)
	int spawn_selection_duration = 30;  // spawn selection duration in seconds
	int spawn_selection_extra_time = 2;  // extra time between spawn selection and next state in seconds
	float spawn_selection_radius = 50;  // radius where the player can spawn
	bool show_spawn_heatmap = true;  // show spawn heatmap (0 = no, 1 = yes)

    int time_until_teleport_unlock = 10;  // seconds before unlock after teleporting & preparing

    ref array<int> zone_notification_minutes = { 1, 2 };  // minutes when notification about the zone shrinking will be displayed
    ref array<int> zone_notification_seconds = { 30, 10 };  // seconds when notification about the zone shrinking will be displayed, when under the minute

    int debug_heal_tick_seconds = 5;  // seconds between debug (lobby) heal ticks

    // Zone damage settings
    int zone_damage_tick_seconds = 5;  // seconds between zone damage ticks
    float zone_damage_delta = 0.1;  // damage per tick
    bool enable_zone_damage = true;  // enable zone damage

    bool hide_players_endgame = false;  // Hide the number of players left in the endgame

    bool show_first_zone_at_start = true;  // Show the first zone at the start of the game

    bool artillery_sound = true;  // Play the artillery sound when the zone shrinks

	// Airdrop settings
    bool airdrop_enabled = true;  // Enable airdrops
    int airdrop_ignore_last_zones = 3;  // Number of last zones to ignore for airdrops

    ref array<string> player_starting_clothes = {
        "TrackSuitJacket_Red",
        "TrackSuitPants_Red",
        "JoggingShoes_Red"
    };

    ref array<string> player_starting_items = {
        "HuntingKnife",
        "BandageDressing",
        "Compass",
        "Battery9V",
        "Battery9V"
    };

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "general_settings.json";
    }

    override string GetMissionPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "general_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyaleGameData>.LoadFile(GetProfilePath(), this, errorMessage))
				ErrorEx(errorMessage);
		}

		// Run the upgrade function here to avoid overrides from mission folder
		Upgrade();

		// Override from mission folder
		if (FileExist(GetMissionPath()))
		{
			if (!JsonFileLoader<BattleRoyaleGameData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);
		}
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyaleGameData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}
};
