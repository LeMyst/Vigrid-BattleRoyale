class MatchmakeCallback_REST extends RestCallback {
    ref BattleRoyaleMatchmakeCallback br_callback;

    void MatchmakeCallback_REST(ref BattleRoyaleMatchmakeCallback callback)
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
        Print("BattleRoyaleAPI::RequestMatchmakeAsync() => WEB RESULT: " + result);

        if(result == "")
		{
			Error("BattleRoyaleAPI::RequestMatchmakeAsync() => ERROR: web_result = NULL");
            br_callback.OnError( DAYZBR_NETWORK_ERRORCODE_NULL_RESULT );
			return;
		}

        if(result.IndexOf("WAIT") == 0)
        {
            //-- wait request
            Print("BattleRoyaleAPI::RequestMatchmakeAsync() => RETURND `WAIT` VALUE");
            ref MatchmakingWaitResult return_value = new MatchmakingWaitResult;
            br_callback.OnSuccess( return_value );
			return;
        }

		JsonSerializer m_Serializer = new JsonSerializer;
		ref ServerData p_ServerWebData;
		string error;
		if(!m_Serializer.ReadFromString( p_ServerWebData, result, error ))
		{
			Print("BattleRoyaleAPI::RequestMatchmakeAsync() => JSON Failed To Parse!");
			br_callback.OnError( DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL_RESULT );
			return;
		}

        br_callback.OnSuccess( p_ServerWebData );
    }
}