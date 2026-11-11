#ifdef SERVER
/**
 *  Client for the stats site's /api/v2 (repo LeMyst/br-hub, whose CLAUDE.md holds the contract).
 *
 *  Every call is a POST whose JSON body carries the API key as `token`: RestContext.SetHeader only
 *  sets the Content-Type, so the engine cannot send an Authorization header. Every v2 write is
 *  idempotent on the site's side, so a timeout or a server error is simply sent again, BR_API_TRIES
 *  in all. A client error (bad key, bad body) is final: sending it again cannot help.
 *
 *  Bodies carry the key and, for RCon, a password: never log one.
 */
class BattleRoyaleApi
{
	protected static bool s_Enabled;
	protected static string s_Url;
	protected static string s_Key;

	static void Configure(bool enabled, string url, string key)
	{
		s_Enabled = enabled;
		s_Url = url.Trim();
		s_Key = key.Trim();
		if (s_Url != "" && s_Url.Substring(s_Url.Length() - 1, 1) != "/")
			s_Url = s_Url + "/";
	}

	static bool IsEnabled()
	{
		return s_Enabled && s_Url != "" && s_Key != "";
	}

	static string GetKey()
	{
		return s_Key;
	}

	static string GetUrl()
	{
		return s_Url;
	}

	//! `body` is the serialised JSON, already carrying `token`. `handler` may be NULL.
	static void Post(string path, string body, BattleRoyaleApiHandler handler = NULL)
	{
		if (!IsEnabled())
			return;

		BattleRoyaleApiRequest request = new BattleRoyaleApiRequest(path, body, handler, BR_API_TRIES);
		request.Send();
	}
};

class BattleRoyaleApiHandler: Managed
{
	//! The body of a successful reply.
	void OnReply(string data) {}

	//! A client error, or every try failed.
	void OnFailed() {}
};

class BattleRoyaleApiRequest: RestCallback
{
	protected string m_Path;
	protected string m_Body;
	protected ref BattleRoyaleApiHandler m_Handler;
	protected int m_TryLeft;

	void BattleRoyaleApiRequest(string path, string body, BattleRoyaleApiHandler handler, int try_left)
	{
		m_Path = path;
		m_Body = body;
		m_Handler = handler;
		m_TryLeft = try_left;
	}

	void Send()
	{
		m_TryLeft = m_TryLeft - 1;

		RestApi rest_api = GetRestApi();
		if (!rest_api)
			rest_api = CreateRestApi();

		RestContext ctx = rest_api.GetRestContext(BattleRoyaleApi.GetUrl());
		ctx.SetHeader("application/json");
		ctx.POST(this, "api/v2/" + m_Path, m_Body);
	}

	override void OnError(int errorCode)
	{
		if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR)
		{
			BattleRoyaleUtils.Warn("[API] " + m_Path + " refused (client error " + errorCode + "), not retried");
			Fail();
			return;
		}

		Retry("error " + errorCode);
	}

	override void OnTimeout()
	{
		Retry("timeout");
	}

	override void OnSuccess(string data, int dataSize)
	{
		BattleRoyaleUtils.Trace("[API] " + m_Path + " ok, " + dataSize + " bytes");
		if (m_Handler)
			m_Handler.OnReply(data);
	}

	protected void Retry(string why)
	{
		if (m_TryLeft <= 0)
		{
			BattleRoyaleUtils.Warn("[API] " + m_Path + " failed (" + why + "), giving up");
			Fail();
			return;
		}

		BattleRoyaleUtils.Warn("[API] " + m_Path + " failed (" + why + "), " + m_TryLeft + " tries left");
		BattleRoyaleApiRequest next = new BattleRoyaleApiRequest(m_Path, m_Body, m_Handler, m_TryLeft);
		next.Send();
	}

	protected void Fail()
	{
		if (m_Handler)
			m_Handler.OnFailed();
	}
};

//--- Request and reply bodies. Field names are the wire format: renaming one breaks the contract.

class BattleRoyaleApiHeartbeatBody
{
	string token;
	string state;      // lobby | countdown | in_match | ended
	int players;
	int max_players;
	string world;      // the map name. `map` is a type name in EnfusionScript and cannot be a member
	int uptime_s;
	string match_id;
};

class BattleRoyaleApiCreateMatchBody
{
	string token;
	ref map<string, string> mods;
	string password;

	void BattleRoyaleApiCreateMatchBody()
	{
		mods = new map<string, string>();
	}
};

class BattleRoyaleApiMatchReply
{
	string match_id;
};

class BattleRoyaleApiMember
{
	string uid;
	string name;
	int group_index;
};

class BattleRoyaleApiStartBody
{
	string token;
	ref array<ref BattleRoyaleApiMember> players;

	void BattleRoyaleApiStartBody()
	{
		players = new array<ref BattleRoyaleApiMember>();
	}
};

class BattleRoyaleApiRconBody
{
	string token;
	string rcon_password;
	string action;     // lock | unlock
};

//! Fire-and-forget calls with no reply worth reading.
class BattleRoyaleApiCalls
{
	static void Rcon(string rcon_password, bool lock)
	{
		if (!BattleRoyaleApi.IsEnabled() || rcon_password == "")
			return;

		BattleRoyaleApiRconBody body = new BattleRoyaleApiRconBody();
		body.token = BattleRoyaleApi.GetKey();
		body.rcon_password = rcon_password;
		body.action = "unlock";
		if (lock)
			body.action = "lock";

		string json;
		string error;
		if (!JsonFileLoader<BattleRoyaleApiRconBody>.MakeData(body, json, error, false))
			return;

		BattleRoyaleUtils.Info("[API] RCon " + body.action);
		BattleRoyaleApi.Post("servers/rcon", json);
	}
};
#endif
