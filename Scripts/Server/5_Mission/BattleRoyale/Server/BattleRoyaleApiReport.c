#ifdef SERVER
/**
 *  What the match machine reports to the stats API (BattleRoyaleApi): the heartbeat, the start-time
 *  roster and the final rows. Five calls in all, against the nine the old API needed; positions,
 *  kills and wins all ride the one finish call, built from the same rows as last_match.json.
 */
class BattleRoyaleApiReport
{
	protected static string s_LastState;
	protected static int s_LastHeartbeatMs;

	//! The heartbeat's view of a state. Not an id the site keys on: only these four values exist.
	static string StateName(BattleRoyaleState state)
	{
		if (!state)
			return "ended";
		if (state.IsInherited(BattleRoyaleDebug))
			return "lobby";
		if (state.IsInherited(BattleRoyaleCountReached) || state.IsInherited(BattleRoyaleSpawnSelection))
			return "countdown";
		if (state.IsInherited(BattleRoyaleWin) || state.IsInherited(BattleRoyaleRestart))
			return "ended";

		return "in_match";
	}

	//! Called at 10 Hz: sends on every state change, and every BR_API_HEARTBEAT_MS otherwise.
	static void Tick(BattleRoyaleServer server)
	{
		if (!BattleRoyaleApi.IsEnabled() || !server)
			return;

		string state = StateName(server.GetCurrentState());
		int now = GetGame().GetTime();
		if (state == s_LastState && s_LastHeartbeatMs > 0 && now - s_LastHeartbeatMs < BR_API_HEARTBEAT_MS)
			return;

		s_LastState = state;
		s_LastHeartbeatMs = now;

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		BattleRoyaleApiHeartbeatBody body = new BattleRoyaleApiHeartbeatBody();
		body.token = BattleRoyaleApi.GetKey();
		body.state = state;
		body.players = players.Count();
		body.max_players = GetGame().ServerConfigGetInt("maxPlayers");
		body.world = GetGame().GetWorldName();
		body.uptime_s = GetGame().GetTickTime();
		body.match_id = server.match_uuid;

		string json;
		string error;
		if (JsonFileLoader<BattleRoyaleApiHeartbeatBody>.MakeData(body, json, error, false))
			BattleRoyaleApi.Post("heartbeat", json);
	}

	//! The roster at the start of the match, one entry per player with its group index.
	static void Start(string match_id, array<ref array<PlayerBase>> groups)
	{
		if (!BattleRoyaleApi.IsEnabled() || match_id == "" || !groups)
			return;

		BattleRoyaleApiStartBody body = new BattleRoyaleApiStartBody();
		body.token = BattleRoyaleApi.GetKey();

		for (int g = 0; g < groups.Count(); g++)
		{
			array<PlayerBase> group = groups.Get(g);
			if (!group)
				continue;

			for (int p = 0; p < group.Count(); p++)
			{
				PlayerBase player = group.Get(p);
				if (!player || !player.GetIdentity())
					continue;

				string uid = player.GetIdentity().GetPlainId();
				BattleRoyaleApiMember member = new BattleRoyaleApiMember();
				member.uid = uid;
				member.name = BattleRoyaleNameService.ResolveUid(uid, player.GetIdentity().GetPlainName());
				member.group_index = g;
				body.players.Insert(member);
			}
		}

		string json;
		string error;
		if (JsonFileLoader<BattleRoyaleApiStartBody>.MakeData(body, json, error, false))
			BattleRoyaleApi.Post("matches/" + match_id + "/start", json);

		BattleRoyaleUtils.Info("[API] start sent, " + body.players.Count() + " players in " + groups.Count() + " groups");
	}

	//! Every row of the match. Called once, after BattleRoyaleMatchStats.EndMatch().
	static void Finish(string match_id)
	{
		if (!BattleRoyaleApi.IsEnabled() || match_id == "")
			return;

		BattleRoyaleMatchStats stats = BattleRoyaleMatchStats.GetInstance();
		if (!stats.HasMatch())
			return;

		BattleRoyaleApiFinishBody body = new BattleRoyaleApiFinishBody();
		body.token = BattleRoyaleApi.GetKey();
		array<ref BattleRoyaleLastMatchRow> rows = stats.GetRows();
		for (int i = 0; i < rows.Count(); i++)
		{
			if (rows.Get(i))
				body.rows.Insert(rows.Get(i));
		}

		string json;
		string error;
		if (JsonFileLoader<BattleRoyaleApiFinishBody>.MakeData(body, json, error, false))
			BattleRoyaleApi.Post("matches/" + match_id + "/finish", json);

		BattleRoyaleUtils.Info("[API] finish sent, " + body.rows.Count() + " rows");
	}
};
#endif
