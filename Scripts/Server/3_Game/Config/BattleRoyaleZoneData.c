#ifdef SERVER
class BattleRoyaleZoneData: BattleRoyaleDataBase
{
	int version = 2;  // Config version

    bool end_in_villages = true;  // The final zone will end in a village/city/town
    ref array<string> end_avoid_type = {"DeerStand", "FeedShack", "Marine"};  // Types to avoid in the final zone (Only if end_in_villages is true)
    ref array<string> end_avoid_city = {"Camp_Shkolnik", "Ruin_Voron", "Settlement_Skalisty"};  // Cities to avoid in the final zone (Only if end_in_villages is true)

    // Final zone polygon restriction
    bool restrict_final_zone = false;  // Whether to restrict the final zone to a polygon
    ref array<string> final_zone_polygon = {}; // Array of vectors defining polygon for the final zone

    // Deprecated, kept so a version 1 config still deserializes and can be migrated by Upgrade().
    // These always restricted the FINAL zone despite the name; do not read them, use the pair above.
    bool restrict_first_zone = false;
    ref array<string> first_zone_polygon = {};

	// Dynamic zones
    bool use_dynamic_zones = true;  // Use dynamic zones based on player count
    int min_zone_num = 4;  // Minimum number of zones to have

    int shrink_type = 3;  // 1 = Exp, 2 = Lin, 3 = Static, 4 = Const
    // TODO: I think Exp, Lin and Const are not used anymore
    // TODO: That is because the logic changed at one point and now the zone is always static

    // Exponential (NOT USED ANYMORE)
    float shrink_base = 2.718281828459; // ~ e
    float shrink_exponent = 3.0;

	// Static
	// SMALLEST ZONE FIRST: index 0 describes the tight final circle, the last index the widest
	// opening one. num_zones (general_settings.json) picks that many tiers from the small end, so
	// lowering it shortens a match by dropping the LARGEST circles and always keeps the endgame one.
	// At the defaults below, num_zones = 6 plays 3375 down to 35 and leaves index 6 (4500) unused.
	// Entries past num_zones are ignored on purpose; FEWER entries than num_zones is a misconfiguration.
    ref array<float> static_sizes = { 35, 140, 562, 1125, 2250, 3375, 4500 };
    ref array<int> static_timers = { 155, 260, 307, 495, 495, 495, 495 };
    ref array<int> min_players = { 10, 10, 10, 11, 22, 33, 44 };

    // Constant (NOT USED ANYMORE)
    float constant_scale = 0.65;

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "zone_settings.json";
    }

    override string GetMissionPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "zone_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyaleZoneData>.LoadFile(GetProfilePath(), this, errorMessage))
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
			if (!JsonFileLoader<BattleRoyaleZoneData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);
		}
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyaleZoneData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}

	override void Upgrade()
	{
		// Future upgrades will be handled here
		if (version == 1)
		{
			// restrict_first_zone / first_zone_polygon always restricted the FINAL zone - they are
			// applied to the smallest, last-played circle. Carry the values over to the honest names.
			restrict_final_zone = restrict_first_zone;

			if (first_zone_polygon)
			{
				final_zone_polygon = new array<string>();
				foreach (string vertex : first_zone_polygon)
				{
					final_zone_polygon.Insert(vertex);
				}
			}

			restrict_first_zone = false;
			first_zone_polygon = new array<string>();

			version = 2;
			Save();  // Save the upgraded config
		}
	}
};
