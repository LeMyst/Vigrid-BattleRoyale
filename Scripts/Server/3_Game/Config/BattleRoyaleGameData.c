#ifdef SERVER
// General match-flow settings that don't belong to a specific subsystem file. Zone geometry/timing
// lives in BattleRoyaleZoneData, lobby/spawn-selection flow in BattleRoyaleLobbyData, voice policy
// in BattleRoyaleVoiceData - see those files before adding a new field here.
//
// admins_steamid64 below is mission-locked: LoadMission() snapshots it before the mission-folder
// deserialize and restores it after, so a mission pack can never grant itself admin immunity even
// though this file otherwise supports mission overrides. Reach for the same snapshot/restore idiom
// for any future field that is a server-operator concern rather than mission content.
class BattleRoyaleGameData: BattleRoyaleDataBase
{
	int version = 3;  // Config version

	// Allowed admins - Are immune to kick and can go outside the play area
	// Mission-locked (see class comment above) - never overridable from a mission pack.
	ref array<string> admins_steamid64 = {
		"123456789123456789" // Dummy SteamID64
	};

    int round_duration_minutes = 5;  // round length in minutes
    int time_until_teleport_unlock = 10;  // seconds before unlock after teleporting & preparing

    bool hide_players_endgame = false;  // Hide the number of players left in the endgame

    bool show_first_zone_at_start = true;  // Show the first zone at the start of the game

    bool artillery_sound = true;  // Play the artillery sound when the zone shrinks

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
			// admins_steamid64 is a server-operator concern (who is immune to kick/zone restriction),
			// not mission content - a mission pack must never be able to grant itself admin immunity.
			ref array<string> lockedAdmins = admins_steamid64;

			if (!JsonFileLoader<BattleRoyaleGameData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);

			admins_steamid64 = lockedAdmins;
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
			//
			// Those voice fields have since moved to BattleRoyaleVoiceData/voice_settings.json - this
			// version stays at 3 (no bump) because that move needs no migration of its own: a v3
			// general_settings.json on disk still has the old keys, JsonFileLoader simply ignores
			// them now that this class no longer declares matching fields, and voice_settings.json
			// starts fresh from BattleRoyaleVoiceData's own defaults.
			version = 3;
			Save();  // Save the upgraded config
		}
	}
};
