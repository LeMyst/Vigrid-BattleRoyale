static string BRDataFileSaveLocation = "$profile:BRData.json";

class StaticBRData 
{
	//Array of items/clothing to give the player before the round starts.
	ref array<string> Player_Items = {"TrackSuitJacket_Red","TrackSuitPants_Red","JoggingShoes_Red"};
	
    vector debug_position = "14829.2 72.3148 14572.3";
	vector cherno_center = "6497.66 6.01245 2519.26";
	
	int num_parallel_matches = 1; // Must be > 0 | The number of matches to run simultaneously
	int death_show_names = 1; // 0 == off, 1 == on, 2 == including distance

	int minimum_players = 8;
	int reference_min_players = 8; // the reference amount of min players for the max area size
	int reference_max_players = 50; // the reference amount of max players for the max area size
	float min_play_area_size = 500.0; // default max area size for 8 people
	float max_play_area_size = 2000.0; // default max area size for 50 people
	int adjustment_type = 0; // adjustment of playtime/areasize by player - 0 == linear

	int shrink_type = 1; // 0 == coefficient, 1 == exponential, 2 == linear
	float shrink_coefficient = 0.75;

	float shrink_base = 2.718281828459; // ~ e
	float shrink_exponent = 2.0;
	float shrink_min_playtime = 10.0; // Measured in minutes
	float shrink_max_playtime = 30.0; // Measured in minutes

    int start_timer = 60; // time to wait until the round starts when the min player count was reached measured in seconds
    int zone_lock_time = 60; // time between zone shrink and punishing players outside the new zone measured in seconds

    int wait_for_players = 5; // Measured in seconds
    int check_round_end = 5; // Measured in seconds
    int start_shrink_zone = 120; // time until the zoning starts measured in seconds
	int shrink_zone_every = 120; // time between zone shrinks measured in seconds

	int include_start_shrink_zone_in_roundtime = 1; // 0 == off, 1 == on

    static ref StaticBRData LoadDataServer()
    {
        ref StaticBRData data = new ref StaticBRData;

        if ( FileExist(BRDataFileSaveLocation) )
        {
            Print( "BR Data exists, loading!" );
            JsonFileLoader<StaticBRData>.JsonLoadFile(BRDataFileSaveLocation, data);
        } else {
            Print( "BR Data doesn't exist, creating file!" );
            SaveDataServer( data );
        }

        return data;
    }

    static void SaveDataServer( ref StaticBRData data )
    {
        JsonFileLoader<StaticBRData>.JsonSaveFile(BRDataFileSaveLocation, data);
    }
}