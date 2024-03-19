#ifdef SERVER
class BattleRoyaleStartMatch: BattleRoyaleState
{
    protected int i_TimeToUnlock;
    protected bool b_IsGameplay;
    protected int i_FirstRoundDelay;
    protected bool b_ShowFirstZone;
    protected bool b_ArtillerySound;

    protected ref array<PlayerBase> m_PlayerList;

    protected ref array<ref Timer> m_MessageTimers;
    protected ref Timer m_UnlockTimer;
    protected ref Timer m_ShowFirstZone;
    protected ref Timer m_ZoneStartTimer;

    void BattleRoyaleStartMatch()
    {
        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();

        i_FirstRoundDelay = (60 * m_GameSettings.round_duration_minutes) / 2;

        //seconds until unlock
        #ifdef DIAG
        i_TimeToUnlock = 1;
        #else
        i_TimeToUnlock = m_GameSettings.time_until_teleport_unlock;
        #endif

        b_ShowFirstZone = m_GameSettings.show_first_zone_at_start;

        b_ArtillerySound = m_GameSettings.artillery_sound;

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
        for(int i = max_time; i > 0; i--)
        {
            m_MessageTimers.Insert( AddTimer(i, this, "MessageUnlock", new Param1<int>(i_TimeToUnlock - i), false) );
        }

        m_UnlockTimer = AddTimer(i_TimeToUnlock, this, "UnlockPlayers", NULL, false);

        if (b_ShowFirstZone)
            m_ShowFirstZone = AddTimer(i_TimeToUnlock, this, "ShowFirstZone", NULL, false);

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
        if(GetPlayers().Count() <= 1 && IsActive() && !BATTLEROYALE_SOLO_GAME)
        {
            Print(GetName() + " IsComplete!");
            // TODO: clean call queue?
            // TODO: toggle to debug game
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
    }

    void ShowFirstZone()
    {
        // Show first circle
        Print("[BattleRoyaleStartMatch] Show first circle");
        BattleRoyaleZone m_Zone = new BattleRoyaleZone;
        m_Zone = m_Zone.GetZone(BattleRoyalePrepare.i_StartingZone);
        m_Zone.OnActivate( GetPlayers() ); //hand players over to the zone (for complex zone size/position calculation)
        ref BattleRoyalePlayArea m_ThisArea = m_Zone.GetArea();

        Print(m_ThisArea.GetCenter());
        Print(m_ThisArea.GetRadius());

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param2<ref BattleRoyalePlayArea, bool>( m_ThisArea, false ), true);
    }

    void HandleUnlock()
    {
        BattleRoyaleUtils.Trace("HandleUnlock");
        for(int i = 0; i < m_PlayerList.Count(); i++)
        {
            PlayerBase player = m_PlayerList[i];
            BattleRoyaleUtils.Trace("Unlock " + player.GetIdentity().GetName());

            player.DisableInput(false); //This will re-enable input
        }

        MessagePlayers( DAYZBR_MSG_MATCH_STARTED );

        b_IsGameplay = true;
    }

    void StartZoning()
    {
        Deactivate();
    }

    override void OnPlayerKilled(PlayerBase player, Object killer)
    {
        if(!b_IsGameplay)
        {
            Error("Player killed before gameplay!");
            return;
        }

        if(ContainsPlayer(player))
            RemovePlayer(player);
        else
            Error("Unknown player killed! Not in current state?");

        if(player.GetIdentity())
        {
            string player_steamid = player.GetIdentity().GetPlainId();
            vector player_position = player.GetPosition();
            int time = GetGame().GetTime(); //MS since mission start (we'll calculate UNIX timestamp on the webserver)

            if(killer)
            {
                EntityAI killer_entity;
                if(Class.CastTo(killer_entity, killer))
                {
                    string killed_with = "";
                    vector killer_position = killer_entity.GetPosition();

                    bool is_vehicle = false;

                    PlayerBase pbKiller;
                    if(!Class.CastTo(pbKiller, killer_entity))
                    {
                        Man root_player = killer_entity.GetHierarchyRootPlayer();
                        if(root_player)
                        {
                            pbKiller = PlayerBase.Cast( root_player );
                            is_vehicle = true;
                        }
                    }

                    if(pbKiller && pbKiller.GetIdentity())
                    {
                        if(!ContainsPlayer( pbKiller ))
                            Error("Killer does not exist in the current game state!");

                        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", new Param1<int>(1), true, pbKiller.GetIdentity(),pbKiller);
                    }
                }
            }
        }
    }
}
