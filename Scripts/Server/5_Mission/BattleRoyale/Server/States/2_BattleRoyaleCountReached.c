#ifdef SERVER
class BattleRoyaleCountReached: BattleRoyaleDebugState
{
    protected int i_TimeToStart;
    protected ref Timer m_StartTimer;

    void BattleRoyaleCountReached()
    {
        BattleRoyaleLobbyData m_LobbySettings = BattleRoyaleConfig.GetConfig().GetLobbyData();
		i_TimeToStart = m_LobbySettings.time_to_start_match_seconds;
    }

    override string GetName()
    {
        return "Player Count Reached State";
    }

    override void Activate()
    {
    	BattleRoyaleUtils.Debug(string.Format("BattleRoyaleCountReached: Activating with time to start: %1 seconds", i_TimeToStart));
        super.Activate();

    	BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleServerData m_ServerData = m_Config.GetServerData();
		if ( m_ServerData.enable_vigrid_api )
		{
			// Lock the server via webhook
			LockServerWebhook serverWebhook = new LockServerWebhook( m_ServerData.webhook_jwt_token );
			serverWebhook.LockServer();
		}

		if ( m_ServerData.use_autolock && m_ServerData.autolock_url != "" && m_ServerData.autolock_ip != "" && m_ServerData.autolock_port > 0 && m_ServerData.autolock_rcon_password != "" )
		{
			AutoLockWebhook autoLockWebhook = new AutoLockWebhook( m_ServerData.autolock_url, m_ServerData.autolock_ip, m_ServerData.autolock_port, m_ServerData.autolock_rcon_password );
			autoLockWebhook.LockServer();
		}

        //--- Two whole message keys rather than one key plus the word as %2. The word used to be
        //--- built here in English and substituted into a LOCALISED sentence, so every non-English
        //--- client read "Матч начнется через 30 seconds !". A word cannot be localised in isolation
        //--- anyway - the plural form is a property of the whole sentence in most of the fourteen
        //--- locales, and in Hungarian the numeral takes the singular.
        string announcement = "STR_BR_ANNOUNCEMENT_PLAYERCOUNTREACHED";
        if ( i_TimeToStart == 1 )
            announcement = "STR_BR_ANNOUNCEMENT_PLAYERCOUNTREACHED_ONE";

        // Track the number of players at start
        i_NumStartingPlayers = m_Players.Count();

        //--- Restrict voice to party members for the whole frozen window (this state, spawn
        //--- selection and prepare). The server is locked from here, so the roster is fixed and the
        //--- matrix never needs rebuilding. Cleared again in 5_BattleRoyaleStartMatch.
        BattleRoyaleVoice.ApplyPartyOnly( GetPlayers() );

        MessagePlayersUntranslated(announcement, i_TimeToStart.ToString());
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
        //--- Deferred: this is a timer callback, i.e. inside TimerQueue.Tick. See
        //--- BattleRoyaleState.DeactivateDeferred().
        DeactivateDeferred();
    }
}
