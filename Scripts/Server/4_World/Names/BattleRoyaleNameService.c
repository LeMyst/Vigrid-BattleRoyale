#ifdef SERVER
/**
 *  Display names for players who never set one in the launcher.
 *
 *  PlayerIdentity cannot be renamed - every accessor on it is a getter and there is no SetName
 *  (P:\scripts\3_game\gameplay.c). So the corrected name lives here, keyed on SteamID64, and every
 *  display site in the mod reads through Resolve*(). Vanilla surfaces that go straight to the
 *  identity - in-game chat, the vanilla player list - are out of reach and still show "Survivor".
 *
 *  The one exception is vanilla's own PlayerBase.m_CachedPlayerName, which is protected and
 *  server-only: Apply() writes through to it via PlayerBase.BR_SetCachedName, which is how Party
 *  and KillFeed pick the corrected name up through the public GetCachedName() without either
 *  addon ever naming a BattleRoyale symbol.
 */
class BattleRoyaleNameService
{
	//--- How long a connect waits before its lookup goes out, so a lobby filling up in a burst
	//--- costs one request instead of one per player. Well under the shortest lobby.
	static const float BR_NAME_BATCH_DELAY_SECONDS = 3.0;

	//--- Steam persona names cap at 32; anything longer is not a name, it is an attack on a widget.
	static const int BR_NAME_MAX_LENGTH = 32;

	static ref map<string, string> s_Overrides;

	//--- There was an s_ClientReported map here, holding names clients read from their own Steam
	//--- session as a fallback for players the Web API could not answer for. Removed after
	//--- measurement: on PC, BiosUser.GetName() returns the *profile* name, not the Steam persona -
	//--- two local clients on one Steam account reported their two different -name= values rather
	//--- than the one persona they share. There is no client-side persona name to fall back to.

	//--- Queued for the next flush.
	static ref array<string> s_Pending;

	//--- Every uid ever asked about this process, resolved or not. Stops a player whose profile is
	//--- private from re-queuing a doomed lookup on every reconnect.
	static ref array<string> s_Requested;

	static float s_FlushAt = 0;
	static bool s_CacheLoaded = false;

	//--- Set when a name resolves, consumed once from BattleRoyaleServer.Update. See Apply().
	static bool s_PartyRefreshPending = false;

	static void Init()
	{
		if (!s_Overrides)
			s_Overrides = new map<string, string>();
		if (!s_Pending)
			s_Pending = new array<string>();
		if (!s_Requested)
			s_Requested = new array<string>();

		LoadCache();
	}

	static bool IsEnabled()
	{
		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if (!server_data)
			return false;

		return server_data.enable_steam_name_lookup;
	}

	// ---------------------------------------------------------------- resolving

	/**
	 *  The display name for a live identity: the resolved override when we have one, otherwise the
	 *  name the player connected with.
	 */
	static string ResolveIdentity(PlayerIdentity identity)
	{
		if (!identity)
			return "";

		return ResolveUid(identity.GetPlainId(), identity.GetName());
	}

	/**
	 *  Same, for the paths that only hold a uid - or hold one whose identity is already gone, which
	 *  is most of the leaderboard.
	 */
	static string ResolveUid(string uid, string fallback)
	{
		if (uid == "")
			return fallback;
		if (!s_Overrides)
			return fallback;

		if (s_Overrides.Contains(uid))
			return s_Overrides.Get(uid);

		return fallback;
	}

	/**
	 *  Is this the name of somebody who never picked one?
	 *
	 *  Feed this GetPlainName(), not GetName(): "nick without any processing" is already free of the
	 *  engine's " (2)" duplicate suffix, so there is no suffix to strip here.
	 */
	static bool IsPlaceholder(string name)
	{
		string trimmed = name;
		trimmed.TrimInPlace();

		if (trimmed == "")
			return true;

		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if (!server_data)
			return false;
		if (!server_data.placeholder_player_names)
			return false;

		string lowered = trimmed;
		lowered.ToLower();

		for (int i = 0; i < server_data.placeholder_player_names.Count(); i++)
		{
			string candidate = server_data.placeholder_player_names.Get(i);
			candidate.TrimInPlace();
			candidate.ToLower();

			if (candidate == "")
				continue;
			if (candidate == lowered)
				return true;
		}

		return false;
	}

	// ---------------------------------------------------------------- requesting

	/**
	 *  Queue a lookup for this player if - and only if - they connected without a name of their own.
	 */
	static void RequestForPlayer(PlayerBase player)
	{
		if (!IsEnabled())
			return;
		if (!player)
			return;
		if (!player.GetIdentity())
			return;

		Init();

		string uid = player.GetIdentity().GetPlainId();
		if (uid == "")
			return;

		//--- Already answered, from this process or from the cache file. Once the cache is warm this
		//--- is the common path, so it has to do everything Apply() does short of the lookup itself:
		//--- the players already in the lobby need telling about this one just as much.
		if (s_Overrides.Contains(uid))
		{
			WriteThrough(uid, s_Overrides.Get(uid));
			BroadcastResolvedName(uid, s_Overrides.Get(uid));
			s_PartyRefreshPending = true;

			//--- Logged because this path is otherwise completely silent, and once the cache is warm
			//--- it is the path every connect takes - leaving it unlogged means the common case is the
			//--- one with no evidence in the log at all.
			BattleRoyaleUtils.Info("BattleRoyaleNameService: applied cached name \"" + s_Overrides.Get(uid) + "\".");
			return;
		}

		if (!IsPlaceholder(player.GetIdentity().GetPlainName()))
			return;

		if (s_Requested.Find(uid) != -1)
			return;
		if (s_Pending.Find(uid) != -1)
			return;

		s_Pending.Insert(uid);
		s_Requested.Insert(uid);

		if (s_FlushAt == 0)
			s_FlushAt = GetGame().GetTickTime() + BR_NAME_BATCH_DELAY_SECONDS;

		BattleRoyaleUtils.Debug("BattleRoyaleNameService: queued a name lookup, " + s_Pending.Count() + " pending.");
	}

	/**
	 *  Driven from BattleRoyaleServer.Update - the same "Update() is the only driver" rule the state
	 *  machine follows, and cheaper than owning a Timer for a class with no instance.
	 */
	static void Tick()
	{
		if (s_FlushAt == 0)
			return;
		if (GetGame().GetTickTime() < s_FlushAt)
			return;

		s_FlushAt = 0;

		if (!s_Pending)
			return;
		if (s_Pending.Count() == 0)
			return;

		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if (!server_data)
			return;

		SteamNameWebhook webhook = new SteamNameWebhook(server_data.steam_web_api_key);
		webhook.Send(s_Pending);

		//--- The webhook copied what it needs; anything still queued after a partial send went out
		//--- as its own overflow request.
		s_Pending.Clear();
	}

	// ---------------------------------------------------------------- applying

	/**
	 *  Record a resolved name and push it everywhere it needs to be. Called from the REST callback.
	 */
	static void Apply(string uid, string persona)
	{
		if (uid == "")
			return;

		Init();

		string clean = Sanitize(persona);
		if (clean == "")
		{
			BattleRoyaleUtils.Debug("BattleRoyaleNameService: Steam returned an unusable name for " + uid + ", keeping the original.");
			return;
		}

		clean = Deduplicate(uid, clean);

		s_Overrides.Set(uid, clean);
		WriteThrough(uid, clean);
		SaveCache();

		//--- Everything below is about surfaces that cached the *old* name and have no reason of their
		//--- own to look again. Resolution happens seconds after the connect, so every one of them has
		//--- already rendered "Survivor" by the time we get here.
		BroadcastResolvedName(uid, clean);

		//--- Party bakes names into its roster message and only re-broadcasts on a *composition*
		//--- change - join, leave, kick, new leader. A name changing is none of those, so without a
		//--- nudge the HUD row and the name plate keep the name the player had when they joined.
		//---
		//--- Flagged here and consumed in BattleRoyaleServer.Update rather than called directly: this
		//--- file is 4_World and VigridPartyAPI is 4_World in a *different* PBO, where cross-addon
		//--- resolution within one stage is not something to rely on. Every proven #ifdef VIGRID_PARTY
		//--- call site in this repo is 5_Mission, so the call belongs there.
		s_PartyRefreshPending = true;

		BattleRoyaleUtils.Info("BattleRoyaleNameService: resolved a placeholder name to \"" + clean + "\".");
	}

	/**
	 *  True at most once per resolved name. Read from 5_Mission, which is where the Party call has to
	 *  live - see the note in Apply().
	 */
	static bool ConsumePartyRefresh()
	{
		if (!s_PartyRefreshPending)
			return false;

		s_PartyRefreshPending = false;
		return true;
	}

	/**
	 *  Push one resolved name to every client.
	 *
	 *  Carries the uid *and* the engine name it replaces, because the client keys on whichever of the
	 *  two it can actually match - see BattleRoyaleRPC.ResolveDisplayName.
	 */
	static void BroadcastResolvedName(string uid, string name)
	{
		string engine_name = "";

		array<Man> everyone = new array<Man>();
		GetGame().GetPlayers(everyone);

		for (int i = 0; i < everyone.Count(); i++)
		{
			PlayerBase owner = PlayerBase.Cast(everyone.Get(i));
			if (!owner)
				continue;
			if (!owner.GetIdentity())
				continue;
			if (owner.GetIdentity().GetPlainId() != uid)
				continue;

			engine_name = owner.GetIdentity().GetName();
			break;
		}

		GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetResolvedName", new Param3<string, string, string>(uid, engine_name, name), true);
	}

	/**
	 *  Hand a joining client every name resolved so far, so somebody who connects after the fact does
	 *  not see stale names on everyone already in the lobby.
	 */
	static void SendAllResolvedNames(PlayerIdentity target)
	{
		if (!target)
			return;
		if (!s_Overrides)
			return;

		//--- Driven off the online players rather than off s_Overrides: the override map is the
		//--- persistent cache and accumulates every name ever resolved on this server, so walking it
		//--- would grow one connect's cost with the size of the player base and push names for people
		//--- who are not here. This is bounded by the lobby and costs one GetPlayers() call.
		array<Man> everyone = new array<Man>();
		GetGame().GetPlayers(everyone);

		for (int i = 0; i < everyone.Count(); i++)
		{
			PlayerBase owner = PlayerBase.Cast(everyone.Get(i));
			if (!owner)
				continue;
			if (!owner.GetIdentity())
				continue;

			string uid = owner.GetIdentity().GetPlainId();
			if (!s_Overrides.Contains(uid))
				continue;

			GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetResolvedName", new Param3<string, string, string>(uid, owner.GetIdentity().GetName(), s_Overrides.Get(uid)), true, target);
		}
	}

	/**
	 *  Trim, cap the length, and drop anything that would fight a widget or a log line. Steam persona
	 *  names are user-controlled text and land in the kill feed, the party HUD and the ADM log.
	 */
	static string Sanitize(string persona)
	{
		string result = persona;
		result.TrimInPlace();

		if (result == "")
			return "";

		//--- Replace rather than a per-character walk, and cut on the Utf8 variants below: Length()
		//--- and Substring() are byte-based, so a naive walk would slice a multi-byte character in
		//--- half - and plenty of Steam names are not ASCII. These five needles are all ASCII, and an
		//--- ASCII byte never occurs inside a multi-byte sequence, so replacing them is safe.
		result.Replace("<", "");
		result.Replace(">", "");
		result.Replace("\n", "");
		result.Replace("\r", "");
		result.Replace("\t", "");

		result.TrimInPlace();

		if (result.LengthUtf8() > BR_NAME_MAX_LENGTH)
			result = result.SubstringUtf8(0, BR_NAME_MAX_LENGTH);

		return result;
	}

	/**
	 *  Two players can share a Steam name, and the engine's own duplicate suffix does not apply to
	 *  strings we mint ourselves - so re-do that here, or the fix reintroduces the bug it exists to
	 *  solve with a nicer word than "Survivor".
	 */
	static string Deduplicate(string uid, string candidate)
	{
		if (!s_Overrides)
			return candidate;

		string result = candidate;
		int suffix = 2;

		while (IsNameTaken(uid, result))
		{
			result = candidate + " (" + suffix.ToString() + ")";
			suffix = suffix + 1;

			if (suffix > 64)
				break;
		}

		return result;
	}

	static bool IsNameTaken(string uid, string candidate)
	{
		//--- Against other resolved names...
		for (int i = 0; i < s_Overrides.Count(); i++)
		{
			if (s_Overrides.GetKey(i) == uid)
				continue;
			if (s_Overrides.GetElement(i) == candidate)
				return true;
		}

		//--- ...and against the names players are already wearing, so a resolved name never collides
		//--- with somebody who set theirs properly in the launcher.
		array<Man> everyone = new array<Man>();
		GetGame().GetPlayers(everyone);

		for (int j = 0; j < everyone.Count(); j++)
		{
			PlayerBase other = PlayerBase.Cast(everyone.Get(j));
			if (!other)
				continue;
			if (!other.GetIdentity())
				continue;
			if (other.GetIdentity().GetPlainId() == uid)
				continue;

			if (other.GetIdentity().GetName() == candidate)
				return true;
		}

		return false;
	}

	/**
	 *  Push a resolved name onto the live player: the mod's own cache, and vanilla's.
	 */
	static void WriteThrough(string uid, string name)
	{
		array<Man> everyone = new array<Man>();
		GetGame().GetPlayers(everyone);

		for (int i = 0; i < everyone.Count(); i++)
		{
			PlayerBase player = PlayerBase.Cast(everyone.Get(i));
			if (!player)
				continue;
			if (!player.GetIdentity())
				continue;
			if (player.GetIdentity().GetPlainId() != uid)
				continue;

			player.player_name = name;
			player.BR_SetCachedName(name);

#ifdef JM_COT
			//--- COT cached the name in its own JMPlayerInstance at connect, before this resolved.
			//--- Correcting that one field fixes every COT surface at once - see BattleRoyaleCOTName.
			JMPlayerInstance cot_player = GetPermissionsManager().GetPlayer(GetPermissionsManager().GetGUIDForSteam(uid));
			if (cot_player)
				cot_player.BR_SetName(name);
#endif

			return;
		}
	}

	// ---------------------------------------------------------------- persistence

	static string GetCachePath()
	{
		return BATTLEROYALE_SETTINGS_FOLDER + "steam_names.json";
	}

	/**
	 *  The server process restarts between matches, so without this every match re-queries every
	 *  returning player.
	 */
	static void LoadCache()
	{
		if (s_CacheLoaded)
			return;

		s_CacheLoaded = true;

		//--- Deliberately gated on the feature too, not just on the cache switch: with the lookup
		//--- off, s_Overrides stays empty and every Resolve*() falls straight through to the vanilla
		//--- name. That is what makes "disabled" mean genuinely unchanged behaviour rather than
		//--- "still serving names it resolved last week".
		if (!IsEnabled())
			return;

		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if (!server_data)
			return;
		if (!server_data.cache_steam_names)
			return;
		if (!FileExist(GetCachePath()))
			return;

		BattleRoyaleNameCache cache = new BattleRoyaleNameCache();

		string error_message;
		if (!JsonFileLoader<BattleRoyaleNameCache>.LoadFile(GetCachePath(), cache, error_message))
		{
			BattleRoyaleUtils.Warn("BattleRoyaleNameService: cannot read the name cache: " + error_message);
			return;
		}

		if (!cache.names)
			return;

		for (int i = 0; i < cache.names.Count(); i++)
		{
			s_Overrides.Set(cache.names.GetKey(i), cache.names.GetElement(i));
		}

		BattleRoyaleUtils.Info("BattleRoyaleNameService: loaded " + s_Overrides.Count() + " cached name(s).");
	}

	static void SaveCache()
	{
		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if (!server_data)
			return;
		if (!server_data.cache_steam_names)
			return;
		if (!s_Overrides)
			return;

		BattleRoyaleNameCache cache = new BattleRoyaleNameCache();
		cache.names = s_Overrides;

		string error_message;
		if (!JsonFileLoader<BattleRoyaleNameCache>.SaveFile(GetCachePath(), cache, error_message))
			BattleRoyaleUtils.Warn("BattleRoyaleNameService: cannot write the name cache: " + error_message);
	}
};

class BattleRoyaleNameCache
{
	int version = 1;
	ref map<string, string> names = new map<string, string>();
};
#endif
