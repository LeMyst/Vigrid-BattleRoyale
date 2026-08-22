#ifdef SERVER
class BattleRoyaleState: Timeable
{
    protected ref array<PlayerBase> m_Players;
    protected bool b_IsActive;
    protected bool b_IsPaused;
    protected bool b_IsDebug;

    //--- Set by the state machine when this state is passed over entirely. A skipped state never
    //--- activates, so anything it owns - a round's circle above all - was never in play and was
    //--- never sent to a client. Distinct from b_IsActive, which only says "not running *now*".
    protected bool b_WasSkipped;

	// Track the number of players in the game when the game starts - shared between all states
    static int i_NumStartingPlayers = 0;

    //--- Memo for GetDynamicStartingZone(). Its answer depends only on num_players plus config and
    //--- the generated circles, and both are fixed for the life of the process, so recomputing it is
    //--- pure waste. It was being called once per spawn candidate through IsSafeForTeleport, which
    //--- meant two config fetches and a zone scan several hundred times per player.
    //--- -1 means "nothing memoized yet"; a real player count is never negative.
    //---
    //--- STATIC, along with GetDynamicStartingZone itself. The comment above is the argument for it -
    //--- the answer depends only on num_players plus config plus the generated circles, all fixed for
    //--- the life of the process - so a per-state memo was recomputing the same answer once per state
    //--- for no reason. It has to be static now anyway: BattleRoyaleZone.RunLadderSelfTest asks the
    //--- same question at boot, before any state instance exists.
    static int i_DynamicZoneMemoPlayers = -1;
    static int i_DynamicZoneMemoResult = 1;

    //static int i_StartingZone = 1; // Default zone is the biggest one

    //--- Plain game setting, unrelated to whether parties exist. It used to sit behind the party
    //--- mod's #ifdef, which meant a build without that mod could not honour it at all.
    bool hide_players_endgame = false;

    //--- The timer the HUD countdown is currently counting down to, or NULL when no countdown is
    //--- showing. Set by SendCountdown() and read by ResendGameInfo().
    //---
    //--- Deliberately NOT a `ref`: the state that started the timer already owns one, and this is a
    //--- second handle to the same object rather than a second owner.
    protected Timer m_CountdownTimer;

    //--- Milliseconds to ADD to m_CountdownTimer's remaining time to get the deadline by which a
    //--- player has to be inside the circle the HUD arrow is pointing at. Zero for every state whose
    //--- countdown IS that deadline - which is all of them except the warm-up, where the clock counts
    //--- down to the first ROUND starting and the circle then stays harmless for another
    //--- BR_ZONE_LOCK_FRACTION of round one on top.
    //---
    //--- Stored rather than recomputed so ResendGameInfo() can re-assert it off the live timer.
    protected int i_CountdownZoneExtraMs;

    //--- The two circles every client should currently be holding, re-asserted by ResendGameInfo().
    //---
    //--- STATIC, and that is load-bearing: a circle outlives the state that announced it. A round
    //--- that is not the last one never sends UpdateCurrentPlayArea at all - it relies on the PREVIOUS
    //--- round's LockNewZone - so a per-instance record would be empty for most of the match, and a
    //--- resend built from it would push the clear payload and wipe the live circle off every HUD and
    //--- every map. Same reasoning as s_PlayAreaDurationOffsets in BattleRoyaleZone.
    //---
    //--- Seeded to the clear payload, which is exactly what BattleRoyaleRPC already defaults to, so a
    //--- resend before anything has been announced is a no-op the client diffs away. That also makes
    //--- 7_BattleRoyaleLastRound's deliberate clears re-assertable for free, with no "has this been
    //--- set yet" flag to get wrong.
    protected static vector s_CurrentAreaCenter = "0 0 0";
    protected static float s_CurrentAreaRadius = 0.0;
    protected static vector s_FutureAreaCenter = "0 0 0";
    protected static float s_FutureAreaRadius = 0.0;

    //--- Read-only accessors, because the four above are `protected` and the COT admin readout is
    //--- neither a state nor a subclass of one. Getters rather than making the fields public: the
    //--- write side is deliberately funnelled through SendCurrentPlayArea / SendFuturePlayArea,
    //--- which also push the RPC, and a public field would let a future caller set a circle that
    //--- no client was ever told about.
    static vector GetAnnouncedCurrentCenter()
    {
        return s_CurrentAreaCenter;
    }

    static float GetAnnouncedCurrentRadius()
    {
        return s_CurrentAreaRadius;
    }

    static vector GetAnnouncedFutureCenter()
    {
        return s_FutureAreaCenter;
    }

    static float GetAnnouncedFutureRadius()
    {
        return s_FutureAreaRadius;
    }

    string GetName()
    {
        return "Unknown State";
    }

    void BattleRoyaleState()
    {
        m_Players = new array<PlayerBase>();

        b_IsActive = false;
        b_IsPaused = false;
        b_IsDebug = false;

        //--- Re-asserts everything the HUD shows - the player/group counter and the countdown - on
        //--- every client. This used to be registered only when the party mod was present, so a
        //--- build without it never refreshed the counter at all: the panel simply froze on
        //--- whatever it was first told.
        AddTimer(5.0, this, "ResendGameInfo", NULL, true);

        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            hide_players_endgame = m_GameSettings.hide_players_endgame;
        }
    }

    void Update(float timeslice)
    {

    }

    //state controls
    void Activate()
    {
    	BattleRoyaleUtils.Debug(string.Format("BattleRoyaleState::Activate: %1", GetName()));
        //Note: this is called AFTER players are added
        b_IsActive = true;
    }

    void Deactivate()
    {
    	BattleRoyaleUtils.Debug(string.Format("BattleRoyaleState::Deactivate: %1", GetName()));
        //Note: this is called BEFORE players are removed
        //--- stop all repeating timers
        StopTimers();

        //--- Whatever this state was counting down to is over. Dropped rather than left dangling so
        //--- a stray resend cannot advertise a dead state's timer.
        m_CountdownTimer = NULL;
        i_CountdownZoneExtraMs = 0;

        b_IsActive = false;
    }

    /**
     *  Deactivate() on the next frame instead of right now.
     *
     *  **Call this instead of Deactivate() from inside a timer callback.**
     *
     *  Stopping a Timer removes it from vanilla's TimerQueue (`TimerBase.SetRunning` ->
     *  `m_timerQueue.Remove`, tools.c:351-375), and Deactivate() stops several at once - the state's
     *  own one-shots plus every looping timer via StopTimers(). Meanwhile `TimerQueue.Tick`
     *  (tools.c:407-420) snapshots Count() *before* its loop and then walks indices downward:
     *
     *      int count = Count();
     *      for (int i = count - 1; i >= 0; i--)
     *          Get(i).Tick(timeslice);
     *
     *  A one-shot timer has already removed itself by the time its callback runs (`TimerBase.Tick`
     *  calls SetRunning(false) at tools.c:287, before OnTimer()), so the snapshot is stale by one
     *  the moment we are entered. Each further timer the callback stops shrinks the array again,
     *  and once Count() drops below the loop's index, Get(i) returns NULL - the
     *  "NULL pointer to instance / Class: 'TimerQueue' / Function: 'Tick'" exception.
     *
     *  Deferring by one frame takes every removal out of TimerQueue.Tick. Nothing observable
     *  changes: the driver only polls IsComplete() at 10 Hz anyway (BattleRoyaleServer.c:195), so a
     *  state that signals completion a frame earlier or later transitions on the same tick.
     *
     *  Deactivate() reached from IsComplete() or from a script coroutine does NOT need this - those
     *  do not run inside TimerQueue.Tick.
     */
    void DeactivateDeferred()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "Deactivate", 0, false);
    }

    bool IsActive()
    {
        return b_IsActive;
    }

    void SetSkipped(bool skipped)
    {
        b_WasSkipped = skipped;
    }

    bool WasSkipped()
    {
        return b_WasSkipped;
    }

    bool IsPaused()
    {
        return b_IsPaused;
    }

    void Pause()
    {
        b_IsPaused = true;
    }

    void Resume()
    {
        b_IsPaused = false;
    }

    bool IsComplete()
    {
        //state cannot complete if paused
        if(b_IsPaused)
            return false;

        return !IsActive();
    }

    /**
     *  Milliseconds left on whatever this state is counting down to, or BR_COUNTDOWN_NONE.
     *
     *  ⚠️ The IsRunning() test is LOAD-BEARING and not defensive, for the same reason SendCountdown
     *  documents: a one-shot Timer that has already fired sets m_time = 0 BEFORE invoking its
     *  callback (TimerBase.Tick, P:\scripts\3_game\tools\tools.c), so GetRemaining() on a fired
     *  timer hands back its FULL DURATION. Drop the guard and a finished countdown reads as a fresh
     *  one - which on a readout looks like a clock that jumps back to the top rather than like a bug.
     */
    int GetCountdownRemainingMs()
    {
        if(!m_CountdownTimer)
            return BR_COUNTDOWN_NONE;

        if(!m_CountdownTimer.IsRunning())
            return BR_COUNTDOWN_NONE;

        int remaining_ms = (int)Math.Round( m_CountdownTimer.GetRemaining() * 1000 );
        if(remaining_ms <= 0)
            return BR_COUNTDOWN_NONE;

        return remaining_ms;
    }

    /**
     *  Fill in whatever this state knows for the COT admin readout.
     *
     *  A virtual rather than the panel reaching in, because the interesting fields are per-state and
     *  most of them are `protected` where they live - BattleRoyaleDebug owns the whole lobby start
     *  gate, and publishing a dozen accessors so an unrelated 5_Mission module could recompute it
     *  would put the knowledge in the wrong place and let the two drift.
     *
     *  The base fills only what every state has. Overriders should call super and then add their own.
     */
    void BR_FillAdminStatus(BattleRoyaleAdminStatus status)
    {
        if(!status)
            return;

        status.state_name = GetName();
        status.state_paused = IsPaused();
        status.players_alive = GetPlayers().Count();
        status.countdown_ms = GetCountdownRemainingMs();

        //--- ⚠️ Ask whether the party manager can actually answer before believing a group count.
        //--- #ifdef VIGRID_PARTY says the addon is COMPILED IN, not that parties are in play: with
        //--- the manager disabled GetGroupCount() degrades to one group per player, a number always
        //--- identical to players_alive. That is correct for the logic tests that consume it and
        //--- wrong to put on screen under a group heading, which is #158 exactly.
        status.groups_alive = BR_ADMIN_GROUPS_UNKNOWN;
#ifdef VIGRID_PARTY
        if( VigridPartyAPI.IsReady() )
            status.groups_alive = VigridPartyAPI.GetGroupCount( GetPlayers() );
#endif
    }

    bool SkipState(BattleRoyaleState _previousState)  //if true, we will skip activating/deactivating this state entirely
    {
        return false;
    }

    /**
     *  "Only one side is left standing" - THE win condition, in one place.
     *
     *  It is a test on GROUPS, not on players: a surviving party has already won, and a match that
     *  waits for one of them to kill the other never ends. Without the party addon - or with it
     *  compiled in but the manager switched off in party_settings.json - GetGroupCount() degrades to
     *  one group per player, so this mirrors the raw player test exactly and no caller needs an
     *  #else branch.
     *
     *  Static, and taking the roster explicitly rather than reading GetPlayers(), because
     *  SkipState() has to ask it about the state it is being handed and not about itself.
     *
     *  Every gameplay state must route through this. It used to be written out four times - twice in
     *  6_BattleRoyaleRound (IsComplete and SkipState), twice in 7_BattleRoyaleLastRound - and
     *  5_BattleRoyaleStartMatch had only the player half of it. The consequence was a party that
     *  wiped the field during the pre-zone countdown holding two live players, so the state never
     *  completed and the win screen did not appear until the countdown ran out on its own
     *  (observed 2026-08-14: field cleared at 13:02:18, Win State entered at 13:02:34).
     */
    static bool IsOneSideLeft(array<PlayerBase> players)
    {
        if(!players)
            return true;

        if(players.Count() <= 1)
            return true;

#ifdef VIGRID_PARTY
        if(VigridPartyAPI.GetGroupCount( players ) <= 1)
            return true;
#endif

        return false;
    }

    array<PlayerBase> GetPlayers()
    {
        return m_Players;
    }

    void AddPlayer(PlayerBase player)
    {
    	//--- GetCachedName, NOT GetIdentityName. The latter is declared on DAYZ-EXPANSION CORE's
    	//--- modded PlayerBase and does not exist in vanilla at all, so this line - unchanged since
    	//--- long before the Expansion removal - took the whole Mission module down the first time a
    	//--- server booted without Expansion loaded. It is invisible to any grep for "Expansion":
    	//--- it is a bare method call that looks exactly like vanilla API.
    	//---
    	//--- GetCachedName is vanilla (playerbase.c:9667) and is already this mod's canonical name
    	//--- accessor - BR_SetCachedName exists to make it authoritative, and Party and KillFeed both
    	//--- read it - so this also picks up the Steam-resolved name where Expansion's returned the
    	//--- raw launcher one.
    	BattleRoyaleUtils.Info(string.Format("BattleRoyaleState::AddPlayer: %1", player.GetCachedName()));
        m_Players.Insert( player );
        OnPlayerCountChanged();
    }

    void RemovePlayer(PlayerBase player)
    {
    	//--- GetCachedName, not Expansion's GetIdentityName - see AddPlayer above.
    	BattleRoyaleUtils.Info(string.Format("BattleRoyaleState::RemovePlayer: %1", player.GetCachedName()));

        //--- The single leaderboard recording point. Every way out of a match funnels through here -
        //--- killed, disconnected, disconnected while unconscious, force-logged out, or kicked as
        //--- the winner - and all three subclass overrides chain to this. RemoveAllPlayers()
        //--- deliberately does not, so state migration never scores.
        //---
        //--- RecordExit is safe to call unconditionally: it gates on its own ranking latch and
        //--- dedupes by SteamID64, which matters because a kill reaches this twice (once from
        //--- OnPlayerKilled, once from BattleRoyaleServer).
        BattleRoyaleLeaderboard.GetInstance().RecordExit(player);

        //--- And the match summary, for the same reason and with the same guarantees: this is the
        //--- one place every exit route funnels through, and it runs BEFORE RemoveItem so
        //--- GetBRPosition() is still the finishing place. It dedupes on its own set rather than the
        //--- leaderboard's, which is additionally gated on enable_leaderboard.
        //---
        //--- The death record is fetched here rather than inside RecordExit: it is held by
        //--- BattleRoyaleSpectators in 5_Mission and BattleRoyaleMatchStats is 4_World, so only this
        //--- caller can reach both. NULL means never eliminated - the winner.
        BattleRoyaleMatchStats.GetInstance().RecordExit(player, BattleRoyaleSpectators.GetInstance().GetDeathRecordFor(player));

        m_Players.RemoveItem(player);
        OnPlayerCountChanged();
    }

    array<PlayerBase> RemoveAllPlayers()
    {
    	BattleRoyaleUtils.Debug(string.Format("BattleRoyaleState::RemoveAllPlayers: removed %1 players", m_Players.Count()));
        array<PlayerBase> result_array = new array<PlayerBase>();
        result_array.InsertAll(m_Players);
        m_Players.Clear();
        OnPlayerCountChanged();
        return result_array;
    }

    bool ContainsPlayer(PlayerBase player)
    {
        return (m_Players.Find(player) >= 0);
    }

    /**
     *  May a player who dies while THIS state is running enter spectate?
     *
     *  Default no. The lobby, countdown, spawn selection and prepare phases have no match to watch,
     *  and the win/restart states are past the point where a new spectator makes sense - a death in
     *  any of them keeps the pre-existing behaviour exactly.
     *
     *  A dedicated predicate rather than reusing BattleRoyaleServer's
     *  `i_CurrentStateIndex > 2 && i_CurrentStateIndex < m_States.Count() - 2` range test, which
     *  shifts by one depending on whether enable_spawn_selection_menu inserted state 3.
     */
    bool AllowsSpectate()
    {
        return false;
    }

    //--- Resolve the engine-supplied RPC `sender` to a player tracked by THIS state.
    //--- Server RPC handlers must use this instead of the `Object target` they receive:
    //--- `target` is chosen by the client and can name any other player, `sender` cannot be spoofed.
    //--- Returns NULL when the identity is unusable or the sender is not part of this state,
    //--- so the membership test that ContainsPlayer used to provide is inherent here.
    PlayerBase GetPlayerFromIdentity(PlayerIdentity identity)
    {
        if(!identity)
            return NULL;

        string sender_id = identity.GetId();
        if(sender_id == "")
            return NULL;

        for(int i = 0; i < m_Players.Count(); ++i)
        {
            PlayerBase candidate = m_Players.Get(i);
            if(!candidate)
                continue;

            PlayerIdentity candidate_identity = candidate.GetIdentity();
            if(!candidate_identity)
                continue;

            if(candidate_identity.GetId() == sender_id)
                return candidate;
        }

        return NULL;
    }

	void OnPlayerTick(PlayerBase player, float timeslice)
	{
	}

	/**
	 *  Push the countdown to every client AND record which timer it is counting down to.
	 *
	 *  Pass NULL to clear the countdown outright.
	 *
	 *  THE WIRE CARRIES MILLISECONDS REMAINING, not seconds - see BR_COUNTDOWN_NONE. The client
	 *  turns it into a deadline against its own clock the instant it lands and never counts down
	 *  locally, which is what removes the drift that used to have the zone locking several seconds
	 *  before the HUD reached 00:00, differently on every screen.
	 *
	 *  Recording the timer is what lets ResendGameInfo() below re-assert this every 5 s from ONE
	 *  place. It replaced four hand-written SendRPC call sites, one of which (7_BattleRoyaleLastRound)
	 *  had no resend at all - the same shape of drift-by-duplication that IsOneSideLeft was
	 *  centralised to stop.
	 *
	 *  THE SECOND INT IS THE DEADLINE THE HUD COLOURS THE CLOCK AGAINST, which is not always the
	 *  countdown itself. It rides this RPC rather than one of its own precisely because the two are
	 *  halves of the same fact: sent together they can never disagree, and ResendGameInfo has one
	 *  thing to keep in step instead of two. `zone_extra_ms` defaults to 0, so four of the five call
	 *  sites say nothing and send exactly the countdown - the colour is unchanged everywhere except
	 *  5_BattleRoyaleStartMatch, which is the one state where the two genuinely differ.
	 *
	 *  Note it is deliberately NOT "milliseconds until the circle bites": at LockNewZone the circle
	 *  is already biting, so that reading is 0 and would pin the clock red for the rest of the match.
	 *  "The deadline the colour is measured against" is one meaning that stays true at all five.
	 */
	void SendCountdown(Timer countdown_timer, int zone_extra_ms = 0)
	{
		m_CountdownTimer = countdown_timer;
		i_CountdownZoneExtraMs = zone_extra_ms;

		int remaining_ms = BR_COUNTDOWN_NONE;

		//--- IsRunning() is LOAD-BEARING, not defensive. A one-shot Timer that has already fired
		//--- sets m_time = 0 *before* invoking its callback (TimerBase.Tick,
		//--- P:\scripts\3_game\tools\tools.c), so GetRemaining() on a fired timer hands back its
		//--- FULL DURATION rather than zero - i.e. dropping this guard restarts the countdown from
		//--- the top every 5 s.
		if(countdown_timer && countdown_timer.IsRunning())
			remaining_ms = (int)Math.Round( countdown_timer.GetRemaining() * 1000 );

		if(remaining_ms <= 0)
			remaining_ms = BR_COUNTDOWN_NONE;

		//--- Only meaningful while a countdown is actually running: with no timer there is nothing to
		//--- add the extra to, and BR_COUNTDOWN_NONE + extra would be a plausible-looking wrong number.
		int zone_ms = BR_COUNTDOWN_NONE;
		if(remaining_ms > 0)
			zone_ms = remaining_ms + zone_extra_ms;

		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownMs", new Param2<int, int>( remaining_ms, zone_ms ), true);
	}

	/**
	 *  Announce the circle that is currently damaging, and record it for the resend.
	 *
	 *  Every UpdateCurrentPlayArea in the mod goes through here. Pass the clear payload
	 *  ("0 0 0" / 0) to retire the circle, exactly as 7_BattleRoyaleLastRound does.
	 */
	void SendCurrentPlayArea(vector center, float radius)
	{
		s_CurrentAreaCenter = center;
		s_CurrentAreaRadius = radius;

		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param2<vector, float>( center, radius ), true);
	}

	/**
	 *  Announce the circle that is coming next, and record it for the resend.
	 *
	 *  `artillery` is an EVENT, not state - it makes the client play a one-shot distant-artillery
	 *  sound at the new centre. It is therefore deliberately absent from what gets re-asserted; see
	 *  ResendGameInfo below.
	 */
	void SendFuturePlayArea(vector center, float radius, bool artillery)
	{
		s_FutureAreaCenter = center;
		s_FutureAreaRadius = radius;

		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param3<vector, float, bool>( center, radius, artillery ), true);
	}

	/**
	 *  Re-assert everything the HUD shows, on a 5 s loop registered in the constructor.
	 *
	 *  This is the only thing that corrects a client which missed a push, joined late, or
	 *  reconnected - and, for the countdown, the only thing that closes the gap between the
	 *  server's clock and the client's.
	 */
	void ResendGameInfo()
	{
		if(!IsActive())
			return;

		//--- Sends SetPlayerCount and SetTopPosition.
		OnPlayerCountChanged();

		//--- The extra has to come along, or every resend would flatten the warm-up's colour deadline
		//--- back onto the countdown five seconds after it was set.
		if(m_CountdownTimer && m_CountdownTimer.IsRunning())
			SendCountdown( m_CountdownTimer, i_CountdownZoneExtraMs );

		//--- The circles. Until this existed, a client that missed either push - a dropped packet, a
		//--- reconnect, an admin joining mid-match - held the wrong circles until the NEXT state
		//--- transition, which on a long round is minutes of a HUD arrow pointing at nothing and a map
		//--- drawing a circle that has already moved.
		//---
		//--- Both go out unconditionally rather than on a "has anything been announced" flag: the
		//--- seeded values are the clear payload, which is what the client already holds, so a resend
		//--- in the lobby raises no diff and costs the client nothing.
		//---
		//--- ARTILLERY IS FALSE HERE, ALWAYS. The flag fires a one-shot sound on any client whose
		//--- future circle actually changed - which is precisely the client this resend exists to
		//--- correct. Passing the real flag would mean anyone who reconnected mid-round heard distant
		//--- artillery announcing a circle that appeared minutes ago.
		SendCurrentPlayArea( s_CurrentAreaCenter, s_CurrentAreaRadius );
		SendFuturePlayArea( s_FutureAreaCenter, s_FutureAreaRadius, false );
	}

	//player count changed event handler
	protected void OnPlayerCountChanged()
	{
		//BattleRoyaleUtils.Trace("OnPlayerCountChanged()");
		if(IsActive())
		{
			int nb_players, nb_groups;

			nb_players = GetPlayers().Count();

			//--- Sentinels the client decodes in BattleRoyaleHud.SetCount, named in
			//--- BattleRoyaleConstants.c: _NONE hides the group panel outright, _CONCEALED shows "???".
			nb_groups = BR_HUD_GROUPS_NONE;
			int groups_count = nb_players;

#ifdef VIGRID_PARTY
			//--- IsReady() rather than the bare GetGroupCount(), because that call degrades to one
			//--- group per player when the party manager is disabled in party_settings.json - a
			//--- figure identical to the player count, which is no information at all and is exactly
			//--- what the _NONE sentinel exists to suppress. Compiling Party in is not the same thing
			//--- as parties being in play, and since Party ships in this repo the #ifdef alone made
			//--- _NONE unreachable on every server.
			//---
			//--- Deliberately NOT extended to "groups == players", which is the same figure twice and
			//--- looks like the same redundancy: with parties enabled that equality is real
			//--- information (everyone left is solo), it would blink the panel out mid-match as the
			//--- last duo dies, and blinking it out is itself a composition tell - the exact thing
			//--- hide_players_endgame exists to prevent.
			if( VigridPartyAPI.IsReady() )
			{
				groups_count = VigridPartyAPI.GetGroupCount( GetPlayers() );
				nb_groups = groups_count;

				//--- Endgame concealment. Only ever applied to a group count that means something:
				//--- with no parties in play there is no composition to hide, and withholding a
				//--- figure the client is not being shown anyway would just re-show an empty panel.
				if(nb_players < BR_HUD_ENDGAME_PLAYERS && hide_players_endgame && !b_IsDebug)
					nb_groups = BR_HUD_GROUPS_CONCEALED;
			}
#endif

			//--- Placement follows groups when parties are in play and raw player count otherwise.
			//--- Note this is the TRUE count either way: concealment is a display choice and must
			//--- never reach br_position, which is the player's finishing place.
			UpdateTopPosition( groups_count );

			//BattleRoyaleUtils.Trace(string.Format("OnPlayerCountChanged: %1 %2", nb_players, nb_groups));
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", new Param2<int, int>( nb_players, nb_groups ), true);
		}
	}

    void UpdateTopPosition( int position )
    {
		for(int i = 0; i < GetPlayers().Count(); i++)
		{
			PlayerBase player = GetPlayers()[i];
			player.SetBRPosition( position );
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetTopPosition", new Param1<int>( position ), true, player.GetIdentity() );
		}
    }

	//--- There were MessagePlayers() and MessagePlayer() here, the localise-on-the-server twins of the
	//--- four Untranslated helpers below. They called ExpansionNotification directly and had NO CALL
	//--- SITES ANYWHERE - every notification in the mod already went out through the Untranslated
	//--- pair, which ships a bare stringtable key over the mod's own NotificationMessage RPC and lets
	//--- the client localise it. They were deleted rather than ported, because porting them would
	//--- have meant keeping a second way to do the same thing, and the wrong one: localising server
	//--- side bakes in the SERVER's language, and Myst's own client runs in French.

	/*
	 * Send a message to all players, with standard message duration
	 * @param message The message to send
	 */
    void MessagePlayersUntranslated(string message, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		MessagePlayersUntranslatedTimed(message, DAYZBR_MSG_TIME, param1, param2, param3, param4, param5);
	}

	/*
	 * Send a message to all players, with custom message duration
	 * @param message The message to send
	 * @param time The time to display the message for
	 */
	void MessagePlayersUntranslatedTimed(string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		BattleRoyaleUtils.Info(string.Format("MessagePlayersUntranslated: %1", message));
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>( message, time, param1, param2, param3, param4, param5 ), true);
	}

    /*
	 * Send a message to a specific player, with standard message duration
	 * @param player The player to send the message to
	 * @param message The message to send
	 */
	void MessagePlayerUntranslated(PlayerBase player, string message, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		MessagePlayerUntranslatedTimed(player, message, DAYZBR_MSG_TIME, param1, param2, param3, param4, param5);
	}

	/*
	 * Send a message to a specific player, with custom message duration
	 * @param player The player to send the message to
	 * @param message The message to send
	 * @param time The time to display the message for
	 */
	void MessagePlayerUntranslatedTimed(PlayerBase player, string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
	    if(player)
	    {
	    	if(player.GetIdentity())
	    	{
	    		BattleRoyaleUtils.Info(string.Format("MessagePlayerUntranslated: %1 %2", player.GetIdentity().GetName(), message));
	    		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>( message, time, param1, param2, param3, param4, param5 ), true, player.GetIdentity());
	    	}
	    }
	}

    //--- Ordered cheapest test first. Spawn searches call this hundreds of times per player, so a
    //--- candidate that is in the sea must be thrown out before anything expensive runs. The zone
    //--- test used to come first even though it is the costliest check in here.
    protected bool IsSafeForTeleport(float x, float y, float z, bool check_zone = true)
    {
        // Avoid the sea
        if(GetGame().SurfaceIsSea(x, z))
            return false;

		// Avoid the ponds
        if(GetGame().SurfaceIsPond(x, z))
            return false;

    	// Check if in zone (if needed)
        if(check_zone)
        {
            if(!BattleRoyaleZone.GetZone(GetDynamicStartingZone(i_NumStartingPlayers)).IsInZone(x, z))
                return false;
        }

		// Avoid the roads
//        if(GetGame().SurfaceRoadY(x, z) != y)
//            return false;

		// Avoid namalsk ice (and others)
        array<string> bad_surface_types = {
            "nam_seaice",
            "nam_lakeice_ext"
        };

        string surface_type;
        GetGame().SurfaceGetType(x, z, surface_type);
        if(bad_surface_types.Find(surface_type) != -1)
            return false;

		// Avoid objects
        vector position;
        position[0] = x;
        position[1] = y;
        position[2] = z;

        vector start = position + Vector( 0, 10, 0 );
        vector end = position;
        float radius = 2.5;

        PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
        Object m_HitObject;
        vector m_HitPosition;
        vector m_HitNormal;
        float m_HitFraction;
        bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );

        if(m_Hit)
            return false;

        // New Geometry check
        array<Object> excludedObjects = new array<Object>();
        array<Object> collidedObjects = new array<Object>();
        vector box_position;
        box_position[0] = position[0];
        box_position[1] = position[1];
        box_position[2] = position[2];
        if( GetGame().IsBoxCollidingGeometry(box_position, "0 0 0", "2 10 2", ObjIntersectFire, ObjIntersectGeom, excludedObjects, collidedObjects) )
        {
            if( collidedObjects.Count() > 0)
            {
                BattleRoyaleUtils.Trace("New IsSafeForTeleport Geometry check is true !");
#ifdef BR_TRACE_ENABLED
                Print( collidedObjects );
#endif
                for (int i = 0; i < collidedObjects.Count(); ++i)
                {
                    string objectClass = collidedObjects.Get(i).GetType();
                    BattleRoyaleUtils.Trace( "objectClass: " + objectClass );
                }

                string text = "";
                foreach (Object object: collidedObjects)
                    text += " | " + Object.GetDebugName(object);
            }

			return false;
        }

        return true;
    }

    //--- Identity-safe name for logging. A PlayerBase can outlive its PlayerIdentity across a
    //--- coroutine yield or a deferred call, and a Trace argument is evaluated whatever the log
    //--- level - so an inlined GetIdentity().GetName() throws from inside a disabled log call and
    //--- takes the caller with it. Lives here rather than in one state because both the Prepare
    //--- coroutine and the unstuck path below need it.
    protected string GetPlayerLogName(PlayerBase player)
    {
        if(!player)
            return "<null player>";

        PlayerIdentity identity = player.GetIdentity();
        if(!identity)
            return "<null identity>";

        return BattleRoyaleNameService.ResolveIdentity(identity);
    }

    //--- Whether this state answers an F2 unstuck request at all. False by default, so Prepare, the
    //--- rounds and Win keep refusing exactly as they always have; only the lobby and the warm-up
    //--- opt in. The client sends the RPC unconditionally (BattleRoyaleClient.Unstuck), so this is
    //--- the only gate.
    bool AllowsUnstuck()
    {
        return false;
    }

    //--- Where an unstuck request may drop this player. Ring search outward from where they stand,
    //--- which is what keeps the teleport short enough to read as "shoved free" rather than as a
    //--- relocation. check_zone is off because an unstuck must work wherever the player currently
    //--- is - refusing to free somebody because they are already outside the circle is backwards.
    protected bool FindUnstuckPosition(PlayerBase player, out vector position)
    {
        position = "0 0 0";

        if(!player)
            return false;

        vector player_position = player.GetPosition();

        for(int search_pos = 0; search_pos < 50; search_pos++)
        {
            float radius = 10.0 + (search_pos * 0.5);
            float angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
            float x = player_position[0] + ( radius * Math.Cos(angle) );
            float z = player_position[2] + ( radius * Math.Sin(angle) );
            float y = GetGame().SurfaceY(x, z);

            if( !IsSafeForTeleport(x, y, z, false) )
                continue;

            position = Vector(x, y, z);
            return true;
        }

        return false;
    }

    //--- Cooldown gate, so F2 cannot be held down as free fast-travel. wait_unstuck alone is not
    //--- enough: it is cleared the moment the teleport lands, which was fine while unstuck only
    //--- existed during the warm-up but is not once the lobby - where players idle for minutes -
    //--- can answer it too.
    void DeferredUnstuck( PlayerBase player )
    {
        //--- Reached from an RPC handler, so the subject may already be gone.
        if( !player )
            return;

        if( player.wait_unstuck )
        {
            MessagePlayerUntranslated( player, "STR_BR_ALREADY_REQUESTED_UNSTUCK" );
            return;
        }

        if( GetGame().GetTickTime() < player.next_unstuck_time )
        {
            MessagePlayerUntranslated( player, "STR_BR_ALREADY_REQUESTED_UNSTUCK" );
            return;
        }

        //--- Note the cooldown is NOT started here. A request that finds nowhere to land refuses and
        //--- leaves the player exactly as stuck as they were, so charging them 30 s for it would
        //--- punish the one case where retrying immediately is the right thing to do. Unstuck()
        //--- starts the clock once a teleport has actually been sent.
        player.wait_unstuck = true;

        MessagePlayerUntranslated( player, "STR_BR_UNSTUCK_TELEPORTATION" );
        BattleRoyaleUtils.Trace( GetPlayerLogName( player ) + " asked for an unstuck teleportation." );
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "Unstuck", Math.RandomFloat(1, 3) * 1000 , false, new Param1<PlayerBase>( player ));
    }

    void Unstuck( PlayerBase player )
    {
        //--- 1-3 seconds have passed since DeferredUnstuck queued this, and a player can disconnect
        //--- inside that window. The failure branch below used to dereference GetIdentity() with no
        //--- guard on either, which was survivable while only the warm-up could reach it.
        if( !player )
            return;

        vector unstuck_position;
        if( !FindUnstuckPosition( player, unstuck_position ) )
        {
            BattleRoyaleUtils.Warn( GetPlayerLogName( player ) + " unstuck failed at " + player.GetPosition() );
            player.wait_unstuck = false;
            return;
        }

#ifdef DIAG_DEVELOPER
        //--- The one sample the juncture cannot give: what the command state was while the player
        //--- was still AT the ladder. If it reads Ladder here and not at "juncture", the command was
        //--- replaced underneath the animation, which is the case HumanCommandLadder.Exit exists for.
        player.BR_LogTeleportState("pre-unstuck");
#endif

        vector playerDir = vector.YawToVector( Math.RandomFloat(0, 360) );
        vector direction = Vector(playerDir[0], 0, playerDir[1]);

        ScriptJunctureData pCtx = new ScriptJunctureData;
        pCtx.Write( unstuck_position );
        pCtx.Write( direction );
        player.SendSyncJuncture( BR_SYNC_JUNCTURE_TELEPORT, pCtx );
        player.SetSynchDirty();
        player.wait_unstuck = false;

        //--- Charged only for a teleport that was actually sent - see the note in DeferredUnstuck.
        player.next_unstuck_time = GetGame().GetTickTime() + BR_UNSTUCK_COOLDOWN_SECONDS;
    }

    /**
     *  Put `player` at `position` down the same sync juncture the unstuck path uses.
     *
     *  NOT #ifdef-guarded, unlike BR_DiagTeleportRing below it: the COT master controls' "teleport
     *  to the live circle" admin action calls it, and that ships. It was inside the diag block until
     *  2026-08-22, which compiled on every local launch (the whole rig is DayZDiag_x64.exe, where
     *  DIAG_DEVELOPER is defined) and killed the Mission module outright on the first release server
     *  - "Undefined function 'BattleRoyaleState.BR_DiagTeleport'". Gate the CALLERS, never this.
     *
     *  Deliberately NOT a SetPosition. The juncture is what carries the teleport to the client half
     *  as well, and it is the code path the mod's own teleport bugs live in - a debug teleport that
     *  went around it would be testing something nobody ships.
     *
     *  The height is left alone: the juncture's server half applies BR_TELEPORT_DROP_HEIGHT itself,
     *  and adding it here as well would stack two seating epsilons.
     */
    void BR_DiagTeleport( PlayerBase player, vector position )
    {
        if( !player )
            return;

        position[1] = GetGame().SurfaceY( position[0], position[2] );

        vector playerDir = vector.YawToVector( Math.RandomFloat(0, 360) );
        vector direction = Vector(playerDir[0], 0, playerDir[1]);

        ScriptJunctureData pCtx = new ScriptJunctureData;
        pCtx.Write( position );
        pCtx.Write( direction );
        player.SendSyncJuncture( BR_SYNC_JUNCTURE_TELEPORT, pCtx );
        player.SetSynchDirty();

        BattleRoyaleUtils.Info( "[Diag] teleported " + GetPlayerLogName( player ) + " to " + position );
    }

#ifdef DIAG_DEVELOPER
    /**
     *  Put `player` exactly `radius` metres from `origin`, for the spectator range test.
     *
     *  The bearing is not the caller's business - only the distance is - so this walks a ring of
     *  candidate bearings and takes the first that survives IsSafeForTeleport, which already knows
     *  about sea, ponds and bad surfaces. Starting from the player's CURRENT bearing relative to
     *  origin keeps the teleport as short as it can be, so the camera's jump is the distance change
     *  and not a jump across the map as well.
     *
     *  Two passes. The first keeps check_zone, because a target dumped outside the circle starts
     *  taking zone damage immediately and a range test wants the target alive for the 20-30 s the
     *  measurement needs. The second drops it, since at 1200 m from a corpse there frequently IS no
     *  in-circle answer - that pass warns, because a target quietly bleeding out changes what the
     *  trace means.
     */
    bool BR_DiagTeleportRing( PlayerBase player, vector origin, float radius )
    {
        if( !player )
            return false;

        vector offset = player.GetPosition() - origin;
        offset[1] = 0;

        float start_yaw = 0;
        if( offset.LengthSq() > 1.0 )
            start_yaw = offset.VectorToAngles()[0];

        int step = 0;
        int pass = 0;
        //--- 24 bearings, 15 degrees apart. Enough to find a gap around a lake without making this a
        //--- search: the spawn code's hundreds-of-candidates approach is not warranted for a button.
        for( pass = 0; pass < 2; pass++ )
        {
            bool check_zone = ( pass == 0 );

            for( step = 0; step < 24; step++ )
            {
                float yaw = start_yaw + ( step * 15.0 );
                vector dir = vector.YawToVector( yaw );
                vector candidate = origin + ( Vector( dir[0], 0, dir[1] ) * radius );
                candidate[1] = GetGame().SurfaceY( candidate[0], candidate[2] );

                if( !IsSafeForTeleport( candidate[0], candidate[1], candidate[2], check_zone ) )
                    continue;

                if( !check_zone )
                    BattleRoyaleUtils.Warn( "[Diag] TP Target: no in-circle spot at " + radius + " m, using one OUTSIDE the zone - the target will take zone damage" );

                BR_DiagTeleport( player, candidate );
                return true;
            }
        }

        BattleRoyaleUtils.Warn( "[Diag] TP Target: no safe spot at " + radius + " m from " + origin + " on any of 24 bearings" );
        return false;
    }
#endif

    /**
     *  How long a whole match starting at zone `start_zone` would run, in seconds (#284 point 3).
     *
     *  Starting at zone Z plays rounds for zones Z..num_zones - i.e. settings indices num_zones-Z down
     *  to 0 - and then 7_BattleRoyaleLastRound plays out the final circle on static_timers[0]. Ahead of
     *  all of it sits 5_BattleRoyaleStartMatch's warm-up, round_duration_minutes / 2.
     *
     *  The `opening` flag is passed explicitly per candidate, which is what stops this and
     *  BattleRoyaleZone.GetZoneTimer calling each other forever - see that method's header.
     */
    static int GetMatchDurationSeconds(int start_zone)
    {
        BattleRoyaleConfig config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData game_settings;
        BattleRoyaleZoneData zone_settings;
        BattleRoyaleZone zone;
        int total = 0;
        int k;
        int num_zones;
        bool is_opening;

        if(!config)
            return 0;

        game_settings = config.GetGameData();
        zone_settings = config.GetZoneData();
        if(!game_settings || !zone_settings)
            return 0;

        num_zones = zone_settings.num_zones;

        //--- The warm-up, which is the same figure 5_BattleRoyaleStartMatch puts in i_FirstRoundDelay.
        total = (60 * game_settings.round_duration_minutes) / 2;

        for(k = start_zone; k <= num_zones; k++)
        {
            zone = BattleRoyaleZone.GetZone(k);
            if(!zone)
                continue;

            is_opening = (k == start_zone);
            total = total + zone.GetZoneTimer(is_opening);
        }

        //--- The endgame. 7_BattleRoyaleLastRound reads static_timers[0] directly rather than going
        //--- through GetZoneTimer, so this reads it the same way - the two must not be able to disagree.
        if(zone_settings.static_timers && zone_settings.static_timers.Count() > 0)
            total = total + zone_settings.static_timers[0];

        return total;
    }

    //--- How many tiers BoundMatchDuration moved the last answer by, signed. Written at its single
    //--- exit and read by RunLadderSelfTest on the very next line - a return value in all but name,
    //--- the same shape as BattleRoyaleSpectators.m_LastResolveTier, and to be read NOWHERE ELSE.
    //---
    //--- It exists because "the bound is quietly deciding every match on its own" is invisible from the
    //--- chosen tier alone: the answer looks like a normal tier, it is just not the one the player
    //--- count and the loot density asked for. That is the shape of failure this repo keeps shipping.
    static int s_LastBoundMove = 0;

    //--- Drop the memo. Only BattleRoyaleZone.RunLadderSelfTest needs this: it walks a range of player
    //--- counts at boot and must not leave the match about to be played reading its last answer.
    static void ResetDynamicZoneMemo()
    {
        i_DynamicZoneMemoPlayers = -1;
        i_DynamicZoneMemoResult = 1;
    }

	// Maybe this should be moved to another class, maybe not
    static int GetDynamicStartingZone(int num_players)
    {
		//--- Answer the memo before touching config. Same num_players always gives the same zone, so
		//--- the repeated calls from the spawn search cost nothing after the first.
		if ( num_players == i_DynamicZoneMemoPlayers )
			return i_DynamicZoneMemoResult;

		int resolved_zone = 1;  // Default to 1 if dynamic zones are not enabled

    	BattleRoyaleZoneData m_ZoneSettings = BattleRoyaleConfig.GetConfig().GetZoneData();
		if ( m_ZoneSettings.use_dynamic_zones )
		{
			// Return the first zone based on number of registered players
			int last_try_zone = 1;

			//--- Starting at zone Z plays zones Z..num_zones, i.e. (num_zones - Z + 1) of them.
			//--- Solving for min_zone_num gives Z = num_zones - min_zone_num + 1; the old test
			//--- omitted the +1 and so guaranteed one zone more than configured. Clamped so a
			//--- min_zone_num >= num_zones still means "play them all" rather than never matching.
			int floor_zone = Math.Max(1, m_ZoneSettings.num_zones - m_ZoneSettings.min_zone_num + 1);

			BattleRoyaleUtils.Trace("Number of players registered: " + num_players);
			for(int i_zone = 1; i_zone < m_ZoneSettings.num_zones; i_zone++)
			{
				BattleRoyaleUtils.Trace("Try zone: " + i_zone);
				last_try_zone = i_zone;

				//--- Resolved once: this used to be evaluated twice per iteration, once for the
				//--- trace and once for the test, and each call re-enters the zone registry.
				int zone_min_players = BattleRoyaleZone.GetZone(i_zone).GetZoneMinPlayers();
				BattleRoyaleUtils.Trace("Min player for zone: " + zone_min_players);
				if(zone_min_players < num_players)
				{
					BattleRoyaleUtils.Trace("It's a match! " + i_zone);
					break;
				}
				if(i_zone == floor_zone)
				{
					BattleRoyaleUtils.Trace("Reach the minimum! " + i_zone);
					break;
				}
				BattleRoyaleUtils.Trace("No chance we continue...");
			}

			resolved_zone = last_try_zone;
		}

		resolved_zone = BoundMatchDuration(resolved_zone, num_players);

		i_DynamicZoneMemoPlayers = num_players;
		i_DynamicZoneMemoResult = resolved_zone;

		return resolved_zone;
	}

    /**
     *  Move the starting tier until the whole match fits the configured duration window (#284 point 3).
     *
     *  The lever is the one the dynamic starting zone already pulls: a HIGHER zone number is a smaller
     *  opening circle and one fewer round, so raising it shortens the match and lowering it lengthens
     *  it. Nothing else changes - no radius, no timer, no geometry.
     *
     *  ⚠️ The walk is bounded on BOTH sides by things that are not the clock. It may not go below zone
     *  1, and it may not go above `floor_zone` (num_zones - min_zone_num + 1), because min_zone_num is
     *  an explicit "never play fewer than this many circles" and a duration target must not quietly
     *  override it. So the window is a preference, not a guarantee: when the ladder cannot reach it,
     *  the closest legal tier is used and the shortfall is logged rather than forced.
     */
    static int BoundMatchDuration(int candidate_zone, int num_players)
    {
        BattleRoyaleZoneData zone_settings = BattleRoyaleConfig.GetConfig().GetZoneData();
        int chosen;
        int floor_zone;
        int steps;
        int total;
        int target;
        int here;
        int there;
        string line;

        s_LastBoundMove = 0;

        if(!zone_settings || !zone_settings.bound_match_duration)
            return candidate_zone;

        floor_zone = Math.Max(1, zone_settings.num_zones - zone_settings.min_zone_num + 1);

        target = Math.Round(zone_settings.match_seconds_per_player * num_players);
        target = Math.Round(Math.Clamp(target, zone_settings.match_min_seconds, zone_settings.match_max_seconds));

        chosen = Math.Round(Math.Clamp(candidate_zone, 1, floor_zone));

        //--- ⚠️ STEP ONLY WHILE THE STEP GETS CLOSER TO THE TARGET. The obvious version - "step up
        //--- while total > target" - has a cliff, because a ladder cannot make a small correction: the
        //--- only move available is a WHOLE CIRCLE. Measured on a real chain at 43 players: zone 2 ran
        //--- 32:47 against a 32:15 target, so it stepped up to zone 3 at 20:48 - giving up TWELVE
        //--- MINUTES of match to save 32 seconds of overshoot, and landing 687 s from the target
        //--- instead of 32.
        //---
        //--- Comparing the distance either side makes a modest overshoot preferable to a huge
        //--- undershoot, which is what "bound the duration" was always supposed to mean. It also folds
        //--- the two directions into one rule: the window is already baked into `target` by the clamp
        //--- above, so moving toward the target IS moving toward the window, and the shorten and
        //--- lengthen walks can no longer disagree about a tier.
        for(steps = 0; steps < BR_MATCH_DURATION_MAX_STEPS; steps++)
        {
            if(chosen >= floor_zone)
                break;

            here = Math.AbsInt(GetMatchDurationSeconds(chosen) - target);
            there = Math.AbsInt(GetMatchDurationSeconds(chosen + 1) - target);
            if(there >= here)
                break;

            chosen++;
        }

        for(steps = 0; steps < BR_MATCH_DURATION_MAX_STEPS; steps++)
        {
            if(chosen <= 1)
                break;

            here = Math.AbsInt(GetMatchDurationSeconds(chosen) - target);
            there = Math.AbsInt(GetMatchDurationSeconds(chosen - 1) - target);
            if(there >= here)
                break;

            chosen--;
        }

        total = GetMatchDurationSeconds(chosen);
        s_LastBoundMove = chosen - candidate_zone;

        //--- Built in steps: six fields in one concatenation is past the expression complexity ceiling.
        line = "[BattleRoyaleState] match duration bound: " + num_players + " player(s), target " + target + " s";
        line = line + " [" + zone_settings.match_min_seconds + ".." + zone_settings.match_max_seconds + "]";
        line = line + " - starting zone " + candidate_zone + " -> " + chosen;
        line = line + " (" + total + " s, floor_zone " + floor_zone + ")";

        //--- Info only when it actually moved the tier. On the live path this runs once per match and
        //--- either level is fine, but BattleRoyaleZone.RunLadderSelfTest calls it once per player
        //--- count - a hundred Info lines saying "no change" would bury the summary it prints itself.
        if(chosen != candidate_zone)
            BattleRoyaleUtils.Info(line);
        else
            BattleRoyaleUtils.Debug(line);

        //--- The window is a preference, not a guarantee - see the header. Say so when the closest
        //--- legal tier still misses it, because a silently missed target is indistinguishable from a
        //--- bound that never ran.
        if(total > zone_settings.match_max_seconds)
            BattleRoyaleUtils.Warn("[BattleRoyaleState] the closest legal tier still runs " + total + " s, above match_max_seconds (" + zone_settings.match_max_seconds + "). Lower min_zone_num, or shorten static_timers.");

        if(total < zone_settings.match_min_seconds)
            BattleRoyaleUtils.Warn("[BattleRoyaleState] the closest legal tier only runs " + total + " s, below match_min_seconds (" + zone_settings.match_min_seconds + "). Raise num_zones, or lengthen static_timers.");

        return chosen;
    }

	void OnPlayerDisconnected(PlayerBase player)
	{
		if(ContainsPlayer( player ))
		{
			RemovePlayer( player );
		}
		else
		{
			Error("Unknown player disconnected! Not in current state?");
		}

		BattleRoyaleServerData m_ServerData = BattleRoyaleConfig.GetConfig().GetServerData();

		if ( m_ServerData.enable_vigrid_api )
		{
			BattleRoyaleUtils.Trace("ScoreWebhook: Sending player score");
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
			ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
			scoreWebhook.Send( br_instance.match_uuid, player.player_steamid, player.GetBRPosition() );
		}
	}

	void OnPlayerKilled( PlayerBase player, Object source )
	{
		if( ContainsPlayer( player ) )
		{
			RemovePlayer( player );
		}
		else
		{
			Error("Unknown player killed! Not in current state?");
		}

		if (!player || !source)
		{
	        BattleRoyaleUtils.Trace("DEBUG: PlayerKilled() player/source does not exist");
	        return;
    	}

		if( player.GetIdentity() )
		{
			map<string, string> json_data = new map<string, string>();
			json_data.Insert( "victim", player.GetIdentity().GetPlainId() );
			json_data.Insert( "victim_position", player.GetPosition().ToString() );
			vector killer_position = "0 0 0";

			BattleRoyaleServerData m_ServerData = BattleRoyaleConfig.GetConfig().GetServerData();
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();

			if ( m_ServerData.enable_vigrid_api )
			{
				BattleRoyaleUtils.Trace("ScoreWebhook: Sending player score");
				ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
				scoreWebhook.Send( br_instance.match_uuid, player.GetIdentity().GetPlainId(), player.GetBRPosition() );
			}

			//--- One resolver for every consumer - see BattleRoyaleKillAttribution. It null-checks the
			//--- EntityAI cast, which the two-line version this replaced did not: a source that is not
			//--- an EntityAI (a building, a vehicle part) dereferenced NULL.
			PlayerBase playerSource = BattleRoyaleKillAttribution.ResolvePlayerSource( source );

			//--- The uid is what gets CREDITED, and it is deliberately not derived from playerSource:
			//--- for a grenade or a trap the responsible player may be dead or gone, so only the uid
			//--- the device recorded at arm time still exists.
			string killer_uid = BattleRoyaleKillAttribution.ResolveKillerUid( player, source );

			if (player == source)	// deaths not caused by another object (starvation, dehydration)
			{
				// Killed by environmental causes but the the player directly
				json_data.Insert( "killer", "environment" );
			}
			else if ( BattleRoyaleKillAttribution.IsProxyDevice( source ) )
			{
				//--- A grenade, a mine, a claymore, an IED, a placed charge. Covers every vanilla
				//--- explosive and trap now, where this used to name only two concrete classes.
				json_data.Insert( "killer", killer_uid );
				json_data.Insert( "weapon", source.GetType() );
				json_data.Insert( "killer_position", source.GetPosition().ToString() );
			}
			else {
				json_data.Insert( "killer_position", source.GetPosition().ToString() );

				if (playerSource)
				{
					json_data.Insert( "killer", killer_uid );
					if (source.IsWeapon() || source.IsMeleeWeapon())
					{
						json_data.Insert( "weapon", source.GetType() );
						if ( !source.IsMeleeWeapon() )
						{
							json_data.Insert( "distance", vector.Distance( player.GetPosition(), playerSource.GetPosition() ).ToString() );
						}
					}
					else
					{
						json_data.Insert( "weapon", "fist" );
					}
				}
				else
				{
					//rest, Animals, Zombies
					json_data.Insert( "killer", source.GetType() );
				}
			}

			//--- The single credit point, deliberately outside the branches above: a grenade kill used
			//--- to fall out of its branch without ever scoring, so no explosive kill has ever reached
			//--- the HUD counter, the spectator overlay tags or the ladder. CreditKill is a no-op on an
			//--- empty uid, which is what an environmental or animal death resolves to.
			BattleRoyaleKillLedger.GetInstance().CreditKill( killer_uid );

			if ( m_ServerData.enable_vigrid_api )
			{
				EventWebhook eventWebhook = new EventWebhook( m_ServerData.webhook_jwt_token );
				eventWebhook.Send( br_instance.match_uuid, "player.kill", JsonFileLoader<map<string, string>>.JsonMakeData( json_data ) );
			}
		}
	}
}

// Base state for the Debug Zone.
// This handles healing / godmode / and teleporting
class BattleRoyaleDebugState: BattleRoyaleState
{
    protected vector v_Center;
    protected float f_Radius;
    protected int i_HealTickTime;
    protected ref array<string> a_AdminsList;

    void BattleRoyaleDebugState()
    {
        BattleRoyaleSpawnsData m_SpawnsSettings = BattleRoyaleConfig.GetConfig().GetSpawnsData();
        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        BattleRoyaleLobbyData m_LobbySettings = BattleRoyaleConfig.GetConfig().GetLobbyData();

        if(m_SpawnsSettings && m_GameSettings)
        {
            v_Center = m_SpawnsSettings.spawn_point;
            f_Radius = m_SpawnsSettings.radius;
            a_AdminsList = m_GameSettings.admins_steamid64;
        }
        else
        {
            Error("DEBUG SETTINGS IS NULL!");
            GetGame().RequestExit(0);  // Exit the game
        }

        if(m_LobbySettings)
        {
            i_HealTickTime = m_LobbySettings.debug_heal_tick_seconds;
        }
        else
        {
            Error("LOBBY SETTINGS IS NULL");
            i_HealTickTime = DAYZBR_DEBUG_HEAL_TICK;
        }
    }

    override void Activate()
    {
        //Note: this is called AFTER players are added
        b_IsDebug = true;
        super.Activate();
    }

    override string GetName()
    {
        return "Unknown Debug State";
    }

    /*
    override void AddPlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(false); //all players in this state are god mode
            player.Heal();
        }

        super.AddPlayer(player);
    }

    //--- Index loop with a null check, not a foreach. The only caller (BattleRoyaleServer.Update)
    //--- already walks the result this way and logs "null player in RemoveAllPlayers result!", so
    //--- nulls demonstrably occur. super.RemoveAllPlayers() has already cleared m_Players by the
    //--- time this runs, so a throw here loses the whole roster mid-migration: the next state
    //--- activates empty and OnPlayerTick force-logs-out every survivor.
    override ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> players = super.RemoveAllPlayers();
        for(int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = players[i];
            if(!player)
                continue;

            player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }
        return players;
    }
    
    override void RemovePlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }
        super.RemovePlayer(player);
    }
    */

    //--- debug states must lock players into the debug zone & heal them
    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        super.OnPlayerTick(player, timeslice);

		// Try to do only a 2D distance between points and not 3D (to avoid the teleport bug on Takistan)
        vector player_position = player.GetPosition();
        float distance = vector.Distance( Vector( player_position[0], 0, player_position[2] ), Vector( v_Center[0], 0, v_Center[2] ) );

        if(distance > f_Radius && !CanGoOutsideLobby(player))
        {
			vector spawn_pos = "0 0 0";
			spawn_pos[0] = Math.RandomFloatInclusive((v_Center[0] - 5), (v_Center[0] + 5));
			spawn_pos[2] = Math.RandomFloatInclusive((v_Center[2] - 5), (v_Center[2] + 5));
			spawn_pos[1] = GetGame().SurfaceY(spawn_pos[0], spawn_pos[2]);

            player.SetPosition(spawn_pos);

			float dir = Math.RandomFloat(0, 360);
			vector playerDir = vector.YawToVector(dir);
			player.SetDirection( Vector(playerDir[0], 0, playerDir[1]) );
        }

        if(player.time_until_heal <= 0)
        {
            player.time_until_heal = i_HealTickTime;
            player.Heal();
        }
        player.time_until_heal -= timeslice;
    }

    bool CanGoOutsideLobby(PlayerBase player)
    {
        if(!player)
        {
            Error("Null player in CanGoOutsideLobby");
            return false;
        }
        PlayerIdentity identity = player.GetIdentity();

        if(!identity)
            return false;

        string steamid = identity.GetPlainId();
        if(steamid == "")
        {
            Error("Blank SteamId from identity!");
            return false;
        }

        return (a_AdminsList.Find(steamid) != -1);
    }

    //--- Same ring search as the base state, but no candidate outside the lobby disc is allowed, and
    //--- there is always an answer.
    //---
    //--- Two reasons the clamp is here rather than left to OnPlayerTick's containment above. It
    //--- would catch an escapee anyway, but only by yanking them back to the centre a frame later -
    //--- a visible double teleport. More importantly it exempts admins (CanGoOutsideLobby), so for
    //--- an admin nothing would catch it at all and an unstuck could quietly drop them into the
    //--- world mid-lobby. Making "cannot leave the lobby" a property of this method covers both.
    //---
    //--- 2D distance, matching OnPlayerTick and for the same reason its comment gives.
    override protected bool FindUnstuckPosition(PlayerBase player, out vector position)
    {
        position = "0 0 0";

        if(!player)
            return false;

        vector player_position = player.GetPosition();

        for(int search_pos = 0; search_pos < 50; search_pos++)
        {
            float radius = 10.0 + (search_pos * 0.5);
            float angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
            float x = player_position[0] + ( radius * Math.Cos(angle) );
            float z = player_position[2] + ( radius * Math.Sin(angle) );
            float y = GetGame().SurfaceY(x, z);

            //--- The ring grows to 34.5 m, so on a small lobby most of it lies outside. Reject
            //--- before the expensive geometry test in IsSafeForTeleport, not after.
            if( vector.Distance( Vector( x, 0, z ), Vector( v_Center[0], 0, v_Center[2] ) ) > f_Radius )
                continue;

            if( !IsSafeForTeleport(x, y, z, false) )
                continue;

            position = Vector(x, y, z);
            return true;
        }

        //--- Nothing safe inside the disc. Fall back to the placement OnPlayerTick already uses to
        //--- put an escapee back, so a lobby unstuck never fails: being shoved to the middle of the
        //--- lobby is the correct outcome for a player who cannot otherwise move.
        //---
        //--- Vetted first, though, which it never used to be. This is where a lobby unstuck almost
        //--- always lands - the ring search above only tends to succeed on a wide open lobby - and
        //--- the position was taken raw from SurfaceY with no geometry test of any kind. A centre
        //--- that happens to sit under a prop seats the capsule inside it, and the player is freed
        //--- from being stuck into being stuck. That is the fault BR_TELEPORT_DROP_HEIGHT spent a
        //--- metre of clearance hiding.
        float centre_x = v_Center[0];
        float centre_z = v_Center[2];
        float centre_y = GetGame().SurfaceY(centre_x, centre_z);

        for(int centre_try = 0; centre_try < 10; centre_try++)
        {
            centre_x = Math.RandomFloatInclusive((v_Center[0] - 5), (v_Center[0] + 5));
            centre_z = Math.RandomFloatInclusive((v_Center[2] - 5), (v_Center[2] + 5));
            centre_y = GetGame().SurfaceY(centre_x, centre_z);

            if( !IsSafeForTeleport(centre_x, centre_y, centre_z, false) )
                continue;

            BattleRoyaleUtils.Trace("No safe unstuck spot inside the lobby, falling back to a vetted position near the centre.");

            position = Vector(centre_x, centre_y, centre_z);
            return true;
        }

        //--- Even the centre is obstructed. Land there regardless: refusing outright would leave a
        //--- genuinely stuck player with no way out at all, and that is the worse of the two.
        BattleRoyaleUtils.Warn("No safe unstuck spot anywhere in the lobby, including around the centre. Using the last centre candidate unvetted.");

        position = Vector(centre_x, centre_y, centre_z);
        return true;
    }

    //TODO:
    //DEPRECATED: should pull value from config
    //GOAL: make this protected
    vector GetCenter()
    {
        v_Center[1] = GetGame().SurfaceY(v_Center[0], v_Center[2]);
        return v_Center;
    }

    //DEPRECATED: should pull value from config
    //GOAL: make this protected
    float GetRadius()
    {
        return f_Radius;
    }
}
