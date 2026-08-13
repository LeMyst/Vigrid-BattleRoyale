#ifndef SERVER
#ifdef DIAG_DEVELOPER
/**
 *  Battle Royale - the diag menu tree.
 *
 *  Reached in DayZDiag_x64 through the engine's own diag window, under `Script - Modded`. This half
 *  declares the ids and builds the tree; PluginDiagMenuClient binds the callbacks.
 *
 *  WHAT THIS IS FOR. Most of this mod is expensive to reach: a kill feed needs two clients, a party
 *  needs three, and anything past the lobby needs a ready-up and a countdown first. Every entry
 *  here exists to collapse one of those loops, and each one drives the *real* code path rather than
 *  a lookalike - fake kill rows go through the network queue, fake zones go through the same
 *  VigridMapAPI.SetZones call a match uses, teleports go through the sync juncture.
 *
 *  WHAT RUNS WHERE. Callbacks are client-side; the state machine is #ifdef SERVER. Anything that
 *  has to happen server-side goes out as one "BRDiagAction" RPC and is refused there unless the
 *  sender is in admins_steamid64. In an offline (LaunchOffline.bat) session no #ifdef SERVER code
 *  is compiled at all, so Match Flow and Spawn / Teleport doing nothing there is expected.
 *
 *  ID BUDGET. Ids come from GetModdedDiagID(), which counts up from DiagMenuIDs.MODDED_MENU (~235)
 *  against an engine hard cap of 512 SHARED WITH EVERY OTHER MOD LOADED. This tree spends 60,
 *  measured from the canary line rather than counted by hand (the previous "47" was one short). Do
 *  not allocate an id for something a bag field can carry: an entry whose value is only ever read
 *  when another entry fires does not need its id kept.
 */
modded class PluginDiagMenu
{
	protected int m_BRDiagRootMenuID;
	protected int m_BRDiagDummyID;

	//--- Match Flow
	protected int m_BRDiagFlowMenuID;
	protected int m_BRDiagSkipStateID;
	protected int m_BRDiagPauseStateID;
	protected int m_BRDiagGotoStateID;
	protected int m_BRDiagGotoGoID;
	protected int m_BRDiagForceReadyID;
	protected int m_BRDiagLogStateID;

	//--- Spawn / Teleport
	protected int m_BRDiagTeleportMenuID;
	protected int m_BRDiagTpZoneID;
	protected int m_BRDiagTpNextZoneID;
	protected int m_BRDiagTpLobbyID;
	protected int m_BRDiagForceUnstuckID;

	//--- HUD & Menus
	protected int m_BRDiagHudMenuID;
	protected int m_BRDiagHudForceID;
	protected int m_BRDiagHudPlayersID;
	protected int m_BRDiagHudGroupsID;
	protected int m_BRDiagHudKillsID;
	protected int m_BRDiagHudCountdownID;
	protected int m_BRDiagOpenSpawnMenuID;
	protected int m_BRDiagOpenLeaderboardID;
	protected int m_BRDiagFakeLeaderboardID;
	protected int m_BRDiagFakeLastMatchID;
	protected int m_BRDiagLastMatchCauseID;
	protected int m_BRDiagLastMatchNotPlayedID;
	protected int m_BRDiagOpenDeathScreenID;
	protected int m_BRDiagFadeID;

	//--- Kill Feed
	protected int m_BRDiagKillFeedMenuID;
	protected int m_BRDiagKfPushID;
	protected int m_BRDiagKfCauseID;
	protected int m_BRDiagKfWeaponID;
	protected int m_BRDiagKfFillID;

	//--- Party
	protected int m_BRDiagPartyMenuID;
	protected int m_BRDiagPartySizeID;
	protected int m_BRDiagPartyApplyID;
	protected int m_BRDiagPartyOnlineCountID;
	protected int m_BRDiagPartyOnlineApplyID;
	protected int m_BRDiagPartyInviteMeID;
	protected int m_BRDiagPartyOfflineID;
	protected int m_BRDiagPartyPingID;
	protected int m_BRDiagPartyClearID;

	//--- Map & Zones
	protected int m_BRDiagZoneMenuID;
	protected int m_BRDiagZonesFakeID;
	protected int m_BRDiagZoneRadiusID;
	protected int m_BRDiagZoneNextRadiusID;
	protected int m_BRDiagLogZoneTableID;
	protected int m_BRDiagClearMarkersID;

	//--- Spectate
	protected int m_BRDiagSpectateMenuID;
	protected int m_BRDiagSpectateEnabledID;
	protected int m_BRDiagAdminSpectateID;
	protected int m_BRDiagKillSelfID;
	protected int m_BRDiagLogSpectatorsID;
	protected int m_BRDiagTpTargetDistID;
	protected int m_BRDiagTpTargetGoID;
	protected int m_BRDiagTpCorpseID;
	protected int m_BRDiagSpectateTraceID;

	//--- Teleport Trace
	protected int m_BRDiagTraceMenuID;
	protected int m_BRDiagTraceTpClientID;
	protected int m_BRDiagTraceTpServerID;
	protected int m_BRDiagTraceTicksID;

	//--- Logging
	protected int m_BRDiagLogMenuID;
	protected int m_BRDiagLogLevelID;
	protected int m_BRDiagLogLevelSrvID;
	protected int m_BRDiagChatMirrorID;

	override protected void RegisterModdedDiagsIDs()
	{
		super.RegisterModdedDiagsIDs();

		m_BRDiagRootMenuID = GetModdedDiagID();
		m_BRDiagDummyID = GetModdedDiagID();

		m_BRDiagFlowMenuID = GetModdedDiagID();
		m_BRDiagSkipStateID = GetModdedDiagID();
		m_BRDiagPauseStateID = GetModdedDiagID();
		m_BRDiagGotoStateID = GetModdedDiagID();
		m_BRDiagGotoGoID = GetModdedDiagID();
		m_BRDiagForceReadyID = GetModdedDiagID();
		m_BRDiagLogStateID = GetModdedDiagID();

		m_BRDiagTeleportMenuID = GetModdedDiagID();
		m_BRDiagTpZoneID = GetModdedDiagID();
		m_BRDiagTpNextZoneID = GetModdedDiagID();
		m_BRDiagTpLobbyID = GetModdedDiagID();
		m_BRDiagForceUnstuckID = GetModdedDiagID();

		m_BRDiagHudMenuID = GetModdedDiagID();
		m_BRDiagHudForceID = GetModdedDiagID();
		m_BRDiagHudPlayersID = GetModdedDiagID();
		m_BRDiagHudGroupsID = GetModdedDiagID();
		m_BRDiagHudKillsID = GetModdedDiagID();
		m_BRDiagHudCountdownID = GetModdedDiagID();
		m_BRDiagOpenSpawnMenuID = GetModdedDiagID();
		m_BRDiagOpenLeaderboardID = GetModdedDiagID();
		m_BRDiagFakeLeaderboardID = GetModdedDiagID();
		m_BRDiagFakeLastMatchID = GetModdedDiagID();
		m_BRDiagLastMatchCauseID = GetModdedDiagID();
		m_BRDiagLastMatchNotPlayedID = GetModdedDiagID();
		m_BRDiagOpenDeathScreenID = GetModdedDiagID();
		m_BRDiagFadeID = GetModdedDiagID();

		m_BRDiagKillFeedMenuID = GetModdedDiagID();
		m_BRDiagKfPushID = GetModdedDiagID();
		m_BRDiagKfCauseID = GetModdedDiagID();
		m_BRDiagKfWeaponID = GetModdedDiagID();
		m_BRDiagKfFillID = GetModdedDiagID();

		m_BRDiagPartyMenuID = GetModdedDiagID();
		m_BRDiagPartySizeID = GetModdedDiagID();
		m_BRDiagPartyApplyID = GetModdedDiagID();
		m_BRDiagPartyOnlineCountID = GetModdedDiagID();
		m_BRDiagPartyOnlineApplyID = GetModdedDiagID();
		m_BRDiagPartyInviteMeID = GetModdedDiagID();
		m_BRDiagPartyOfflineID = GetModdedDiagID();
		m_BRDiagPartyPingID = GetModdedDiagID();
		m_BRDiagPartyClearID = GetModdedDiagID();

		m_BRDiagZoneMenuID = GetModdedDiagID();
		m_BRDiagZonesFakeID = GetModdedDiagID();
		m_BRDiagZoneRadiusID = GetModdedDiagID();
		m_BRDiagZoneNextRadiusID = GetModdedDiagID();
		m_BRDiagLogZoneTableID = GetModdedDiagID();
		m_BRDiagClearMarkersID = GetModdedDiagID();

		m_BRDiagSpectateMenuID = GetModdedDiagID();
		m_BRDiagSpectateEnabledID = GetModdedDiagID();
		m_BRDiagAdminSpectateID = GetModdedDiagID();
		m_BRDiagKillSelfID = GetModdedDiagID();
		m_BRDiagLogSpectatorsID = GetModdedDiagID();
		m_BRDiagTpTargetDistID = GetModdedDiagID();
		m_BRDiagTpTargetGoID = GetModdedDiagID();
		m_BRDiagTpCorpseID = GetModdedDiagID();
		m_BRDiagSpectateTraceID = GetModdedDiagID();

		m_BRDiagTraceMenuID = GetModdedDiagID();
		m_BRDiagTraceTpClientID = GetModdedDiagID();
		m_BRDiagTraceTpServerID = GetModdedDiagID();
		m_BRDiagTraceTicksID = GetModdedDiagID();

		m_BRDiagLogMenuID = GetModdedDiagID();
		m_BRDiagLogLevelID = GetModdedDiagID();
		m_BRDiagLogLevelSrvID = GetModdedDiagID();
		m_BRDiagChatMirrorID = GetModdedDiagID();
	}

	override protected void RegisterModdedDiags()
	{
		//--- MANDATORY and first: vanilla registers the "Script - Modded" root here, and unregisters
		//--- it again if no mod moved m_ModdedDiagID. Skip this and the whole branch disappears.
		super.RegisterModdedDiags();

		//--- The plugin is recreated on every world change, so this is also the per-session reset.
		BattleRoyaleDiag.Reset();

		DiagMenu.RegisterMenu(m_BRDiagRootMenuID, "BattleRoyale - Myst", GetModdedRootMenu());
		{
			DiagMenu.RegisterBool(m_BRDiagDummyID, "", "Dummy Option", m_BRDiagRootMenuID);

			//--- Match Flow. Server-side; every entry is refused unless you are in admins_steamid64,
			//--- and none of them exist at all in an offline session.
			DiagMenu.RegisterMenu(m_BRDiagFlowMenuID, "Match Flow", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterBool(m_BRDiagSkipStateID, "", "Skip State", m_BRDiagFlowMenuID);
				DiagMenu.RegisterBool(m_BRDiagPauseStateID, "", "Pause State", m_BRDiagFlowMenuID);
				//--- Target and trigger are two entries on purpose: a range callback fires while the
				//--- value is being scrubbed, which would send one RPC per step on the way to 10.
				DiagMenu.RegisterRange(m_BRDiagGotoStateID, "", "Jump To State", m_BRDiagFlowMenuID, "0, 16, 0, 1");
				DiagMenu.RegisterBool(m_BRDiagGotoGoID, "", "Jump: Go", m_BRDiagFlowMenuID);
				DiagMenu.RegisterBool(m_BRDiagForceReadyID, "", "Force Ready All", m_BRDiagFlowMenuID);
				DiagMenu.RegisterBool(m_BRDiagLogStateID, "", "Log State", m_BRDiagFlowMenuID);
			}

			//--- Spawn / Teleport. All four go through BR_SYNC_JUNCTURE_TELEPORT, never a raw
			//--- SetPosition, so they exercise the path under test rather than a parallel one.
			DiagMenu.RegisterMenu(m_BRDiagTeleportMenuID, "Spawn / Teleport", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterBool(m_BRDiagTpZoneID, "", "TP: Zone Centre", m_BRDiagTeleportMenuID);
				DiagMenu.RegisterBool(m_BRDiagTpNextZoneID, "", "TP: Next Zone", m_BRDiagTeleportMenuID);
				DiagMenu.RegisterBool(m_BRDiagTpLobbyID, "", "TP: Lobby", m_BRDiagTeleportMenuID);
				//--- Ignores the 30 s cooldown, which is what makes the ladder repro iterable.
				DiagMenu.RegisterBool(m_BRDiagForceUnstuckID, "", "Force Unstuck", m_BRDiagTeleportMenuID);
			}

			//--- HUD & Menus. Client only - works with no server at all.
			DiagMenu.RegisterMenu(m_BRDiagHudMenuID, "HUD & Menus", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterBool(m_BRDiagHudForceID, "", "Force HUD", m_BRDiagHudMenuID);
				DiagMenu.RegisterRange(m_BRDiagHudPlayersID, "", "Fake Players", m_BRDiagHudMenuID, "0, 100, 60, 1");
				//--- Starts at -2, not 0: the group figure doubles as a two-value enum, and the two
				//--- sentinel renders are the only part of this counter that has ever been wrong.
				//--- -2 (BR_HUD_GROUPS_NONE) hides the group panel, -1 (BR_HUD_GROUPS_CONCEALED)
				//--- shows "???" for the endgame. A 0-40 range could reach neither, so both had to be
				//--- provoked with a real ten-player endgame and a settings edit.
				DiagMenu.RegisterRange(m_BRDiagHudGroupsID, "", "Fake Groups", m_BRDiagHudMenuID, "-2, 40, 20, 1");
				DiagMenu.RegisterRange(m_BRDiagHudKillsID, "", "Fake Kills", m_BRDiagHudMenuID, "0, 20, 3, 1");
				DiagMenu.RegisterRange(m_BRDiagHudCountdownID, "", "Fake Countdown", m_BRDiagHudMenuID, "0, 300, 60, 1");
				DiagMenu.RegisterBool(m_BRDiagOpenSpawnMenuID, "", "Open Spawn Menu", m_BRDiagHudMenuID);
				DiagMenu.RegisterBool(m_BRDiagOpenLeaderboardID, "", "Open Leaderboard", m_BRDiagHudMenuID);
				DiagMenu.RegisterBool(m_BRDiagFakeLeaderboardID, "", "Fake Leaderboard", m_BRDiagHudMenuID);
				//--- The client half of the match summary, testable with no server at all: SERVER is
				//--- undefined offline, so nothing that PRODUCES this data is even compiled in.
				DiagMenu.RegisterBool(m_BRDiagFakeLastMatchID, "", "Fake Last Match", m_BRDiagHudMenuID);
				//--- Order matches BattleRoyaleKillCause exactly. NONE is the winner's card and
				//--- UNKNOWN the degraded disconnect path; both have their own render branch, so both
				//--- are pickable rather than only reachable by dying the right way on a live server.
				DiagMenu.RegisterItem(m_BRDiagLastMatchCauseID, "", "Recap Cause", m_BRDiagHudMenuID, "Unknown,Won,Firearm,Melee,Barehands,Explosive,Zone,Infected,Animal,Environment");
				DiagMenu.SetValue(m_BRDiagLastMatchCauseID, BattleRoyaleKillCause.FIREARM);
				DiagMenu.RegisterBool(m_BRDiagLastMatchNotPlayedID, "", "Did Not Play", m_BRDiagHudMenuID);
				DiagMenu.RegisterBool(m_BRDiagOpenDeathScreenID, "", "Open Death Screen", m_BRDiagHudMenuID);
				DiagMenu.RegisterBool(m_BRDiagFadeID, "", "Fade", m_BRDiagHudMenuID);
			}

#ifdef KILLFEED
			//--- Kill Feed. A kill needs two players, so this is the submenu that replaces a whole
			//--- second client. Rows go in through the network queue, so what renders them is the
			//--- production path - preview entity, attachments and all.
			DiagMenu.RegisterMenu(m_BRDiagKillFeedMenuID, "Kill Feed", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterBool(m_BRDiagKfPushID, "", "Push Fake Kill", m_BRDiagKillFeedMenuID);
				DiagMenu.RegisterItem(m_BRDiagKfCauseID, "", "Kill Cause", m_BRDiagKillFeedMenuID, "Weapon,Melee,Barehands,Explosive,Zone,Infected,Animal,Environment");
				DiagMenu.RegisterBool(m_BRDiagKfWeaponID, "", "With Weapon", m_BRDiagKillFeedMenuID);
				DiagMenu.SetValue(m_BRDiagKfWeaponID, true);
				//--- Overflows the row pool deliberately: eviction and the matching Release() are
				//--- otherwise unreachable without four real deaths in a row.
				DiagMenu.RegisterBool(m_BRDiagKfFillID, "", "Fill Feed", m_BRDiagKillFeedMenuID);
			}
#endif

#ifdef VIGRID_PARTY
			//--- Party. Normally three clients: two partied plus one solo, because a round never
			//--- advances while everyone is in one group.
			//---
			//--- Two independent fabrications, and both are needed to reach the whole party menu.
			//--- "Apply Fake Party" builds the ROSTER - the menu's right column, plus the HUD panel,
			//--- the world nametags and the map's team layer. "Apply Fake Online" builds the ONLINE
			//--- LIST - the left column, which is what every outgoing action needs a target from and
			//--- which is empty on a one-client session because it comes from a real VP_PlayerList.
			//---
			//--- With either applied the client is LATCHED: every server push that would overwrite
			//--- the fabrication is discarded, and the menu's buttons act on it locally instead of
			//--- going on the wire. So Invite / Kick / Promote / Leave / Disband / Accept / Decline
			//--- all work solo. "Clear Fakes" is what hands the session back to the real server, and
			//--- it must be pressed - a latch left down looks exactly like a broken connection.
			DiagMenu.RegisterMenu(m_BRDiagPartyMenuID, "Party", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterRange(m_BRDiagPartySizeID, "", "Fake Party Size", m_BRDiagPartyMenuID, "1, 6, 3, 1");
				DiagMenu.RegisterBool(m_BRDiagPartyApplyID, "", "Apply Fake Party", m_BRDiagPartyMenuID);
				//--- Ranges up to a realistically full server. Two reasons for the headroom: past the
				//--- default max_party_size of 4, inviting is the only way to see the Invite buttons
				//--- disappear on a full party; and the column fits about 8 rows, so the default of
				//--- 20 overflows it immediately - which is what makes scrolling testable at all
				//--- without touching the slider first.
				DiagMenu.RegisterRange(m_BRDiagPartyOnlineCountID, "", "Fake Online Players", m_BRDiagPartyMenuID, "0, 60, 20, 1");
				DiagMenu.RegisterBool(m_BRDiagPartyOnlineApplyID, "", "Apply Fake Online", m_BRDiagPartyMenuID);
				DiagMenu.RegisterBool(m_BRDiagPartyInviteMeID, "", "Fake Incoming Invite", m_BRDiagPartyMenuID);
				DiagMenu.RegisterBool(m_BRDiagPartyOfflineID, "", "Toggle Member Offline", m_BRDiagPartyMenuID);
				DiagMenu.RegisterBool(m_BRDiagPartyPingID, "", "Add Fake Ping", m_BRDiagPartyMenuID);
				DiagMenu.RegisterBool(m_BRDiagPartyClearID, "", "Clear Fakes", m_BRDiagPartyMenuID);
			}
#endif

			//--- Map & Zones.
			DiagMenu.RegisterMenu(m_BRDiagZoneMenuID, "Map & Zones", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterBool(m_BRDiagZonesFakeID, "", "Fake Zones", m_BRDiagZoneMenuID);
				DiagMenu.RegisterRange(m_BRDiagZoneRadiusID, "", "Zone Radius", m_BRDiagZoneMenuID, "50, 5000, 1500, 50");
				DiagMenu.RegisterRange(m_BRDiagZoneNextRadiusID, "", "Next Radius", m_BRDiagZoneMenuID, "25, 2500, 600, 25");
				//--- Generation runs smallest-first, so m_PlayAreas[0] is the FINAL circle. One dump
				//--- settles that argument permanently, which is why it earns an id.
				DiagMenu.RegisterBool(m_BRDiagLogZoneTableID, "", "Log Zone Table", m_BRDiagZoneMenuID);
#ifdef VIGRID_MAP
				DiagMenu.RegisterBool(m_BRDiagClearMarkersID, "", "Clear Map Markers", m_BRDiagZoneMenuID);
#endif
			}

			//--- Teleport Trace. The measurement CLAUDE.md asks for before any more code goes near
			//--- the ladder / F2-unstuck bug: command id and ladder-command state, both sides, at
			//--- juncture receipt and for a few ticks after.
			//--- Spectate. Server-side, and the whole point of it is Kill Me: without that, reaching
			//--- the death screen at all needs a SECOND client to land a kill, so verifying the
			//--- feature cost a three-client session every time.
			DiagMenu.RegisterMenu(m_BRDiagSpectateMenuID, "Spectate", m_BRDiagRootMenuID);
			{
				//--- spectate_enabled ships OFF and lives in the PROFILE general_settings.json, so
				//--- without this entry turning it on means editing a file and restarting the server.
				//--- The flip is in memory only and is not persisted.
				DiagMenu.RegisterBool(m_BRDiagSpectateEnabledID, "", "Spectate Enabled", m_BRDiagSpectateMenuID);
				//--- admin_spectate_enabled ships ON, so this is here to turn it OFF and confirm the
				//--- kill switch actually refuses - the negative test is the one worth having, since
				//--- the positive one happens every time F3 is pressed. In memory only, not persisted.
				//--- Note this does NOT grant admin: the sender must still be in admins_steamid64.
				DiagMenu.RegisterBool(m_BRDiagAdminSpectateID, "", "Admin Spectate Enabled", m_BRDiagSpectateMenuID);
				DiagMenu.RegisterBool(m_BRDiagKillSelfID, "", "Kill Me", m_BRDiagSpectateMenuID);
				//--- Reports the resolved chain tier per spectator, which is the one thing about the
				//--- five-tier target search that is otherwise only inferable from who you end up
				//--- watching.
				DiagMenu.RegisterBool(m_BRDiagLogSpectatorsID, "", "Log Spectators", m_BRDiagSpectateMenuID);

				//--- The range test. Walking a target past 1 km takes minutes and usually ends
				//--- early - one measured run stopped at 929 m because a wolf killed the target -
				//--- so this puts them at an exact radius from the spectator's corpse in one press.
				//--- Target and trigger are two entries for the same reason Jump To State is.
				DiagMenu.RegisterRange(m_BRDiagTpTargetDistID, "", "TP Target: metres", m_BRDiagSpectateMenuID, "100, 3000, 1200, 50");
				DiagMenu.RegisterBool(m_BRDiagTpTargetGoID, "", "TP Target: Go", m_BRDiagSpectateMenuID);
				//--- The bubble probe. Press it while the target is out past ~1 km and entity=0: if
				//--- entity comes back, the bubble really is on the corpse and a fix is worth
				//--- designing. It drags the victim's gear along, so it is a MEASUREMENT, not a fix.
				DiagMenu.RegisterBool(m_BRDiagTpCorpseID, "", "TP Corpse to Target", m_BRDiagSpectateMenuID);
				//--- Turn this down BEFORE pressing Go: a teleport always produces a transient
				//--- entity=0, so the answer is whether it returns to 1 and stays, and 5 s sampling
				//--- cannot tell those apart.
				DiagMenu.RegisterRange(m_BRDiagSpectateTraceID, "", "Trace Interval (s)", m_BRDiagSpectateMenuID, "0.5, 10, 5, 0.5");
			}

			DiagMenu.RegisterMenu(m_BRDiagTraceMenuID, "Teleport Trace", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterBool(m_BRDiagTraceTpClientID, "", "Trace TP (Client)", m_BRDiagTraceMenuID);
				DiagMenu.RegisterBool(m_BRDiagTraceTpServerID, "", "Trace TP (Server)", m_BRDiagTraceMenuID);
				DiagMenu.RegisterRange(m_BRDiagTraceTicksID, "", "Trace Ticks", m_BRDiagTraceMenuID, "0, 60, 20, 1");
			}

			//--- Logging. Index 0 is "Default", i.e. resolve from the -br-* flags and serverDZ.cfg
			//--- as usual; that is also the initial value, so no override is in force until touched.
			DiagMenu.RegisterMenu(m_BRDiagLogMenuID, "Logging", m_BRDiagRootMenuID);
			{
				DiagMenu.RegisterItem(m_BRDiagLogLevelID, "", "Log Level", m_BRDiagLogMenuID, "Default,Error,Warn,Info,Debug,Trace");
				DiagMenu.RegisterItem(m_BRDiagLogLevelSrvID, "", "Log Level (Srv)", m_BRDiagLogMenuID, "Default,Error,Warn,Info,Debug,Trace");
				DiagMenu.RegisterBool(m_BRDiagChatMirrorID, "", "Chat Mirror", m_BRDiagLogMenuID);
				DiagMenu.SetValue(m_BRDiagChatMirrorID, true);
			}
		}

		BRDiagVerifyRegistration();
	}

	/**
	 *  Confirm the tree actually landed, and say what it cost.
	 *
	 *  This is the id-budget canary. Script diag ids are capped at 512 across every mod on the
	 *  machine, and the behaviour at the cap is not documented anywhere - a silently dropped tail is
	 *  a plausible failure mode, and it would look exactly like "the last submenu does nothing".
	 *  Checking the LAST id allocated is what catches that; the first would still be fine.
	 *
	 *  IsRegistered is used because it is the only reliable signal here. BindCallback also returns a
	 *  bool, but it is NOT a success flag: measured 2026-08-09, it returns false for a valid, freshly
	 *  registered id with a documented-valid callback signature, twice in a row on the same id. That
	 *  is why vanilla discards it at all ~100 of its own call sites. Do not re-add a check on it -
	 *  it produces one false alarm per entry and hides nothing.
	 */
	protected void BRDiagVerifyRegistration()
	{
		if ( !DiagMenu.IsRegistered(m_BRDiagChatMirrorID) )
		{
			BattleRoyaleUtils.Warn("Diag: entry " + m_BRDiagChatMirrorID + " did not register - the 512 script diag id cap is the first thing to suspect");
			return;
		}

		BattleRoyaleUtils.Info("Diag: menu registered, ids " + m_BRDiagRootMenuID + "-" + m_BRDiagChatMirrorID + " (" + (m_BRDiagChatMirrorID - m_BRDiagRootMenuID + 1) + " of the 512 shared script diag ids)");
	}
}
#endif // DIAG_DEVELOPER
#endif // SERVER
