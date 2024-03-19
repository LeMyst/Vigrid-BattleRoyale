#ifdef SERVER
class LockServerWebhook
{
	protected string s_ServerID, s_ServerSecret;
	protected int i_TryLeft = 5;

	void LockServerWebhook(string server_id, string server_secret)
	{
		Print("LockServerWebhook()");

		s_ServerID = server_id;
		s_ServerSecret = server_secret;
	};

	void LockServer()
	{
		Send( true );
	};

	void UnlockServer()
	{
		Send( false );
	};

	void Send(bool lock_server)
	{
		Print("LockServerWebhook().Send()");

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
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext("https://api.vigrid.ovh/");
        ctx.POST(new LockServerCallback( s_ServerID, s_ServerSecret, lock_server, i_TryLeft ) , arguments.ToQuery(string.Format("servers/%1/%2/%3", s_ServerID, s_ServerSecret, action)), "");
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};
};

class LockServerCallback: RestCallback
{
	protected string s_ServerID, s_ServerSecret;
	protected bool b_LockServer;
	protected int i_TryLeft;

	void LockServerCallback(string server_id, string server_secret, bool lock_server, int try_left)
	{
        Print("LockServerCallback() " + try_left);

		s_ServerID = server_id;
		s_ServerSecret = server_secret;
		b_LockServer = lock_server;
		i_TryLeft = try_left;
	}

	override void OnError( int errorCode )
	{
		Print(" !!! OnError(): " + errorCode);

		LockServerWebhook serverWebhook = new LockServerWebhook(s_ServerID, s_ServerSecret);
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.Send( b_LockServer );
	};

	override void OnTimeout()
	{
		Print(" !!! OnTimeout() ");

		LockServerWebhook serverWebhook = new LockServerWebhook(s_ServerID, s_ServerSecret);
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
