#ifdef SERVER
class LockServerWebhook: WebApiBase
{
	protected string s_ServerID, s_ServerSecret;
	protected int i_TryLeft = 5;

	void LockServerWebhook(string server_id, string server_secret)
	{
		Print("LockServerWebhook()");
		s_ServerID = server_id;
		s_ServerSecret = server_secret;

		m_RestContext.SetHeader("application/json");
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

		m_RestContext.POST( new LockServerCallback( s_ServerID, s_ServerSecret, lock_server, i_TryLeft ) , arguments.ToQuery(string.Format("/servers/%1/%2/%3", s_ServerID, s_ServerSecret, action)), "" );
	};

	string GetServerId()
	{
		return s_ServerID;
	};

	string GetServerSecret()
	{
		return s_ServerSecret;
	};

	void SetTryLeft(bool try_left)
	{
		i_TryLeft = try_left;
	};

	override string GetBaseUrl()
	{
		return "https://api.vigrid.ovh";
	};
};

class LockServerCallback: RestCallbackBase
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
		// override this with your implementation
		Print(" !!! OnError(): " + errorCode);
        LockServerWebhook serverWebhook = new LockServerWebhook(s_ServerID, s_ServerSecret);
        serverWebhook.SetTryLeft( i_TryLeft );
        serverWebhook.Send( b_LockServer );
	};

	/**
	\brief Called in case request timed out or handled improperly (no error, no success, no data)
	*/
	override void OnTimeout()
	{
		// override this with your implementation
		Print(" !!! OnTimeout() ");
        LockServerWebhook serverWebhook = new LockServerWebhook(s_ServerID, s_ServerSecret);
        serverWebhook.SetTryLeft( i_TryLeft );
        serverWebhook.Send( b_LockServer );
	};

	/**
	\brief Called when data arrived and/ or response processed successfully
	*/
	override void OnSuccess( string data, int dataSize )
	{
		// override this with your implementation
		Print(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			Print(data); // !!! NOTE: Print() will not output string longer than 1024b, check your dataSize !!!
	};
};
