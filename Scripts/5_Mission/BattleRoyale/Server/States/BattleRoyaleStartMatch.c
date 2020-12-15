class BattleRoyaleStartMatch extends BattleRoyaleState
{
    protected int i_TimeToUnlock;
    protected bool b_IsGameplay;
    protected int i_FirstRoundDelay;
    
    protected ref array<PlayerBase> m_PlayerList;

    protected ref array<ref Timer> m_MessageTimers;
    protected ref Timer m_UnlockTimer;
    protected ref Timer m_ZoneStartTimer;

    void BattleRoyaleStartMatch()
    {
        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
		BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
		i_FirstRoundDelay = (60 * m_GameSettings.round_duration_minutes) / 2;

        //seconds until unlock
        i_TimeToUnlock = m_GameSettings.time_until_teleport_unlock;

        b_IsGameplay = false;
        
        m_PlayerList = new array<PlayerBase>;

        m_MessageTimers = new array<ref Timer>;
    }
    override string GetName()
	{
		return DAYZBR_SM_START_MATCH_NAME;
	}
	override void Activate()
	{
		super.Activate();

        //send start match RPC (this will enable UI such as kill count)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "StartMatch", new Param1<bool>(true), true); //don't need a param, but id rather keep it just so i know nothing wierd occurs (eventually find out if we can remove it)

        int max_time = i_TimeToUnlock - 1;
        for(int i = max_time;i > 0;i--)
        {
            m_MessageTimers.Insert( AddTimer(i, this, "MessageUnlock", new Param1<int>(i_TimeToUnlock - i), false) );
        }

        m_UnlockTimer = AddTimer(i_TimeToUnlock, this, "UnlockPlayers", NULL, false);

        m_ZoneStartTimer = AddTimer( i_FirstRoundDelay, this, "StartZoning", NULL, false);

        //timer before first zone appears
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>(i_FirstRoundDelay), true); 
	}
	override void Deactivate()
	{
        //deactivate all one-time timers
        m_UnlockTimer.Stop();
        m_ZoneStartTimer.Stop();
        for(int i = 0; i < m_MessageTimers.Count(); i++)
            m_MessageTimers[i].Stop();

		super.Deactivate();
	}
    
	override bool IsComplete()
	{
        if(GetPlayers().Count() <= 1 && IsActive())
		{
			//clean call queue?
			Deactivate();
		}
		return super.IsComplete();
	}

    //TODO: add this to battleroyaleconstants and use string replace to insert seconds_till
    void MessageUnlock(int seconds_till)
    {
        string second = "second";
        if(seconds_till != 1)
            second = "seconds";

        MessagePlayers("Starting in " + seconds_till.ToString() + " " + second + "!");
    }
    void UnlockPlayers()
    {
        m_PlayerList.InsertAll( m_Players );

        //enable player input on clients (we'll do this on server in another thread)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(false), true);

        GetGame().GameScript.Call(this, "HandleUnlock", NULL); //spin up unlocking thread
        
        BattleRoyaleServer.Cast(GetBR()).GetLootSystem().Start(); //start the loot system
        BattleRoyaleServer.Cast(GetBR()).GetVehicleSystem().Start();  //start spawning vehicles
    }

    void HandleUnlock()
    {
        for(int i = 0; i < m_PlayerList.Count(); i++)
        {
            PlayerBase player = m_PlayerList[i];

            player.DisableInput(false); //This will re-enable input
        }
        
        MessagePlayers( DAYZBR_MSG_MATCH_STARTED );

        BattleRoyaleServer.Cast( GetBR() ).GetMatchData().SetStart(GetGame().GetTime()); //match start time logging
        b_IsGameplay = true;
    }

    void StartZoning()
    {
        Deactivate();
    }

    void OnPlayerKilled(PlayerBase player, Object killer)
	{
        if(!b_IsGameplay)
        {
            Error("Player killed before gameplay!");
            return;
        }
        if(ContainsPlayer(player))
		{
			RemovePlayer(player);
		}
        /*
		if(player.GetIdentity())
			GetGame().DisconnectPlayer(player.GetIdentity()); //TODO: delay this disconnect (perhaps do it through the BattleRoyaleServer object's call queue)
		else
			Error("FAILED TO GET KILLED PLAYER IDENTITY!");
        */
	}
}