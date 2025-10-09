#ifdef SERVER
class WinWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void WinWebhook(string server_token)
	{
		BattleRoyaleUtils.Trace("WinWebhook()");

		s_ServerToken = server_token;
	};

	void Send(string match_uuid, string player_steamid)
	{
		BattleRoyaleUtils.Trace("WinWebhook().Send()");

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("WinWebhook: Too much try, giving up.");
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
        ctx.POST(new WinCallback( s_ServerToken, match_uuid, player_steamid, i_TryLeft ) , arguments.ToQuery(string.Format("players/%1/wins/%2/add", player_steamid, match_uuid )), jatString);
	};

	void SetTryLeft(int try_left)
	{
		i_TryLeft = try_left;
	};
};

class WinCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected string s_MatchUUID;
	protected string s_PlayerSteamID;

	void WinCallback(string server_token, string match_uuid, string player_steamid, int try_left)
	{
        BattleRoyaleUtils.Trace("WinCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		s_MatchUUID = match_uuid;
		s_PlayerSteamID = player_steamid;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		WinWebhook serverWebhook = new WinWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_MatchUUID, s_PlayerSteamID );
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		WinWebhook serverWebhook = new WinWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_MatchUUID, s_PlayerSteamID );
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			BattleRoyaleUtils.Trace(data);
	};
};
