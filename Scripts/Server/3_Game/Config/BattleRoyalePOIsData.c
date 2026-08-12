#ifdef SERVER
class BattleRoyalePOIsData: BattleRoyaleDataBase
{
	int version = 2;  // Config version

	// Allow to override the position of the POIs.
	// Empty by default ON PURPOSE: these entries are applied for real, they are not inert placeholders
	// like the dummy SteamID in general_settings.json. The key is the CfgWorlds class name (the same
	// name BattleRoyaleZone.InitializePOIs and 4_BattleRoyalePrepare look up), the value an [x, z] pair.
	// Example:
	//   new BattleRoyaleOverridePOIPosition("Settlement_Chernogorsk", { 6600, 2800 })
	ref array<ref BattleRoyaleOverridePOIPosition> override_poi_positions = {};

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
			if (!JsonFileLoader<BattleRoyalePOIsData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);
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
