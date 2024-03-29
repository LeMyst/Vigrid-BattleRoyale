#ifdef SERVER
class StartMatchWebhook
{
	protected string s_ServerID, s_ServerSecret;
	protected int i_TryLeft = 5;

	void StartMatchWebhook(string server_id, string server_secret)
	{
		Print("StartMatchWebhook()");

		s_ServerID = server_id;
		s_ServerSecret = server_secret;
	};

	void Send(bool lock_server)
	{
		Print("StartMatchWebhook().Send()");

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
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext("https://api.vigrid.ovh/");
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
        Print("StartMatchCallback() " + try_left);

		s_ServerID = server_id;
		s_ServerSecret = server_secret;
		b_LockServer = lock_server;
		i_TryLeft = try_left;
	}

	override void OnError( int errorCode )
	{
		Print(" !!! OnError(): " + errorCode);

		StartMatchWebhook serverWebhook = new StartMatchWebhook(s_ServerID, s_ServerSecret);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( b_LockServer );
	};

	override void OnTimeout()
	{
		Print(" !!! OnTimeout() ");

		StartMatchWebhook serverWebhook = new StartMatchWebhook(s_ServerID, s_ServerSecret);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( b_LockServer );
	};

	override void OnSuccess( string data, int dataSize )
	{
		Print(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			Print(data);
	};
};
