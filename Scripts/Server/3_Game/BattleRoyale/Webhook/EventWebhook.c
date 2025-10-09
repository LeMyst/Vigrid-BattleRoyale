#ifdef SERVER
class EventWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void EventWebhook(string server_token)
	{
		BattleRoyaleUtils.Trace("EventWebhook()");

		s_ServerToken = server_token;
	};

	void Send(string match_uuid, string event_name, string json_data)
	{
		BattleRoyaleUtils.Trace("EventWebhook().Send()");
		BattleRoyaleUtils.Trace( event_name );
		BattleRoyaleUtils.Trace( json_data );

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("EventWebhook: Too much try, giving up.");
			return;
		}

		HttpArguments arguments = {
			new HttpArgument("version", "1"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		JATEvent jatEvent = new JATEvent( s_ServerToken, event_name, json_data );

		string jatString;
		string jatError;
		JsonFileLoader<JATEvent>.MakeData(jatEvent, jatString, jatError);

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
        ctx.POST(new EventCallback( s_ServerToken, match_uuid, event_name, json_data, i_TryLeft ) , arguments.ToQuery(string.Format("events/%1", match_uuid)), jatString);
	};

	void SetTryLeft(int try_left)
	{
		i_TryLeft = try_left;
	};
};

class EventCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected string s_MatchUUID;
	protected string s_EventName;
	protected string s_JSONData;

	void EventCallback(string server_token, string match_uuid, string event_name, string json_data, int try_left)
	{
        BattleRoyaleUtils.Trace("EventCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		s_MatchUUID = match_uuid;
		s_EventName = event_name;
		s_JSONData = json_data;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		EventWebhook serverWebhook = new EventWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_MatchUUID, s_EventName, s_JSONData );
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		EventWebhook serverWebhook = new EventWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_MatchUUID, s_EventName, s_JSONData );
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			BattleRoyaleUtils.Trace(data);
	};
};
