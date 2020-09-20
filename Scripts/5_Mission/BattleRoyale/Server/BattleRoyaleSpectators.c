class BattleRoyaleSpectators
{
    protected ref array<PlayerBase> m_Players;
    protected ref ScriptCallQueue m_CallQueue;
    protected ref array<string> a_AllowedSteamIds;
    protected bool b_AllowAllSpectators;

    void BattleRoyaleSpectators()
    {
        m_Players = new array<PlayerBase>();
        m_CallQueue = new ScriptCallQueue;
        
        a_AllowedSteamIds = BattleRoyaleConfig.GetConfig().GetGameData().allowed_spectate_steamid64;
        if(!a_AllowedSteamIds)
        {
            Error("Missing Allowed SteamIds In Game Config");
            a_AllowedSteamIds = new array<string>();
        }

        b_AllowAllSpectators = !(BattleRoyaleConfig.GetConfig().GetGameData().use_spectate_whitelist);
        
    }

    bool CanIdSpectate(PlayerIdentity identity)
    {
        string steamid = identity.GetPlainId();

        return (a_AllowedSteamIds.Find(steamid) != -1);
    }
    bool CanSpectate(PlayerBase player)
    {
        if(b_AllowAllSpectators)
            return true;
            
        if(!player)
            return false;

        PlayerIdentity identity = player.GetIdentity();

        if(!identity)
            return false;

        return CanIdSpectate( identity );
    }
    bool ContainsPlayer(PlayerBase player)
    {
        return (m_Players.Find(player) != -1);
    }
    void AddPlayer(PlayerBase player)
    {
        m_Players.Insert(player);
        //spin up a thread to handle spectator camera initialization
        GetGame().GameScript.Call(this, "InitSpectatorCamera", player); 
    }
    void Update(float delta)
    {
        m_CallQueue.Tick( delta );
    }
    void OnPlayerTick(PlayerBase player, float delta)
    {
        // just in case we need it
    }

    protected void InitSpectatorCamera(PlayerBase player)
    {
        //we need to initialize some system on this client that lets them freecam around while updating their network bubble to the camera
        PlayerIdentity identity = player.GetIdentity();
        if(identity)
        {
            //if we fail to figure out the correct position, we'll just use the pre-existing head position of the spectators body
            vector position = player.GetBonePositionWS( player.GetBoneIndexByName( "Head" ) );
            
            //This is a very hacky way to get the players previous death position, we'll obviously need to find a better way to do so
            string steam_id = identity.GetPlainId();
            BattleRoyaleServer server = BattleRoyaleServer.Cast( GetBR() );
            MatchData match_data = server.GetMatchData();
            if(match_data.deaths)
            {
                for(int i = 0; i < match_data.deaths.Count(); i++)
                {
                    if(match_data.deaths[i])
                    {
                        if(match_data.deaths[i].steam_id == steam_id)
                        {
                            if(match_data.deaths[i].death_data)
                            {
                                position = match_data.deaths[i].death_data.position
                            }
                        }
                    }
                }
            }

            GetGame().ObjectDelete( player ); // this is for network bubble fix

            GetGame().SelectPlayer( identity, NULL ); //null their character

            GetGame().SelectSpectator( identity, "BattleRoyaleCamera", position );

            //activate camera on client side
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ActivateSpectatorCamera", new Param1<int>(1), true, identity, player);
        }
        else
        {
            Error("What the fuck? No Player Identity in Spectator Init");
        }
        
    }
}