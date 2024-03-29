#ifdef SERVER
class WinWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void WinWebhook(string server_token)
	{
		Print("WinWebhook()");

		s_ServerToken = server_token;
	};

	void Send(string player_steamid)
	{
		Print("WinWebhook().Send()");

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("WinWebhook: Too much try, giving up.");
			return;
		}

		HttpArguments arguments = {
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		JSONAuthToken jsonAuthToken = new JSONAuthToken( s_ServerToken );

		string jatString;
		string jatError;
		JsonFileLoader<JSONAuthToken>.MakeData(jsonAuthToken, jatString, jatError);

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext("https://api.vigrid.ovh/");
        ctx.POST(new WinCallback( s_ServerToken, player_steamid, i_TryLeft ) , arguments.ToQuery(string.Format("players/%1/wins/add", player_steamid)), jatString);
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};
};

class WinCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected string s_PlayerSteamID;

	void WinCallback(string server_token, string player_steamid, int try_left)
	{
        Print("WinCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		s_PlayerSteamID = player_steamid;
	}

	override void OnError( int errorCode )
	{
		Print(" !!! OnError(): " + errorCode);

		WinWebhook serverWebhook = new WinWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_PlayerSteamID );
	};

	override void OnTimeout()
	{
		Print(" !!! OnTimeout() ");

		WinWebhook serverWebhook = new WinWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( s_PlayerSteamID );
	};

	override void OnSuccess( string data, int dataSize )
	{
		Print(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			Print(data);
	};
};
