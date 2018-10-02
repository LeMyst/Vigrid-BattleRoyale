static string BRDataFileSaveLocation = "$profile:BRData.json";

class StaticBRData 
{
    vector debug_position = "14829.2 72.3148 14572.3";
	vector cherno_center = "6497.66 6.01245 2519.26";
	
	int minimum_players = 2;
	float play_area_size = 500.0;
	float shrink_coefficient = 0.75;

    static ref StaticBRData LoadDataServer()
    {
        ref StaticBRData data = new ref StaticBRData;

        JsonFileLoader<CharacterData>.JsonLoadFile(BRDataFileSaveLocation, data);

        return data;
    }

    static void SaveDataServer( ref StaticBRData data )
    {
        JsonFileLoader<CharacterData>.JsonSaveFile(BRDataFileSaveLocation, data);
    }
}