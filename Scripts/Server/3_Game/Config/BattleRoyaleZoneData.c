#ifdef SERVER
class BattleRoyaleZoneData: BattleRoyaleDataBase
{
	int version = 7;  // Config version

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

    // --- Placement and timing derivation (v6) ---

    // Let a squeezed circle GROW, per match, instead of accepting a worse position. #19 asked for the
    // generator to "determine the best between the zone maximum time and the zone size" when the
    // preferred distance window cannot be respected; this is that trade. static_sizes is untouched -
    // the growth lives only in the generated chain - and it is bounded by the BR_ZONE_GROW_* constants,
    // reported per chain in the boot log, and counted as its own column by zone_selftest_runs above.
    //
    // It only fires after tier 3 has failed AND the chain still owes centre-ward travel, so a healthy
    // map barely sees it: at stock sizes ChernarusPlus leaves tier 1 on ~5% of placements and Sakhal on
    // ~20%. Turn it OFF to compare a self-test run against a baseline recorded before v6.
    bool allow_zone_size_flex = true;

    // Derive each round's length from the geometry instead of reading static_timers: how far a player
    // at the far edge of the current circle has to run to reach the next one, at sprint speed, plus a
    // fixed allowance for everything a round is for besides running. #19 asked for timing "derived from
    // mathematics values: zone size factor, maximum sprinting speed, etc."
    //
    // OFF by default because it changes pacing on every existing server, even though the constants are
    // tuned to be near-neutral against the stock static_timers (see BR_ZONE_TIMER_FIGHT_SECONDS). Two
    // rounds keep their hand-authored value even when this is on, because neither has any travel to
    // derive from: the OPENING round, which has no predecessor circle, and the endgame in
    // 7_BattleRoyaleLastRound, which plays out an already-locked circle. Ignored unless shrink_type is
    // 3 (static), the only mode static_timers applies to.
    bool derive_timers_from_geometry = false;

    // --- Derived ladder (v7) ---

    // Derive WHICH circle a given population opens on, and HOW MANY circles exist, instead of reading
    // the hand-authored min_players table and num_zones. static_sizes stays yours: this changes the
    // selection, never the geometry, so the generator and every proof in BattleRoyaleZone.c are
    // untouched. OFF by default like every other derivation in this file.
    //
    // Two things become derived:
    //   * min_players[i], from the circle's radius and from how much LOOT it actually encloses - the
    //     count of POIs inside the placed circle, against this map's mean POI density. See
    //     BattleRoyaleZone.BuildDerivedLadder for the formula and for why a loot-RICH circle gets a
    //     HIGHER min_players (it reads backwards and is right).
    //   * num_zones, from zone_opening_world_fraction below.
    bool derive_zone_ladder = false;

    // Metres of opening-circle RADIUS per player. The shipped min_players table is linear in radius -
    // 3375/33 = 2250/22 = 1125/11 = 102.3 - so this default reproduces it exactly WHEN
    // zone_poi_density_weight is 0. LOWER means a bigger opening circle for the same crowd.
    float zone_metres_per_player = 102.3;

    // Nobody opens on a circle rated for fewer than this. The shipped table's four smallest tiers all
    // sit on 10, which is what this reproduces.
    int zone_min_players_floor = 10;

    // How much the enclosed loot density is allowed to move the answer. 0 = ignore POIs entirely and
    // rate every circle on its radius alone; 1 = apply the full density ratio. The result is clamped
    // to [zone_poi_factor_min, zone_poi_factor_max] either way, because a circle over a single dense
    // town should not be rated for triple the players.
    //
    // SET IT TO 0 FIRST when tuning zone_metres_per_player: that is the setting under which the derived
    // table must reproduce min_players entry for entry, and it is the only clean way to tell a mistuned
    // radius from a loud density term. Above 0 the derived numbers will NOT match the authored ones -
    // every circle is nested around a village seed, so every circle reads denser than the map mean. See
    // BattleRoyaleZone.BuildDerivedLadder for which half of that is signal and which half is bias.
    float zone_poi_density_weight = 1.0;
    float zone_poi_factor_min = 0.5;
    float zone_poi_factor_max = 1.5;

    // The opening circle's radius as a fraction of the map width, used to DERIVE num_zones: tiers
    // larger than this are dropped, so a bigger opening circle keeps more shrink steps and a small one
    // does not drag. 0.22 is the figure Validate()'s advisory below already recommends (PUBG's Erangel
    // is 8 km with a ~2 km first circle, i.e. 0.25), and on a 15360 m map it keeps static_sizes up to
    // 3375 and drops 4500 - exactly the shipped num_zones of 6.
    float zone_opening_world_fraction = 0.22;

    // --- Match duration bound (v7) ---

    // Bound how long a whole match runs by moving the starting tier. Starting one tier smaller drops
    // the largest circle and its round, so this is the same lever the dynamic starting zone already
    // pulls - it just pulls it against a clock instead of against a player count. OFF by default.
    //
    // NOTE it can only move the tier within [1, num_zones - min_zone_num + 1]; min_zone_num still wins.
    bool bound_match_duration = false;

    // Target match length as seconds per starting player, then clamped to the two bounds below.
    //
    // ⚠️ THESE THREE MUST OVERLAP WHAT THE LADDER CAN ACTUALLY PRODUCE, or the bound decides every
    // match on its own and the player-count and loot terms never reach the answer. #284's rough
    // starting point was 0.5 min per player; measured on ChernarusPlus at stock sizes with derived
    // timers on, the legal matches are 1260 s (zone 3, the min_zone_num floor), 1903 s (zone 2) and
    // ~2560 s (zone 1) - so 30 s/player with a 2400 s ceiling made the largest circle UNREACHABLE at
    // any population and collapsed a hundred player counts onto two tiers. Caught by
    // zone_ladder_selftest_players, which is exactly what it is for; it now reports the achievable
    // range and warns when the window sits outside it.
    //
    // 45 s/player spans that range over a realistic lobby: 28 players -> 1260 s, 42 -> 1890, 57 -> 2565.
    float match_seconds_per_player = 45.0;
    int match_min_seconds = 1200;  // 20 min - just under the shortest match the stock ladder can play
    int match_max_seconds = 2700;  // 45 min - just over the longest, so the full ladder stays reachable

    // Walk player counts 1..N at boot and report the tier each one would open on, the circle's radius,
    // the POIs inside it and the resulting match length. 0 = off. This is the acceptance gate for the
    // three settings above, for the same reason zone_selftest_runs is the one for placement: the
    // interesting cases are combinatorial and LaunchLocalMP.bat tops out at three clients. It also
    // reports a TIER HISTOGRAM, which is what catches the failure mode this codebase keeps hitting -
    // a derivation that compiles, runs, and never actually changes the answer.
    int zone_ladder_selftest_players = 0;

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

    // Only hot zones in or near the STARTING circle are sent to clients - a red ring in the far
    // corner of the map is noise, since nobody will ever go there. A hot zone is kept when its own
    // circle touches the starting circle, i.e. when
    //     distance(centres) <= starting_radius + hot_radius + hot_zone_margin_m
    // so 0 means "must actually overlap the starting circle" and a positive value widens that.
    // Filtering happens at send time, not here: the starting circle depends on the player count at
    // the countdown, so it is not known when this file is read.
    float hot_zone_margin_m = 0;

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
			//--- Every ref array in this class is snapshotted and restored if the mission pass leaves
			//--- it empty - see the note on BattleRoyaleDataBase.CopyStrings for why that is necessary
			//--- and why the copy has to be of the contents rather than the reference.
			//---
			//--- This is not hypothetical here. The shipped mission zone_settings.json lists
			//--- end_avoid_type, end_avoid_city, first_zone_polygon, static_sizes, static_timers and
			//--- min_players - and says nothing about the rest, so on any server using it
			//--- zone_notification_minutes and zone_notification_seconds were both loading EMPTY and
			//--- the shrink warnings were silently switched off. hot_zone_centers / hot_zone_radii and
			//--- avoid_surface_types were in the same position; they merely happen to default empty,
			//--- so nobody noticed.
			//---
			//--- A mission file written before a field existed is the normal case, not an edge case:
			//--- every array added to this class from now on lands in exactly that situation.
			ref array<int> keptNotifyMinutes = CopyInts(zone_notification_minutes);
			ref array<int> keptNotifySeconds = CopyInts(zone_notification_seconds);
			ref array<string> keptAvoidType = CopyStrings(end_avoid_type);
			ref array<string> keptAvoidCity = CopyStrings(end_avoid_city);
			ref array<string> keptFinalPolygon = CopyStrings(final_zone_polygon);
			ref array<string> keptFirstPolygon = CopyStrings(first_zone_polygon);
			ref array<float> keptStaticSizes = CopyFloats(static_sizes);
			ref array<int> keptStaticTimers = CopyInts(static_timers);
			ref array<int> keptMinPlayers = CopyInts(min_players);
			ref array<string> keptAvoidSurfaces = CopyStrings(avoid_surface_types);
			ref array<string> keptHotCenters = CopyStrings(hot_zone_centers);
			ref array<float> keptHotRadii = CopyFloats(hot_zone_radii);

			if (!JsonFileLoader<BattleRoyaleZoneData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);

			if (!zone_notification_minutes || zone_notification_minutes.Count() == 0)
				zone_notification_minutes = keptNotifyMinutes;

			if (!zone_notification_seconds || zone_notification_seconds.Count() == 0)
				zone_notification_seconds = keptNotifySeconds;

			if (!end_avoid_type || end_avoid_type.Count() == 0)
				end_avoid_type = keptAvoidType;

			if (!end_avoid_city || end_avoid_city.Count() == 0)
				end_avoid_city = keptAvoidCity;

			if (!final_zone_polygon || final_zone_polygon.Count() == 0)
				final_zone_polygon = keptFinalPolygon;

			if (!first_zone_polygon || first_zone_polygon.Count() == 0)
				first_zone_polygon = keptFirstPolygon;

			if (!static_sizes || static_sizes.Count() == 0)
				static_sizes = keptStaticSizes;

			if (!static_timers || static_timers.Count() == 0)
				static_timers = keptStaticTimers;

			if (!min_players || min_players.Count() == 0)
				min_players = keptMinPlayers;

			if (!avoid_surface_types || avoid_surface_types.Count() == 0)
				avoid_surface_types = keptAvoidSurfaces;

			//--- The pair Validate() truncates to equal length. Restoring them independently is
			//--- correct: a mission that specifies one and not the other is a misconfiguration
			//--- Validate() already clamps, and it should clamp what the admin actually wrote rather
			//--- than a half-restored mixture.
			if (!hot_zone_centers || hot_zone_centers.Count() == 0)
				hot_zone_centers = keptHotCenters;

			if (!hot_zone_radii || hot_zone_radii.Count() == 0)
				hot_zone_radii = keptHotRadii;
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

		if (version < 6)
		{
			// allow_zone_size_flex/derive_timers_from_geometry were INTRODUCED in v6, and like the v5
			// branch above this one refills nothing - both are SCALARS, and a missing scalar key leaves
			// the field initialiser in place. Only a ref array needs the refill treatment the v3 and v4
			// branches give it. So every server upgrading to v6 picks up flex ON and derived timers OFF
			// from the declarations above, which is what those defaults are chosen for.
			version = 6;
			Save();  // Save the upgraded config
		}

		if (version < 7)
		{
			// The derived-ladder and match-duration fields were INTRODUCED in v7. Like the v5 and v6
			// branches above, this one refills NOTHING: every field added is a SCALAR, and a missing
			// scalar key leaves the field initialiser in place. Only a ref array needs the refill
			// treatment the v3 and v4 branches give it. So every server upgrading to v7 picks up
			// derive_zone_ladder OFF and bound_match_duration OFF from the declarations above, which
			// is what those defaults are chosen for - nothing about an existing server moves.
			version = 7;
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
		//--- Effective array sizes, read back off the live object AFTER both the profile and the
		//--- mission pass. This is the tell for the mission-override array wipe: a count of 0 for
		//--- something that ships non-empty means the mission JSON omitted the key and the field was
		//--- silently emptied, which for the two notify arrays means the shrink warnings are off and
		//--- nothing else says so. Cheap, once per boot, and it is what makes the failure visible
		//--- rather than something a player reports weeks later.
		//--- Counted through null guards, matching how the rest of this method reads these arrays.
		//--- A JSON field can legitimately deserialize to null, and a DIAGNOSTIC that dereferences it
		//--- would take server init down - which is a far worse outcome than the silent emptying it
		//--- exists to report.
		int c_notify_min = 0;
		int c_notify_sec = 0;
		int c_avoid_type = 0;
		int c_avoid_surface = 0;
		int c_hot = 0;

		if (zone_notification_minutes)
			c_notify_min = zone_notification_minutes.Count();
		if (zone_notification_seconds)
			c_notify_sec = zone_notification_seconds.Count();
		if (end_avoid_type)
			c_avoid_type = end_avoid_type.Count();
		if (avoid_surface_types)
			c_avoid_surface = avoid_surface_types.Count();
		if (hot_zone_centers)
			c_hot = hot_zone_centers.Count();

		string counts = "[BattleRoyaleZone] effective arrays - notify_min ";
		counts = counts + c_notify_min.ToString();
		counts = counts + ", notify_sec " + c_notify_sec.ToString();
		counts = counts + ", avoid_type " + c_avoid_type.ToString();
		counts = counts + ", avoid_surface " + c_avoid_surface.ToString();
		counts = counts + ", hot_zones " + c_hot.ToString();
		BattleRoyaleUtils.Info(counts);

		//--- EnfusionScript allows one declaration per name per method scope, so every local the
		//--- checks below need is declared here rather than at first use.
		int limit = num_zones;
		int i;
		int shortest;
		int limit_before_fit;
		float world_size = 0;
		float factor;
		float smallest;

		//--- Derived ladder locals (v7). Same reason as above: one declaration per name per method
		//--- scope, so they are here rather than at first use.
		int derived_limit;
		float opening_cap;
		string derive_line;

		//--- Hot zone locals. Same reason: one declaration per name per method scope.
		int hz;
		int hz_count;
		int hz_dropped;
		string hz_raw;
		vector hz_center;
		float hz_radius;
		array<string> hz_centers_kept;
		array<float> hz_radii_kept;

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

		//--- (0b) DERIVE num_zones from the map (#284 point 2), after any scaling above so it sees the
		//--- real sizes and before every clamp below so they run over the result. Keep the largest tier
		//--- that still fits zone_opening_world_fraction of the world width, and drop the rest.
		//---
		//--- This has to happen HERE and not in the generator: BattleRoyaleServer.Init() reads num_zones
		//--- to size the state list before a single circle is placed, so every reader must already agree
		//--- on the derived number. They all go through GetZoneData(), so they do.
		//---
		//--- Only ever LOWERS num_zones. Raising it would index past whatever static_timers/min_players
		//--- the admin actually wrote, and clamp (1) below would take it straight back off anyway.
		if (derive_zone_ladder && static_sizes && static_sizes.Count() > 0 && world_size > 0)
		{
			opening_cap = world_size * zone_opening_world_fraction;

			derived_limit = 0;
			for (i = 0; i < static_sizes.Count(); i++)
			{
				if (static_sizes[i] > opening_cap)
					break;

				derived_limit = i + 1;
			}

			//--- A cap below even the final circle would leave nothing to play. One tier is a match
			//--- with a single circle, which is degenerate but playable; zero is not.
			if (derived_limit < 1)
				derived_limit = 1;

			//--- Built in steps. Eleven concatenated terms in one expression is at the "Formula too
			//--- complex" ceiling, and that is a hard compile error packing does not catch.
			derive_line = "[BattleRoyaleZoneData] derive_zone_ladder: opening circle capped at " + opening_cap + " m";
			derive_line = derive_line + " (" + zone_opening_world_fraction + " x " + world_size + ")";
			derive_line = derive_line + " - num_zones " + limit + " -> " + derived_limit + ".";
			BattleRoyaleUtils.Info(derive_line);

			limit = derived_limit;
		}

		//--- (0c) Sanity-bound the tuning knobs, so a typo in the JSON degrades instead of producing a
		//--- ladder nothing can open on. Only the ones that divide or multiply; the rest are read
		//--- through Math.Clamp at the point of use.
		if (derive_zone_ladder)
		{
			if (zone_metres_per_player < BR_ZONE_LADDER_MIN_M_PER_PLAYER || zone_metres_per_player > BR_ZONE_LADDER_MAX_M_PER_PLAYER)
			{
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] zone_metres_per_player is " + zone_metres_per_player + ", outside [" + BR_ZONE_LADDER_MIN_M_PER_PLAYER + ", " + BR_ZONE_LADDER_MAX_M_PER_PLAYER + "] - clamping for this boot. The shipped min_players table works out to 102.3.");
				zone_metres_per_player = Math.Clamp(zone_metres_per_player, BR_ZONE_LADDER_MIN_M_PER_PLAYER, BR_ZONE_LADDER_MAX_M_PER_PLAYER);
			}

			if (zone_poi_factor_min <= 0 || zone_poi_factor_max < zone_poi_factor_min)
			{
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] zone_poi_factor_min/max are " + zone_poi_factor_min + "/" + zone_poi_factor_max + ", which is not a usable range - resetting to 0.5/1.5 for this boot.");
				zone_poi_factor_min = 0.5;
				zone_poi_factor_max = 1.5;
			}

			if (zone_min_players_floor < 1)
			{
				BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] zone_min_players_floor is " + zone_min_players_floor + " - raising it to 1. A floor of zero would make every circle match a one-player lobby.");
				zone_min_players_floor = 1;
			}
		}

		//--- (0d) The duration bound's own range. An inverted pair would make the "too short" and "too
		//--- long" tests both true and the walk would oscillate; BR_MATCH_DURATION_MAX_STEPS bounds it
		//--- either way, but a silently inverted target is worth naming.
		if (bound_match_duration && match_max_seconds < match_min_seconds)
		{
			BattleRoyaleUtils.Warn("[BattleRoyaleZoneData] match_max_seconds (" + match_max_seconds + ") is below match_min_seconds (" + match_min_seconds + ") - using the minimum as the maximum for this boot.");
			match_max_seconds = match_min_seconds;
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
