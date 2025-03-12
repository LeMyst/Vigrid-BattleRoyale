#ifdef SERVER
class BattleRoyalePOIsData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

	// Allow to override the position of the POIs
	ref array<ref BattleRoyaleOverridePOIPosition> override_poi_positions = {
		new BattleRoyaleOverridePOIPosition("POI1", { 100, 200 }),
		new BattleRoyaleOverridePOIPosition("POI2", { 300, 400 })
	};

	[NonSerialized()]
	ref map<string, vector> m_OverrideSpawnPositions;

	override string GetPath()
	{
		return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "pois_settings.json";
	}

	override void Load()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyalePOIsData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyalePOIsData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
	}

	vector GetOverrodePosition(string poi_name)
	{
		if( !m_OverrideSpawnPositions )
		{
			Load();

			BattleRoyaleUtils.Trace("Load m_OverrideSpawnPositions!");
			m_OverrideSpawnPositions = new map<string, vector>();

			foreach(BattleRoyaleOverridePOIPosition position: override_poi_positions)
			{
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
    string poi_name;
    array<int> new_position;

    void BattleRoyaleOverridePOIPosition(string in_poi_name, array<int> in_new_position)
	{
		this.poi_name = in_poi_name;
		this.new_position = in_new_position;
	}
};
#endif
