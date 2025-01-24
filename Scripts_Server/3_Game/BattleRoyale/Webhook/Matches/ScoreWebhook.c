#ifdef SERVER
class ScoreWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void ScoreWebhook(string server_token)
	{
		Print("ScoreWebhook()");

		s_ServerToken = server_token;
	};

	void Send(string match_uuid, string player_steamid, int position)
	{
		Print("ScoreWebhook().Send()");

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("ScoreWebhook: Too much try, giving up.");
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
        ctx.POST(new ScoreCallback( s_ServerToken, match_uuid, player_steamid, position, i_TryLeft ) , arguments.ToQuery(string.Format("matches/%1/players/%2/score/%3", match_uuid, player_steamid, position )), jatString);
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};
};

class ScoreCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected string s_MatchUUID;
	protected string s_PlayerSteamID;
	protected int i_Position;

	void ScoreCallback(string server_token, string match_uuid, string player_steamid, int position, int try_left)
	{
        Print("ScoreCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		s_MatchUUID = match_uuid;
		s_PlayerSteamID = player_steamid;
		i_Position = position;
	}

	override void OnError( int errorCode )
	{
		Print(" !!! OnError(): " + errorCode);

		ScoreWebhook serverWebhook = new ScoreWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_MatchUUID, s_PlayerSteamID, i_Position );
	};

	override void OnTimeout()
	{
		Print(" !!! OnTimeout() ");

		ScoreWebhook serverWebhook = new ScoreWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_MatchUUID, s_PlayerSteamID, i_Position );
	};

	override void OnSuccess( string data, int dataSize )
	{
		Print(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			Print(data);
	};
};
