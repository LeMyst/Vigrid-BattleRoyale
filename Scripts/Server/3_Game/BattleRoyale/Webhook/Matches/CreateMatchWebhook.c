#ifdef SERVER
//--- Registers the match asynchronously. POST_now blocked the main thread for up to the REST timeout,
//--- at boot and on every in-place reset. The answer lands in the statics below and is polled by
//--- BattleRoyaleServer (5_Mission), which this 3_Game class cannot call.
class CreateMatchWebhook
{
	//--- The request whose answer counts. Bumping it makes every older reply and retry stale.
	static int s_RequestId;
	//--- Equals s_RequestId once that request has a final answer: the body on success, "" when every try failed.
	static int s_AnsweredId;
	static string s_Response;

	protected string s_ServerToken;
	protected string s_ServerPassword;
	protected int i_TryLeft = 3;
	protected int i_RequestId;

	void CreateMatchWebhook(string server_token, string server_password)
	{
		BattleRoyaleUtils.Trace("CreateMatchWebhook()");

		s_ServerToken = server_token;
		s_ServerPassword = server_password;
	};

	//--- Starts a new registration and returns its id, for IsAnswered().
	int Request()
	{
		s_RequestId++;
		i_RequestId = s_RequestId;
		Send();
		return i_RequestId;
	};

	void SetRetry(int request_id, int try_left)
	{
		i_RequestId = request_id;
		i_TryLeft = try_left;
	};

	void Send()
	{
		if (i_RequestId != s_RequestId)
			return;

		if (i_TryLeft <= 0)
		{
			Answer(i_RequestId, "");
			return;
		}

		HttpArguments arguments = {
			new HttpArgument("version", "1"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		map<string, string> mods = new map<string, string>; // TODO: Dummy mods
		array<ref ModInfo> modList = new array<ref ModInfo>();
		GetGame().GetModInfos(modList);

		for (int i = 0; i < modList.Count(); i++)
		{
			ModInfo modInfo = modList.Get(i);
			mods.Insert(modInfo.GetName(), modInfo.GetVersion());
		}

		JATMods jatMods = new JATMods( s_ServerToken, s_ServerPassword, mods );

		string jatString;
		string jatError;
		JsonFileLoader<JATMods>.MakeData(jatMods, jatString, jatError);

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
		ctx.POST(new CreateMatchCallback( s_ServerToken, s_ServerPassword, i_RequestId, i_TryLeft ), arguments.ToQuery("matches"), jatString);
	};

	static void Answer(int request_id, string response)
	{
		if (request_id != s_RequestId)
			return;

		s_Response = response;
		s_AnsweredId = request_id;
	};

	static bool IsAnswered(int request_id)
	{
		return request_id == s_AnsweredId;
	};

	static string GetResponse()
	{
		return s_Response;
	};

	//--- Drops whatever is in flight: its reply and its retries become stale.
	static void Cancel()
	{
		s_RequestId++;
	};
};

class CreateMatchCallback: RestCallback
{
	protected string s_ServerToken;
	protected string s_ServerPassword;
	protected int i_RequestId;
	protected int i_TryLeft;

	void CreateMatchCallback(string server_token, string server_password, int request_id, int try_left)
	{
		BattleRoyaleUtils.Trace("CreateMatchCallback() " + try_left);

		s_ServerToken = server_token;
		s_ServerPassword = server_password;
		i_RequestId = request_id;
		i_TryLeft = try_left;
	}

	protected void Retry()
	{
		CreateMatchWebhook webhook = new CreateMatchWebhook( s_ServerToken, s_ServerPassword );
		webhook.SetRetry( i_RequestId, i_TryLeft );
		webhook.Send();
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Warn("CreateMatchCallback: error " + errorCode + ", tries left " + i_TryLeft);
		Retry();
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Warn("CreateMatchCallback: timeout, tries left " + i_TryLeft);
		Retry();
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace("CreateMatchCallback: success size=" + dataSize);
		CreateMatchWebhook.Answer( i_RequestId, data );
	};
};
#endif
