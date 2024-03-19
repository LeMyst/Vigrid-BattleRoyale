#ifdef SERVER
class BattleRoyaleDebug: BattleRoyaleDebugState
{
    protected ref array<PlayerBase> m_ReadyList;

    protected int i_MinPlayers;
    protected int i_TimeBetweenMessages;
    protected bool b_UseVoteSystem;
    protected float f_VoteThreshold;
    protected float f_MinWaitingTime;

    void BattleRoyaleDebug()
    {
        m_ReadyList = new array<PlayerBase>();

        BattleRoyaleDebugData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();

		i_MinPlayers = m_DebugSettings.minimum_players;
		i_TimeBetweenMessages = 45;
		b_UseVoteSystem = (m_DebugSettings.use_ready_up == 1);
		f_VoteThreshold = m_DebugSettings.ready_up_percent;
		f_MinWaitingTime = m_DebugSettings.min_waiting_time;
    }

    override string GetName()
    {
        return DAYZBR_SM_DEBUG_ZONE_NAME;
    }

    //returns true when this state is complete
    override bool IsComplete()
    {
        if( IsActive() && !b_UseVoteSystem && GetPlayers().Count() >= i_MinPlayers && GetGroups().Count() > 1 && GetGame().GetTickTime() >= f_MinWaitingTime )
        {
            Deactivate();
        }
        
        return super.IsComplete();
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
        if( IsActive() && IsVoteReady() && GetGame().GetTickTime() >= f_MinWaitingTime )
            Deactivate();
    }

    void MessageWaiting()
    {
        MessagePlayers(GetWaitingMessage());
    }

    string GetWaitingMessage()
    {
        int waiting_on_count = i_MinPlayers - GetPlayers().Count();
        string message = "";

        if( waiting_on_count > 0)
        {
            //TODO: add this to battleroyaleconstants & use string replace for count & plural
            message = "Waiting for " + waiting_on_count.ToString() + " more ";
            if(waiting_on_count > 1)
                message += "players";
            else
                message += "player";

            message += " to connect.";
        }

        if( b_UseVoteSystem )
        {
            int ready_count = GetReadyCount();

            if( message != "" )
                message += " ";

            message += ready_count.ToString() + " player";
            if(ready_count > 1)
                message += "s";

            message += " are ready!";

            string key_name = InputUtils.GetButtonNameFromInput("UADayZBRReadyUp", EInputDeviceType.MOUSE_AND_KEYBOARD);
            if(key_name == "")
                message += " (Define a button in the options to ready up)";
            else
                message += " (Press " + key_name + " (default) to ready up)";

			if( GetGame().GetTickTime() < f_MinWaitingTime )
			{
				message += ". ";

				int seconds_left = Math.Ceil( f_MinWaitingTime - GetGame().GetTickTime() );

				if( !IsVoteReady() )
					message += "The game cannot start before " + seconds_left + " seconds.";
				else
					message += "The game will automatically start in " + seconds_left + " seconds.";
			}
        }

        return message;
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

        if( !BATTLEROYALE_SOLO_GAME )
        {
            if( player_count <= 1 ) // need more than 1 player
                return false;

            if( player_count <= i_MinPlayers ) // need more than the minimum player
                return false;

            if( GetGroups().Count() <= 1 ) // need more than one group
            	return false;
        }

        float percent = (ready_count / player_count);
        return (percent >= f_VoteThreshold);
    }

    void ReadyUp(PlayerBase player)
    {
        if(m_ReadyList.Find(player) != -1)
        {
            MessagePlayer(player, "You have already readied up...");
            return;
        }

        MessagePlayer(player, "You have readied up!");
        m_ReadyList.Insert( player );

        //this is here because we don't want someone mass spamming all players by spamming F1
        int count = GetReadyCount();
        int max = GetPlayers().Count();
        MessagePlayers("Player readied up. (" + count.ToString() + "/" + max.ToString() + " players)");

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
