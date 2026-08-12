#ifndef SERVER
#ifdef DIAG_DEVELOPER
/**
 *  Battle Royale - the diag menu's callbacks. See PluginDiagMenu.c for the tree and the rationale.
 *
 *  THREE RULES EVERY CALLBACK HERE OBEYS.
 *
 *  1. Callbacks must be `static void` with one of the signatures vanilla documents - () , (int),
 *     (int,int), (bool), (bool,int), or for a range (float) / (float,int). A wrong signature binds
 *     NOTHING, with no error anywhere and an entry that simply does not respond - so a new entry is
 *     only proven by pressing it.
 *
 *     BindCallback's bool return is NOT that proof and must not be checked. Measured 2026-08-09: it
 *     returns false for a valid, freshly registered id whose IsRegistered() is true and whose
 *     callback signature is one of the documented ones, twice in a row on the same id. A wrapper
 *     that warned on false produced one false alarm for every entry in the tree and caught nothing.
 *     Vanilla discards the return at all ~100 of its own call sites, which is now explained.
 *
 *  2. A callback either writes BattleRoyaleDiag or calls an addon API - it does not reach into
 *     5_Mission, because this class is compiled in 4_World and cannot see it. Consumers poll.
 *
 *  3. Never BattleRoyaleUtils.Error() - it raises a VM exception. Warn, Info or Debug only.
 *
 *  A momentary button is a RegisterBool taking (bool, int): it acts on the rising edge and then
 *  resets its own entry with DiagMenu.SetValue(id, false), the idiom vanilla's DiagButtonAction uses.
 */
modded class PluginDiagMenuClient
{
	//! Rotates the fake kill feed rows through different weapons and distances, so repeated presses
	//! exercise the preview path with varied input rather than re-rendering one identical row.
	static int s_BRDiagKfCounter = 0;

	override protected void BindCallbacks()
	{
		super.BindCallbacks();

		DiagMenu.BindCallback(m_BRDiagDummyID, CBBRDiagDummy);

		DiagMenu.BindCallback(m_BRDiagSkipStateID, CBBRDiagSkipState);
		DiagMenu.BindCallback(m_BRDiagPauseStateID, CBBRDiagPauseState);
		DiagMenu.BindCallback(m_BRDiagGotoStateID, CBBRDiagGotoState);
		DiagMenu.BindCallback(m_BRDiagGotoGoID, CBBRDiagGotoGo);
		DiagMenu.BindCallback(m_BRDiagForceReadyID, CBBRDiagForceReady);
		DiagMenu.BindCallback(m_BRDiagLogStateID, CBBRDiagLogState);

		DiagMenu.BindCallback(m_BRDiagTpZoneID, CBBRDiagTpZone);
		DiagMenu.BindCallback(m_BRDiagTpNextZoneID, CBBRDiagTpNextZone);
		DiagMenu.BindCallback(m_BRDiagTpLobbyID, CBBRDiagTpLobby);
		DiagMenu.BindCallback(m_BRDiagForceUnstuckID, CBBRDiagForceUnstuck);

		DiagMenu.BindCallback(m_BRDiagHudForceID, CBBRDiagHudForce);
		DiagMenu.BindCallback(m_BRDiagHudPlayersID, CBBRDiagHudPlayers);
		DiagMenu.BindCallback(m_BRDiagHudGroupsID, CBBRDiagHudGroups);
		DiagMenu.BindCallback(m_BRDiagHudKillsID, CBBRDiagHudKills);
		DiagMenu.BindCallback(m_BRDiagHudCountdownID, CBBRDiagHudCountdown);
		DiagMenu.BindCallback(m_BRDiagOpenSpawnMenuID, CBBRDiagOpenSpawnMenu);
		DiagMenu.BindCallback(m_BRDiagOpenLeaderboardID, CBBRDiagOpenLeaderboard);
		DiagMenu.BindCallback(m_BRDiagFakeLeaderboardID, CBBRDiagFakeLeaderboard);
		DiagMenu.BindCallback(m_BRDiagFadeID, CBBRDiagFade);

#ifdef KILLFEED
		DiagMenu.BindCallback(m_BRDiagKfPushID, CBBRDiagKfPush);
		DiagMenu.BindCallback(m_BRDiagKfCauseID, CBBRDiagKfCause);
		DiagMenu.BindCallback(m_BRDiagKfWeaponID, CBBRDiagKfWeapon);
		DiagMenu.BindCallback(m_BRDiagKfFillID, CBBRDiagKfFill);
#endif

#ifdef VIGRID_PARTY
		DiagMenu.BindCallback(m_BRDiagPartySizeID, CBBRDiagPartySize);
		DiagMenu.BindCallback(m_BRDiagPartyApplyID, CBBRDiagPartyApply);
		DiagMenu.BindCallback(m_BRDiagPartyPingID, CBBRDiagPartyPing);
		DiagMenu.BindCallback(m_BRDiagPartyClearID, CBBRDiagPartyClear);
#endif

		DiagMenu.BindCallback(m_BRDiagZonesFakeID, CBBRDiagZonesFake);
		DiagMenu.BindCallback(m_BRDiagZoneRadiusID, CBBRDiagZoneRadius);
		DiagMenu.BindCallback(m_BRDiagZoneNextRadiusID, CBBRDiagZoneNextRadius);
		DiagMenu.BindCallback(m_BRDiagLogZoneTableID, CBBRDiagLogZoneTable);
#ifdef VIGRID_MAP
		DiagMenu.BindCallback(m_BRDiagClearMarkersID, CBBRDiagClearMarkers);
#endif

		DiagMenu.BindCallback(m_BRDiagTraceTpClientID, CBBRDiagTraceTpClient);
		DiagMenu.BindCallback(m_BRDiagTraceTpServerID, CBBRDiagTraceTpServer);
		DiagMenu.BindCallback(m_BRDiagTraceTicksID, CBBRDiagTraceTicks);

		DiagMenu.BindCallback(m_BRDiagLogLevelID, CBBRDiagLogLevel);
		DiagMenu.BindCallback(m_BRDiagLogLevelSrvID, CBBRDiagLogLevelSrv);
		DiagMenu.BindCallback(m_BRDiagChatMirrorID, CBBRDiagChatMirror);
	}

	//=============================================================================================
	//--- Match Flow. Every one of these is a no-op offline: there is no server to refuse it.
	//=============================================================================================

	static void CBBRDiagSkipState(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SKIP_STATE, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagPauseState(bool enabled)
	{
		int paused = 0;
		if ( enabled )
			paused = 1;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SET_PAUSED, paused, 0);
	}

	static void CBBRDiagGotoState(float value)
	{
		//--- Stored only. "Jump: Go" is what sends, so scrubbing this cannot fire an RPC per step.
		BattleRoyaleDiag.goto_state = (int)value;
	}

	static void CBBRDiagGotoGo(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.GOTO_STATE, BattleRoyaleDiag.goto_state, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagForceReady(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.FORCE_READY_ALL, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagLogState(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.LOG_STATE, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	//=============================================================================================
	//--- Spawn / Teleport
	//=============================================================================================

	static void CBBRDiagTpZone(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.TP_ZONE_CENTER, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagTpNextZone(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.TP_NEXT_ZONE, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagTpLobby(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.TP_LOBBY, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagForceUnstuck(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.FORCE_UNSTUCK, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	//=============================================================================================
	//--- HUD & Menus. Client only.
	//=============================================================================================

	static void CBBRDiagHudForce(bool enabled)
	{
		BattleRoyaleDiag.hud_force = enabled;
	}

	static void CBBRDiagHudPlayers(float value)
	{
		BattleRoyaleDiag.hud_players = (int)value;
	}

	static void CBBRDiagHudGroups(float value)
	{
		BattleRoyaleDiag.hud_groups = (int)value;
	}

	static void CBBRDiagHudKills(float value)
	{
		BattleRoyaleDiag.hud_kills = (int)value;
	}

	static void CBBRDiagHudCountdown(float value)
	{
		BattleRoyaleDiag.hud_countdown = (int)value;
	}

	static void CBBRDiagOpenSpawnMenu(bool enabled, int id)
	{
		if ( !enabled )
			return;

		//--- A counter, not a flag: BattleRoyaleClient.Update owns the actual opening, because the
		//--- menu class is 5_Mission and this is 4_World.
		BattleRoyaleDiag.req_open_spawn_menu = BattleRoyaleDiag.req_open_spawn_menu + 1;
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagOpenLeaderboard(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.req_open_leaderboard = BattleRoyaleDiag.req_open_leaderboard + 1;
		DiagMenu.SetValue(id, false);
	}

	/**
	 *  Fill both ladders with plausible rows, so the leaderboard menu can be laid out without a
	 *  server that has ever finished a match.
	 *
	 *  Writes BattleRoyaleRPC exactly as the SetLeaderboard handler does, including the sequence
	 *  bump the menu repaints on - it is the same data by the time anything reads it.
	 */
	static void CBBRDiagFakeLeaderboard(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
		BRDiagFillBoard( br_rpc.GetLeaderboardBoard(BR_LEADERBOARD_BOARD_SOLO), 12 );
		BRDiagFillBoard( br_rpc.GetLeaderboardBoard(BR_LEADERBOARD_BOARD_GROUP), 8 );

		br_rpc.lb_season = 1;
		br_rpc.leaderboard_seq = br_rpc.leaderboard_seq + 1;

		DiagMenu.SetValue(id, false);
	}

	static void BRDiagFillBoard(BattleRoyaleLeaderboardBoard board, int rows)
	{
		if ( !board )
			return;

		board.Clear();

		for ( int i = 0; i < rows; i++ )
		{
			board.names.Insert("Fake Player " + (i + 1));
			board.matches.Insert(40 - i);
			board.wins.Insert(12 - i);
			board.kills.Insert(180 - (i * 11));
			board.points.Insert(2400 - (i * 137));
		}

		//--- Mid-table on purpose: a self rank of 1 hides every bug in how the row is highlighted.
		board.self_rank = 5;
		board.self_wins = 3;
		board.self_points = 900;
		board.valid = true;
	}

	static void CBBRDiagFade(bool enabled)
	{
		//--- Sets the same field the wire sets, so the real FadeIn / FadeOut edge in
		//--- BattleRoyaleClient.Update runs. Nothing here duplicates the effect.
		BattleRoyaleRPC.GetInstance().fade_state = enabled;
	}

#ifdef KILLFEED
	//=============================================================================================
	//--- Kill Feed
	//=============================================================================================

	static void CBBRDiagKfPush(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BRDiagPushFakeKill();
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagKfCause(int value)
	{
		BattleRoyaleDiag.kf_cause = value;
	}

	static void CBBRDiagKfWeapon(bool enabled)
	{
		BattleRoyaleDiag.kf_with_weapon = enabled;
	}

	static void CBBRDiagKfFill(bool enabled, int id)
	{
		if ( !enabled )
			return;

		//--- Two more than fit, so eviction and the Release() that goes with it actually happen.
		int rows = KillFeedAPI.GetMaxRows() + 2;
		for ( int i = 0; i < rows; i++ )
			BRDiagPushFakeKill();

		DiagMenu.SetValue(id, false);
	}

	/**
	 *  One synthetic row, varied by a rolling counter.
	 *
	 *  The weapon rotates because the middle cell spawns a client-local copy of the classname and
	 *  re-attaches accessories onto it - the part of a row most likely to break - and pushing the
	 *  same gun four times would never notice a classname-specific failure.
	 */
	static void BRDiagPushFakeKill()
	{
		s_BRDiagKfCounter = s_BRDiagKfCounter + 1;

		int cause = BattleRoyaleDiag.kf_cause;
		string sep = KillFeedAPI.GetAttachmentSeparator();

		string weapon = "";
		string attachments = "";

		//--- Only the two player-weapon causes render a model at all; anything else falls back to
		//--- an icon and a phrase, and handing those a classname would test nothing.
		bool wants_weapon = BattleRoyaleDiag.kf_with_weapon;
		if ( cause != KillFeedCause.WEAPON && cause != KillFeedCause.MELEE )
			wants_weapon = false;

		if ( wants_weapon && cause == KillFeedCause.MELEE )
		{
			weapon = "Machete";
		}
		else if ( wants_weapon )
		{
			int pick = s_BRDiagKfCounter % 3;
			if ( pick == 0 )
			{
				weapon = "M4A1";
				attachments = "M4_RISHndgrd" + sep + "M4_MPBttstck" + sep + "ACOGOptic";
			}
			else if ( pick == 1 )
			{
				weapon = "AKM";
				attachments = "AK_WoodBttstck" + sep + "AK_WoodHndgrd";
			}
			else
			{
				weapon = "Mosin9130";
				attachments = "PUScopeOptic";
			}
		}

		//--- Distance is only meaningful for a shot; -1 hides the field, as KillFeedEntry documents.
		int distance = -1;
		if ( cause == KillFeedCause.WEAPON )
			distance = 25 + ((s_BRDiagKfCounter * 37) % 400);

		//--- No killer for the causes nobody is credited with - a zone death, a fall, an animal.
		string killer = "Fake Killer " + s_BRDiagKfCounter;
		if ( cause == KillFeedCause.ZONE || cause == KillFeedCause.ENVIRONMENT )
			killer = "";

		KillFeedAPI.DebugPush( killer, "Fake Victim " + s_BRDiagKfCounter, weapon, attachments, distance, cause );
	}
#endif

#ifdef VIGRID_PARTY
	//=============================================================================================
	//--- Party
	//=============================================================================================

	static void CBBRDiagPartySize(float value)
	{
		BattleRoyaleDiag.party_size = (int)value;
	}

	static void CBBRDiagPartyApply(bool enabled, int id)
	{
		if ( !enabled )
			return;

		VigridPartyAPI.DebugSetRoster( BattleRoyaleDiag.party_size );
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagPartyClear(bool enabled, int id)
	{
		if ( !enabled )
			return;

		VigridPartyAPI.DebugClearRoster();
		DiagMenu.SetValue(id, false);
	}

	/**
	 *  Drop a ping roughly where the player is looking.
	 *
	 *  A straight projection down the camera rather than a raycast: a debug marker only has to land
	 *  somewhere plausible and visible, and a miss is obvious the moment it is drawn.
	 */
	static void CBBRDiagPartyPing(bool enabled, int id)
	{
		if ( !enabled )
			return;

		vector camera_pos = GetGame().GetCurrentCameraPosition();
		vector camera_dir = GetGame().GetCurrentCameraDirection();

		vector ping_pos = camera_pos + (camera_dir * 150);
		ping_pos[1] = GetGame().SurfaceY(ping_pos[0], ping_pos[2]);

		VigridPartyAPI.DebugAddPing( ping_pos, 0 );
		DiagMenu.SetValue(id, false);
	}
#endif

	//=============================================================================================
	//--- Map & Zones
	//=============================================================================================

	static void CBBRDiagZonesFake(bool enabled)
	{
		BattleRoyaleDiag.zones_fake = enabled;
	}

	static void CBBRDiagZoneRadius(float value)
	{
		BattleRoyaleDiag.zone_radius = value;
	}

	static void CBBRDiagZoneNextRadius(float value)
	{
		BattleRoyaleDiag.zone_next_radius = value;
	}

	static void CBBRDiagLogZoneTable(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.LOG_ZONE_TABLE, 0, 0);
		DiagMenu.SetValue(id, false);
	}

#ifdef VIGRID_MAP
	static void CBBRDiagClearMarkers(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.CLEAR_MAP_MARKERS, 0, 0);
		DiagMenu.SetValue(id, false);
	}
#endif

	//=============================================================================================
	//--- Teleport Trace
	//=============================================================================================

	static void CBBRDiagTraceTpClient(bool enabled)
	{
		BattleRoyaleDiag.trace_teleport = enabled;
	}

	static void CBBRDiagTraceTpServer(bool enabled)
	{
		int on = 0;
		if ( enabled )
			on = 1;

		//--- Tick budget rides along, so both sides always agree on the window without a second
		//--- entry that could be set on one side only.
		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SET_TRACE_TP, on, BattleRoyaleDiag.trace_teleport_ticks);
	}

	static void CBBRDiagTraceTicks(float value)
	{
		BattleRoyaleDiag.trace_teleport_ticks = (int)value;
	}

	//=============================================================================================
	//--- Logging. Index 0 is "Default", which maps to -1: resolve normally.
	//=============================================================================================

	static void CBBRDiagLogLevel(int value)
	{
		BattleRoyaleDiag.log_level_override = value - 1;
	}

	static void CBBRDiagLogLevelSrv(int value)
	{
		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SET_LOG_LEVEL, value - 1, 0);
	}

	static void CBBRDiagChatMirror(bool enabled)
	{
		int on = 0;
		if ( enabled )
			on = 1;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SET_CHAT_MIRROR, on, 0);
	}

	static void CBBRDiagDummy(int value)
	{
		BattleRoyaleUtils.Trace("CBBRDiagDummy: " + value);
	}
}
#endif // DIAG_DEVELOPER
#endif // SERVER
