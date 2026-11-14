#ifndef SERVER
enum MatchMakingRequestStatus
{
	MM_PENDING,
	MM_SUCCESS,
	MM_ERROR,
	MM_TIMEOUT
}

/**
 *  One asynchronous matchmaker request. The engine fills it in from its own thread of events and the
 *  main menu polls it from Update(), so nothing here needs to know about the menu (which compiles two
 *  stages later). Only the first outcome is kept: OnError can fire once per engine retry.
 */
class MatchMakingRequest : RestCallback
{
	protected int m_Status = MatchMakingRequestStatus.MM_PENDING;
	protected string m_Data;
	protected int m_ErrorCode;
	protected string m_Name;

	void MatchMakingRequest(string name)
	{
		m_Name = name;
	}

	int GetStatus()
	{
		return m_Status;
	}

	string GetData()
	{
		return m_Data;
	}

	int GetErrorCode()
	{
		return m_ErrorCode;
	}

	override void OnSuccess(string data, int dataSize)
	{
		if (m_Status != MatchMakingRequestStatus.MM_PENDING)
			return;

		m_Data = data;
		m_Status = MatchMakingRequestStatus.MM_SUCCESS;
		BattleRoyaleUtils.Trace("[MatchMaking] " + m_Name + " replied: " + data);
	}

	override void OnError(int errorCode)
	{
		if (m_Status != MatchMakingRequestStatus.MM_PENDING)
			return;

		m_ErrorCode = errorCode;
		m_Status = MatchMakingRequestStatus.MM_ERROR;
		BattleRoyaleUtils.Trace("[MatchMaking] " + m_Name + " failed, error " + errorCode);
	}

	override void OnTimeout()
	{
		if (m_Status != MatchMakingRequestStatus.MM_PENDING)
			return;

		m_Status = MatchMakingRequestStatus.MM_TIMEOUT;
		BattleRoyaleUtils.Trace("[MatchMaking] " + m_Name + " timed out");
	}
}

/**
 *  The stats site's /api/v2/matchmaking routes (repo LeMyst/br-hub, whose CLAUDE.md holds the
 *  contract). Both are POSTs with a JSON body and a JSON reply, and carry no token: a client has none.
 */
class MatchMakingWebhook
{
	/**
	 *  The client never creates the REST API itself elsewhere - it used to rely on another mod having
	 *  done it first. Diag builds can point at a local instance with -br-mm-endpoint=http://host:port/.
	 */
	protected static RestContext GetContext()
	{
		RestApi rest_api = GetRestApi();
		if (!rest_api)
			rest_api = CreateRestApi();

		if (!rest_api)
		{
			BattleRoyaleUtils.Warn("[MatchMaking] no REST API available");
			return NULL;
		}

		string endpoint = BATTLEROYALE_API_ENDPOINT;
#ifdef DIAG
		string diag_endpoint;
		if (GetCLIParam("br-mm-endpoint", diag_endpoint) && diag_endpoint != "")
		{
			endpoint = diag_endpoint;
			BattleRoyaleUtils.Info("[MatchMaking] endpoint overridden: " + endpoint);
		}
#endif

		RestContext ctx = rest_api.GetRestContext(endpoint);
		if (ctx)
			ctx.SetHeader("application/json");

		return ctx;
	}

	protected static MatchMakingRequest Post(string route, string body)
	{
		RestContext ctx = GetContext();
		if (!ctx)
			return NULL;

		MatchMakingRequest request = new MatchMakingRequest(route);
		ctx.POST(request, "api/v2/matchmaking/" + route, body);
		return request;
	}

	//! Is `server` ("ip:port", the -connect target) enabled for matchmaking? See ParseAvailable.
	static MatchMakingRequest CheckAvailability(string server)
	{
		MatchMakingAvailableBody body = new MatchMakingAvailableBody();
		body.server = server;

		string json;
		string error;
		if (!JsonFileLoader<MatchMakingAvailableBody>.MakeData(body, json, error, false))
			return NULL;

		return Post("available", json);
	}

	//! Ask for a lobby in `server`'s shard that this client's mods can join. See ParseServer / ParseRetryMs.
	static MatchMakingRequest Search(string server, array<ref ModInfo> mod_list)
	{
		MatchMakingSearchBody body = new MatchMakingSearchBody();
		body.server = server;
		foreach (ModInfo mod_info : mod_list)
		{
			//--- Vanilla and DLC entries say nothing about whether a client can join; the server skips them too.
			if (mod_info.GetDefault() || mod_info.GetIsDLC())
				continue;

			body.mods.Insert(mod_info.GetName(), mod_info.GetVersion());
		}

		string json;
		string error;
		if (!JsonFileLoader<MatchMakingSearchBody>.MakeData(body, json, error, false))
			return NULL;

		return Post("search", json);
	}

	//! {"available": true} - anything else, an unreadable reply included, is false.
	static bool ParseAvailable(string data)
	{
		MatchMakingAvailableReply reply = new MatchMakingAvailableReply();
		string error;
		if (!JsonFileLoader<MatchMakingAvailableReply>.LoadData(data, reply, error))
		{
			BattleRoyaleUtils.Warn("[MatchMaking] unreadable availability reply: " + error);
			return false;
		}

		return reply.available;
	}

	//! {"status": "found", "ip", "port", "password"}. False on anything else.
	static bool ParseServer(string data, out string ip, out int port, out string password)
	{
		MatchMakingSearchReply reply = ReadSearchReply(data);
		if (!reply || reply.status != "found")
			return false;

		ip = reply.ip.Trim();
		port = reply.port;
		password = reply.password;
		return ip != "" && port > 0;
	}

	/**
	 *  {"status": "wait", "retry_ms"}: the site's own retry hint. Clamped so a bad hint can neither
	 *  hammer the API nor stall the search; a missing or unreadable one gets the default.
	 */
	static int ParseRetryMs(string data)
	{
		MatchMakingSearchReply reply = ReadSearchReply(data);
		if (!reply || reply.retry_ms <= 0)
			return BR_MATCHMAKING_RETRY_DEFAULT_MS;

		return Math.Clamp(reply.retry_ms, BR_MATCHMAKING_RETRY_MIN_MS, BR_MATCHMAKING_RETRY_MAX_MS);
	}

	protected static MatchMakingSearchReply ReadSearchReply(string data)
	{
		//--- A match with no password is sent as null; read it as an empty string rather than trust the
		//--- serializer with a null in a string field.
		string json = data;
		json.Replace("\"password\":null", "\"password\":\"\"");
		json.Replace("\"password\": null", "\"password\":\"\"");

		MatchMakingSearchReply reply = new MatchMakingSearchReply();
		string error;
		if (!JsonFileLoader<MatchMakingSearchReply>.LoadData(json, reply, error))
		{
			BattleRoyaleUtils.Warn("[MatchMaking] unreadable search reply: " + error);
			return NULL;
		}

		return reply;
	}
}

//--- Request and reply bodies. Field names are the wire format: renaming one breaks the contract.

class MatchMakingAvailableBody
{
	string server;
}

class MatchMakingSearchBody
{
	string server;
	ref map<string, string> mods;

	void MatchMakingSearchBody()
	{
		mods = new map<string, string>();
	}
}

class MatchMakingAvailableReply
{
	bool available;
}

class MatchMakingSearchReply
{
	string status;     // found | wait
	string ip;
	int port;
	string password;
	int retry_ms;
}
#endif
