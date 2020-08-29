#define BR_BETA_LOGGING

class BattleRoyaleStartMatch extends BattleRoyaleState
{
    protected int i_TimeToUnlock;
    protected bool b_IsGameplay;
    protected int i_FirstRoundDelay;
    
    protected ref array<ref PlayerBase> m_PlayerList;

    void BattleRoyaleStartMatch()
    {
        #ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleStartMatch::Constructor()");
		#endif

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
		BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
		i_FirstRoundDelay = (60 * m_GameSettings.round_duration_minutes) / 2;

        //seconds until unlock
        i_TimeToUnlock = m_GameSettings.time_until_teleport_unlock;

        b_IsGameplay = false;
        
        m_PlayerList = new array<ref PlayerBase>;
    }
    override string GetName()
	{
		return "Start Match State";
	}
	override void Activate()
	{
		super.Activate();

        int max_time = i_TimeToUnlock - 1;
        for(int i = max_time;i > 0;i--)
        {
            m_CallQueue.CallLater(this.MessageUnlock, i*1000, false, i_TimeToUnlock - i); //30 seconds until zone locks
        }
        m_CallQueue.CallLater(this.UnlockPlayers, i_TimeToUnlock*1000, false); //delay before we unlock player input
        
        m_CallQueue.CallLater(this.StartZoning, i_FirstRoundDelay*1000, false); //delay before first zone appears
	}
	override void Deactivate()
	{
		super.Deactivate();
	}
    
	override bool IsComplete()
	{
        if(GetPlayers().Count() <= 1 && IsActive())
		{
			//clean call queue?
            m_CallQueue.Remove(this.StartZoning);
            m_CallQueue.Remove(this.UnlockPlayers);
            m_CallQueue.Remove(this.MessageUnlock);
			Deactivate();
		}
		return super.IsComplete();
	}

    void MessageUnlock(int seconds_till)
    {
        string second = "second";
        if(seconds_till != 1)
            second = "seconds";

        MessagePlayers("Starting in " + seconds_till.ToString() + " " + second + "!");
    }
    void UnlockPlayers()
    {
        foreach(PlayerBase player : m_Players)
        {
            m_PlayerList.Insert(player);
        }

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
            ref PlayerBase player = m_PlayerList[i];

            player.DisableInput(false); //This will re-enable input
        }
        
        MessagePlayers("The match has started!");
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