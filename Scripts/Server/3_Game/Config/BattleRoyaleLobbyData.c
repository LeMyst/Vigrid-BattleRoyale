#ifdef SERVER
class BattleRoyaleLobbyData: BattleRoyaleDataBase
{
	int version = 3;  // Config version

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

	// Forced team size - "duos", "trios", and so on.
	// At 1 (the default) nothing happens and everyone who did not build a party plays solo.
	// At 2 or more, the server fills every player into a party of at least this size the instant the
	// lobby closes: undersized existing parties are topped up first, then the remaining solos are
	// grouped at random - and being drawn at random is also what picks each new party's leader.
	// A party that already meets the size is never touched and never split.
	// This can only ever REDUCE the number of teams, and the match ends as soon as one team is left,
	// so it will always stop short rather than collapse the lobby to a single party. When that
	// happens somebody stays short-handed and the server log says so.
    int min_party_size = 1;

	// What to do with the players left over once no more full parties can be made from them - there
	// are always fewer than min_party_size of them, so one rule or the other has to bend:
	//   0 = put them in the smallest team going. Nobody plays alone, but if every team is already at
	//       max_party_size (party_settings.json) then one of them ends up over that cap.
	//   1 = let them form one short-handed team of their own. Never exceeds max_party_size.
	//   2 = leave them solo. Team sizes stay exact and somebody plays alone.
	// USE 1 IF min_party_size EQUALS max_party_size. Mode 0 has nowhere under the cap to put anyone,
	// so it stacks the whole remainder onto one team - measured at min 4 / max 4, a team of 6.
    int min_party_remainder = 0;

	// Diagnostic. Runs N synthetic auto-group passes at boot and logs the resulting team sizes,
	// then plays normally. 0 = off.
	// Worth doing once after changing min_party_size: a local test rig only reaches three players,
	// which is not enough to produce a four-way split, a top-up that also has a remainder, or the
	// max_party_size overflow. This answers "can this configuration strand a player" in one boot.
    int auto_group_selftest = 0;

    int debug_heal_tick_seconds = 5;  // seconds between debug (lobby) heal ticks

	// Spawn selection settings - the pre-match map where players pick where to drop in
	bool enable_spawn_selection_menu = true;  // show spawn selection menu (0 = no, 1 = yes)
	bool gather_party_for_spawn_selection = true;  // pull party members next to their leader when the spawn map opens
	int spawn_selection_duration = 30;  // spawn selection duration in seconds
	int spawn_selection_extra_time = 2;  // extra time between spawn selection and next state in seconds
	float spawn_selection_radius = 50;  // radius where the player can spawn
	bool show_spawn_heatmap = true;  // show spawn heatmap (0 = no, 1 = yes)

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
			// Both keys were INTRODUCED in v2, so a v1 file has neither and deserialization leaves the
			// field initialisers above untouched - there is nothing to add. Only fill an array that
			// genuinely came back empty; inserting unconditionally is what used to double the loadout
			// to 12 items and give a shortcut array of [4, 4], which Save() then made permanent.
			if (!player_lobby_items || player_lobby_items.Count() == 0)
			{
				player_lobby_items = new array<string>();
				player_lobby_items.Insert("TShirt_DBR");
				player_lobby_items.Insert("Jeans_Black");
				player_lobby_items.Insert("Sneakers_Black");
				player_lobby_items.Insert("Apple");
				player_lobby_items.Insert("Zucchini");
				player_lobby_items.Insert("Apple");
			}

			if (!player_lobby_items_shortcut || player_lobby_items_shortcut.Count() == 0)
			{
				player_lobby_items_shortcut = new array<int>();
				player_lobby_items_shortcut.Insert(4);  // Zucchini
			}

			version = 2;
			Save();
		}

		if (version < 3)
		{
			// v3 added min_party_size, min_party_remainder and auto_group_selftest. All three are
			// scalars, so a v2 file simply has no such key and deserialization leaves the field
			// initialisers above in place - which are already the intended defaults, and default to
			// the behaviour the server had before this version existed. Load()'s re-save is what
			// materialises the keys in an existing server's JSON. Nothing to migrate.
			version = 3;
			Save();
		}
	}

	//--- Runs after the mission override, so a mission that sets these is checked too. Must not
	//--- Save() - see BattleRoyaleDataBase.Validate().
	override void Validate()
	{
		if (min_party_size < 1)
		{
			BattleRoyaleUtils.Warn(string.Format("[Lobby] min_party_size %1 is below 1, clamping to 1 (auto-grouping off)", min_party_size));
			min_party_size = 1;
		}

		if (min_party_size > BR_MIN_PARTY_SIZE_CEILING)
		{
			BattleRoyaleUtils.Warn(string.Format("[Lobby] min_party_size %1 exceeds %2, clamping", min_party_size, BR_MIN_PARTY_SIZE_CEILING));
			min_party_size = BR_MIN_PARTY_SIZE_CEILING;
		}

		if (min_party_remainder < 0 || min_party_remainder > 2)
		{
			BattleRoyaleUtils.Warn(string.Format("[Lobby] min_party_remainder %1 is not 0, 1 or 2, using 0", min_party_remainder));
			min_party_remainder = 0;
		}

		if (auto_group_selftest < 0)
			auto_group_selftest = 0;
		if (auto_group_selftest > BR_AUTO_GROUP_SELFTEST_MAX)
			auto_group_selftest = BR_AUTO_GROUP_SELFTEST_MAX;

		//--- Warned about rather than clamped: it is a legal configuration, it just cannot deliver
		//--- what it asks for. Two full teams need twice the team size, and below that the match
		//--- would have to end the moment it started, so the group floor stops the pass short and
		//--- somebody plays short-handed every single match.
		if (min_party_size > 1 && (min_party_size * 2) > minimum_players)
		{
			string warning = string.Format("[Lobby] min_party_size %1 needs at least %2 players", min_party_size, min_party_size * 2);
			warning = warning + string.Format(" to make two full teams, but minimum_players is %1", minimum_players);
			BattleRoyaleUtils.Warn(warning);
		}
	}
};
