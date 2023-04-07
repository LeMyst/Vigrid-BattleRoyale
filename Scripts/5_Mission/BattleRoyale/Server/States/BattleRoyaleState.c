class BattleRoyaleState extends Timeable {

    protected ref array<PlayerBase> m_Players;
    protected bool b_IsActive;
    protected bool b_IsPaused;

    string GetName()
    {
        return DAYZBR_SM_UNKNOWN_NAME;
    }

    void BattleRoyaleState()
    {
        m_Players = new array<PlayerBase>();

        b_IsActive = false;
        b_IsPaused = false;
    }

    void Update(float timeslice)
    {

    }

    //state controls
    void Activate()
    {
        //Note: this is called AFTER players are added
        b_IsActive = true;
    }
    void Deactivate()
    {
        //Note: this is called BEFORE players are removed
        //--- stop all repeating timers
        StopTimers();

        b_IsActive = false;
    }

    bool IsActive()
    {
        return b_IsActive;
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

    bool SkipState(BattleRoyaleState m_PreviousState)  //if true, we will skip activating/deactivating this state entirely
    {
        return false;
    }

    ref array<PlayerBase> GetPlayers()
    {
        return m_Players;
    }

    void AddPlayer(PlayerBase player)
    {
        m_Players.Insert( player );
        OnPlayerCountChanged();
    }

    void RemovePlayer(PlayerBase player)
    {
        m_Players.RemoveItem(player);
        OnPlayerCountChanged();
    }

    ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> result_array = new array<PlayerBase>();
        result_array.InsertAll(m_Players);
        m_Players.Clear();
        OnPlayerCountChanged();
        return result_array;
    }

    bool ContainsPlayer(PlayerBase player)
    {
        if(m_Players.Find(player) >= 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void OnPlayerTick(PlayerBase player, float timeslice)
    {
        if(player)
        {
            if(player.UpdateHealthStatsServer( player.GetHealth01("", "Health"), player.GetHealth01("", "Blood"), timeslice ))
            {
                //Print("Player Health Changed! Syncing Network...");
                //the player's stats changed (sync it over the network)
                GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateEntityHealth", new Param2<float, float>( player.health_percent, player.blood_percent ), true, NULL, player);
            }

            //UpdateHealthStatsServer will ensure that time_since_last_net_sync == 0 when a potential health update gets triggered. We can use this same window to also send map entity data updates
            //TODO: look for a more performant way of doing this maybe?
            if(player.time_since_last_net_sync == 0)
            {
                GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateMapEntityData", new Param4<string, string, vector, vector>( player.GetIdentity().GetId(), player.GetIdentityName(), player.GetPosition(), player.GetDirection() ), true);
            }
        }
    }

    //player count changed event handler
    protected void OnPlayerCountChanged()
    {
        //BattleRoyaleUtils.Trace("OnPlayerCountChanged()");
        if(IsActive())
        {
#ifdef SCHANAMODPARTY
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", new Param2<int, int>( GetPlayers().Count(), GetGroups().Count() ), true);
#else
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", new Param2<int, int>( GetPlayers().Count(), 0 ), true);
#endif
        }
    }

    //CreateNotification( ref StringLocaliser title, ref StringLocaliser text, string icon, int color, float time, PlayerIdentity identity ) ()
    void MessagePlayers(string message, string title = DAYZBR_MSG_TITLE, string icon = DAYZBR_MSG_IMAGE, int color = COLOR_EXPANSION_NOTIFICATION_INFO, float time = DAYZBR_MSG_TIME)
    {
        StringLocaliser title_sl = new StringLocaliser( title ); //This comes form CommunityFramework (if Translatestring fails, we get default text value here)
        StringLocaliser text = new StringLocaliser( message );
        ExpansionNotification(title_sl, text, icon, color, time).Create();
    }

    void MessagePlayer(PlayerBase player, string message, string title = DAYZBR_MSG_TITLE, string icon = DAYZBR_MSG_IMAGE, int color = COLOR_EXPANSION_NOTIFICATION_INFO, float time = DAYZBR_MSG_TIME)
    {
        if(player)
        {
            PlayerIdentity identity = player.GetIdentity();
            if(identity)
            {
                StringLocaliser title_sl = new StringLocaliser( title ); //This comes form CommunityFramework (if Translatestring fails, we get default text value here)
                StringLocaliser text = new StringLocaliser( message );
                ExpansionNotification(title_sl, text, icon, color, time).Create(identity);
            }
        }
    }

#ifdef SCHANAMODPARTY
    ref array<ref set<PlayerBase>> GetGroups()
    {
        int i;
        ref array<ref set<PlayerBase>> teleport_groups = new array<ref set<PlayerBase>>;
        ref array<PlayerBase> m_PlayerWaitList = new array<PlayerBase>();
        m_PlayerWaitList.Copy( m_Players );
        // Create teleport groups

        // Recuperation des parties
        // Recuperation de la partie du joueur
        // Ajout des membres de la partie dans un groupe
        // Suppression de chaque membre de la liste m_PlayerWaitList

        SchanaPartyManagerServer manager = GetSchanaPartyManagerServer();
        if (manager)
        {
            //BattleRoyaleUtils.Trace("Manager OK");
            autoptr map<string, autoptr set<string>> parties = manager.GetParties();
            if (parties)
            {
                //BattleRoyaleUtils.Trace("Parties OK");
                // Create map player id <-> player object
                auto id_map = new map<string, PlayerBase>();
                array<Man> players = new array<Man>;
                GetGame().GetPlayers(players);
                for (i = 0; i < players.Count(); ++i)
                {
                    PlayerBase player = PlayerBase.Cast(players.Get(i));
                    if (player && player.GetIdentity() && player.IsAlive())
                    {
                        //BattleRoyaleUtils.Trace("Player: " + player.GetIdentity().GetName());
                        id_map.Insert(player.GetIdentity().GetId(), player);
                    }
                }

                // Iterate over parties
                ref set<PlayerBase> group;
                int partyCount = parties.Count();
                //BattleRoyaleUtils.Trace("There is " + partyCount + " parties");
                for (i = 0; i < partyCount; ++i)
                {
                    group = new ref set<PlayerBase>;
                    PlayerBase plr = PlayerBase.Cast(id_map.Get(parties.GetKey(i)));
                    BattleRoyaleUtils.Trace("Party leader is " + plr.GetIdentity().GetName());
                    if(m_PlayerWaitList.Find(plr) != -1)
                    {
                        BattleRoyaleUtils.Trace(plr.GetIdentity().GetName() + " is in the wait list");
                        m_PlayerWaitList.Remove(m_PlayerWaitList.Find(plr));
                        if (plr && plr.GetIdentity() && plr.IsAlive())
                        {
                            BattleRoyaleUtils.Trace("Add leader " + plr.GetIdentity().GetName() + " to the group");
                            group.Insert(plr);  // Insert guild leader into group
                            if (parties.GetElement(i))
                            {
                                int tmpPlayPartCount = parties.GetElement(i).Count();
                                BattleRoyaleUtils.Trace("Party have " + tmpPlayPartCount + " more members");
                                for(int j = 0; j < tmpPlayPartCount; j++)  // Iterate over party members
                                {
                                    PlayerBase plrpart = PlayerBase.Cast(id_map.Get(parties.GetElement(i).Get(j)));
                                    BattleRoyaleUtils.Trace("Try to add player " + plrpart.GetIdentity().GetName() + " to teleport group");
                                    if(m_PlayerWaitList.Find(plrpart) != -1)
                                    {
                                        BattleRoyaleUtils.Trace("Added player " + plrpart.GetIdentity().GetName() + " to teleport group");
                                        m_PlayerWaitList.Remove(m_PlayerWaitList.Find(plrpart));
                                        group.Insert(plrpart);  // Insert party member into group
                                    }
                                }
                            }
                            teleport_groups.Insert(group);  // Insert group into list of groups
                        }
                    } else {
                        BattleRoyaleUtils.Trace("Party leader is not in waiting list, do nothing");
                    }

                    if(m_PlayerWaitList.Count() <= 0)
                    {
                        BattleRoyaleUtils.Trace("No more players in waiting list, skip the remaining players");
                        break;
                    }
                }
            }
        }
        // Add remaining players
        ref set<PlayerBase> solo_group;
        int pRemCount = m_PlayerWaitList.Count();
        //BattleRoyaleUtils.Trace("Remaining players: " + pRemCount);
        for (i = 0; i < pRemCount; i++)
        {
            //BattleRoyaleUtils.Trace("Remaining player: " + m_PlayerWaitList.Get(i).GetName());
            solo_group = new set<PlayerBase>;
            solo_group.Insert(m_PlayerWaitList.Get(i));
            teleport_groups.Insert(solo_group);
        }

        return teleport_groups;
    }
#endif
}

// Base state for the Debug Zone.
// This handles healing / godmode / and teleporting
class BattleRoyaleDebugState extends BattleRoyaleState {
    protected vector v_Center;
    protected float f_Radius;
    protected int i_HealTickTime;
    protected ref array<string> a_AllowedOutsideSpawn;

    void BattleRoyaleDebugState()
    {
        BattleRoyaleDebugData m_DebugSettings = BattleRoyaleConfig.GetConfig().GetDebugData();
        if(m_DebugSettings)
        {
            v_Center = m_DebugSettings.spawn_point;
            f_Radius = m_DebugSettings.radius;
            a_AllowedOutsideSpawn = m_DebugSettings.allowed_outside_spawn;
        }
        else
        {
            Error("DEBUG SETTINGS IS NULL!");
            v_Center = DAYZBR_DEBUG_CENTER;
            f_Radius = DAYZBR_DEBUG_RADIUS;
            a_AllowedOutsideSpawn = new array<string>();
        }
        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            i_HealTickTime = m_GameSettings.debug_heal_tick_seconds;
        }
        else
        {
            Error("GAME SETTINGS IS NULL");
            i_HealTickTime = DAYZBR_DEBUG_HEAL_TICK;
        }
    }

    override string GetName()
    {
        return DAYZBR_SM_UNKNOWN_DEBUG_NAME;
    }

    override void AddPlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(false); //all players in this state are god mode
            player.Heal();
        }
        super.AddPlayer(player);
    }

    /*override ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> players = super.RemoveAllPlayers();
        foreach(PlayerBase player : players)
        {
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
    }*/

    //--- debug states must lock players into the debug zone & heal them
    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        super.OnPlayerTick(player, timeslice);

        vector spawn_pos = "0 0 0";
        spawn_pos[0] = Math.RandomFloatInclusive((v_Center[0] - 5), (v_Center[0] + 5));
        spawn_pos[1] = GetGame().SurfaceY(v_Center[0], v_Center[2]);
        spawn_pos[2] = Math.RandomFloatInclusive((v_Center[2] - 5), (v_Center[2] + 5));

        vector playerPos = player.GetPosition();
        float distance = vector.Distance(playerPos, spawn_pos);

        if(distance > f_Radius && !CanGoOutsideSpawn(player))
        {
            player.SetPosition(spawn_pos);
        }

        if(player.time_until_heal <= 0)
        {
            player.time_until_heal = i_HealTickTime;
            player.Heal();
        }
        player.time_until_heal -= timeslice;
    }

    bool CanGoOutsideSpawn(PlayerBase player)
    {
        if(!player)
        {
            Error("Null player in CanSpectate");
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

        return (a_AllowedOutsideSpawn.Find(steamid) != -1);
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
