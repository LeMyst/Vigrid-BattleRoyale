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

	void LockServer(bool lock_server)
	{
		Print("LockServerWebhook().Send()");

		HttpArguments arguments = {
			new HttpArgument("id", s_ServerID),
			new HttpArgument("secret", s_ServerSecret),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};
		Print(arguments.ToQuery("lockserver"));

		if ( lock_server )
			m_RestContext.GET(new RestCallbackBase(), arguments.ToQuery("lockserver.php"));
		else
			m_RestContext.GET(new RestCallbackBase(), arguments.ToQuery("unlockserver.php"));
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
		return "https://api.vigrid.ovh/";
	}
}
