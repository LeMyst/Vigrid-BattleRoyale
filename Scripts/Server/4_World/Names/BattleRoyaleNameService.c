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

	//--- Two maps, and the split is the whole point of this class.
	//---
	//---   s_Overrides is what is IN FORCE: the uids whose name the mod is currently replacing. Every
	//---   Resolve*() reads this and nothing else, so a uid that is not in it is shown under the name
	//---   they connected with, full stop.
	//---
	//---   s_Cache is what we KNOW: every persona ever resolved on this server, with the hour it was
	//---   resolved, loaded from and saved to steam_names.json.
	//---
	//--- They were one map until it was noticed that a player who once connected as "Survivor" and has
	//--- since set a name of their own in the launcher was still being shown the resolved one - the
	//--- warm-cache branch of RequestForPlayer ran before the placeholder test, so "we have an answer
	//--- for this uid" was silently standing in for "this uid still needs one". A cache entry is
	//--- promoted into s_Overrides only by a connect that is actually wearing a placeholder name.
	static ref map<string, string> s_Overrides;
	static ref map<string, ref BattleRoyaleNameCacheEntry> s_Cache;

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
		if (!s_Cache)
			s_Cache = new map<string, ref BattleRoyaleNameCacheEntry>();
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
	 *  Is this cache entry old enough to be worth asking Steam about again?
	 */
	static bool IsCacheStale(BattleRoyaleNameCacheEntry entry)
	{
		if (!entry)
			return true;

		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if (!server_data)
			return false;
		if (server_data.steam_name_cache_max_age_hours <= 0)
			return false;

		//--- No timestamp at all: a v1 cache file, written before entries were dated. Its age is not
		//--- knowable, so refresh it once - after which it carries a stamp like everything else.
		if (entry.resolved_at_hours <= 0)
			return true;

		int age = BattleRoyaleTime.NowHours() - entry.resolved_at_hours;

		return (age > server_data.steam_name_cache_max_age_hours);
	}

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

		//--- First question, and it has to be first: are they still wearing a placeholder name? A
		//--- player who has since set one of their own in the launcher gets to keep it, and whatever
		//--- we resolved for them before stops applying this instant. The cache entry itself is left
		//--- alone - it is still a true steamid -> persona record, and it costs nothing sitting there.
		if (!IsPlaceholder(player.GetIdentity().GetPlainName()))
		{
			ClearOverride(uid);
			return;
		}

		//--- Already answered, from this process or from the cache file. Once the cache is warm this
		//--- is the common path, so it has to do everything Apply() does short of the lookup itself:
		//--- the players already in the lobby need telling about this one just as much.
		BattleRoyaleNameCacheEntry cached = NULL;
		if (s_Cache.Contains(uid))
			cached = s_Cache.Get(uid);

		if (cached)
		{
			string cached_name = cached.name;

			s_Overrides.Set(uid, cached_name);
			WriteThrough(uid, cached_name);
			BroadcastResolvedName(uid, cached_name);
			s_PartyRefreshPending = true;

			//--- Logged because this path is otherwise completely silent, and once the cache is warm
			//--- it is the path every connect takes - leaving it unlogged means the common case is the
			//--- one with no evidence in the log at all.
			BattleRoyaleUtils.Info("BattleRoyaleNameService: applied cached name \"" + cached_name + "\".");

			if (!IsCacheStale(cached))
				return;

			//--- Old enough to be worth re-asking. Deliberately applied first and refreshed second: the
			//--- player is not left as "Survivor" for the length of the batch window, and Apply() will
			//--- overwrite and re-broadcast if the persona has actually changed. Falls through into the
			//--- queueing below, whose s_Requested guard is exactly right here too - it lets the
			//--- refresh out once and stops a permanently unanswerable one (a private profile) going
			//--- again on every reconnect for the life of the process.
			BattleRoyaleUtils.Info("BattleRoyaleNameService: that name is stale, refreshing it from Steam.");
		}

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

		//--- Apply() only ever runs off a lookup RequestForPlayer already gated on the placeholder
		//--- test, so this uid is by construction one whose name we are meant to be replacing.
		s_Overrides.Set(uid, clean);
		RememberInCache(uid, clean);
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
	 *  Record a resolved name against the hour it was resolved, so IsCacheStale() has something to
	 *  measure. Updating an existing entry in place rather than replacing it keeps the map's element
	 *  identity stable for anything holding the reference - RequestForPlayer does, briefly.
	 */
	static void RememberInCache(string uid, string name)
	{
		Init();

		BattleRoyaleNameCacheEntry entry = NULL;
		if (s_Cache.Contains(uid))
			entry = s_Cache.Get(uid);

		if (!entry)
		{
			entry = new BattleRoyaleNameCacheEntry();
			entry.uid = uid;
			s_Cache.Set(uid, entry);
		}

		entry.name = name;
		entry.resolved_at_hours = BattleRoyaleTime.NowHours();
	}

	/**
	 *  Stop replacing this player's name: they have one of their own now.
	 *
	 *  Only the override goes. The cache entry stays - the persona we resolved is still theirs, and if
	 *  they ever go back to connecting as "Survivor" it applies again for free.
	 */
	static void ClearOverride(string uid)
	{
		if (!s_Overrides)
			return;
		if (!s_Overrides.Contains(uid))
			return;

		string dropped = s_Overrides.Get(uid);

		s_Overrides.Remove(uid);
		BroadcastClearedName(uid);

		//--- Party bakes names into its roster message - same reason as Apply(), in reverse.
		s_PartyRefreshPending = true;

		BattleRoyaleUtils.Info("BattleRoyaleNameService: dropped the resolved name \"" + dropped + "\", that player set their own.");
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
	 *  Tell every client to forget whatever they hold for this uid.
	 *
	 *  Reuses SetResolvedName rather than adding a message: an EMPTY third parameter is the wire
	 *  contract for "drop any override for this uid" - see BattleRoyaleRPC.SetResolvedName, which
	 *  reads it the same way. The engine-name field is left empty because it only ever feeds the
	 *  client's secondary key, and the client finds the stale entries there from the value it is
	 *  already holding.
	 *
	 *  A client that connects AFTER this needs nothing: SendAllResolvedNames walks the online players
	 *  against s_Overrides, which no longer has the uid. This exists for the ones already here.
	 */
	static void BroadcastClearedName(string uid)
	{
		GetRPCManager().SendRPC(RPC_DAYZBR_NAMESPACE, "SetResolvedName", new Param3<string, string, string>(uid, "", ""), true);
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
		//--- off, s_Cache stays empty, nothing is ever promoted into s_Overrides, and every Resolve*()
		//--- falls straight through to the vanilla name. That is what makes "disabled" mean genuinely
		//--- unchanged behaviour rather than "still serving names it resolved last week".
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

		if (cache.entries)
		{
			for (int i = 0; i < cache.entries.Count(); i++)
			{
				BattleRoyaleNameCacheEntry entry = cache.entries.Get(i);
				if (!entry)
					continue;
				if (entry.uid == "")
					continue;
				if (entry.name == "")
					continue;

				s_Cache.Set(entry.uid, entry);
			}
		}

		//--- v1 held a bare uid -> name map with no dates on it. Import those, undated: IsCacheStale
		//--- treats an entry with no timestamp as stale, so each one is re-asked once and then carries
		//--- a stamp like everything written since. The old key is not written back out.
		if (s_Cache.Count() == 0 && cache.names)
		{
			for (int j = 0; j < cache.names.Count(); j++)
			{
				BattleRoyaleNameCacheEntry migrated = new BattleRoyaleNameCacheEntry();
				migrated.uid = cache.names.GetKey(j);
				migrated.name = cache.names.GetElement(j);
				migrated.resolved_at_hours = 0;

				s_Cache.Set(migrated.uid, migrated);
			}

			if (s_Cache.Count() > 0)
				BattleRoyaleUtils.Info("BattleRoyaleNameService: migrated " + s_Cache.Count() + " undated name(s) from a v1 cache file.");
		}

		BattleRoyaleUtils.Info("BattleRoyaleNameService: loaded " + s_Cache.Count() + " cached name(s).");
	}

	static void SaveCache()
	{
		BattleRoyaleServerData server_data = BattleRoyaleConfig.GetConfig().GetServerData();
		if (!server_data)
			return;
		if (!server_data.cache_steam_names)
			return;
		if (!s_Cache)
			return;

		BattleRoyaleNameCache cache = new BattleRoyaleNameCache();
		cache.saved_at = BattleRoyaleTime.NowSeconds();

		for (int i = 0; i < s_Cache.Count(); i++)
		{
			cache.entries.Insert(s_Cache.GetElement(i));
		}

		string error_message;
		if (!JsonFileLoader<BattleRoyaleNameCache>.SaveFile(GetCachePath(), cache, error_message))
			BattleRoyaleUtils.Warn("BattleRoyaleNameService: cannot write the name cache: " + error_message);
	}
};

/**
 *  One cached persona. Dated, so a name that has had years to go stale can be re-asked rather than
 *  served forever - see BattleRoyaleNameService.IsCacheStale.
 */
class BattleRoyaleNameCacheEntry
{
	string uid;                //!< SteamID64, from PlayerIdentity.GetPlainId(). Never GetPlayerId().
	string name;               //!< the resolved persona, already sanitized and deduplicated
	int    resolved_at_hours;  //!< BattleRoyaleTime.NowHours() when Steam answered; 0 = unknown (v1)
};

/**
 *  On-disk shape of $profile:Vigrid-BattleRoyale\steam_names.json.
 *
 *  An array of records rather than the uid -> name map v1 used, because each entry now carries its
 *  own date. Note every array member needs `ref`: a missing `ref` on a JSON-deserialised array is a
 *  live bug class in this repo.
 */
class BattleRoyaleNameCache
{
	int version = 2;
	int saved_at;  //!< BattleRoyaleTime.NowSeconds(), so a human reading the file can date it
	ref array<ref BattleRoyaleNameCacheEntry> entries;

	//--- The v1 shape, still declared purely so an existing file deserialises and can be migrated.
	//--- SaveCache never fills it, but note the serialiser writes a null ref map out as "names": {}
	//--- rather than omitting the key - verified in a written file. Harmless: LoadCache only consults
	//--- it when entries came back empty, and an empty map migrates nothing.
	ref map<string, string> names;

	void BattleRoyaleNameCache()
	{
		entries = new array<ref BattleRoyaleNameCacheEntry>();
	}
};
#endif
