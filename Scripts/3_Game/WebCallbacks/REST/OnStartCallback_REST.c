
class OnStartCallback_REST extends RestCallback {
    ref BattleRoyaleOnStartCallback br_callback;

    void OnStartCallback_REST(ref BattleRoyaleOnStartCallback callback)
    {
        br_callback = callback;
    }
    override void OnError(int errorCode)
    {
        br_callback.OnError(errorCode);
    }
    override void OnTimeout()
    {
        br_callback.OnTimeout();
    }
    override void OnSuccess( string data, int dataSize )
	{
        string result = data;
        Print("BattleRoyaleAPI::RequestStartAsync() => WEB RESULT: " + result);
        if(result == "")
		{
			Error("BattleRoyaleAPI::RequestStartAsync() => ERROR: web_result = NULL");
			br_callback.OnError( DAYZBR_NETWORK_ERRORCODE_NULL_RESULT );
            return;
		}

        JsonSerializer m_Serializer = new JsonSerializer;
		string error;

        ref ClientStartData m_ClientStartData;
		ref PlayerData m_PlayerData;
        ref RegionData m_RegionData;

		if(!m_Serializer.ReadFromString( m_ClientStartData, result, error ))
		{
            m_PlayerData = NULL; //failed to parse, make sure our object is null
            m_RegionData = NULL;
			Print("BattleRoyaleAPI::RequestStartAsync() => JSON Failed To Parse!");
			Error(error);
            br_callback.OnError( DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL_RESULT );
			return;
		}
        else
        {
            m_PlayerData = m_ClientStartData.player;
            m_RegionData = m_ClientStartData.region;
        }
        

        
        BattleRoyaleAPI.GetAPI().SetRegionData( m_RegionData );
        BattleRoyaleAPI.GetAPI().SetCurrentPlayer( m_PlayerData ); //update player in the BR api

		br_callback.OnSuccess( m_PlayerData );
    }
}
