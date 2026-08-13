#ifdef SERVER
class BattleRoyaleDebug: BattleRoyaleDebugState
{
    protected ref array<PlayerBase> m_ReadyList;

    protected int i_MinPlayers;
    protected int i_TimeBetweenMessages;
    protected bool b_UseVoteSystem;
    protected float f_VoteThreshold;
    protected float f_MinWaitingTime;
    protected float f_AutoStartDelay;
    protected bool b_AutoStartGame;
    protected int i_FirstPlayerTick;
    protected int i_MinPartySize;
    protected int i_MinPartyRemainder;
    protected int i_AutoGroupSelfTest;

    void BattleRoyaleDebug()
    {
        m_ReadyList = new array<PlayerBase>();

        BattleRoyaleLobbyData m_LobbySettings = BattleRoyaleConfig.GetConfig().GetLobbyData();

		i_MinPlayers = m_LobbySettings.minimum_players;
		i_TimeBetweenMessages = 45;
		b_UseVoteSystem = (m_LobbySettings.use_ready_up == 1);
		f_VoteThreshold = m_LobbySettings.ready_up_percent;
		f_MinWaitingTime = m_LobbySettings.min_waiting_time;
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

    //returns true when this state is complete
    override bool IsComplete()
    {
        //--- Hoisted into a local because EnfusionScript has no multi-line if conditions, so the
        //--- guarded term cannot simply be appended to the condition below.
        bool enough_groups = true;
#ifdef VIGRID_PARTY
        enough_groups = VigridPartyAPI.GetGroupCount( GetPlayers() ) > 1;
#endif

        if( IsActive() && !b_UseVoteSystem && GetPlayers().Count() >= i_MinPlayers && GetGame().GetTickTime() >= f_MinWaitingTime && enough_groups )
        {
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
			//--- Deferred: this runs from a looping timer, i.e. inside TimerQueue.Tick. See
			//--- BattleRoyaleState.DeactivateDeferred().
			if( IsActive() && IsVoteReady() )
				DeactivateDeferred();

			int t_MaxPlayers = GetGame().ServerConfigGetInt( "maxPlayers" );
			if( b_AutoStartGame && GetReadyCount() > 1 && GetReadyCount() >= ( t_MaxPlayers - ( ( t_MaxPlayers * ( GetGame().GetTickTime() - i_FirstPlayerTick ) ) / f_AutoStartDelay ) ) )
				DeactivateDeferred();
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
		int waiting_on_count = i_MinPlayers - GetPlayers().Count();

		if( waiting_on_count > 0)
		{
			if( waiting_on_count == 1 )
				MessagePlayersUntranslated("STR_BR_WAITING_FOR_PLAYER");
			else
				MessagePlayersUntranslated("STR_BR_WAITING_FOR_PLAYERS", waiting_on_count.ToString());
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

		//--- At least the minimum, matching the non-vote path in IsComplete() which starts on
		//--- `Count() >= i_MinPlayers`. This used to demand strictly more, so a lobby sitting at
		//--- exactly minimum_players could never vote-start however many players readied up.
		if( player_count < i_MinPlayers )
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
