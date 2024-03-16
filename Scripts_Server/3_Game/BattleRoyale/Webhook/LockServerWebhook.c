#ifdef SERVER
class LockServerWebhook: WebApiBase
{
	protected string s_ServerID, s_ServerSecret;

	void LockServerWebhook(string server_id, string server_secret)
	{
		Print("LockServerWebhook()");
		s_ServerID = server_id;
		s_ServerSecret = server_secret;

		m_RestContext.SetHeader("application/json");
	}

	void LockServer()
	{
		Send( true );
	}

	void UnlockServer()
	{
		Send( false );
	}

	void Send(bool lock_server)
	{
		Print("LockServerWebhook().Send()");

		string action;
		if ( lock_server )
			action = "lock";
		else
			action = "unlock";

		HttpArguments arguments = {
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};
		Print( arguments.ToQuery(string.Format("/servers/%1/%2/%3", s_ServerID, s_ServerSecret, action)) );

		m_RestContext.POST( new RestCallbackBase(), arguments.ToQuery(string.Format("/servers/%1/%2/%3", s_ServerID, s_ServerSecret, action)), "" );
	}

	string GetServerId()
	{
		return s_ServerID;
	}

	string GetServerSecret()
	{
		return s_ServerSecret;
	}

	override string GetBaseUrl()
	{
		return "https://api.vigrid.ovh";
	}
}
