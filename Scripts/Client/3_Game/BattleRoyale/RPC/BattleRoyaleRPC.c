#ifndef SERVER
class BattleRoyaleRPC
{
	void BattleRoyaleRPC()
	{
		BattleRoyaleUtils.Trace("BattleRoyaleClient::Init");

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetFade", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetInput", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "StartMatch", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetLobbyPhase", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetLobbyNames", this );

		lobby_net_low = new array<int>();
		lobby_net_high = new array<int>();
		lobby_names = new array<string>();
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownMs", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetTopPosition", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ShowWinScreen", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ChatLog", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetLeaderboard", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetVoiceSettings", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetSpeakingPlayers", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetSpectateOffer", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetSpectateTarget", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "EndSpectate", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetAdminFlag", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetAdminPlayerList", this );

		admin_uids = new array<string>();
		admin_names = new array<string>();
		admin_positions = new array<vector>();
		admin_prev_positions = new array<vector>();
		admin_healths = new array<float>();
		admin_kills = new array<int>();
		admin_slots = new array<int>();

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetResolvedName", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateHotZones", this );

		hot_zone_centers = new array<vector>();
		hot_zone_radii = new array<float>();

		resolved_by_uid = new map<string, string>();
		resolved_by_name = new map<string, string>();

		lb_solo = new BattleRoyaleLeaderboardBoard();
		lb_group = new BattleRoyaleLeaderboardBoard();

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetLastMatchTable", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetLastMatchRecap", this );

		last_match = new BattleRoyaleLastMatch();

		BattleRoyaleUtils.Trace("BattleRoyaleClient::Init - Done");
	}

	void ~BattleRoyaleRPC()
	{

	}

	/**
	 *  Resolved display names for players who connected as "Survivor".
	 *
	 *  The correction lives on the server - PlayerIdentity cannot be renamed and the client's copy
	 *  still carries the launcher name - so any client-side surface that reads the identity has to
	 *  come through here. Vanilla's own player tag ("looking at someone") is exactly that case.
	 *
	 *  Keyed twice on purpose. GetPlainId() is the identity that matters, but it is not certain to be
	 *  populated on a *client-side* identity for another player, and guessing wrong would fail
	 *  silently. GetName() is a guaranteed-unique per-session fallback, since it is precisely the
	 *  string the engine already deduplicates with " (2)". Lookup tries the uid, then the name.
	 */
	ref map<string, string> resolved_by_uid;
	ref map<string, string> resolved_by_name;

	private static ref BattleRoyaleRPC m_Instance;
	static BattleRoyaleRPC GetInstance()
	{
		if ( m_Instance == NULL )
		{
			m_Instance = new BattleRoyaleRPC();
		}

		return m_Instance;
	}

	void Reset()
	{
		nb_players = 0;
		nb_groups = 0;
		fade_state = false;
		input_state = false;
		player_kills = 0;
		match_started = false;
		lobby_phase = false;
		lobby_net_low.Clear();
		lobby_net_high.Clear();
		lobby_names.Clear();
		countdown_deadline_ms = BR_COUNTDOWN_NONE;
		current_play_area_center = "0 0 0";
		current_play_area_radius = 0.0;
		future_play_area_center = "0 0 0";
		future_play_area_radius = 0.0;
		top_position = 0;
		winner_screen = false;
		lb_solo.Clear();
		lb_group.Clear();
		lb_season = 1;
		leaderboard_seq = 0;
		resolved_by_uid.Clear();
		resolved_by_name.Clear();
		//--- Defaults match BattleRoyaleGameData, so a missed sync leaves shipped behaviour rather
		//--- than a silently dead feature. The server corrects both on connect.
		speaking_list_enabled = true;
		speaking_list_during_match = true;
		speaking_names.Clear();
		speaking_self_index = -1;
		speaking_seq = 0;
		dead_placement = "";
		dead_flavour = "";
		spectate_offered = false;
		spectate_active = false;
		spectate_seq = 0;
		spectate_target_uid = "";
		spectate_target_name = "";
		spectate_target_pos = "0 0 0";
		spectate_mode = 0;
		spectate_target_obj = NULL;
		//--- is_admin and death_locked are deliberately NOT reset here. is_admin is a property of WHO
		//--- IS CONNECTED, pushed once and never again, so clearing it would silently disarm the
		//--- admin keys mid-session. death_locked is a one-shot handshake that BattleRoyaleClient
		//--- consumes on the very next frame - clearing it in between would skip the matching focus
		//--- release and leave input locked for good.
		admin_uids.Clear();
		admin_names.Clear();
		admin_positions.Clear();
		admin_prev_positions.Clear();
		admin_healths.Clear();
		admin_kills.Clear();
		admin_slots.Clear();
		admin_recv_ms = 0;
		admin_prev_recv_ms = 0;
		admin_seq = 0;
		//--- Cleared, and the sequence bumped rather than zeroed: a renderer comparing against its
		//--- own last-seen value has to see this as a change, or it keeps drawing the old circles.
		hot_zone_centers.Clear();
		hot_zone_radii.Clear();
		hot_zone_seq++;
		//--- The last-match card and table ARE reset, and the note above about is_admin and
		//--- death_locked invites exactly the wrong inference here. Those two are respectively a
		//--- property of who is connected and a one-shot handshake. This is match data belonging to
		//--- one server process, and a world load means a different process.
		//--- self_index back to -1, NOT 0: zero would make this client believe it is row 0 - the
		//--- winner - against an empty table.
		last_match.Clear();
		last_match_seq = 0;
		recap_killer_name = "";
		recap_weapon_type = "";
		recap_cause = BattleRoyaleKillCause.UNKNOWN;
		recap_distance_m = -1;
		recap_killer_health_pct = -1;
		recap_damage_to_killer = 0;
		recap_self_group = -1;
		recap_hits = 0;
		recap_valid = false;
		recap_seq = 0;
	}

	//------------------------------------------------------------------------------------------
	//--- Last match: the persisted summary of the PREVIOUS match, read back in the lobby.
	//------------------------------------------------------------------------------------------

	ref BattleRoyaleLastMatch last_match;

	//! Its OWN sequence counter, never leaderboard_seq. Two producers - a ladder arriving must not
	//! repaint the last-match tab, and vice versa.
	int last_match_seq = 0;

	/**
	 *  The final standings of the previous match.
	 *
	 *  Parallel primitive arrays, never an array of structs, following every other table on this
	 *  class. No SteamID64s: self_index is what identifies the local player's row.
	 */
	void SetLastMatchTable(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param9< array<string>, array<int>, array<int>, array<int>, array<int>, array<int>, int, int, int > data;

		if ( !ctx.Read( data ) )
		{
			//--- Warn, never Error: the global Error() raises a VM exception and would take the
			//--- client's whole mission down over a malformed cosmetic payload.
			BattleRoyaleUtils.Warn("SetLastMatchTable: FAILED TO READ");
			return;
		}

		if ( type == CallType.Client )
		{
			//--- Copy, never adopt the Param's arrays - they are transient.
			last_match.names.Copy( data.param1 );
			last_match.places.Copy( data.param2 );
			last_match.kills.Copy( data.param3 );
			last_match.damage.Copy( data.param4 );
			last_match.survived.Copy( data.param5 );
			last_match.groups.Copy( data.param6 );
			last_match.self_index = data.param7;
			last_match.field_size = data.param8;
			last_match.flags = data.param9;
			last_match.valid = true;

			string line = "SetLastMatchTable: rows=" + last_match.Count();
			line = line + " self=" + last_match.self_index;
			line = line + " field=" + last_match.field_size;
			line = line + " flags=" + last_match.flags;
			BattleRoyaleUtils.Debug(line);
		}

		//--- Unconditionally at the end, so an edge-triggered renderer repaints even on a payload
		//--- that changed nothing.
		last_match_seq++;
	}

	//------------------------------------------------------------------------------------------
	//--- Death recap. ONE handler, TWO send points: pushed live at the moment of death so the death
	//--- screen can paint it, and sent again with the table when the lobby asks.
	//---
	//--- The two can never be confused within a session, and the reason is structural rather than
	//--- lucky: the server restarts between matches and this instance is per-session, so a session
	//--- spans exactly one process and therefore exactly one match. The one exception is an admin,
	//--- who is exempt from the late-join kick: they can read the previous match in the lobby, then
	//--- play and die, and the fields are overwritten under the old header. Documented rather than
	//--- engineered around - it affects admins only, and both fixes cost more than the bug.
	//------------------------------------------------------------------------------------------

	string recap_killer_name = "";
	string recap_weapon_type = "";   //!< classname; localised here on the client, per viewer
	int recap_cause = BattleRoyaleKillCause.UNKNOWN;
	int recap_distance_m = -1;
	int recap_killer_health_pct = -1;
	int recap_damage_to_killer = 0;
	int recap_self_group = -1;
	int recap_hits = 0;              //!< rides here because the table carries no hits column
	bool recap_valid = false;
	int recap_seq = 0;

	void SetLastMatchRecap(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param8< string, string, int, int, int, int, int, int > data;

		if ( !ctx.Read( data ) )
		{
			BattleRoyaleUtils.Warn("SetLastMatchRecap: FAILED TO READ");
			return;
		}

		if ( type == CallType.Client )
		{
			recap_killer_name = data.param1;
			recap_weapon_type = data.param2;
			recap_cause = data.param3;
			recap_distance_m = data.param4;
			recap_killer_health_pct = data.param5;
			recap_damage_to_killer = data.param6;
			recap_self_group = data.param7;
			recap_hits = data.param8;
			recap_valid = true;

			string line = "SetLastMatchRecap: cause=" + recap_cause;
			line = line + " killer=" + recap_killer_name;
			line = line + " weapon=" + recap_weapon_type;
			line = line + " dist=" + recap_distance_m;
			BattleRoyaleUtils.Debug(line);
		}

		recap_seq++;
	}

	// Set the number of players and groups

	int nb_players = 0;
	int nb_groups = 0;

	void SetPlayerCount(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<int, int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETPLAYERCOUNT RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetPlayerCount: %1 %2", data.param1, data.param2));
			nb_players = data.param1;
			nb_groups = data.param2;
		}
	}

	// Voice settings

	bool speaking_list_enabled = true;
	bool speaking_list_during_match = true;

	// Who this player can currently hear. Pushed by the server only when the set changes - the
	// client cannot work this out for itself, because IsPlayerSpeaking() returns the local mic
	// level for every entity client-side (see BattleRoyaleConstants.c).

	ref array<string> speaking_names = new array<string>();
	int speaking_self_index = -1;
	int speaking_seq = 0;

	// Spectating. Pure field latching, like every other handler here - BattleRoyaleClient.Update()
	// polls these and does the work.
	//
	// spectate_target_obj is CF's `Object target`, marshalled by network id. At the first push the
	// spectator's network bubble is still at their corpse, so the target entity may not exist on
	// this client yet and it can arrive NULL. That is expected and handled: spectate_target_pos is
	// authoritative until an entity can be latched, and the server re-sends both once a second.

	// The two lines the death screen shows. They are NOT sent by the server - they are written
	// locally by DayZPlayerImplement.ShowDeadScreen and read by DeathScreenMenu.
	//
	// They live here, on a 3_Game singleton, purely so the two can talk: ShowDeadScreen compiles in
	// 4_World and DeathScreenMenu in 5_Mission, so the earlier stage cannot name the later one. That
	// is the same stage-ordering rule that forces LeaveServer's spectate check to read this class
	// rather than BattleRoyaleClient.
	string dead_placement = "";
	string dead_flavour = "";

	//! Did the death path take an input-focus lock? Written by ShowDeadScreen, read by
	//! BattleRoyaleClient before it releases one. Travels here for the same stage-ordering reason as
	//! the two strings above - and it exists because an admin can enter spectate ALIVE, having never
	//! run SimulateDeath, in which case there is no lock to release and releasing anyway drives the
	//! additive focus counter negative. See BattleRoyaleClient.EnterSpectate.
	bool death_locked = false;

	//! The server has decided this player is dead and eligible - the death screen may offer Spectate.
	bool spectate_offered = false;
	bool spectate_active = false;
	int spectate_seq = 0;
	string spectate_target_uid = "";
	string spectate_target_name = "";
	vector spectate_target_pos = "0 0 0";
	int spectate_mode = 0;
	Object spectate_target_obj = NULL;

	//------------------------------------------------------------------------------------------
	//--- Admin spectate.
	//------------------------------------------------------------------------------------------

	//! Does the server consider this client an admin? Pushed once on connect. PRESENTATION ONLY -
	//! it gates which keys send a packet and whether the death screen offers its admin button, and
	//! is never trusted as authorization. Every admin RPC is re-checked against admins_steamid64
	//! server-side, so a client that sets this itself gains nothing but rejected packets.
	bool is_admin = false;

	//! The admin overlay roster, as parallel arrays. Sent ONLY to a connection the server has
	//! already established is an admin in a spectate session - never broadcast, because it carries
	//! SteamID64s, which SetLeaderboard deliberately keeps off the wire for exactly that reason.
	//!
	//! admin_prev_positions plus the two timestamps are what let the overlay interpolate between
	//! 2 Hz pushes, the same shape VigridPartyAPI.ResolveBodyPos uses for party members. Without it
	//! a tag on a player outside the network bubble visibly steps twice a second.
	ref array<string> admin_uids;
	ref array<string> admin_names;
	ref array<vector> admin_positions;
	ref array<vector> admin_prev_positions;
	ref array<float> admin_healths;
	ref array<int> admin_kills;
	ref array<int> admin_slots;
	int admin_recv_ms = 0;
	int admin_prev_recv_ms = 0;
	int admin_seq = 0;

	void SetAdminFlag(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			BattleRoyaleUtils.Warn("FAILED TO READ SETADMINFLAG RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("[Spectate] SetAdminFlag: " + data.param1);
			is_admin = data.param1;
		}
	}

	void SetAdminPlayerList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param6<array<string>, array<string>, array<vector>, array<float>, array<int>, array<int>> data;
		if( !ctx.Read( data ) )
		{
			BattleRoyaleUtils.Warn("FAILED TO READ SETADMINPLAYERLIST RPC");
			return;
		}
		if ( type != CallType.Client )
			return;

		//--- Carry the previous snapshot forward BEFORE overwriting, so the overlay has two samples
		//--- to interpolate between. Copied rather than swapped: the incoming arrays are owned by the
		//--- Param and must not be aliased into two fields.
		admin_prev_positions.Clear();
		int previous = admin_positions.Count();
		for( int i = 0; i < previous; i++ )
		{
			admin_prev_positions.Insert( admin_positions.Get(i) );
		}
		admin_prev_recv_ms = admin_recv_ms;

		admin_uids.Copy( data.param1 );
		admin_names.Copy( data.param2 );
		admin_positions.Copy( data.param3 );
		admin_healths.Copy( data.param4 );
		admin_kills.Copy( data.param5 );
		admin_slots.Copy( data.param6 );

		//--- A roster that changed LENGTH cannot be interpolated against the previous one - index i
		//--- is a different player now - so drop the old sample rather than lerping between two
		//--- unrelated positions, which reads as every tag flying across the map for half a second.
		if( admin_prev_positions.Count() != admin_positions.Count() )
			admin_prev_positions.Clear();

		admin_recv_ms = GetGame().GetTime();
		admin_seq = admin_seq + 1;
	}

	void SetSpectateOffer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			BattleRoyaleUtils.Warn("FAILED TO READ SETSPECTATEOFFER RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("[Spectate] SetSpectateOffer: " + data.param1);
			spectate_offered = data.param1;
		}
	}

	void SetSpectateTarget(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param4<string, string, vector, int> data;
		if( !ctx.Read( data ) )
		{
			//--- Warn, not Error: the global Error() halts the script VM, and a malformed push must
			//--- not take the client's whole mission down.
			BattleRoyaleUtils.Warn("FAILED TO READ SETSPECTATETARGET RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("[Spectate] SetSpectateTarget: %1 %2 %3 %4", data.param1, data.param2, data.param3, data.param4));

			//--- Only bump the sequence on a real target change, so the camera does not re-snap on
			//--- every keepalive.
			if( data.param1 != spectate_target_uid )
				spectate_seq = spectate_seq + 1;

			spectate_target_uid = data.param1;
			spectate_target_name = data.param2;
			spectate_target_pos = data.param3;
			spectate_mode = data.param4;
			spectate_target_obj = target;
			spectate_active = true;
		}
	}

	void EndSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("[Spectate] EndSpectate");

			//--- The camera is deliberately NOT deactivated. With no player entity to fall back to,
			//--- SetActive(false) renders nothing at all; it keeps orbiting its last anchor while the
			//--- server shuts the match down.
			spectate_active = false;

			//--- Withdraw the offer too. EndAll() also reaches spectators who are still sitting on
			//--- the death screen - anyone who died in the seconds before the last survivor did -
			//--- and leaving spectate_offered set left them looking at a Spectate button the server
			//--- had already stopped honouring: RequestSpectate returns at its m_Ended guard without
			//--- a word, so the click did nothing and the countdown ran down to nothing.
			//--- Seen live: registered 17:41:35, EndAll the same second, player clicked at 17:41:42
			//--- and gave up at 17:41:45.
			spectate_offered = false;
		}
	}

	void SetSpeakingPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<array<string>, int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETSPEAKINGPLAYERS RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			speaking_names.Clear();
			if ( data.param1 )
				speaking_names.InsertAll( data.param1 );

			speaking_self_index = data.param2;
			speaking_seq++;
		}
	}

	void SetVoiceSettings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<bool, bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETVOICESETTINGS RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetVoiceSettings: %1 %2", data.param1, data.param2));
			speaking_list_enabled = data.param1;
			speaking_list_during_match = data.param2;
		}
	}

	void SetResolvedName(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<string, string, string> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETRESOLVEDNAME RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetResolvedName: %1 -> %2", data.param2, data.param3));

			//--- An empty name is the server saying "drop whatever you hold for this uid" - the player
			//--- has set a name of their own, so theirs is the one to show from now on. See
			//--- BattleRoyaleNameService.BroadcastClearedName.
			if ( data.param3 == "" )
			{
				ClearResolvedName( data.param1 );
				return;
			}

			if ( data.param1 != "" )
				resolved_by_uid.Set( data.param1, data.param3 );

			if ( data.param2 != "" )
				resolved_by_name.Set( data.param2, data.param3 );
		}
	}

	/**
	 *  Forget the override held for one player.
	 *
	 *  resolved_by_name is keyed on the name the player was WEARING when they were resolved, which is
	 *  not something we still know - but its value is the override we are dropping, so every key
	 *  pointing at that goes with it. Collected first and removed second: removing while walking a map
	 *  moves the ground under the walk.
	 */
	void ClearResolvedName( string uid )
	{
		if ( uid == "" )
			return;
		if ( !resolved_by_uid.Contains( uid ) )
			return;

		string stale = resolved_by_uid.Get( uid );
		resolved_by_uid.Remove( uid );

		array<string> doomed = new array<string>();

		for ( int i = 0; i < resolved_by_name.Count(); i++ )
		{
			if ( resolved_by_name.GetElement( i ) == stale )
				doomed.Insert( resolved_by_name.GetKey( i ) );
		}

		for ( int j = 0; j < doomed.Count(); j++ )
		{
			resolved_by_name.Remove( doomed.Get( j ) );
		}
	}

	/**
	 *  The name to render for another player, given their client-side identity. Returns their
	 *  ordinary name unchanged when nothing was resolved for them, so callers can use it everywhere.
	 */
	string ResolveDisplayName(PlayerIdentity identity)
	{
		if ( !identity )
			return "";

		string engine_name = identity.GetName();

		string uid = identity.GetPlainId();
		if ( uid != "" && resolved_by_uid.Contains( uid ) )
			return resolved_by_uid.Get( uid );

		if ( resolved_by_name.Contains( engine_name ) )
			return resolved_by_name.Get( engine_name );

		return engine_name;
	}

	// Set the fade state

	bool fade_state = false;

	void SetFade(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETFADE RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetFade: %1", data.param1));
			if ( data.param1 )
			{
				fade_state = true;
			}
			else
			{
				fade_state = false;
			}
		}
	}

	// Set the input state

	bool input_state = false;

	void SetInput(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETINPUT RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetInput: %1", data.param1));
			input_state = data.param1;
		}
	}

	// Add a player kill

	int player_kills = 0;

	void AddPlayerKill(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ ADDPLAYERKILL RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("AddPlayerKill");
			//--- The payload is the server's AUTHORITATIVE running total, not an increment. It used to
			//--- be ignored in favour of a local +1, which drifted from the server the moment a kill
			//--- was scored while this client had no entity to receive it - a spectating killer whose
			//--- grenade landed after they died.
			player_kills = data.param1;
		}
	}

	// Start the match

	bool match_started = false;

	void StartMatch(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("StartMatch");
			match_started = true;
		}
	}

	/**
	 *  Are we in the pre-match lobby right now?
	 *
	 *  A SEPARATE FACT FROM match_started, AND NOT ITS INVERSE. match_started is a one-way latch set
	 *  by the StartMatch broadcast, so a client that connects AFTER that broadcast never receives it
	 *  and reads `match_started == false` for its entire session - which is only ever an admin, since
	 *  everyone else is kicked, and is exactly the case that made the lobby-only name tags follow an
	 *  admin into a live match. This one is pushed on every state transition AND per-identity when a
	 *  client reports in, so a late joiner is told the truth.
	 *
	 *  The window is narrower than !match_started as well: the server sends true only while the
	 *  current state is the lobby or the pre-match countdown, so spawn selection and the drop are
	 *  outside it. Nobody's name hangs over their head at the spawn point.
	 */
	bool lobby_phase = false;

	void SetLobbyPhase(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
		if ( !ctx.Read( data ) )
			return;

		if ( type == CallType.Client )
		{
			lobby_phase = data.param1;
			BattleRoyaleUtils.Trace("SetLobbyPhase " + lobby_phase);
		}
	}

	/**
	 *  Who to draw a lobby name tag over, as parallel arrays of network id and name.
	 *
	 *  ⚠️ THE NETWORK ID IS HERE BECAUSE A CLIENT CANNOT IDENTIFY ANOTHER PLAYER'S ENTITY.
	 *  PlayerBase.GetIdentity() is not reliably populated client-side for a REMOTE player, so
	 *  matching an entity to a name through GetPlainId() silently matches nothing. Both existing
	 *  consumers of that idiom - VigridPartyAPI.FindLocalPlayer and BattleRoyaleSpectatorTags - hide
	 *  it, because each falls back to a server-pushed position when the lookup fails and so still
	 *  renders. This overlay had no such fallback and drew nothing at all, which is how the gap was
	 *  finally noticed. Object.GetNetworkID / GetGame().GetObjectByNetworkId are the pair that do
	 *  work on both sides, and both have real vanilla call sites.
	 *
	 *  PER-IDENTITY, and teammates are dropped SERVER-SIDE before the packet is built - the client
	 *  draws every row it receives. That keeps party composition off the wire entirely, and it is
	 *  why there are no uids here: the leaderboard keeps SteamID64s off broadcasts deliberately and
	 *  a name tag needs no identifier beyond the entity it hangs over.
	 */
	ref array<int> lobby_net_low;
	ref array<int> lobby_net_high;
	ref array<string> lobby_names;

	void SetLobbyNames(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<array<int>, array<int>, array<string>> data;
		if ( !ctx.Read( data ) )
		{
			BattleRoyaleUtils.Warn("FAILED TO READ SETLOBBYNAMES RPC");
			return;
		}

		if ( type != CallType.Client )
			return;

		lobby_net_low.Clear();
		lobby_net_high.Clear();
		lobby_names.Clear();

		//--- Copied, never aliased: the incoming arrays are owned by the Param and go away with it.
		if ( data.param1 )
			lobby_net_low.Copy( data.param1 );
		if ( data.param2 )
			lobby_net_high.Copy( data.param2 );
		if ( data.param3 )
			lobby_names.Copy( data.param3 );

		BattleRoyaleUtils.Trace("SetLobbyNames " + lobby_names.Count() + " row(s)");
	}

	// Set the countdown deadline

	/**
	 *  When the countdown reaches zero, on THIS client's own clock, or BR_COUNTDOWN_NONE.
	 *
	 *  A deadline rather than a remaining time, and latched here at the instant the packet lands
	 *  rather than on an edge in BattleRoyaleClient.Update(). Two things follow, and both are the
	 *  point:
	 *
	 *  - The reader has nothing to edge-detect. It subtracts this from GetGame().GetTime() every
	 *    frame and is correct whether a push arrived this frame, five seconds ago, or never. There
	 *    is no edge to raise and therefore no edge to miss - the failure recorded against the map's
	 *    marker layer, where a snapshot that raised no edge waited out a one-second watchdog.
	 *  - Error does not accumulate. The old field held whole seconds and the client decremented it
	 *    from a 1 Hz CallQueue tick, so a round's worth of quantisation error piled up and each
	 *    client piled up its own. What is left here is the one-way latency at the moment of the
	 *    push, and the server re-asserts every 5 s anyway.
	 *
	 *  See BR_COUNTDOWN_NONE for why the wire carries milliseconds.
	 */
	int countdown_deadline_ms = BR_COUNTDOWN_NONE;

	void SetCountdownMs(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETCOUNTDOWNMS RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetCountdownMs: %1", data.param1));

			if ( data.param1 > 0 )
				countdown_deadline_ms = GetGame().GetTime() + data.param1;
			else
				countdown_deadline_ms = BR_COUNTDOWN_NONE;
		}
	}

	// Update the current play area

	vector current_play_area_center = "0 0 0";
	float current_play_area_radius = 0.0;

	void UpdateCurrentPlayArea(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<vector, float> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ UPDATECURRENTPLAYAREA RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("UpdateCurrentPlayArea: %1 %2", data.param1, data.param2));
			current_play_area_center = data.param1;
			current_play_area_radius = data.param2;
		}
	}

	// Update the future play area

	vector future_play_area_center = "0 0 0";
	float future_play_area_radius = 0.0;
	bool b_ArtillerySound = false;

	void UpdateFuturePlayArea(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<vector, float, bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ UPDATEFUTUREPLAYAREA RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("UpdateFuturePlayArea: %1 %2 %3", data.param1, data.param2, data.param3));
			future_play_area_center = data.param1;
			future_play_area_radius = data.param2;
			b_ArtillerySound = data.param3;
		}
	}

	// Set the top position

	int top_position = 0;

	void SetTopPosition(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETTOPPOSITION RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetTopPosition: %1", data.param1));
			top_position = data.param1;
		}
	}

	// Show the win screen

	bool winner_screen = false;

	void ShowWinScreen(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("ShowWinScreen");
			winner_screen = true;
		}
	}

	// The leaderboard, one ladder at a time
	//
	// Sent only in answer to a RequestLeaderboard, never broadcast. Parallel primitive arrays rather
	// than an array of structs, matching every other list this mod puts on the wire. No uids: the
	// menu renders names only, so shipping SteamID64s to every client would be pure liability.

	// Each ladder is cached separately and kept for the rest of the session - see
	// BattleRoyaleLeaderboardBoard for why that is correctness, not just an optimisation.

	ref BattleRoyaleLeaderboardBoard lb_solo;
	ref BattleRoyaleLeaderboardBoard lb_group;

	// Which season the ladders belong to. Bumping `season` server-side archives the old ladder and
	// starts an empty one, so showing this makes a sudden reset self-explanatory.
	int lb_season = 1;

	// Bumped on every payload; the menu repaints when it changes rather than every frame.
	int leaderboard_seq = 0;

	//! Never returns NULL - anything that is not the group ladder is the solo ladder.
	BattleRoyaleLeaderboardBoard GetLeaderboardBoard(int board)
	{
		if ( board == BR_LEADERBOARD_BOARD_GROUP )
			return lb_group;

		return lb_solo;
	}

	void SetLeaderboard(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param10<array<string>, array<int>, array<int>, array<int>, array<int>, int, int, int, int, int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETLEADERBOARD RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			//--- Routed by the board the SERVER answered for, never by whichever tab happens to be on
			//--- screen. A reply that arrives after the player switched tabs still lands in its own
			//--- cache slot instead of being dropped or painted under the wrong header.
			BattleRoyaleLeaderboardBoard target_board = GetLeaderboardBoard( data.param6 );
			target_board.Clear();

			//--- Copy rather than adopt the param's array: the Param is transient and reusing its
			//--- reference leaves these fields dangling once it goes away.
			if ( data.param1 )
				target_board.names.Copy( data.param1 );
			if ( data.param2 )
				target_board.matches.Copy( data.param2 );
			if ( data.param3 )
				target_board.wins.Copy( data.param3 );
			if ( data.param4 )
				target_board.kills.Copy( data.param4 );
			if ( data.param5 )
				target_board.points.Copy( data.param5 );

			target_board.self_rank = data.param7;
			target_board.self_wins = data.param8;
			target_board.self_points = data.param9;
			target_board.valid = true;

			lb_season = data.param10;

			leaderboard_seq = leaderboard_seq + 1;

			BattleRoyaleUtils.Trace(string.Format("SetLeaderboard: board %1, %2 rows, self rank %3", data.param6, target_board.Count(), target_board.self_rank));
		}
	}

	void ChatLog(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string, string> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ CHATLOG RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			Print("[ChatLog] " + data.param1);

			GetGame().Chat("S:" + data.param1, data.param2);
		}
	}

	void NotificationMessage(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param7<string, float, string, string, string, string, string> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ NOTIFICATIONMESSAGE RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("NotificationMessage: %1 %2 %3 %4 %5 %6 %7", data.param1, data.param2, data.param3, data.param4, data.param5, data.param6, data.param7));
			StringLocaliser message = new StringLocaliser(data.param1, data.param3, data.param4, data.param5, data.param6, data.param7);
			string translated_message = message.Format();

			// Special case
			// Finish the translation client side to get the correct key.
			// The token table is shared with the loading-screen hints, which need seven more of
			// these; see BattleRoyaleKeyTokens for why every lookup has a fallback.
			translated_message = BattleRoyaleKeyTokens.Substitute(translated_message);

			ExpansionNotification(DAYZBR_MSG_TITLE, translated_message, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, data.param2).Create();
		}
	}

	/**
	 *  Hot zones - static, purely cosmetic circles marking regions of interest.
	 *
	 *  Sent per-identity from BattleRoyaleServer.PlayerLoadedIn, so every client gets them the moment
	 *  it is provably listening, including one that connects mid-match. There is deliberately NO
	 *  "already received" latch: the payload is server config that never changes within a process, so
	 *  a repeat is idempotent, and a latch that Reset() forgot to clear would kill the feature for the
	 *  rest of the session with nothing in the log to say so.
	 *
	 *  Centres arrive as "x y z" strings because that is how zone_settings.json spells a world
	 *  position; they are converted once here so no renderer has to.
	 */
	ref array<vector> hot_zone_centers;
	ref array<float> hot_zone_radii;

	//--- Bumped on every accepted payload, so an edge-triggered renderer can repaint the frame the
	//--- data lands rather than waiting out its watchdog. Raised UNCONDITIONALLY at the end of the
	//--- handler - a seq bump hidden behind an early return is the documented way this goes wrong.
	int hot_zone_seq;

	void UpdateHotZones(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<array<string>, array<float>> data;
		if ( !ctx.Read( data ) )
		{
			BattleRoyaleUtils.Warn("FAILED TO READ UPDATEHOTZONES RPC");
			return;
		}

		if ( type != CallType.Client )
			return;

		hot_zone_centers.Clear();
		hot_zone_radii.Clear();

		if ( data.param1 && data.param2 )
		{
			//--- The server already truncated the pair in BattleRoyaleZoneData.Validate(), so this
			//--- is belt and braces against a future sender - but it costs one comparison and the
			//--- alternative is an out-of-bounds read on the shorter array.
			int count = data.param1.Count();
			if ( data.param2.Count() < count )
				count = data.param2.Count();

			for ( int i = 0; i < count; i++ )
			{
				//--- One array read per line, never nested in a call: a read sharing an expression
				//--- with a call has been measured here to return a different array's contents.
				string raw = data.param1[i];
				float radius = data.param2[i];
				hot_zone_centers.Insert( raw.ToVector() );
				hot_zone_radii.Insert( radius );
			}
		}

		hot_zone_seq++;

		BattleRoyaleUtils.Trace("UpdateHotZones " + hot_zone_centers.Count() + " zone(s), seq " + hot_zone_seq);
	}
}
