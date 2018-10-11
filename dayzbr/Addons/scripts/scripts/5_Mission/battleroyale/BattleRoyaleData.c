static string BRDataFileSaveLocation = "$profile:BRData.json";

class StaticBRData 
{
    vector debug_position = "14829.2 72.3148 14572.3";
	vector cherno_center = "6497.66 6.01245 2519.26";
	
	int minimum_players = 2;
	float play_area_size = 500.0;
	int shrink_type = 1; // 0 == coefficient, 1 == exponential, 2 == linear
	float shrink_coefficient = 0.75;

	float shrink_base = 2.718281828459; // ~ e
	float shrink_exponent = 2.0;
	float shrink_max_playtime = 15.0; // Measured in minutes

    int start_timer = 60; // Measured in seconds
    int zone_lock_time = 60; // Measured in seconds

    int wait_for_players = 5; // Measured in seconds
    int check_round_end = 5; // Measured in seconds
    int start_shrink_zone = 120; // Measured in seconds

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