#ifdef SERVER
//--- Registers the match asynchronously. The answer lands in the statics below and is polled by
//--- BattleRoyaleServer (5_Mission), which this 3_Game class cannot call.
class BattleRoyaleApiMatchRegistration
{
	//--- The request whose answer counts. Bumping it makes every older reply stale.
	static int s_RequestId;
	//--- Equals s_RequestId once that request has a final answer: the match id, or "" on failure.
	static int s_AnsweredId;
	static string s_Response;

	//! Starts a new registration and returns its id, for IsAnswered().
	static int Request(string server_password)
	{
		s_RequestId++;
		int request_id = s_RequestId;

		BattleRoyaleApiCreateMatchBody body = new BattleRoyaleApiCreateMatchBody();
		body.token = BattleRoyaleApi.GetKey();
		body.password = server_password;

		array<ref ModInfo> mod_list = new array<ref ModInfo>();
		GetGame().GetModInfos(mod_list);
		foreach (ModInfo mod_info : mod_list)
		{
			//--- Vanilla and DLC entries say nothing about whether a client can join.
			if (mod_info.GetDefault() || mod_info.GetIsDLC())
				continue;

			body.mods.Insert(mod_info.GetName(), mod_info.GetVersion());
		}

		string json;
		string error;
		if (!JsonFileLoader<BattleRoyaleApiCreateMatchBody>.MakeData(body, json, error, false))
		{
			Answer(request_id, "");
			return request_id;
		}

		BattleRoyaleApi.Post("matches", json, new BattleRoyaleApiMatchRegistrationHandler(request_id));
		return request_id;
	}

	static void Answer(int request_id, string match_id)
	{
		if (request_id != s_RequestId)
			return;

		s_Response = match_id;
		s_AnsweredId = request_id;
	}

	static bool IsAnswered(int request_id)
	{
		return request_id == s_AnsweredId;
	}

	static string GetResponse()
	{
		return s_Response;
	}

	//--- Drops whatever is in flight: its reply becomes stale.
	static void Cancel()
	{
		s_RequestId++;
	}
};

class BattleRoyaleApiMatchRegistrationHandler: BattleRoyaleApiHandler
{
	protected int m_RequestId;

	void BattleRoyaleApiMatchRegistrationHandler(int request_id)
	{
		m_RequestId = request_id;
	}

	override void OnReply(string data)
	{
		BattleRoyaleApiMatchReply reply = new BattleRoyaleApiMatchReply();
		string error;
		if (!JsonFileLoader<BattleRoyaleApiMatchReply>.LoadData(data, reply, error))
		{
			BattleRoyaleUtils.Warn("[API] matches: unreadable reply");
			BattleRoyaleApiMatchRegistration.Answer(m_RequestId, "");
			return;
		}

		BattleRoyaleApiMatchRegistration.Answer(m_RequestId, reply.match_id);
	}

	override void OnFailed()
	{
		BattleRoyaleApiMatchRegistration.Answer(m_RequestId, "");
	}
};
#endif
