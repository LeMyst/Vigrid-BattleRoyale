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
#ifdef Carim
        if( IsActive() && !b_UseVoteSystem && GetPlayers().Count() >= i_MinPlayers && GetGame().GetTickTime() >= f_MinWaitingTime && GetGroupsCount() > 1 )
#else
        if( IsActive() && !b_UseVoteSystem && GetPlayers().Count() >= i_MinPlayers && GetGame().GetTickTime() >= f_MinWaitingTime )
#endif
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

        super.Activate();
    }

    void CheckReadyState()
    {
		if( GetGame().GetTickTime() >= f_MinWaitingTime && GetPlayers().Count() > 1 )
		{
			if( IsActive() && IsVoteReady() )
				Deactivate();

			int t_MaxPlayers = GetGame().ServerConfigGetInt( "maxPlayers" );
			if( b_AutoStartGame && GetReadyCount() > 1 && GetReadyCount() >= ( t_MaxPlayers - ( ( t_MaxPlayers * ( GetGame().GetTickTime() - i_FirstPlayerTick ) ) / f_AutoStartDelay ) ) )
				Deactivate();
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

		if( player_count <= i_MinPlayers ) // need more than the minimum player
			return false;

#ifdef Carim
		if( GetGroupsCount() <= 1 ) // need more than one group
			return false;
#endif

        float percent = (ready_count / player_count);
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
