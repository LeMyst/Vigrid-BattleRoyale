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

    void BattleRoyaleDebug()
    {
        m_ReadyList = new array<PlayerBase>();

        BattleRoyaleLobbyData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();

		i_MinPlayers = m_DebugSettings.minimum_players;
		i_TimeBetweenMessages = 45;
		b_UseVoteSystem = (m_DebugSettings.use_ready_up == 1);
		f_VoteThreshold = m_DebugSettings.ready_up_percent;
		f_MinWaitingTime = m_DebugSettings.min_waiting_time;
		b_AutoStartGame = m_DebugSettings.autostart_enabled;
		f_AutoStartDelay = m_DebugSettings.autostart_delay;
    }

    override string GetName()
    {
        return "Debug Zone State";
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

        super.Activate();
    }

    override void Deactivate()
    {
        super.Deactivate();

#ifdef VIGRID_PARTY
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
