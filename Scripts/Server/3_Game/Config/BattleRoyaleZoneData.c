#ifdef SERVER
class BattleRoyaleZoneData: BattleRoyaleDataBase
{
	int version = 5;  // Config version

    int num_zones = 6;  // number of zones

    ref array<int> zone_notification_minutes = { 1, 2 };  // minutes when notification about the zone shrinking will be displayed
    ref array<int> zone_notification_seconds = { 30, 10 };  // seconds when notification about the zone shrinking will be displayed, when under the minute

    // Zone damage settings
    int zone_damage_tick_seconds = 5;  // seconds between zone damage ticks
    float zone_damage_delta = 0.1;  // damage per tick
    bool enable_zone_damage = true;  // enable zone damage

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
	// opening one. num_zones (above) picks that many tiers from the small end, so lowering it
	// shortens a match by dropping the LARGEST circles and always keeps the endgame one.
	// At the defaults below, num_zones = 6 plays 3375 down to 35 and leaves index 6 (4500) unused.
	// Entries past num_zones are ignored on purpose; FEWER entries than num_zones is a misconfiguration.
    ref array<float> static_sizes = { 35, 140, 562, 1125, 2250, 3375, 4500 };
    ref array<int> static_timers = { 155, 260, 307, 495, 495, 495, 495 };
    ref array<int> min_players = { 10, 10, 10, 11, 22, 33, 44 };

    // Constant (NOT USED ANYMORE)
    float constant_scale = 0.65;

    // --- Generation tuning (v4) ---

    // 0 = derive a seed, log it, and leave the engine RNG alone. Set it to a value printed in a
    // previous boot's log to replay that exact layout. NOTE this reseeds the GLOBAL RNG, so a
    // non-zero value also fixes loot, weather and spawn placement - it is a debugging tool.
    int zone_generation_seed = 0;

    // Fraction of a large circle that must be dry land for it to be accepted at the normal search
    // tier. Sampled on rings inside the circle rather than testing the centre pixel, so a big circle
    // that is mostly land is no longer rejected for being centred 20 m offshore. Per-map tuning
    // without a rebuild; the relaxed tiers below it are compile-time constants.
    float zone_min_land_fraction = 0.6;

    // Surface types a circle centre may never sit on, beyond sea and pond. Shared with the airdrop
    // placement, which used to hardcode this list.
    ref array<string> avoid_surface_types = { "nam_seaice", "nam_lakeice_ext" };

    // Scale static_sizes to this map. The final circle is held fixed and every larger circle's
    // distance from it is scaled by world_size / reference_world_size, so the endgame stays the size
    // you tuned it to while the opening circle follows the map. Off by default: the mission override
    // ($mission:Vigrid-BattleRoyale\zone_settings.json) is per-map already and is the explicit way to
    // do this. Turn it on when one config has to serve several maps.
    bool scale_sizes_to_world = false;
    int reference_world_size = 15360;  // the world static_sizes was tuned against (ChernarusPlus)

    // Generate this many throwaway chains at boot, report the failure/backtrack/tier distribution,
    // then play normally. 0 = off. This is the acceptance gate for a new map or a new static_sizes:
    // 200 runs inside one boot answers "can this configuration ever dead-end here", which relaunching
    // the server twenty times never could. Costs a few hundred ms.
    int zone_selftest_runs = 0;

    // --- Hot zones (v5) ---

    // Static circles drawn on the map, the minimap and the spawn-selection screen to mark regions
    // of interest. PURELY COSMETIC: nothing reads these but the renderers, so a hot zone changes no
    // loot, no damage and no placement. Two parallel arrays - entry i is a circle of hot_zone_radii[i]
    // metres centred on hot_zone_centers[i].
    //
    // Centres are "x y z" strings rather than a vector array, matching final_zone_polygon above:
    // that is how this file has always spelled a list of world positions, and JsonFileLoader handles
    // strings without surprises. The y component is ignored.
    //
    // Validate() below truncates the pair to the shorter array and drops entries that cannot be
    // drawn, so a half-edited config degrades to fewer circles instead of a bad read.
    ref array<string> hot_zone_centers = {};
    ref array<float> hot_zone_radii = {};

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

		if (version < 3)
		{
			// zone_notification_minutes/_seconds were INTRODUCED in v3 (moved here from
			// general_settings.json). Unlike a missing scalar key, which leaves the field initialiser
			// above in place, a missing array key deserializes to an EMPTY array - confirmed against a
			// pre-v3 zone_settings.json, which naturally has neither key. Left unguarded, every server
			// upgrading to v3 would silently lose the zone-shrink countdown notifications. Only fill an
			// array that came back genuinely empty, so an admin who deliberately empties it later (to
			// turn notifications off) stays empty on the next boot.
			if (!zone_notification_minutes || zone_notification_minutes.Count() == 0)
			{
				zone_notification_minutes = new array<int>();
				zone_notification_minutes.Insert(1);
				zone_notification_minutes.Insert(2);
			}

			if (!zone_notification_seconds || zone_notification_seconds.Count() == 0)
			{
				zone_notification_seconds = new array<int>();
				zone_notification_seconds.Insert(30);
				zone_notification_seconds.Insert(10);
			}

			version = 3;
			Save();  // Save the upgraded config
		}

		if (version < 4)
		{
			// avoid_surface_types was INTRODUCED in v4. Same trap as the notification arrays above:
			// a missing array key deserializes to an EMPTY array rather than keeping the field
			// initialiser, so without this every existing server would load it empty and the
			// surface-type rejection would silently do nothing. Only refill an array that came back
			// genuinely empty, so an admin who deliberately clears it stays cleared.
			if (!avoid_surface_types || avoid_surface_types.Count() == 0)
			{
				avoid_surface_types = new array<string>();
				avoid_surface_types.Insert("nam_seaice");
				avoid_surface_types.Insert("nam_lakeice_ext");
			}

			version = 4;
			Save();  // Save the upgraded config
		}

		if (version < 5)
		{
			// hot_zone_centers/hot_zone_radii were INTRODUCED in v5. NOTE this branch deliberately
			// refills NOTHING, unlike the three above it. The array-initialiser trap they guard
			// against - a missing key deserializing to an empty array instead of keeping the field
			// initialiser - only bites a field whose default is non-empty. These two default to {},
			// so "loaded empty" and "shipped empty" are the same state and there is nothing to
			// restore. The branch exists only to move the version number.
			version = 5;
			Save();  // Save the upgraded config
		}
	}

	//--- Clamp anything internally inconsistent so a misconfiguration degrades into a shorter but
	//--- playable match instead of halting boot. This runs after BOTH the profile and the mission
	//--- pass (see BattleRoyaleConfig.Load), and deliberately NEVER calls Save() - the clamp is for
	//--- this boot only and must not rewrite the admin's file.
	//---
	//--- It has to live here rather than in the generator: BattleRoyaleServer.Init() reads num_zones
	//--- to build the state list BEFORE any circle is generated, so every reader must already agree
	//--- on the clamped number. They all go through GetZoneData(), so they do.
	override void Validate()
	{
		//--- EnfusionScript allows one declaration per name per method scope, so every local the
		//--- checks below need is declared here rather than at first use.
		int limit = num_zones;
		int i;
		int shortest;
		int limit_before_fit;
		float world_size = 0;
		float factor;
		float smallest;

		//--- Hot zone locals. Same reason: one declaration per name per method scope.
		int hz;
		int hz_count;
		int hz_dropped;
		string hz_raw;
		vector hz_center;
		float hz_radius;
		ref array<string> hz_centers_kept;
		ref array<float> hz_radii_kept;

		if (GetGame() && GetGame().GetWorld())
			world_size = GetGame().GetWorld().GetWorldSize();

		//--- (0) Optional per-map scaling, before every check below, so the clamps see real sizes.
		//--- The FINAL circle is held fixed and each larger circle's distance from it is scaled: a
		//--- flat multiply would shrink the endgame too, and the endgame size is a function of how
		//--- many players are left, not of how big the map is.
		if (scale_sizes_to_world && world_size > 0 && reference_world_size > 0 && static_sizes && static_sizes.Count() > 0)
		{
			factor = world_size / reference_world_size;
			smallest = static_sizes[0];

			for (i = 1; i < static_sizes.Count(); i++)
			{
				static_sizes[i] = smallest + ((static_sizes[i] - smallest) * factor);
			}

			BattleRoyaleUtils.Info("[BattleRoyaleZoneData] scale_sizes_to_world: world " + world_size + " / reference " + reference_world_size + " = x" + factor + " on the span above the final circle. Largest circle is now " + static_sizes[static_sizes.Count() - 1] + ".");
		}

		//--- (1) num_zones may not exceed the shortest of the three parallel settings arrays. This
		//--- replaces the fatal Error that used to fire from BattleRoyaleZone.LogUnusedTail before
		//--- generation had even started.
		shortest = limit;
		if (static_sizes && static_sizes.Count() < shortest)
			shortest = static_sizes.Count();
		if (static_timers && static_timers.Count() < shortest)
			shortest = static_timers.Count();
		if (min_players && min_players.Count() < shortest)
			shortest = min_players.Count();

		if (shortest < limit)
		{
			BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] num_zones is " + limit + " but the shortest of static_sizes/static_timers/min_players has only " + shortest + " entries - clamping num_zones to " + shortest + " for this boot. Add the missing entries to play a full-length match.");
			limit = shortest;
		}

		//--- (2) Each circle must be strictly LARGER than the one before it: the span r_i - r_{i-1}
		//--- is that step's entire travel budget, so a non-increasing pair makes it <= 0 and silently
		//--- produces a circle that does not contain its parent. Nothing checked this before.
		if (static_sizes)
		{
			for (i = 1; i < limit; i++)
			{
				if (static_sizes[i] > static_sizes[i - 1])
					continue;

				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] static_sizes is not strictly increasing at index " + i + " (" + static_sizes[i - 1] + " -> " + static_sizes[i] + ") - clamping num_zones to " + i + ". The array is SMALLEST ZONE FIRST: index 0 is the final circle.");
				limit = i;
				break;
			}
		}

		//--- (3) The opening circle must fit the world box [r, W-r]^2, which is empty unless
		//--- 2*r <= W. A circle larger than that can never be placed anywhere at all.
		if (static_sizes && world_size > 0)
		{
			limit_before_fit = limit;
			while (limit > 1 && (2 * static_sizes[limit - 1]) > world_size)
			{
				limit--;
			}

			if (limit < limit_before_fit)
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] a circle of radius " + static_sizes[limit_before_fit - 1] + " cannot fit in a " + world_size + " m world - clamping num_zones from " + limit_before_fit + " to " + limit + ". Lower static_sizes for this map, or turn on scale_sizes_to_world.");
		}

		//--- (4) Advisory. Past a quarter of the map width the opening circle leaves so little room
		//--- inside the world box that its centre is pinned near the map centre every match - the
		//--- same opening every time, and the tightest possible funnel for the rest of the chain.
		//--- PUBG's Erangel is 8 km with a ~2 km first circle, i.e. exactly 0.25.
		if (static_sizes && world_size > 0 && limit > 0)
		{
			if (static_sizes[limit - 1] > (world_size * 0.25))
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] the opening circle (" + static_sizes[limit - 1] + " m) is more than a quarter of this " + world_size + " m map, leaving only " + ((world_size / 2) - static_sizes[limit - 1]) + " m of freedom for its centre. Expect the same opening every match. A radius near " + (world_size * 0.22) + " m suits this map.");
		}

		if (limit < 1)
			limit = 1;

		num_zones = limit;

		//--- min_zone_num indexes the same window, so it cannot exceed it either.
		if (min_zone_num > num_zones)
		{
			BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] min_zone_num (" + min_zone_num + ") exceeds num_zones (" + num_zones + ") - clamping it.");
			min_zone_num = num_zones;
		}

		//--- (5) Hot zones. Two parallel arrays edited by hand, so the failure mode is a half-finished
		//--- edit: three centres and two radii, or a centre with a typo in it. Both are silent
		//--- otherwise - the mismatch reads past the end of the shorter array, and ToVector() answers
		//--- "0 0 0" for anything it cannot parse, which draws a circle on the map corner. Filter here
		//--- so every consumer downstream can trust the pair, and warn per entry so the admin can see
		//--- which line of their JSON is wrong.
		if (!hot_zone_centers)
			hot_zone_centers = new array<string>();
		if (!hot_zone_radii)
			hot_zone_radii = new array<float>();

		hz_count = hot_zone_centers.Count();
		if (hot_zone_radii.Count() < hz_count)
			hz_count = hot_zone_radii.Count();

		if (hz_count < hot_zone_centers.Count() || hz_count < hot_zone_radii.Count())
			BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] hot_zone_centers has " + hot_zone_centers.Count() + " entries but hot_zone_radii has " + hot_zone_radii.Count() + " - using the first " + hz_count + ". The two arrays are parallel: entry i is a circle of hot_zone_radii[i] metres centred on hot_zone_centers[i].");

		hz_centers_kept = new array<string>();
		hz_radii_kept = new array<float>();
		hz_dropped = 0;

		for (hz = 0; hz < hz_count; hz++)
		{
			//--- Every array read on its own line, never nested inside a call. A read sharing an
			//--- expression with a call has been measured in this codebase to return another array's
			//--- contents entirely.
			hz_raw = hot_zone_centers[hz];
			hz_radius = hot_zone_radii[hz];
			hz_center = hz_raw.ToVector();

			if (hz_radius <= 0)
			{
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] hot zone " + hz + " has radius " + hz_radius + " - dropping it. A radius must be greater than zero.");
				hz_dropped++;
				continue;
			}

			//--- ToVector() does not report failure, so an unparseable string is indistinguishable
			//--- from a deliberate "0 0 0". Neither is usable - the map corner is not a region of
			//--- interest - so both are rejected under one test.
			if (hz_center == vector.Zero)
			{
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] hot zone " + hz + " has centre \"" + hz_raw + "\", which reads as 0 0 0 - dropping it. Expected three space-separated numbers, e.g. \"6000 0 7000\".");
				hz_dropped++;
				continue;
			}

			if (world_size > 0 && (hz_center[0] < 0 || hz_center[0] > world_size || hz_center[2] < 0 || hz_center[2] > world_size))
			{
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] hot zone " + hz + " is centred at " + hz_center + ", outside this " + world_size + " m world - dropping it.");
				hz_dropped++;
				continue;
			}

			hz_centers_kept.Insert(hz_raw);
			hz_radii_kept.Insert(hz_radius);
		}

		hot_zone_centers = hz_centers_kept;
		hot_zone_radii = hz_radii_kept;

		if (hot_zone_centers.Count() > 0)
			BattleRoyaleUtils.Info("[BattleRoyaleZoneData] " + hot_zone_centers.Count() + " hot zone(s) will be drawn, " + hz_dropped + " dropped.");
		else if (hz_dropped > 0)
			BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] all " + hz_dropped + " configured hot zone(s) were unusable - none will be drawn.");
	}
};
