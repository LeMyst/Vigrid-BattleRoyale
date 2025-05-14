#ifdef SERVER
class BattleRoyaleCountReached: BattleRoyaleDebugState
{
    protected int i_TimeToStart;
    protected ref Timer m_StartTimer;

    void BattleRoyaleCountReached()
    {
        BattleRoyaleLobbyData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();
		i_TimeToStart = m_DebugSettings.time_to_start_match_seconds;
    }

    override string GetName()
    {
        return "Player Count Reached State";
    }

    override void Activate()
    {
        super.Activate();

        string second = "seconds";
        if ( i_TimeToStart == 1 )
            second = "second";

        // Track the number of players at start
        i_NumStartingPlayers = m_Players.Count();

        MessagePlayersUntranslated("STR_BR_ANNOUNCEMENT_PLAYERCOUNTREACHED", i_TimeToStart.ToString(), second);
        m_StartTimer = AddTimer(i_TimeToStart, this, "DoStart", NULL, false);
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input on all clients (we'll do this on the server in another thread)
    }

    override void Deactivate()
    {
        if ( m_StartTimer && m_StartTimer.IsRunning() )
        {
            m_StartTimer.Stop();
        }
        
        super.Deactivate();
    }

    void DoStart()
    {
        Deactivate();
    }
}
