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
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", this );
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

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetResolvedName", this );

		resolved_by_uid = new map<string, string>();
		resolved_by_name = new map<string, string>();

		lb_solo = new BattleRoyaleLeaderboardBoard();
		lb_group = new BattleRoyaleLeaderboardBoard();

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
		countdown_seconds = 0;
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

	//! The server has decided this player is dead and eligible - the death screen may offer Spectate.
	bool spectate_offered = false;
	bool spectate_active = false;
	int spectate_seq = 0;
	string spectate_target_uid = "";
	string spectate_target_name = "";
	vector spectate_target_pos = "0 0 0";
	int spectate_mode = 0;
	Object spectate_target_obj = NULL;

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
			player_kills += 1;
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

	// Set the countdown seconds

	int countdown_seconds = 0;

	void SetCountdownSeconds(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SETCOUNTDOWNSECONDS RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace(string.Format("SetCountdownSeconds: %1", data.param1));
			countdown_seconds = data.param1;
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
			// Finish the translation client side to get the correct key
			if (translated_message.Contains("READY_KEY"))
			{
				translated_message.Replace("READY_KEY", InputUtils.GetButtonNameFromInput("UADayZBRReadyUp", EInputDeviceType.MOUSE_AND_KEYBOARD));
			}
			if (translated_message.Contains("UNSTUCK_KEY"))
			{
				translated_message.Replace("UNSTUCK_KEY", InputUtils.GetButtonNameFromInput("UADayZBRUnstuck", EInputDeviceType.MOUSE_AND_KEYBOARD));
			}

			ExpansionNotification(DAYZBR_MSG_TITLE, translated_message, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, data.param2).Create();
		}
	}
}