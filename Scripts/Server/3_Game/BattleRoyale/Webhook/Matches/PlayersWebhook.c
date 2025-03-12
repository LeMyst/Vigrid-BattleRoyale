#ifdef SERVER
class PlayersWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void PlayersWebhook(string server_token)
	{
		BattleRoyaleUtils.Trace("PlayersWebhook()");

		s_ServerToken = server_token;
	};

	void postPlayers(string match_uuid, map<string, string> players)
	{
		BattleRoyaleUtils.Trace("PlayersWebhook().postPlayers()");
		Print( players );

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("PlayersWebhook: Too much try, giving up.");
			return;
		}

		HttpArguments arguments = {
			new HttpArgument("version", "1"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		JATPlayers jatPlayers = new JATPlayers( s_ServerToken, players );

		string jatString;
		string jatError;
		JsonFileLoader<JATPlayers>.MakeData(jatPlayers, jatString, jatError);

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
        ctx.POST(new PlayersCallback( s_ServerToken, match_uuid, players, i_TryLeft ) , arguments.ToQuery(string.Format("matches/%1/players", match_uuid)), jatString);
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};
};

class PlayersCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected string s_MatchUUID;
	protected map<string, string> a_Players;

	void PlayersCallback(string server_token, string match_uuid, map<string, string> players, int try_left)
	{
        BattleRoyaleUtils.Trace("PlayersCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		s_MatchUUID = match_uuid;
		a_Players = players;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		PlayersWebhook serverWebhook = new PlayersWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.postPlayers( s_MatchUUID, a_Players );
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		PlayersWebhook serverWebhook = new PlayersWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.postPlayers( s_MatchUUID, a_Players );
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			BattleRoyaleUtils.Trace(data);
	};
};
