#ifdef SERVER
/**
 *  Steam Web API lookup for players who connected without setting a name in the launcher.
 *
 *  Shaped like EventWebhook: an i_TryLeft counter carried into the callback, which rebuilds the
 *  webhook and re-Sends from OnError/OnTimeout until the budget runs out.
 *
 *  The API key must never reach a log. It is passed as an HttpArgument and every trace here
 *  prints the bare endpoint path, never the composed query - which is also why Send() does not
 *  log the URL it is about to request.
 */
class SteamNameWebhook
{
	//--- GetPlayerSummaries accepts up to 100 comma-separated ids per call, so a whole lobby is
	//--- one request rather than one per player.
	static const int STEAM_MAX_IDS_PER_CALL = 100;

	protected static const string STEAM_API_ENDPOINT = "https://api.steampowered.com/";
	protected static const string STEAM_SUMMARIES_PATH = "ISteamUser/GetPlayerSummaries/v0002/";

	protected string s_ApiKey;
	protected int i_TryLeft = 3;

	//--- ctx.GET() does not take ownership of the callback, so a bare `new SteamNameCallback(...)`
	//--- is unreachable the moment Send() returns and the VM reports it at shutdown:
	//--- "Leaked 'SteamNameCallback' script instance (1x)!", plus the array<string> it holds. Parking
	//--- it in a static ref keeps it owned; each request replaces the previous one, which is safe
	//--- because a replaced callback has already fired.
	protected static ref SteamNameCallback s_InFlight;

	void SteamNameWebhook(string api_key)
	{
		BattleRoyaleUtils.Trace("SteamNameWebhook()");

		s_ApiKey = api_key;
	};

	void Send(array<string> uids)
	{
		BattleRoyaleUtils.Trace("SteamNameWebhook().Send()");

		if (!uids)
			return;
		if (uids.Count() == 0)
			return;

		if (s_ApiKey == "")
		{
			//--- Warn, never Error: BattleRoyaleUtils.Error raises a VM exception and would take the
			//--- server down over a missing optional key.
			BattleRoyaleUtils.Warn("SteamNameWebhook: steam_web_api_key is empty, cannot resolve names.");
			return;
		}

		if (i_TryLeft <= 0)
		{
			BattleRoyaleUtils.Warn("SteamNameWebhook: too many retries, giving up on " + uids.Count() + " name(s).");
			return;
		}

		string id_list = "";
		int count = uids.Count();
		if (count > STEAM_MAX_IDS_PER_CALL)
			count = STEAM_MAX_IDS_PER_CALL;

		for (int i = 0; i < count; i++)
		{
			if (i > 0)
				id_list = id_list + ",";

			id_list = id_list + uids.Get(i);
		}

		//--- Copy rather than alias: the caller's array is the live pending queue and is cleared
		//--- the moment we return, but a retry needs these ids again minutes later.
		array<string> sent_uids = new array<string>;
		for (int j = 0; j < count; j++)
		{
			sent_uids.Insert(uids.Get(j));
		}

		HttpArguments arguments = {
			new HttpArgument("key", s_ApiKey),
			new HttpArgument("steamids", id_list)
		};

		i_TryLeft = i_TryLeft - 1;

		BattleRoyaleUtils.Debug("SteamNameWebhook: requesting " + count + " name(s) from " + STEAM_SUMMARIES_PATH);

		s_InFlight = new SteamNameCallback(s_ApiKey, sent_uids, i_TryLeft);

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(STEAM_API_ENDPOINT);
		ctx.GET(s_InFlight, arguments.ToQuery(STEAM_SUMMARIES_PATH));

		//--- Anything past the first 100 goes out as its own request.
		if (uids.Count() > count)
		{
			array<string> remainder = new array<string>;
			for (int k = count; k < uids.Count(); k++)
			{
				remainder.Insert(uids.Get(k));
			}

			SteamNameWebhook overflow = new SteamNameWebhook(s_ApiKey);
			overflow.Send(remainder);
		}
	};

	void SetTryLeft(int try_left)
	{
		i_TryLeft = try_left;
	};
};

class SteamNameCallback: RestCallback
{
	protected string s_ApiKey;
	protected ref array<string> a_Uids;
	protected int i_TryLeft;

	void SteamNameCallback(string api_key, array<string> uids, int try_left)
	{
		BattleRoyaleUtils.Trace("SteamNameCallback() " + try_left);

		s_ApiKey = api_key;
		a_Uids = uids;
		i_TryLeft = try_left;
	}

	override void OnError(int errorCode)
	{
		BattleRoyaleUtils.Warn("SteamNameCallback.OnError(): " + errorCode);

		Retry();
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Warn("SteamNameCallback.OnTimeout()");

		Retry();
	};

	override void OnSuccess(string data, int dataSize)
	{
		BattleRoyaleUtils.Trace("SteamNameCallback.OnSuccess() size=" + dataSize);

		if (dataSize <= 0)
		{
			BattleRoyaleUtils.Warn("SteamNameCallback: empty response from Steam.");
			return;
		}

		SteamSummariesEnvelope envelope = new SteamSummariesEnvelope();

		string error_message;
		if (!JsonFileLoader<SteamSummariesEnvelope>.LoadData(data, envelope, error_message))
		{
			BattleRoyaleUtils.Warn("SteamNameCallback: cannot parse Steam response: " + error_message);
			return;
		}

		if (!envelope.response)
		{
			BattleRoyaleUtils.Warn("SteamNameCallback: Steam response carried no 'response' object.");
			return;
		}

		if (!envelope.response.players)
		{
			BattleRoyaleUtils.Warn("SteamNameCallback: Steam response carried no 'players' array.");
			return;
		}

		//--- A private profile is simply absent from the array rather than an error, so a short
		//--- answer is normal and those players keep the name they connected with.
		int count = envelope.response.players.Count();
		BattleRoyaleUtils.Debug("SteamNameCallback: Steam returned " + count + " of " + a_Uids.Count() + " requested name(s).");

		for (int i = 0; i < count; i++)
		{
			SteamPlayerSummary summary = envelope.response.players.Get(i);
			if (!summary)
				continue;

			BattleRoyaleNameService.Apply(summary.steamid, summary.personaname);
		}
	};

	protected void Retry()
	{
		if (i_TryLeft <= 0)
			return;

		SteamNameWebhook webhook = new SteamNameWebhook(s_ApiKey);
		webhook.SetTryLeft(i_TryLeft);
		webhook.Send(a_Uids);
	}
};

//--- Only the two fields we need are declared; JsonSerializer ignores every other key Steam sends.
class SteamPlayerSummary
{
	string steamid;
	string personaname;
};

class SteamSummariesResponse
{
	ref array<ref SteamPlayerSummary> players;
};

class SteamSummariesEnvelope
{
	ref SteamSummariesResponse response;
};
#endif
