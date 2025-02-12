#ifdef SERVER
class PartiesWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void PartiesWebhook(string server_token)
	{
		BattleRoyaleUtils.Trace("PartiesWebhook()");

		s_ServerToken = server_token;
	};

	void postParties(string match_uuid, array<ref map<string, string>> parties)
	{
		BattleRoyaleUtils.Trace("PartiesWebhook().postParties()");
		Print( parties );

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("PartiesWebhook: Too much try, giving up.");
			return;
		}

		HttpArguments arguments = {
			new HttpArgument("version", "2"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		JATParties jatParties = new JATParties( s_ServerToken, parties );

		string jatString;
		string jatError;
		JsonFileLoader<JATParties>.MakeData(jatParties, jatString, jatError);

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
        ctx.POST(new PartiesCallback( s_ServerToken, match_uuid, parties, i_TryLeft ) , arguments.ToQuery(string.Format("matches/%1/parties", match_uuid)), jatString);
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};
};

class PartiesCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected string s_MatchUUID;
	protected array<ref map<string, string>> a_Parties;

	void PartiesCallback(string server_token, string match_uuid, array<ref map<string, string>> parties, int try_left)
	{
        BattleRoyaleUtils.Trace("PartiesCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		s_MatchUUID = match_uuid;
		a_Parties = parties;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		PartiesWebhook serverWebhook = new PartiesWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.postParties( s_MatchUUID, a_Parties );
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		PartiesWebhook serverWebhook = new PartiesWebhook(s_ServerToken);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.postParties( s_MatchUUID, a_Parties );
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			BattleRoyaleUtils.Trace(data);
	};
};
