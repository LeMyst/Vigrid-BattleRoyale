#ifdef SERVER
class LockServerWebhook
{
	protected string s_ServerToken;
	protected int i_TryLeft = 5;

	void LockServerWebhook(string server_token)
	{
		BattleRoyaleUtils.Trace("LockServerWebhook()");

		s_ServerToken = server_token;
	};

	void Send(bool lock_server)
	{
		BattleRoyaleUtils.Trace("LockServerWebhook().Send()");

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("LockServerWebhook: Too much try, giving up.");
			return;
		}

		string action;
		if ( lock_server )
			action = "lock";
		else
			action = "unlock";

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
        ctx.POST(new LockServerCallback( s_ServerToken, lock_server, i_TryLeft ) , arguments.ToQuery(string.Format("servers/%1", action)), jatString);
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};

	void LockServer()
	{
		Send( true );
	};

	void UnlockServer()
	{
		Send( false );
	};
};

class LockServerCallback: RestCallback
{
	protected string s_ServerToken;
	protected int i_TryLeft;
	protected bool b_LockServer;

	void LockServerCallback(string server_token, bool lock_server, int try_left)
	{
        BattleRoyaleUtils.Trace("LockServerCallback() " + try_left);

		s_ServerToken = server_token;
		i_TryLeft = try_left;
		b_LockServer = lock_server;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		LockServerWebhook serverWebhook = new LockServerWebhook( s_ServerToken );
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( b_LockServer );
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		LockServerWebhook serverWebhook = new LockServerWebhook( s_ServerToken );
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( b_LockServer );
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			BattleRoyaleUtils.Trace(data);
	};
};
