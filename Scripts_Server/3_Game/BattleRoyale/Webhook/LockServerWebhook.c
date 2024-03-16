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

	void Send()
	{
		Print("LockServerWebhook().Send()");

		HttpArguments arguments = {
			new HttpArgument("id", s_ServerID),
			new HttpArgument("secret", s_ServerSecret)
		};
		Print(arguments.ToQuery("lockserver"));

		m_RestContext.GET(new RestCallbackBase(), arguments.ToQuery("lockserver.php"));
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
