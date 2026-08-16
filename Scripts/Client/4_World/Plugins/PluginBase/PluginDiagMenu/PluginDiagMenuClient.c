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
		DiagMenu.BindCallback(m_BRDiagFakeUnloadedID, CBBRDiagFakeUnloaded);
		DiagMenu.BindCallback(m_BRDiagLogGateID, CBBRDiagLogGate);

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
		DiagMenu.BindCallback(m_BRDiagFakeLastMatchID, CBBRDiagFakeLastMatch);
		DiagMenu.BindCallback(m_BRDiagLastMatchCauseID, CBBRDiagLastMatchCause);
		DiagMenu.BindCallback(m_BRDiagLastMatchNotPlayedID, CBBRDiagLastMatchNotPlayed);
		DiagMenu.BindCallback(m_BRDiagOpenDeathScreenID, CBBRDiagOpenDeathScreen);
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
		DiagMenu.BindCallback(m_BRDiagPartyOnlineCountID, CBBRDiagPartyOnlineCount);
		DiagMenu.BindCallback(m_BRDiagPartyOnlineApplyID, CBBRDiagPartyOnlineApply);
		DiagMenu.BindCallback(m_BRDiagPartyInviteMeID, CBBRDiagPartyInviteMe);
		DiagMenu.BindCallback(m_BRDiagPartyOfflineID, CBBRDiagPartyOffline);
		DiagMenu.BindCallback(m_BRDiagPartyPingID, CBBRDiagPartyPing);
		DiagMenu.BindCallback(m_BRDiagPartyClearID, CBBRDiagPartyClear);
#endif

		DiagMenu.BindCallback(m_BRDiagZonesFakeID, CBBRDiagZonesFake);
		DiagMenu.BindCallback(m_BRDiagZonesNoCurrentID, CBBRDiagZonesNoCurrent);
		DiagMenu.BindCallback(m_BRDiagZoneRadiusID, CBBRDiagZoneRadius);
		DiagMenu.BindCallback(m_BRDiagZoneNextRadiusID, CBBRDiagZoneNextRadius);
		DiagMenu.BindCallback(m_BRDiagLogZoneTableID, CBBRDiagLogZoneTable);
#ifdef VIGRID_MAP
		DiagMenu.BindCallback(m_BRDiagClearMarkersID, CBBRDiagClearMarkers);
#endif

		DiagMenu.BindCallback(m_BRDiagSpectateEnabledID, CBBRDiagSpectateEnabled);
		DiagMenu.BindCallback(m_BRDiagAdminSpectateID, CBBRDiagAdminSpectate);
		DiagMenu.BindCallback(m_BRDiagKillSelfID, CBBRDiagKillSelf);
		DiagMenu.BindCallback(m_BRDiagLogSpectatorsID, CBBRDiagLogSpectators);
		DiagMenu.BindCallback(m_BRDiagTpTargetDistID, CBBRDiagTpTargetDist);
		DiagMenu.BindCallback(m_BRDiagTpTargetGoID, CBBRDiagTpTargetGo);
		DiagMenu.BindCallback(m_BRDiagTpCorpseID, CBBRDiagTpCorpse);
		DiagMenu.BindCallback(m_BRDiagSpectateTraceID, CBBRDiagSpectateTrace);

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

	/**
	 *  A LATCHING toggle, deliberately unlike every other entry in this menu.
	 *
	 *  The momentary idiom above - act on the rising edge, then SetValue(id, false) - is wrong here:
	 *  it would send "mark everyone unloaded" and immediately show the entry as off, leaving no way
	 *  to send the matching "mark them loaded again" and no on-screen record that the gate is being
	 *  held. So both edges are sent, and the entry keeps whatever the tester set it to.
	 *
	 *  Turning it OFF is half the test: that is what releases the gate and lets the match start.
	 */
	static void CBBRDiagFakeUnloaded(bool enabled, int id)
	{
		int flag = 0;
		if ( enabled )
			flag = 1;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SET_FAKE_UNLOADED, flag, 0);
	}

	static void CBBRDiagLogGate(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.LOG_LOBBY_GATE, 0, 0);
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

		//--- Both counts deliberately OVERFLOW the list. The scroll viewport is 440 px against 26 px
		//--- rows, so it holds about 16: at the old 12 and 8 neither ladder ever needed a scrollbar,
		//--- which made scrolling untestable here and left the shorter-refresh case unexercised.
		//--- They stay different so that switching Solo -> Group is still a shrink.
		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
		BRDiagFillBoard( br_rpc.GetLeaderboardBoard(BR_LEADERBOARD_BOARD_SOLO), 40 );
		BRDiagFillBoard( br_rpc.GetLeaderboardBoard(BR_LEADERBOARD_BOARD_GROUP), 25 );

		br_rpc.lb_season = 1;
		br_rpc.leaderboard_seq = br_rpc.leaderboard_seq + 1;

		DiagMenu.SetValue(id, false);
	}

	static void BRDiagFillBoard(BattleRoyaleLeaderboardBoard board, int rows)
	{
		if ( !board )
			return;

		board.Clear();

		//--- Clamped at zero. The old figures went negative past about a dozen rows, so a longer
		//--- ladder rendered "-38 wins" and read as a formatting bug rather than as fake data.
		for ( int i = 0; i < rows; i++ )
		{
			board.names.Insert("Fake Player " + (i + 1));
			board.matches.Insert(Math.Max(1, 120 - (i * 2)));
			board.wins.Insert(Math.Max(0, 40 - i));
			board.kills.Insert(Math.Max(0, 620 - (i * 14)));
			board.points.Insert(Math.Max(0, 7800 - (i * 178)));
		}

		//--- Mid-table on purpose: a self rank of 1 hides every bug in how the row is highlighted.
		board.self_rank = 5;
		board.self_wins = 3;
		board.self_points = 900;
		board.valid = true;
	}

	/**
	 *  Fill the Last Match tab with a plausible previous match.
	 *
	 *  Writes BattleRoyaleRPC exactly as the SetLastMatchTable and SetLastMatchRecap handlers do,
	 *  sequence bumps included, so what the menu reads is the production data by the time it reads it.
	 *
	 *  This is the whole client half of the feature made testable offline, where SERVER is undefined
	 *  and none of the server code that produces this data exists at all.
	 */
	static void CBBRDiagFakeLastMatch(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
		BattleRoyaleLastMatch data = br_rpc.last_match;

		data.Clear();

		//--- 40 rows against a 342 px viewport at 26 px each - about 13 visible. Deliberately
		//--- overflowing: a fixture that FITS cannot reach the scrolling it exists to exercise, which
		//--- is how the leaderboard's own scroll bug survived two diagnoses.
		for ( int i = 0; i < 40; i++ )
		{
			data.names.Insert("Fake Player " + (i + 1));
			data.places.Insert(i + 1);
			data.kills.Insert(Math.Max(0, 12 - (i / 3)));
			data.damage.Insert(Math.Max(0, 940 - (i * 21)));
			data.survived.Insert(Math.Max(0, 1450 - (i * 33)));
			//--- Squads of three, so the squad block has real rows to sum and the self row is not
			//--- alone in its group.
			data.groups.Insert(i / 3);
		}

		//--- Mid-table, and NOT row 0: a self index of 0 hides every bug in the row highlight and in
		//--- the squad sum at once.
		data.self_index = 23;
		if ( BattleRoyaleDiag.lastmatch_not_played )
			data.self_index = -1;

		data.field_size = 14;
		data.flags = BR_LASTMATCH_FLAG_GROUPED;
		data.valid = true;

		br_rpc.recap_cause = BattleRoyaleDiag.lastmatch_cause;
		br_rpc.recap_killer_name = "Myst";
		br_rpc.recap_weapon_type = "M4A1";
		br_rpc.recap_distance_m = 212;
		br_rpc.recap_killer_health_pct = 23;
		br_rpc.recap_damage_to_killer = 87;
		br_rpc.recap_self_group = 7;
		br_rpc.recap_hits = 11;
		br_rpc.recap_valid = true;

		br_rpc.last_match_seq = br_rpc.last_match_seq + 1;
		br_rpc.recap_seq = br_rpc.recap_seq + 1;

		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagLastMatchCause(int value)
	{
		//--- Offset by the enum's first *resolvable* cause, so the picker reads as causes rather than
		//--- as raw numbers. UNKNOWN and NONE are included: NONE is the winner's card and UNKNOWN is
		//--- the degraded disconnect path, and both have their own render branch.
		BattleRoyaleDiag.lastmatch_cause = value;
	}

	static void CBBRDiagLastMatchNotPlayed(bool enabled)
	{
		BattleRoyaleDiag.lastmatch_not_played = enabled;
	}

	static void CBBRDiagOpenDeathScreen(bool enabled, int id)
	{
		if ( !enabled )
			return;

		//--- Offline the local player never dies, and DeathScreenMenu.Tick() closes itself on
		//--- IsAlive(). Without this the screen shuts on its first tick and looks like a layout that
		//--- failed to load.
		BattleRoyaleDiag.suppress_alive_close = true;
		BattleRoyaleDiag.req_open_death_screen = BattleRoyaleDiag.req_open_death_screen + 1;
		DiagMenu.SetValue(id, false);
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

	static void CBBRDiagPartyOnlineCount(float value)
	{
		BattleRoyaleDiag.party_online_count = (int)value;
	}

	/**
	 *  Fabricate the party menu's left column - the connected players you can invite.
	 *
	 *  The half "Apply Fake Party" never covered. That one builds the roster; this one builds the
	 *  list of people to build a roster FROM, which on a one-client session is empty because it only
	 *  ever comes from a real server reply. With it applied every button in the menu has a target
	 *  and acts locally, so the whole screen is reachable alone.
	 */
	static void CBBRDiagPartyOnlineApply(bool enabled, int id)
	{
		if ( !enabled )
			return;

		VigridPartyAPI.DebugSetPlayerList( BattleRoyaleDiag.party_online_count );
		DiagMenu.SetValue(id, false);
	}

	//! Fabricate an invitation addressed to you, so the banner and its Accept / Decline are
	//! reachable. Accepting deliberately lands you in a party you do NOT lead - the branch that
	//! hides Promote, Kick and every Invite button.
	static void CBBRDiagPartyInviteMe(bool enabled, int id)
	{
		if ( !enabled )
			return;

		VigridPartyAPI.DebugReceiveInvite();
		DiagMenu.SetValue(id, false);
	}

	//! Flip the last teammate between online and offline, for the grey "(Offline)" row. Resets on
	//! the next roster change, which is a deliberate simplification rather than a bug.
	static void CBBRDiagPartyOffline(bool enabled, int id)
	{
		if ( !enabled )
			return;

		VigridPartyAPI.DebugToggleMemberOffline();
		DiagMenu.SetValue(id, false);
	}

	/**
	 *  Drop every fabrication AND the latch that has been discarding server pushes.
	 *
	 *  Must be pressed before reading anything real off this client: while the latch is down the
	 *  party state is frozen, which on a live server presents as a broken connection rather than as
	 *  a debug switch left on.
	 */
	static void CBBRDiagPartyClear(bool enabled, int id)
	{
		if ( !enabled )
			return;

		VigridPartyAPI.DebugClearFakes();
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

	static void CBBRDiagZonesNoCurrent(bool enabled)
	{
		BattleRoyaleDiag.zones_fake_no_current = enabled;
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
	//--- Spectate. All three are server actions, so all three are no-ops offline.
	//=============================================================================================

	static void CBBRDiagSpectateEnabled(bool enabled)
	{
		int on = 0;
		if ( enabled )
			on = 1;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SET_SPECTATE, on, 0);
	}

	/**
	 *  Flip admin_spectate_enabled for this process.
	 *
	 *  Registered so the OFF case is reachable: the setting ships on, so the interesting test is that
	 *  turning it off actually makes AdminEligibility refuse. In memory only - Load() re-saves on the
	 *  next boot, so a diag toggle must never become a persisted setting.
	 *
	 *  Grants nothing on its own: the sender still has to be in admins_steamid64.
	 */
	static void CBBRDiagAdminSpectate(bool enabled)
	{
		int on = 0;
		if ( enabled )
			on = 1;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SET_ADMIN_SPECTATE, on, 0);
	}

	/**
	 *  Kill the local player, to reach the death screen without a second client.
	 *
	 *  Deliberately fires the SERVER action rather than touching the local PlayerBase: health is
	 *  server-authoritative, and a client-side SetHealth would either be rejected or - worse -
	 *  produce a local-only death that never reaches EEKilled and so never registers a spectator.
	 *  The button would look like it worked and test nothing.
	 *
	 *  Reset to false immediately, like every other one-shot here, so it can be pressed again next
	 *  match without having to toggle it off first.
	 */
	static void CBBRDiagKillSelf(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.KILL_SELF, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagLogSpectators(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.LOG_SPECTATORS, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagTpTargetDist(float value)
	{
		BattleRoyaleDiag.tp_target_distance = (int)value;
	}

	/**
	 *  Fling the watched target to the chosen radius from this spectator's corpse.
	 *
	 *  The distance rides on the action rather than being pushed by the range callback, so scrubbing
	 *  the slider from 100 to 3000 does not teleport a live player 59 times on the way.
	 */
	static void CBBRDiagTpTargetGo(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SPECTATE_TP_TARGET, BattleRoyaleDiag.tp_target_distance, 0);
		DiagMenu.SetValue(id, false);
	}

	/**
	 *  Move this spectator's own corpse onto the player they are watching.
	 *
	 *  A measurement, not a feature. Press it while the target is out past ~1 km with entity=0: if
	 *  entity returns to 1, the replication bubble really is centred on the corpse and a proper fix
	 *  is worth designing. If it stays 0, the assumption is wrong and nothing further should be built
	 *  on it. The corpse carries the victim's loot with it, which is why this is diag-only.
	 */
	static void CBBRDiagTpCorpse(bool enabled, int id)
	{
		if ( !enabled )
			return;

		BattleRoyaleDiag.SendServerAction(BattleRoyaleDiagAction.SPECTATE_TP_CORPSE, 0, 0);
		DiagMenu.SetValue(id, false);
	}

	static void CBBRDiagSpectateTrace(float value)
	{
		BattleRoyaleDiag.spectate_trace_interval = value;
	}

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
