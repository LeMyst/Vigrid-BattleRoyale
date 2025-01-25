#ifdef SERVER
class StartMatchWebhook
{
	protected string s_ServerID, s_ServerSecret;
	protected int i_TryLeft = 5;

	void StartMatchWebhook(string server_id, string server_secret)
	{
		BattleRoyaleUtils.Trace("StartMatchWebhook()");

		s_ServerID = server_id;
		s_ServerSecret = server_secret;
	};

	void Send(bool lock_server)
	{
		BattleRoyaleUtils.Trace("StartMatchWebhook().Send()");

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("StartMatchWebhook: Too much try, giving up.");
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

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
        ctx.POST(new StartMatchCallback( s_ServerID, s_ServerSecret, lock_server, i_TryLeft ) , arguments.ToQuery(string.Format("servers/%1/%2/%3", s_ServerID, s_ServerSecret, action)), "");
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};
};

class StartMatchCallback: RestCallback
{
	protected string s_ServerID, s_ServerSecret;
	protected bool b_LockServer;
	protected int i_TryLeft;

	void StartMatchCallback(string server_id, string server_secret, bool lock_server, int try_left)
	{
        BattleRoyaleUtils.Trace("StartMatchCallback() " + try_left);

		s_ServerID = server_id;
		s_ServerSecret = server_secret;
		b_LockServer = lock_server;
		i_TryLeft = try_left;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		StartMatchWebhook serverWebhook = new StartMatchWebhook(s_ServerID, s_ServerSecret);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( b_LockServer );
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		StartMatchWebhook serverWebhook = new StartMatchWebhook(s_ServerID, s_ServerSecret);
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
