#ifdef SERVER
class BattleRoyaleDebug: BattleRoyaleDebugState
{
    protected ref array<PlayerBase> m_ReadyList;

    protected int i_MinPlayers;
    protected int i_TimeBetweenMessages;
    protected bool b_UseVoteSystem;
    protected float f_VoteThreshold;
    protected float f_MinWaitingTime;
    protected float f_MinWaitingTimeNotFull;
    protected float f_AutoStartDelay;
    protected bool b_AutoStartGame;
    protected int i_FirstPlayerTick;
    protected int i_MinPartySize;
    protected int i_MinPartyRemainder;
    protected int i_AutoGroupSelfTest;

    //--- GetGame().GetTime() at which the load gate first refused a start that was otherwise ready,
    //--- or 0 when no hold is running. Owned by IsLoadGateClear / ResetLoadHold and read by
    //--- MessageWaiting, which uses "is a hold running" as its "is loading what is holding us up".
    protected int i_LoadHoldStartedMs;

    void BattleRoyaleDebug()
    {
        m_ReadyList = new array<PlayerBase>();

        BattleRoyaleLobbyData m_LobbySettings = BattleRoyaleConfig.GetConfig().GetLobbyData();

		i_MinPlayers = m_LobbySettings.minimum_players;
		i_TimeBetweenMessages = 45;
		b_UseVoteSystem = (m_LobbySettings.use_ready_up == 1);
		f_VoteThreshold = m_LobbySettings.ready_up_percent;
		f_MinWaitingTime = m_LobbySettings.min_waiting_time;
		f_MinWaitingTimeNotFull = m_LobbySettings.min_waiting_time_not_full;
		b_AutoStartGame = m_LobbySettings.autostart_enabled;
		f_AutoStartDelay = m_LobbySettings.autostart_delay;
		i_MinPartySize = m_LobbySettings.min_party_size;
		i_MinPartyRemainder = m_LobbySettings.min_party_remainder;
		i_AutoGroupSelfTest = m_LobbySettings.auto_group_selftest;
    }

#ifdef VIGRID_PARTY
    //--- Diagnostic, off unless lobby_settings.json asks for it. Plans synthetic populations with
    //--- the settings this match will use and logs the sizes that come out; changes nothing.
    //--- It is here because the interesting cases are combinatorial and the local rig runs three
    //--- clients, which cannot produce a four-way split, a top-up that also leaves a remainder, or
    //--- the max_party_size overflow.
    protected void RunAutoGroupSelfTest()
    {
        if ( i_AutoGroupSelfTest <= 0 )
            return;
        if ( i_MinPartySize <= 1 )
            return;

        VigridPartyAPI.AutoGroupSelfTest( i_AutoGroupSelfTest, i_MinPartySize, BR_AUTO_GROUP_MIN_GROUPS, i_MinPartyRemainder );
    }
#endif

    override string GetName()
    {
        return "Debug Zone State";
    }

    //--- The lobby needs this at least as much as the warm-up does, and used not to have it. A
    //--- player wedged in the scenery here has no way out on their own and stays wedged for the
    //--- whole wait - and if they are still wedged when Prepare runs, the fall command that pinned
    //--- them has their inventory locked and they start the match with no loadout at all (see
    //--- BattleRoyalePrepare.ClearStuckMovementState). Observed 2026-08-08: Client_B was stuck for
    //--- ~80s, pressed F2 to no effect, and spawned naked.
    //---
    //--- BattleRoyaleDebugState.FindUnstuckPosition clamps the landing spot to the lobby disc, so
    //--- this cannot be used to get out of the lobby, and the cooldown in DeferredUnstuck stops it
    //--- being used as fast-travel within it.
    override bool AllowsUnstuck()
    {
        return true;
    }

    /**
     *  How many players in the lobby are still on their loading screen.
     *
     *  A player is in this state's roster from BattleRoyaleServer.OnPlayerConnected, which vanilla
     *  reaches from the ClientNew path - i.e. while their client is still loading the world. So they
     *  count towards minimum_players and towards the vote denominator well before they can see
     *  anything, and a match started inside that window takes them through spawn selection and the
     *  BattleRoyalePrepare teleport with nobody home. Measured 2026-08-10: ClientPrepare -> ClientNew
     *  alone took 20 s, and for a new character ClientReadyEventTypeID never fires at all.
     *
     *  A plain count, with no per-player deadline in it - the bound lives in IsLoadGateClear, and the
     *  comment on BR_LOBBY_LOAD_MAX_HOLD_SECONDS explains why it has to be there and not here.
     *  The answer is a count rather than a bool because MessageWaiting has to name a number.
     */
    protected int GetNotLoadedCount()
    {
        int not_loaded = 0;

        ref array<PlayerBase> players = GetPlayers();
        for(int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = players.Get(i);
            if(!player)
                continue;

            if(!player.br_loaded_in)
                not_loaded++;
        }

        return not_loaded;
    }

    /**
     *  Players in the lobby whose client has reported itself loaded in - i.e. who could actually
     *  play a match that started right now.
     *
     *  This, not GetPlayers().Count(), is what minimum_players is measured against: "ten players"
     *  should mean ten players who can play, not ten connections of which three are on a loading
     *  screen. Note it is deliberately NOT used for the vote PERCENTAGE denominator, for the HUD
     *  counter, for i_NumStartingPlayers or for the party group count - a roster that shrinks and
     *  grows as clients load would make the displayed count jitter and would let the zone sizing
     *  disagree with the roster that actually plays. The dilution it causes in IsVoteReady errs
     *  towards refusing to start, which is the safe direction.
     */
    protected int GetLoadedPlayerCount()
    {
        return GetPlayers().Count() - GetNotLoadedCount();
    }

    /**
     *  Is the lobby at the server's player cap?
     *
     *  Raw connection count, NOT GetLoadedPlayerCount, and the difference is deliberate: "full" here
     *  means "no further player can join", so waiting any longer cannot add anyone. Whether those
     *  players have finished loading is #8's question and is answered by the load gate below.
     *
     *  maxPlayers comes from serverDZ.cfg via ServerConfigGetInt, which is server-only - fine here,
     *  and where the autostart curve already reads it from. A missing or zero key answers TRUE:
     *  fullness cannot be determined, and the alternative is a gate that can never be satisfied,
     *  i.e. a server that never starts a match because its config was misread.
     */
    protected bool IsLobbyFull()
    {
        int max_players = GetGame().ServerConfigGetInt( "maxPlayers" );
        if( max_players <= 0 )
            return true;

        return (GetPlayers().Count() >= max_players);
    }

    /**
     *  Issue #9: may a match start while the lobby is below the server's player cap?
     *
     *  A ready-up vote reaching ready_up_percent used to start the match at any population from
     *  minimum_players upwards, so a dozen players could vote a 60-slot server into a match minutes
     *  before it filled. While min_waiting_time_not_full has not elapsed, only a full lobby may go.
     *
     *  Measured from i_FirstPlayerTick rather than from GetTickTime() alone, so it means "X seconds
     *  of the lobby actually having players in it" rather than X seconds of server uptime. That is
     *  the same basis the autostart curve uses, and it is the one an admin means when they set it.
     *
     *  Deliberately NOT applied to the autostart path - see the comment on the setting itself.
     */
    protected bool IsNotFullWaitSatisfied()
    {
        if( f_MinWaitingTimeNotFull <= 0 )
            return true;

        if( IsLobbyFull() )
            return true;

        return (GetGame().GetTickTime() - i_FirstPlayerTick) >= f_MinWaitingTimeNotFull;
    }

    //--- Seconds left on the not-full wait, for the notification. 0 when nothing is waiting.
    protected int GetNotFullSecondsLeft()
    {
        if( IsNotFullWaitSatisfied() )
            return 0;

        //--- Math.Ceil returns a float; the narrowing happens on the assignment, which is the same
        //--- idiom the min_waiting_time countdown in MessageWaiting already uses.
        float elapsed = GetGame().GetTickTime() - i_FirstPlayerTick;
        int seconds_left = Math.Ceil( f_MinWaitingTimeNotFull - elapsed );
        return seconds_left;
    }

    /**
     *  May a match start that is otherwise ready to go proceed?
     *
     *  **Only call this once every other start condition is satisfied.** It starts a clock on its
     *  first refusal, and that clock is what bounds the delay - so consulting it from somewhere the
     *  match was never going to start anyway (the 10 Hz IsComplete poll of a lobby that is still
     *  half empty, say) would run the bound down against nothing and leave the gate permanently open
     *  by the time it mattered.
     *
     *  The hold is bounded rather than absolute because an absolute one cannot survive a busy lobby:
     *  arrivals are continuous, each is unloaded for ~20 s, and "nobody is loading" is a condition a
     *  filling server may simply never satisfy. Capping the hold means the stragglers get their few
     *  seconds in the ordinary case and a churning lobby still starts on time in the bad one.
     */
    protected bool IsLoadGateClear()
    {
        if( GetNotLoadedCount() == 0 )
        {
            i_LoadHoldStartedMs = 0;
            return true;
        }

        int now = GetGame().GetTime();

        if( i_LoadHoldStartedMs == 0 )
        {
            i_LoadHoldStartedMs = now;
            BattleRoyaleUtils.Info("Lobby load gate: holding the match start, " + GetNotLoadedCount() + " of " + GetPlayers().Count() + " players still loading (at most " + BR_LOBBY_LOAD_MAX_HOLD_SECONDS + "s).");
            return false;
        }

        if( (now - i_LoadHoldStartedMs) >= (BR_LOBBY_LOAD_MAX_HOLD_SECONDS * 1000) )
        {
            BattleRoyaleUtils.Warn("Lobby load gate: held the match start for " + BR_LOBBY_LOAD_MAX_HOLD_SECONDS + "s and " + GetNotLoadedCount() + " players are still loading. Starting anyway.");
            i_LoadHoldStartedMs = 0;
            return true;
        }

        return false;
    }

    //--- Called from the paths above whenever the match is NOT otherwise ready to start, so a hold
    //--- that began under conditions which have since lapsed - players left, the roster fell back
    //--- below minimum_players - does not keep burning its bound while nothing is waiting on it.
    protected void ResetLoadHold()
    {
        i_LoadHoldStartedMs = 0;
    }

#ifdef DIAG_DEVELOPER
    /**
     *  Diag only: fake a lobby where the first `minimum_players` are in and the rest are still
     *  loading - or put everybody back.
     *
     *  The window this fakes is real but far too short to test against. Measured 2026-08-13 on three
     *  local clients, the gap between `AddPlayer` and the `PlayerLoadedIn` report was under ONE
     *  SECOND - so a test built on connecting a player at the right moment is a coin flip against a
     *  10 Hz poll, and a miss is indistinguishable from a broken gate.
     *
     *  ⚠️ IT LEAVES EXACTLY minimum_players LOADED, and that is the entire point rather than a
     *  detail. Marking *everyone* unloaded is the obvious implementation and it tests NOTHING: the
     *  start test now counts loaded players (`GetLoadedPlayerCount() >= i_MinPlayers`), so a lobby
     *  with nobody loaded fails at the minimum and the start never goes green - meaning the load gate
     *  is never consulted and the hold never arms. The gate only has an opinion when the match would
     *  otherwise start, so the fixture has to produce exactly that: enough loaded to be startable,
     *  plus at least one straggler. Which is also the real-world case it stands in for.
     *
     *  The clients never re-report - SendLoadedInOnce is a one-shot for the session - so this holds
     *  indefinitely until either it is called again with false, which is how the "gate clears and the
     *  match starts" half is exercised, or BR_LOBBY_LOAD_MAX_HOLD_SECONDS expires and starts the
     *  match anyway, which is the other half.
     */
    void BR_DiagSetAllUnloaded(bool unloaded)
    {
        ref array<PlayerBase> players = GetPlayers();
        int marked = 0;

        for(int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = players.Get(i);
            if(!player)
                continue;

            //--- Restoring: everybody back to loaded, no arithmetic needed.
            if( !unloaded )
            {
                player.br_loaded_in = true;
                continue;
            }

            //--- Faking: keep the first i_MinPlayers loaded so the start still goes green, and turn
            //--- everyone after them into a straggler.
            if( i < i_MinPlayers )
            {
                player.br_loaded_in = true;
                continue;
            }

            player.br_loaded_in = false;
            marked++;
        }

        //--- Cleared too, so a second press starts a fresh bound rather than resuming one that has
        //--- already half run down.
        ResetLoadHold();

        if( unloaded )
            BattleRoyaleUtils.Info("[Diag] faked " + marked + " straggler(s) of " + players.Count() + ", keeping " + i_MinPlayers + " loaded so the start still goes green.");
        else
            BattleRoyaleUtils.Info("[Diag] marked all " + players.Count() + " lobby players loaded again.");

        if( unloaded && marked == 0 )
            BattleRoyaleUtils.Warn("[Diag] nobody left to fake as loading - you need more than minimum_players (" + i_MinPlayers + ") in the lobby for this to do anything.");
    }

    /**
     *  Diag only: say why the lobby is or is not starting, one condition at a time.
     *
     *  Every term of both start paths, so "it did not start and I do not know why" is one keypress
     *  instead of a log dig. Emitted as two lines and built in steps rather than as one concatenation
     *  - a single expression carrying this many fields hits EnfusionScript's "Formula too complex",
     *  which is a hard compile error that only surfaces when the module loads.
     *
     *  The bools go through int locals because only their numeric form is safe to concatenate here.
     */
    void BR_DiagLogGate()
    {
        int total = GetPlayers().Count();
        int loaded = GetLoadedPlayerCount();
        int tick_now = GetGame().GetTickTime();
        int min_wait = f_MinWaitingTime;

        int groups = total;
#ifdef VIGRID_PARTY
        groups = VigridPartyAPI.GetGroupCount( GetPlayers() );
#endif

        string line1 = "[Diag] lobby gate #8:";
        line1 += " players=" + total;
        line1 += " loaded=" + loaded;
        line1 += " minimum=" + i_MinPlayers;
        line1 += " groups=" + groups;
        line1 += " uptime=" + tick_now + "/" + min_wait;
        BattleRoyaleUtils.Info( line1 );

        int is_full = 0;
        if( IsLobbyFull() )
            is_full = 1;

        int notfull_ok = 0;
        if( IsNotFullWaitSatisfied() )
            notfull_ok = 1;

        int hold_running = 0;
        if( i_LoadHoldStartedMs != 0 )
            hold_running = 1;

        int vote_system = 0;
        if( b_UseVoteSystem )
            vote_system = 1;

        string line2 = "[Diag] lobby gate #9:";
        line2 += " full=" + is_full;
        line2 += " notfull_wait_ok=" + notfull_ok;
        line2 += " notfull_secs_left=" + GetNotFullSecondsLeft();
        line2 += " vote_system=" + vote_system;
        line2 += " load_hold_running=" + hold_running;
        BattleRoyaleUtils.Info( line2 );
    }
#endif

    //returns true when this state is complete
    override bool IsComplete()
    {
        //--- The whole body is guarded on !b_UseVoteSystem, which is the condition the start test
        //--- below already carried. Hoisted so that a server running the vote system - the default -
        //--- never touches the load hold clock from here: CheckReadyState owns it in that case, and
        //--- a ResetLoadHold() called at this method's 10 Hz would wipe the clock it is running.
        if( !b_UseVoteSystem )
        {
            //--- Hoisted into a local because EnfusionScript has no multi-line if conditions, so the
            //--- guarded term cannot simply be appended to the condition below.
            bool enough_groups = true;
#ifdef VIGRID_PARTY
            enough_groups = VigridPartyAPI.GetGroupCount( GetPlayers() ) > 1;
#endif

            //--- Split into named locals rather than one long condition: EnfusionScript caps how
            //--- complex a single expression may be ("Formula too complex" is a hard compile error),
            //--- and this one now carries five independent tests.
            bool enough_players = GetLoadedPlayerCount() >= i_MinPlayers;
            bool waited_long_enough = GetGame().GetTickTime() >= f_MinWaitingTime;
            bool start_wanted = IsActive() && enough_players && waited_long_enough && enough_groups && IsNotFullWaitSatisfied();

            //--- Load gate consulted LAST, once the start is otherwise green - see IsLoadGateClear.
            if( !start_wanted )
                ResetLoadHold();
            else if( IsLoadGateClear() )
                Deactivate();
        }

        return super.IsComplete();
    }

    override void AddPlayer(PlayerBase player)
    {
        super.AddPlayer( player );

		// Track the first connected player tick time to determine when to start the game
        if( GetPlayers().Count() == 1 )
        	i_FirstPlayerTick = GetGame().GetTickTime();
    }

    override void Activate()
    {
        //these loop & will be automatically cleaned up on Deactivation
        AddTimer(i_TimeBetweenMessages, this, "MessageWaiting", NULL, true);

        //--- Name the previous winner once, as the lobby opens. The record is already in memory -
        //--- the summary store loads it at boot - so this costs one string and reaches every player
        //--- rather than only the ones who think to press F4.
        AnnouncePreviousWinner();

        if( b_UseVoteSystem )
        	AddTimer(2.0, this, "CheckReadyState", NULL, true);

#ifdef KILLFEED
        //--- Nothing that happens in the lobby belongs in the feed, and a feed running here would
        //--- just advertise who is fighting whom before the match has started.
        KillFeedAPI.SetActive( false );
#endif

#ifdef VIGRID_SAFEZONE
        //--- Lobby truce: the trigger does nothing and nothing another player does can hurt you.
        //--- Aiming and melee swings still work, so players can still punch each other while they
        //--- wait - which is the point, and the reason this is not Expansion's safezone.
        VigridSafeZoneAPI.SetActive( true );
#endif

#ifdef VIGRID_MAP
        //--- Markers do not expire, so any left from an earlier match in this same process would
        //--- still be on the map when the next lobby opens. A real process restart clears them
        //--- anyway; this covers the in-process reset.
        VigridMapAPI.ClearAllMarkers();
#endif

#ifdef VIGRID_PARTY
        //--- Here rather than in BattleRoyaleServer.Init(): the party manager is created from its
        //--- own modded MissionServer.OnInit, and nothing pins the order of the two overrides, so
        //--- Init() may well run before it exists. By the time the lobby opens it certainly does.
        //--- One match per process, so this runs at most once.
        RunAutoGroupSelfTest();
#endif

        super.Activate();
    }

    override void Deactivate()
    {
        super.Deactivate();

#ifdef VIGRID_PARTY
        //--- Forced team size, if the server wants one. This is the last instant the roster is still
        //--- intact - BattleRoyaleServer.Update calls Deactivate() and only then migrates the
        //--- players to the next state - and it is after the group-count gate in IsComplete(), so
        //--- grouping cannot stall the lobby it is closing.
        //---
        //--- It also has to land BEFORE the lock below, for two reasons. Players are about to be
        //--- told composition is frozen and must not then watch it change; and everything downstream
        //--- that reads parties - party-only voice, spawn selection gathering, the grouped teleport,
        //--- the leaderboard field size - runs from the next state onwards and picks the new teams
        //--- up for free.
        if ( i_MinPartySize > 1 )
            VigridPartyAPI.AutoGroup( GetPlayers(), i_MinPartySize, BR_AUTO_GROUP_MIN_GROUPS, i_MinPartyRemainder );

        //--- The lobby is the last moment party composition may change. Freezing it here stops a
        //--- player splitting off mid-match, which would raise the group count and stall the
        //--- round-end condition that waits for a single group to remain.
        VigridPartyAPI.SetFormationLocked( true );
#endif
    }

    void CheckReadyState()
    {
		if( GetGame().GetTickTime() >= f_MinWaitingTime && GetPlayers().Count() > 1 )
		{
			//--- The not-full wait (#9) gates the VOTE only. The autostart term below is left alone
			//--- on purpose: its threshold already decays from "a full lobby" to nothing across
			//--- autostart_delay, so it is the same rule in continuous form, and it is the path that
			//--- guarantees a match eventually happens at all.
			bool vote_start = IsActive() && IsVoteReady() && IsNotFullWaitSatisfied();

			int t_MaxPlayers = GetGame().ServerConfigGetInt( "maxPlayers" );
			bool auto_start = b_AutoStartGame && GetReadyCount() > 1 && GetReadyCount() >= ( t_MaxPlayers - ( ( t_MaxPlayers * ( GetGame().GetTickTime() - i_FirstPlayerTick ) ) / f_AutoStartDelay ) );

			//--- Merged into one exit where these used to be two independent DeactivateDeferred()
			//--- calls, so the load gate is consulted exactly once and cannot be satisfied by one
			//--- path while the other starts the match behind its back.
			if( !vote_start && !auto_start )
			{
				ResetLoadHold();
				return;
			}

			//--- The hole issue #8 describes is on the vote path: a lobby at 80% readiness starts the
			//--- moment min_waiting_time elapses, however many players are still on a loading screen.
			//--- Consulted LAST, once the start is otherwise green - see IsLoadGateClear.
			if( !IsLoadGateClear() )
				return;

			//--- Deferred: this runs from a looping timer, i.e. inside TimerQueue.Tick. See
			//--- BattleRoyaleState.DeactivateDeferred().
			DeactivateDeferred();
		}
		else
		{
			//--- Below the floor entirely - nothing is waiting on a load, so nothing should be
			//--- spending the hold's bound.
			ResetLoadHold();
		}
    }

    /**
     *  "Last match won by X with N kills."
     *
     *  Silent when there is no previous match - first boot, or an operator who cleared the file -
     *  rather than announcing an empty one.
     */
    void AnnouncePreviousWinner()
    {
        BattleRoyaleLastMatchFile previous = BattleRoyaleMatchStats.GetInstance().GetPrevious();
        if (!previous)
            return;
        if (previous.winner_name == "")
            return;

        MessagePlayersUntranslated("STR_BR_LASTMATCH_WINNER_ANNOUNCE", previous.winner_name, previous.winner_kills.ToString());
    }

    void MessageWaiting()
    {
		//--- Over loaded players, so this agrees with the minimum_players test that actually gates the
		//--- start. Against the raw roster it would say "waiting for 0 more" while the lobby sat
		//--- there waiting, which is the shape of report that sends an admin looking for a bug.
		int waiting_on_count = i_MinPlayers - GetLoadedPlayerCount();

		if( waiting_on_count > 0)
		{
			if( waiting_on_count == 1 )
				MessagePlayersUntranslated("STR_BR_WAITING_FOR_PLAYER");
			else
				MessagePlayersUntranslated("STR_BR_WAITING_FOR_PLAYERS", waiting_on_count.ToString());
		}

		//--- Keyed on a hold actually running, not on the raw unloaded count. Players trickle into a
		//--- busy lobby continuously and each is loading for ~20 s, so that count is almost always
		//--- non-zero and announcing it every cycle would be pure noise. A running hold means the
		//--- match would have started by now, which is the only version of this worth telling anyone.
		//--- Only while the lobby is otherwise big enough to go, which is the only time the not-full
		//--- wait is what is actually holding things up - below minimum_players the message above
		//--- already explains it, and two competing explanations is worse than one.
		int not_full_seconds = GetNotFullSecondsLeft();
		if( not_full_seconds > 0 && GetLoadedPlayerCount() >= i_MinPlayers )
			MessagePlayersUntranslated("STR_BR_WAITING_FOR_FULL_LOBBY", not_full_seconds.ToString());

		int not_loaded_count = GetNotLoadedCount();
		if( i_LoadHoldStartedMs != 0 && not_loaded_count > 0 )
		{
			if( not_loaded_count == 1 )
				MessagePlayersUntranslated("STR_BR_WAITING_FOR_LOADING_PLAYER");
			else
				MessagePlayersUntranslated("STR_BR_WAITING_FOR_LOADING_PLAYERS", not_loaded_count.ToString());
		}

		if( b_UseVoteSystem )
		{
			// TODO: Move that to client side
			int ready_count = GetReadyCount();
			if( ready_count == 1 )
				MessagePlayersUntranslated("STR_BR_PLAYER_READY_UP");
			else
				MessagePlayersUntranslated("STR_BR_PLAYERS_READY_UP", ready_count.ToString());

			if( GetGame().GetTickTime() < f_MinWaitingTime )
			{
				int seconds_left = Math.Ceil( f_MinWaitingTime - GetGame().GetTickTime() );

				if( !IsVoteReady() )
					MessagePlayersUntranslated("STR_BR_CANNOT_START_BEFORE_SECOND", seconds_left.ToString());
				else
					MessagePlayersUntranslated("STR_BR_GAME_WILL_AUTO_START_IN", seconds_left.ToString());
			}
		}
    }

    int GetReadyCount()
    {
        int ready_count = 0;
        for(int a = 0; a < m_ReadyList.Count(); a++)
        {
            if(m_ReadyList[a])
                ready_count++;
        }

        return ready_count;
    }

    bool IsVoteReady()
    {
        if(!b_UseVoteSystem)
            return false;

        int ready_count = GetReadyCount();
        int player_count = GetPlayers().Count();

		if( player_count <= 1 ) // need more than 1 player
			return false;

		//--- At least the minimum, matching the non-vote path in IsComplete(). This used to demand
		//--- strictly more, so a lobby sitting at exactly minimum_players could never vote-start
		//--- however many players readied up.
		//---
		//--- Counted over LOADED players, like IsComplete()'s enough_players - minimum_players means
		//--- players who can play. Note the percentage below still divides by the full roster: a
		//--- player who is still loading cannot have readied up, so including them makes the vote
		//--- harder to pass, and erring towards not starting is the safe direction here.
		if( GetLoadedPlayerCount() < i_MinPlayers )
			return false;

#ifdef VIGRID_PARTY
		if( VigridPartyAPI.GetGroupCount( GetPlayers() ) <= 1 ) // need more than one group
			return false;
#endif

        //--- Cast before dividing. Both operands are int, so this truncated to 0 for any partial
        //--- readiness and to 1 only at 100% - which made ready_up_percent dead config, the vote
        //--- passing only when every single player had readied up.
        float percent = ready_count / (float)player_count;
        return (percent >= f_VoteThreshold);
    }

    void ReadyUp(PlayerBase player)
    {
        if(m_ReadyList.Find(player) != -1)
        {
            MessagePlayerUntranslated(player, "STR_BR_YOU_ALREADY_READIED_UP");
            return;
        }

        MessagePlayerUntranslated(player, "STR_BR_YOU_READIED_UP");
        m_ReadyList.Insert( player );

        //this is here because we don't want someone mass spamming all players by spamming F1
        int count = GetReadyCount();
        int max = GetPlayers().Count();
        MessagePlayersUntranslated("STR_BR_PLAYER_READIED_UP", count.ToString(), max.ToString());
    }

    override void RemovePlayer(PlayerBase player)
    {
        super.RemovePlayer( player );
        
        m_ReadyList.RemoveItem( player );
    }

    override ref array<PlayerBase> RemoveAllPlayers()
    {
        m_ReadyList.Clear();
        ref array<PlayerBase> players = super.RemoveAllPlayers();
        return players;
    }
}
