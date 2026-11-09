#ifdef SERVER
/**
 *  In-place reset: start the next match in the same server process instead of restarting it.
 *  Off unless server_settings.json in_place_reset is true. The full walkthrough, settings and
 *  measurements are in Docs/in-place-reset.md.
 *
 *      9_BattleRoyaleRestart.Activate   CanRun()? -> Begin(), else today's 10 s RequestExit
 *      PH_EXPIRE    GetCEApi().TimeShift: every CE item expires, the drain starts
 *      PH_KICK      kick-all mode only: notice, then DisconnectPlayer for everyone
 *      PH_REBUILD   delete chickens, reset the match statics, replace BattleRoyaleServer (its
 *                   Init unlocks the server and opens the lobby), then in the SAME frame give
 *                   every still-connected player a fresh lobby body and delete every corpse
 *      lobby        the refill waits for the drain (in_place_reset_drain_seconds), or at the
 *                   latest for the start countdown, then runs in steps one second apart
 *
 *  STATIC, and driven from MissionServer.OnUpdate rather than from a state, because the whole
 *  state machine is replaced halfway through. The counters and latches are process statics, so
 *  they survive that replacement.
 *
 *  Anything going wrong falls back to what the server did before: a process restart.
 */

const int BR_INPLACE_PH_IDLE = 0;
const int BR_INPLACE_PH_EXPIRE = 1;
const int BR_INPLACE_PH_KICK = 2;
const int BR_INPLACE_PH_REBUILD = 3;

//! Seconds between the kick-all notice and the disconnects, and between the disconnects and the
//! rebuild (so the engine has let go of the kicked players' bodies before the corpse sweep).
const int BR_INPLACE_KICK_NOTICE_MS = 5000;
const int BR_INPLACE_KICK_SETTLE_MS = 2000;

//! Seconds between two refill steps. Each step freezes the server for 1-3 s (measured), so they
//! are spread out rather than run back to back.
const int BR_INPLACE_REFILL_STEP_MS = 1000;

//! Seconds between a failed refill's notice and the fallback restart.
const int BR_INPLACE_FAIL_EXIT_MS = 10000;

class BattleRoyaleInPlaceReset
{
	protected static int s_Phase;
	protected static int s_PhaseDueMs;

	//! In-place matches started in this process, for in_place_reset_max_matches.
	protected static int s_ResetsDone;

	//! Latched on any failure: every later match end restarts the process instead.
	protected static bool s_Failed;

	//! GetTime() at which a failed reset restarts the process, 0 when none is scheduled.
	protected static int s_ExitAtMs;

	//! Loot refill bookkeeping. s_RefillStep is the next step to run (0..3), -1 when nothing is
	//! pending. s_RefillDone means the steps ran this match, so the countdown only tops up.
	protected static int s_ShiftAtMs;
	protected static int s_RefillStep = -1;
	protected static int s_RefillNextMs;
	protected static int s_RefillTotal;
	protected static bool s_RefillForced;
	protected static bool s_RefillDone;

	//------------------------------------------------------------------------------------------
	//--- Decisions
	//------------------------------------------------------------------------------------------

	/**
	 *  Why the next match end must restart the process, or "" when an in-place reset can run.
	 *  Each reason is a plain sentence for the log.
	 */
	static string CannotRunReason(bool check_ce = true)
	{
		BattleRoyaleServerData settings = BattleRoyaleConfig.GetConfig().GetServerData();

		if ( !settings.in_place_reset )
			return "in_place_reset is off";

		if ( !BattleRoyaleNativeRefill.IsAvailable() )
			return "the dzhooks ce_refill module is not loaded (DZHOOK_CE_REFILL)";

		//--- AutoLockWebhook has no unlock call, so the lobby could never reopen.
		if ( settings.use_autolock )
			return "use_autolock is on and has no unlock";

		//--- Skipped by LogMode: the first BattleRoyaleServer is built in MissionServer.OnInit, before
		//--- the mission's Central Economy exists, so GetCEApi() is still NULL at boot (measured).
		if ( check_ce && !GetCEApi() )
			return "this mission has no Central Economy";

		if ( s_Failed )
			return "an earlier in-place reset failed in this process";

		if ( settings.in_place_reset_max_matches > 0 && s_ResetsDone >= settings.in_place_reset_max_matches )
			return "in_place_reset_max_matches (" + settings.in_place_reset_max_matches + ") reached";

		return "";
	}

	static bool CanRun()
	{
		return CannotRunReason() == "";
	}

	//! True when the players left at match end will be kept for the next match (not kicked).
	static bool KeepsPlayersConnected()
	{
		if ( !CanRun() )
			return false;

		return !BattleRoyaleConfig.GetConfig().GetServerData().in_place_reset_kick_all;
	}

	//! True when a dead player should wait for the next match instead of leaving the server.
	static bool KeepsDeadPlayers()
	{
		if ( !KeepsPlayersConnected() )
			return false;

		return BattleRoyaleConfig.GetConfig().GetServerData().in_place_reset_keep_dead_players;
	}

	//! One line at boot, so the log says up front whether matches will reset in place and why not.
	static void LogMode()
	{
		string reason = CannotRunReason( false );

		if ( reason == "" )
		{
			Print("[DayZ-BattleRoyale][InPlace] enabled: the next match starts in this process");
			return;
		}

		//--- Silent for the default configuration, so a server that never asked for it logs nothing.
		if ( !BattleRoyaleConfig.GetConfig().GetServerData().in_place_reset )
			return;

		Print("[DayZ-BattleRoyale][InPlace] requested but OFF: " + reason + ". The server restarts between matches.");
	}

	//------------------------------------------------------------------------------------------
	//--- Match end
	//------------------------------------------------------------------------------------------

	/**
	 *  Called by 9_BattleRoyaleRestart.Activate after its flushes. Returns false when the caller
	 *  must restart the process as before. Never does the work itself: this runs inside the old
	 *  BattleRoyaleServer's own Update, which must not replace itself.
	 */
	static bool Begin()
	{
		string reason = CannotRunReason();
		if ( reason != "" )
		{
			Print("[DayZ-BattleRoyale][InPlace] restarting the process: " + reason);
			return false;
		}

		if ( s_Phase != BR_INPLACE_PH_IDLE )
			return true;

		s_Phase = BR_INPLACE_PH_EXPIRE;
		s_PhaseDueMs = GetGame().GetTime();
		Print("[DayZ-BattleRoyale][InPlace] match over, resetting in place (match " + (s_ResetsDone + 2) + " of this process)");
		return true;
	}

	/**
	 *  Driven from MissionServer.OnUpdate every frame, after the match server's own Update. Costs
	 *  two integer tests while nothing is pending.
	 */
	static void Tick(MissionServer mission)
	{
		int now = GetGame().GetTime();

		if ( s_ExitAtMs > 0 && now >= s_ExitAtMs )
		{
			s_ExitAtMs = 0;
			GetGame().RequestExit( 0 );
			return;
		}

		if ( s_Phase != BR_INPLACE_PH_IDLE && now >= s_PhaseDueMs )
			RunPhase( mission, now );

		if ( s_RefillStep >= 0 )
			TickRefill( now );
	}

	protected static void RunPhase(MissionServer mission, int now)
	{
		BattleRoyaleServerData settings = BattleRoyaleConfig.GetConfig().GetServerData();

		if ( s_Phase == BR_INPLACE_PH_EXPIRE )
		{
			ExpireLoot( now );

			if ( settings.in_place_reset_kick_all )
			{
				NotifyAll( "STR_BR_INPLACE_KICK" );
				s_Phase = BR_INPLACE_PH_KICK;
				s_PhaseDueMs = now + BR_INPLACE_KICK_NOTICE_MS;
				return;
			}

			NotifyAll( "STR_BR_INPLACE_NEXT_MATCH" );
			s_Phase = BR_INPLACE_PH_REBUILD;
			s_PhaseDueMs = now;
			return;
		}

		if ( s_Phase == BR_INPLACE_PH_KICK )
		{
			int kicked = DisconnectAll();
			Print("[DayZ-BattleRoyale][InPlace] kick-all: disconnected " + kicked + " player(s)");
			s_Phase = BR_INPLACE_PH_REBUILD;
			s_PhaseDueMs = now + BR_INPLACE_KICK_SETTLE_MS;
			return;
		}

		if ( s_Phase == BR_INPLACE_PH_REBUILD )
		{
			s_Phase = BR_INPLACE_PH_IDLE;
			Rebuild( mission, !settings.in_place_reset_kick_all, settings.in_place_reset_keep_dead_players );
		}
	}

	//------------------------------------------------------------------------------------------
	//--- Steps
	//------------------------------------------------------------------------------------------

	protected static void ExpireLoot(int now)
	{
		CEApi ce = GetCEApi();
		if ( ce )
			ce.TimeShift( 1000000000 );

		s_ShiftAtMs = now;
		s_RefillStep = 0;
		s_RefillNextMs = 0;
		s_RefillTotal = 0;
		s_RefillForced = false;
		s_RefillDone = false;

		Print("[DayZ-BattleRoyale][InPlace] loot expired (TimeShift), draining while players wait in the lobby");
	}

	protected static int DisconnectAll()
	{
		array<PlayerIdentity> identities = new array<PlayerIdentity>();
		GetGame().GetPlayerIndentities( identities );

		int count = 0;
		for ( int i = 0; i < identities.Count(); i++ )
		{
			PlayerIdentity identity = identities.Get( i );
			if ( !identity )
				continue;

			GetGame().DisconnectPlayer( identity );
			count++;
		}

		return count;
	}

	/**
	 *  Replace the match server and put everyone who stays into the new lobby, all in one frame.
	 *
	 *  One frame matters: between the new server existing and the old bodies being replaced, an
	 *  old LIVING body holds no state in the new machine, and OnPlayerTick would schedule a
	 *  late-join kick for it - which then arms on that player's next PlayerLoadedIn.
	 */
	protected static void Rebuild(MissionServer mission, bool keep_players, bool keep_dead)
	{
		BattleRoyaleServer old_server = BattleRoyaleServer.Cast( GetBR() );

		//--- 1. Birds. The lobby's are owned by the old lobby state, which is about to go.
		int chickens = BattleRoyaleWin.DeleteWinChickens();
		if ( old_server )
		{
			BattleRoyaleDebug old_lobby = BattleRoyaleDebug.Cast( old_server.GetState( 0 ) );
			if ( old_lobby )
				chickens += old_lobby.DeleteLobbyChickens();

			old_server.CancelQueuedCalls();
		}

		//--- 2. Match-wide statics a process restart used to clear.
		BattleRoyaleState.ResetForNewMatch();
		BattleRoyaleZone.ResetForNewMatch();
		BattleRoyaleSpectators.GetInstance().ResetForNewMatch();
		BattleRoyaleMatchStats.ResetForNewMatch();
		BattleRoyaleVoice.ClearAll();
#ifdef VIGRID_PARTY
		VigridPartyAPI.ResetForNewMatch();
#endif
#ifdef VIGRID_AUTORUN
		//--- A match that ended before HandleUnlock never re-allowed auto-run.
		VigridAutoRunAPI.SetAllowedForAll( true );
#endif
#ifdef JM_COT
		BRMasterControlsModule.ResetPartyIndex();
#endif

		s_ResetsDone++;

		//--- 3. The new server. Its Init does everything a boot does: a new match id, a new circle
		//--- chain, new weather, the unlock, and the lobby.
		BattleRoyaleServer fresh = mission.BR_ReplaceServer();

		//--- 4. Everyone still connected.
		int respawned = 0;
		int dropped = 0;

		array<PlayerIdentity> identities = new array<PlayerIdentity>();
		GetGame().GetPlayerIndentities( identities );

		for ( int i = 0; i < identities.Count(); i++ )
		{
			PlayerIdentity identity = identities.Get( i );
			if ( !identity )
				continue;

			PlayerBase old_body = FindBody( identity );
			bool was_alive = old_body && old_body.IsAlive();

			if ( !keep_players || ( !was_alive && !keep_dead ) )
			{
				GetGame().DisconnectPlayer( identity );
				dropped++;
				continue;
			}

			string character = "";
			if ( old_body )
				character = old_body.GetType();

			GetGame().MuteAllPlayers( identity.GetPlainId(), false );

			PlayerBase body = mission.BR_SpawnLobbyBody( identity, character );
			if ( !body )
			{
				Print("[DayZ-BattleRoyale][InPlace] could not create a lobby body for " + identity.GetPlainId() + ", disconnecting them");
				GetGame().DisconnectPlayer( identity );
				dropped++;
				continue;
			}

			//--- AFTER the body, carrying its network id, so the client can tell which body is the new
			//--- one whatever order the RPC and the entity arrive in. Sent before the body, the RPC was
			//--- measured landing 1.3 s AFTER the body had already been selected on the client, which
			//--- then took the new body for the old one and waited for ever.
			int body_low;
			int body_high;
			body.GetNetworkID( body_low, body_high );
			Param3<bool, int, int> reset_payload = new Param3<bool, int, int>( keep_dead, body_low, body_high );
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "MatchReset", reset_payload, true, identity );

			if ( old_body && old_body != body )
				GetGame().ObjectDelete( old_body );

			respawned++;
		}

		//--- 5. Every other corpse from the match, with its gear.
		int corpses = BattleRoyaleCorpses.DeleteAll();

		//--- Two Print calls with literal heads: Print( variable ) prints "string line = '...'".
		Print( "[DayZ-BattleRoyale][InPlace] lobby reopened: " + respawned + " player(s) kept, " + dropped + " disconnected" );
		Print( "[DayZ-BattleRoyale][InPlace] deleted " + corpses + " corpse(s) and " + chickens + " chicken(s)" );
	}

	//! The body the engine ties to this identity, or the corpse a spectator left behind.
	protected static PlayerBase FindBody(PlayerIdentity identity)
	{
		PlayerBase body = PlayerBase.Cast( identity.GetPlayer() );
		if ( body )
			return body;

		string uid = identity.GetPlainId();
		array<Man> players = new array<Man>();
		GetGame().GetPlayers( players );

		for ( int i = 0; i < players.Count(); i++ )
		{
			PlayerBase candidate = PlayerBase.Cast( players.Get( i ) );
			if ( candidate && candidate.player_steamid == uid )
				return candidate;
		}

		return NULL;
	}

	//------------------------------------------------------------------------------------------
	//--- Loot refill
	//------------------------------------------------------------------------------------------

	/**
	 *  2_BattleRoyaleCountReached.Activate: the match is about to start. After an in-place reset,
	 *  run the four refill steps (again), one per second, without waiting for the drain.
	 *
	 *  AGAIN, even when the lobby already ran them: the refill counts what is on the map, and the
	 *  expired items the drain had not deleted yet still counted. Once they go, their share is
	 *  missing (measured: 108 old items left at the 100 % step, 28 one batch later, and the next call
	 *  put back 845 items map-wide). A single CE_RefillLoot() here could therefore have to place most
	 *  of the map in one frame; the steps keep each frame to a quarter. On a full map a step costs
	 *  ~10 ms, so the repeat is free when it is not needed.
	 *
	 *  Nothing after a process restart: the boot pass filled the map, and s_RefillDone is false.
	 */
	static void OnCountdownStarted()
	{
		if ( s_RefillStep < 0 && !s_RefillDone )
			return;

		if ( s_RefillStep < 0 )
		{
			s_RefillStep = 0;
			s_RefillNextMs = 0;
			s_RefillTotal = 0;
			Print("[DayZ-BattleRoyale][InPlace] countdown started: topping the map up again in steps");
		}
		else
		{
			Print("[DayZ-BattleRoyale][InPlace] countdown started before the drain finished, refilling now");
		}

		s_RefillForced = true;
		s_RefillDone = false;
	}

	//! 4_BattleRoyalePrepare.Activate: last chance before players leave the lobby. Runs whatever
	//! refill steps are left in one go.
	static void OnPrepareStarted()
	{
		int guard = 0;
		while ( s_RefillStep >= 0 && guard < 8 )
		{
			RunRefillStep();
			guard++;
		}
	}

	protected static void TickRefill(int now)
	{
		if ( now < s_RefillNextMs )
			return;

		//--- Only while everyone is in the lobby. A refill near a player places nothing within
		//--- LootSpawnAvoidance of them, and refilling mid-match would put loot back in looted houses.
		BattleRoyaleServer server = BattleRoyaleServer.Cast( GetBR() );
		if ( !server )
			return;

		BattleRoyaleDebugState lobby = BattleRoyaleDebugState.Cast( server.GetCurrentState() );
		if ( !lobby )
			return;

		int drain_ms = BattleRoyaleConfig.GetConfig().GetServerData().in_place_reset_drain_seconds * 1000;
		if ( !s_RefillForced && now < s_ShiftAtMs + drain_ms )
			return;

		//--- Announced only for the lobby's pass. The countdown's repeat is a top-up that costs
		//--- ~10 ms a step on a map the first pass already filled.
		if ( s_RefillStep == 0 && !s_RefillForced )
			NotifyAll( "STR_BR_INPLACE_REFILL" );

		RunRefillStep();
		s_RefillNextMs = now + BR_INPLACE_REFILL_STEP_MS;
	}

	protected static void RunRefillStep()
	{
		int step = s_RefillStep;
		int spawned;
		string call;

		if ( step < 3 )
		{
			int percent = 25 * ( step + 1 );
			call = "CE_RefillLootTo(" + percent + ")";
			spawned = BattleRoyaleNativeRefill.RefillTo( percent );
		}
		else
		{
			call = "CE_RefillLoot()";
			spawned = BattleRoyaleNativeRefill.Refill();
		}

		Print("[DayZ-BattleRoyale][InPlace] refill " + call + ": " + spawned + " item(s)");

		if ( spawned < 0 )
		{
			Fail( "the refill did not run (" + call + " returned -1, see dzhooks_win.log)" );
			return;
		}

		s_RefillTotal += spawned;

		if ( step >= 3 )
		{
			s_RefillStep = -1;

			//--- Done for good once the countdown's pass (s_RefillForced) has run; after the lobby's
			//--- early pass the countdown still runs one more (see OnCountdownStarted).
			s_RefillDone = !s_RefillForced;
			s_RefillForced = false;

			Print("[DayZ-BattleRoyale][InPlace] refill done: " + s_RefillTotal + " item(s) in total");
			BattleRoyaleCorpses.DeleteAll();
			return;
		}

		s_RefillStep = step + 1;
	}

	/**
	 *  Latch the failure and restart the process. Players are in the lobby on a map with no fresh
	 *  loot, so a match must not start: the restart is today's behaviour, just earlier.
	 */
	protected static void Fail(string reason)
	{
		s_Failed = true;
		s_RefillStep = -1;
		Print("[DayZ-BattleRoyale][InPlace] FAILED: " + reason + ". Restarting the process.");
		NotifyAll( "STR_BR_INPLACE_FAILED" );
		s_ExitAtMs = GetGame().GetTime() + BR_INPLACE_FAIL_EXIT_MS;
	}

	//! Same RPC as BattleRoyaleState.MessagePlayersUntranslated, a broadcast to every client
	//! (spectators included), sent directly so it works mid-rebuild when no state is current.
	protected static void NotifyAll(string key)
	{
		Param7<string, float, string, string, string, string, string> payload = new Param7<string, float, string, string, string, string, string>( key, DAYZBR_MSG_TIME, "", "", "", "", "" );
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", payload, true );
	}
}
#endif
