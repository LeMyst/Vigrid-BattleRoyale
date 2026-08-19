#ifdef SERVER
class BattleRoyalePOIsData: BattleRoyaleDataBase
{
	int version = 4;  // Config version

	// Allow to override the position of the POIs.
	// Empty by default ON PURPOSE: these entries are applied for real, they are not inert placeholders
	// like the dummy SteamID in general_settings.json. The key is the CfgWorlds class name (the same
	// name BattleRoyaleZone.InitializePOIs and 4_BattleRoyalePrepare look up), the value an [x, z] pair.
	// Example:
	//   new BattleRoyaleOverridePOIPosition("Settlement_Chernogorsk", { 6600, 2800 })
	ref array<ref BattleRoyaleOverridePOIPosition> override_poi_positions = {};

	//--- POI RESOLUTION FROM BUILDINGS -------------------------------------------------------------
	//
	// A CfgWorlds "Names" position is a map-LABEL anchor, not a town centre: it is placed to keep the
	// text off the buildings. Measured on ChernarusPlus, Settlement_Chernogorsk has two buildings
	// within 100 m of its label and Settlement_Prigorodki four. Spawning around the label therefore
	// puts players in the field beside the town, and end_in_villages centres the final circle there.
	//
	// With this on, each POI's anchor and extent are derived once at boot from the buildings actually
	// around it. override_poi_positions still wins outright - an admin override is never rescanned.
	bool resolve_poi_from_buildings = true;

	// Radius of one scan pass, in metres.
	//
	// This is deliberately GENEROUS, and the reason is that the derived extent can never exceed it -
	// scan at 200 and a city whose buildings run past 300 is permanently described as 200 m wide.
	// Measured on ChernarusPlus, Settlement_Chernogorsk goes 67 buildings at 200 m to 173 at 300 m, so
	// the big coastal cities genuinely need the range.
	//
	// The obvious objection is that a wide circle swallows a neighbouring cluster - an adjacent farm,
	// a satellite hamlet. Two things answer it, and NEITHER is shrinking this value. The centroid is
	// distance-weighted (see BattleRoyalePOIResolver.WeightFor), so a far cluster pulls proportionally
	// less; and the mean-shift re-centres on the densest mass, which is the town. For the record, the
	// farm case was over-estimated once from a 10-item log sample that is emitted in engine iteration
	// order rather than by distance - by COUNT that farm added 11 buildings against the village's 77.
	// Judge this by counts, never by the sample line.
	float poi_scan_radius_m = 350.0;

	// Mean-shift passes. The first scan is centred on the label, which is off-town by construction, so
	// one pass would weight half its sample on empty ground. Each later pass re-centres on the running
	// centroid. Stops early once a pass moves the anchor less than BR_POI_RESOLVE_CONVERGE_M.
	int poi_scan_iterations = 3;

	// Below this many buildings the scan result is REJECTED and the label is kept. A lone deer stand
	// is not a town, and moving a POI onto one is worse than leaving it where it was.
	int poi_min_buildings = 5;

	// CfgWorlds `type` values that get resolved. Everything else keeps its label.
	//
	// Only a SETTLEMENT label has the problem this whole feature exists to fix - it is offset to keep
	// the map text off the buildings. A Hill or a ViewPoint has no buildings to be offset from, so
	// there is nothing to correct, and scanning one does not find "the town at this label", it finds
	// the nearest UNRELATED town. Measured on ChernarusPlus: the three largest shifts were all
	// non-settlements - Hill_Kikimora moved 580 m, Hill_Kopyto 510 m onto a completely different
	// coastal town, ViewPoint_47 483 m onto a hamlet.
	//
	// That is not cosmetic. The zone side's end_in_villages walks the FULL POI list, so an unfiltered
	// run makes a hill and a real town resolve to nearly the same coordinates - duplicate seeds, which
	// collapses final-circle spread. BattleRoyaleZone's own self test warns about exactly that, so the
	// symptom would surface far from its cause.
	//
	// Chernarus uses Capital / City / Village / Local / Camp / Hill / Ruin / Marine. A map with a
	// different vocabulary needs this list edited - and poi_max_shift_m below is the backstop that
	// keeps such a map safe in the meantime.
	//
	// EMPTY means "no type filter", i.e. every POI is eligible - deliberately, because that is the
	// benign failure: it degrades to the unfiltered behaviour, which poi_max_shift_m still guards.
	// The alternative reading (empty matches nothing) would turn one cleared line into a total,
	// silent feature outage on a map whose type names simply differ from Chernarus's.
	ref array<string> poi_resolve_types = {"Capital", "City", "Village"};

	// Reject a resolution that moves the anchor further than this many metres, and keep the label.
	//
	// The type list above is the principled filter; this is the map-agnostic one, and it is what makes
	// the feature safe on a map whose `type` vocabulary nobody has checked yet. The reasoning is that a
	// scan which lands this far away has not found this POI's town, it has found a different place.
	// Sized against measurement: the largest legitimate settlement correction on ChernarusPlus is
	// Chernogorsk at 291 m, and every non-settlement failure was 483 m or more.
	float poi_max_shift_m = 350.0;

	// Extent is the distance covering this fraction of the buildings, measured from the final anchor -
	// not the maximum, or one outlying barn triples the pad.
	float poi_extent_percentile = 0.8;

	// Clamps on the derived extent. These replace the hardcoded 500 / 300 / 100 pads that
	// 4_BattleRoyalePrepare carried with a "TODO: Move this to configuration file".
	float poi_extent_min_m = 60.0;
	float poi_extent_max_m = 500.0;

	// Class-name fragments that disqualify a Building from counting as town fabric. Matched as a
	// substring of the config class name, case-sensitive.
	//
	// This is NOT optional tidying. `Building` is a broad script class and the measured samples are
	// full of furniture that is both common and positionally misleading: Land_Wreck_* are roadside car
	// wrecks and would bias every anchor toward the nearest road, and Land_Misc_Through_Static (cattle
	// troughs) appeared three times in a single 10-item sample. Note Well and FuelStation are counted
	// on purpose - they are genuine town furniture - which is also why the scan tests Building.Cast
	// rather than IsBuilding(), since both of those override IsBuilding() to return false.
	ref array<string> poi_scan_exclude = {"Wreck", "Misc_Through", "Toilet_Mobile", "Wall_Gate", "Pipe_", "Misc_Fence"};

	// Number of POIs to report in the boot self-test table, 0 = off. Same reasoning as
	// zone_selftest_runs and auto_group_selftest: a setting rather than a diag entry, so it runs on a
	// headless dedicated server. The column that matters is the SHIFT distance - an all-zero shift
	// column means the scan returned nothing and the feature is silently dead.
	int poi_resolve_selftest = 0;

	[NonSerialized()]
	ref map<string, vector> m_OverrideSpawnPositions;

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "pois_settings.json";
    }

    override string GetMissionPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "pois_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyalePOIsData>.LoadFile(GetProfilePath(), this, errorMessage))
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
			//--- ⚠️ THE MISSION PASS CLEARS EVERY `ref array` THE MISSION FILE DOES NOT MENTION.
			//---
			//--- The documented model for a mission override - "not a merge; only keys present in the
			//--- mission JSON get overwritten" - holds for SCALARS and is false for arrays: an absent
			//--- array key leaves the field EMPTY rather than untouched, exactly like the initialiser
			//--- that does not survive deserialization.
			//---
			//--- Measured 2026-08-19 on a mission pois_settings.json carrying only `version` and
			//--- `override_poi_positions`: with the file present the scan reported `skipped by type 0`
			//--- and walked all 306 POIs; with the identical file renamed away, `skipped by type 229`
			//--- and 77 resolved. One variable, and the profile JSON on disk held the correct list the
			//--- whole time - which is what makes this so hard to see from the outside.
			//---
			//--- So each array is snapshotted and restored IF AND ONLY IF the mission pass left it
			//--- empty. A mission that really does specify one still overrides it; a mission that says
			//--- nothing about it no longer silently disables it.
			//---
			//--- Note this is NOT the mission-LOCK idiom (BattleRoyaleGameData.admins_steamid64),
			//--- which restores unconditionally to forbid the override outright. These fields are
			//--- legitimate mission content; the bug is only the silent clear.
			//---
			//--- The mod's older settings files happen to escape this because their mission JSONs
			//--- enumerate their arrays - zone_settings.json lists end_avoid_type and end_avoid_city -
			//--- so the trap has never fired before. Any NEW array field in any of these classes is
			//--- exposed to it, and hot_zone_centers / hot_zone_radii in BattleRoyaleZoneData are in
			//--- exactly that position today.
			//--- ⚠️ THE SNAPSHOT MUST BE A COPY OF THE CONTENTS, NOT OF THE REFERENCE.
			//---
			//--- `array<string> kept = poi_resolve_types;` compiles, reads correctly, and does
			//--- nothing at all: the deserializer CLEARS THE EXISTING ARRAY IN PLACE rather than
			//--- assigning a fresh one, so both names point at the same object and the "snapshot"
			//--- is emptied along with the field. Restoring it then restores an empty array.
			//---
			//--- Established by elimination on a live boot: had the loader assigned a NEW empty
			//--- array, the reference snapshot would have worked. It did not - the effective-config
			//--- line still logged `types [], excludes []` - so the clear is in place.
			ref array<string> keptTypes = CopyStrings(poi_resolve_types);
			ref array<string> keptExclude = CopyStrings(poi_scan_exclude);

			if (!JsonFileLoader<BattleRoyalePOIsData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);

			if (!poi_resolve_types || poi_resolve_types.Count() == 0)
				poi_resolve_types = keptTypes;

			if (!poi_scan_exclude || poi_scan_exclude.Count() == 0)
				poi_scan_exclude = keptExclude;
		}
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyalePOIsData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}

	override void Upgrade()
	{
		if (version < 2)
		{
			// v1 shipped two example overrides that were applied for real - GetOverrodePosition() is
			// keyed on the CfgWorlds class name, which is exactly what those entries carried, so on any
			// map using those names both POIs were dropped at the map corner. Clear them, but only when
			// the list is still verbatim what shipped, so an edited or extended list is left alone.
			if (IsShippedExampleOverride())
			{
				BattleRoyaleUtils.Warn("Removing the two example override_poi_positions entries - they were applied as real overrides.");
				override_poi_positions = new array<ref BattleRoyaleOverridePOIPosition>();
			}

			version = 2;
			Save();  // Save the upgraded config
		}

		if (version < 3)
		{
			//--- poi_scan_exclude is a `ref array`, and a field initialiser does NOT survive
			//--- deserialization for arrays the way it does for scalars - so on every server that
			//--- already has a v1/v2 pois_settings.json the list would load back EMPTY and every car
			//--- wreck on the map would count as town fabric. Refill it here.
			//---
			//--- Only when empty: an admin who deliberately cleared the list keeps their choice.
			if (!poi_scan_exclude || poi_scan_exclude.Count() == 0)
				poi_scan_exclude = {"Wreck", "Misc_Through", "Toilet_Mobile", "Wall_Gate", "Pipe_", "Misc_Fence"};

			version = 3;
			Save();  // Save the upgraded config
		}

		if (version < 4)
		{
			//--- Same `ref array` rule as poi_scan_exclude above, and this one is not hypothetical:
			//--- it was hit on the development machine within minutes of adding the field, because a
			//--- v3 pois_settings.json was already on disk. Left unrefilled the list loads EMPTY, and
			//--- an empty list is read as "no type filter", so a migrating server would silently lose
			//--- the guard and go back to resolving hills onto other people's towns.
			if (!poi_resolve_types || poi_resolve_types.Count() == 0)
				poi_resolve_types = {"Capital", "City", "Village"};

			version = 4;
			Save();  // Save the upgraded config
		}
	}

	//--- Element-wise copy into a NEW array. Assigning the field to a local only copies the reference,
	//--- and the JSON deserializer empties the existing array in place - so a reference "snapshot" is
	//--- cleared along with the thing it was meant to preserve. Always copy the contents.
	protected ref array<string> CopyStrings(array<string> source)
	{
		ref array<string> copy = new array<string>;

		if (!source)
			return copy;

		int i;
		for (i = 0; i < source.Count(); i++)
		{
			string value = source.Get(i);
			copy.Insert(value);
		}

		return copy;
	}

	//--- Clamp rather than halt. Runs after BOTH the profile and mission passes, so a mission override
	//--- is checked too. MUST NOT Save() - Load() re-saves before the mission pass, so persisting a
	//--- clamp would overwrite the admin's file permanently.
	override void Validate()
	{
		if (poi_scan_radius_m < 25.0)
		{
			BattleRoyaleUtils.Warn("poi_scan_radius_m " + poi_scan_radius_m + " is too small to contain a settlement - clamping to 25.");
			poi_scan_radius_m = 25.0;
		}

		if (poi_scan_iterations < 1)
		{
			BattleRoyaleUtils.Warn("poi_scan_iterations " + poi_scan_iterations + " would skip the scan entirely - clamping to 1.");
			poi_scan_iterations = 1;
		}

		if (poi_min_buildings < 1)
			poi_min_buildings = 1;

		//--- 0 or negative would reject every resolution, including the correct ones, and present as
		//--- the feature doing nothing at all. Anyone wanting that should set
		//--- resolve_poi_from_buildings = false, which says so.
		if (poi_max_shift_m <= 0.0)
		{
			BattleRoyaleUtils.Warn("poi_max_shift_m " + poi_max_shift_m + " would reject every resolution - clamping to 350. Use resolve_poi_from_buildings to turn the feature off.");
			poi_max_shift_m = 350.0;
		}

		//--- Outside (0, 1] the percentile indexes outside the sorted distance array. 1.0 is legal and
		//--- means "the furthest building", which is a defensible if generous reading of extent.
		if (poi_extent_percentile <= 0.0 || poi_extent_percentile > 1.0)
		{
			BattleRoyaleUtils.Warn("poi_extent_percentile " + poi_extent_percentile + " is outside (0, 1] - clamping to 0.8.");
			poi_extent_percentile = 0.8;
		}

		if (poi_extent_min_m < 0.0)
			poi_extent_min_m = 0.0;

		//--- An inverted pair would make every extent take whichever bound was applied last, silently.
		if (poi_extent_max_m < poi_extent_min_m)
		{
			BattleRoyaleUtils.Warn("poi_extent_max_m is below poi_extent_min_m - raising the max to match.");
			poi_extent_max_m = poi_extent_min_m;
		}
	}

	// True only when override_poi_positions is still exactly the pair of examples shipped in version 1.
	protected bool IsShippedExampleOverride()
	{
		if (!override_poi_positions || override_poi_positions.Count() != 2)
			return false;

		if (!MatchesEntry(override_poi_positions[0], "Settlement_Chernogorsk", 100, 200))
			return false;

		return MatchesEntry(override_poi_positions[1], "Settlement_Novodmitrovsk", 300, 400);
	}

	protected bool MatchesEntry(BattleRoyaleOverridePOIPosition entry, string expected_name, int expected_x, int expected_z)
	{
		if (!entry || entry.poi_name != expected_name)
			return false;

		if (!entry.new_position || entry.new_position.Count() != 2)
			return false;

		return (entry.new_position[0] == expected_x && entry.new_position[1] == expected_z);
	}

	vector GetOverrodePosition(string poi_name)
	{
		if( !m_OverrideSpawnPositions )
		{
			//--- Deliberately does NOT call Load() here. Everything reaching this method comes through
			//--- BattleRoyaleConfig.GetConfig(), which already ran Load() AND LoadMission(); re-reading
			//--- the profile JSON into this instance would overwrite the mission-folder overrides that
			//--- LoadMission() applied. The map build below stays lazy for exactly that reason - it has
			//--- to run after the mission pass, not during it.
			BattleRoyaleUtils.Trace("Load m_OverrideSpawnPositions!");
			m_OverrideSpawnPositions = new map<string, vector>();

			foreach(BattleRoyaleOverridePOIPosition position: override_poi_positions)
			{
				if( !position || !position.new_position || position.new_position.Count() < 2 )
				{
					//--- Warn, NOT Error: BattleRoyaleUtils.Error routes to the engine's Error2(), which
					//--- raises a VM exception and stops the script VM - on a server that kills init
					//--- right here, in MissionServer.OnInit -> BattleRoyaleServer.Init -> InitializePOIs,
					//--- with the stack landing in crash_*.log rather than the .rpt. This branch already
					//--- continues, so one malformed entry must never take the server down with it.
					BattleRoyaleUtils.Warn("Skipping malformed override_poi_positions entry - expected a [x, z] pair.");
					continue;
				}

				BattleRoyaleUtils.Trace(position.poi_name + " " + position.new_position);
				vector temp_pos;
				temp_pos[0] = position.new_position[0];
				temp_pos[2] = position.new_position[1];
				temp_pos[1] = GetGame().SurfaceY( temp_pos[0], temp_pos[2] );
				m_OverrideSpawnPositions.Set( position.poi_name, temp_pos );
			}
		}

		if( m_OverrideSpawnPositions.Contains( poi_name ) )
		{
			BattleRoyaleUtils.Trace("Asked for " + poi_name + " position!");
			return m_OverrideSpawnPositions.Get( poi_name );
		}

		return "0 0 0";
	}
};

class BattleRoyaleOverridePOIPosition
{
    // CfgWorlds class name of the POI being overridden - see the example in the class comment above.
    string poi_name;

    // [x, z] world position to use instead of the POI's real one. Y (height) is resolved at read
    // time via GetGame().SurfaceY() rather than stored, so this only ever needs the two values.
    // Must be `ref`: without it nothing strongly holds the array the ctor (or JSON deserialization)
    // assigns, and GetOverrodePosition() reads a destroyed object.
    ref array<int> new_position;

    // Every parameter defaults so the class stays default-constructible. JsonSerializer instantiates
    // the elements of a `ref array<ref ...>` itself, which is why every vanilla JSON helper class
    // (JsonUndergroundAreaBreadcrumb, BreadcrumbExternalValueController, ...) declares no constructor
    // at all. Deserialization does work without the defaults - a live run has produced populated
    // entries - so this is hygiene, not a fix: it stops the class depending on the engine's willingness
    // to construct a type that offers no no-argument path. Same shape as vanilla's NutritionalProfile.
    void BattleRoyaleOverridePOIPosition(string in_poi_name = "", array<int> in_new_position = NULL)
	{
		this.poi_name = in_poi_name;
		this.new_position = in_new_position;
	}
};
