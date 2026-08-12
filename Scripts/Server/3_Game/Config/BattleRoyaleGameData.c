#ifdef SERVER
class BattleRoyaleGameData: BattleRoyaleDataBase
{
	int version = 3;  // Config version

	// Allowed admins - Are immune to kick and can go outside the play area
	ref array<string> admins_steamid64 = {
		"123456789123456789" // Dummy SteamID64
	};

    int num_zones = 6;  // number of zones
    int round_duration_minutes = 5;  // round length in minutes

	bool enable_spawn_selection_menu = true;  // show spawn selection menu (0 = no, 1 = yes)
	bool gather_party_for_spawn_selection = true;  // pull party members next to their leader when the spawn map opens
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

    // Voice settings
    // While players are frozen (countdown, spawn selection, prepare) a player hears only their own
    // party. Solo players hear nobody. The lobby is deliberately not covered - it stays open voice.
    bool party_only_voice = true;  // restrict voice to party members while players are frozen
    bool show_speaking_players = true;  // show the on-screen list of who is currently speaking
    bool speaking_list_during_match = true;  // keep that list up after the match has started

    // How far a voice carries, per voice level, in metres. Used ONLY to decide who appears in the
    // speaking list - it does not change what players actually hear, which the engine decides and
    // does not expose. While party-only voice is active these are ignored, because membership then
    // answers audibility exactly. Tune these if the list names people you cannot hear, or misses
    // people you can.
    //
    // These are the community-reported vanilla ranges rather than measured ones - the engine does
    // not expose the real values anywhere in script, so they cannot be verified from code. Erring
    // small is the safer direction: a radius that is too large names people you cannot actually
    // hear, which is worse than occasionally missing someone at the edge of earshot.
    float voice_radius_whisper = 7;
    float voice_radius_talk = 25;
    float voice_radius_shout = 45;

	// Airdrop settings
    bool airdrop_enabled = true;  // Enable airdrops
    int airdrop_ignore_last_zones = 3;  // Number of last zones to ignore for airdrops

	// Clothing the player starts with
    ref array<string> player_starting_clothes = {
        "TrackSuitJacket_Red",
        "TrackSuitPants_Red",
        "JoggingShoes_Red"
    };

	// Items the player starts with
    ref array<string> player_starting_items = {
        "HuntingKnife",
        "BandageDressing",
        "Compass",
        "Battery9V",
        "Battery9V"
    };

    // Items that can be quickly accessed in the inventory hotbar
    // The index corresponds to the item in the player_starting_items array (0 = first item, 1 = second item, etc.)
    ref array<int> player_starting_items_shortcut = {
		0  // HuntingKnife
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
	}

	override void LoadMission()
	{
		string errorMessage;
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

	override void Upgrade()
	{
		// Future upgrades will be handled here
		if (version < 2)
		{
			// The key was INTRODUCED in v2, so a v1 file does not carry it and the field initialiser
			// above survives deserialization - there is nothing to set. Only fill an array that
			// genuinely came back empty; inserting unconditionally produced [0, 0], binding the knife
			// to two hotbar slots, which Save() then made permanent.
			if (!player_starting_items_shortcut || player_starting_items_shortcut.Count() == 0)
			{
				player_starting_items_shortcut = new array<int>();
				player_starting_items_shortcut.Insert(0);  // HuntingKnife
			}

			version = 2;
			Save();  // Save the upgraded config
		}

		if (version < 3)
		{
			// The voice keys were INTRODUCED in v3, so an older file does not carry them and the
			// field initialisers above survive deserialization. Nothing to migrate - the bump exists
			// so Save() writes the new keys into the existing profile JSON.
			version = 3;
			Save();  // Save the upgraded config
		}
	}
};
