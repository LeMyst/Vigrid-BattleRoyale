#ifdef SERVER
class StartMatchWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void StartMatchWebhook(string server_token)
	{
		BattleRoyaleUtils.Trace("StartMatchWebhook()");

		s_ServerToken = server_token;
	};

	void startMatch( string match_uuid )
	{
		BattleRoyaleUtils.Trace("StartMatchWebhook().startMatch()");

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("StartMatchWebhook: Too much try, giving up.");
			return;
		}

		HttpArguments arguments = {
			new HttpArgument("version", "1"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		JSONAuthToken jsonAuthToken = new JSONAuthToken( s_ServerToken );

		string jatString;
		string jatError;
		JsonFileLoader<JSONAuthToken>.MakeData(jsonAuthToken, jatString, jatError);

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
        ctx.POST(new StartMatchCallback( s_ServerToken, match_uuid, i_TryLeft ) , arguments.ToQuery(string.Format("matches/%1/start", match_uuid)), jatString);
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};
};

class StartMatchCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected string s_MatchUUID;

	void StartMatchCallback(string server_token, string match_uuid, int try_left)
	{
        BattleRoyaleUtils.Trace("StartMatchCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		s_MatchUUID = match_uuid;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		StartMatchWebhook serverWebhook = new StartMatchWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.startMatch( s_MatchUUID );
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		StartMatchWebhook serverWebhook = new StartMatchWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.startMatch( s_MatchUUID );
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			BattleRoyaleUtils.Trace(data);
	};
};
