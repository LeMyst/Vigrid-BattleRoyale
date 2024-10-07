#ifdef SERVER
#ifdef SPECTATOR
class BattleRoyaleSpectators
{
    protected ref array<PlayerBase> m_Spectators;
    protected ref Timer m_Timer;
    protected ref array<string> a_AllowedSteamIds;
    protected bool b_AllowAllSpectators;

    void BattleRoyaleSpectators()
    {
        m_Spectators = new array<PlayerBase>();
        m_Timer = new Timer;

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
        if(b_AllowAllSpectators)
            return true;

        string steamid = identity.GetPlainId();
        if(steamid == "")
        {
            Error("Blank SteamId from identity!");
            return false;
        }
        bool result = (a_AllowedSteamIds.Find(steamid) != -1);

        if(!result)
        {
            Print("DEBUG: Player with id '" + steamid  + "' is not whitelisted as a spectator!");
        }
        return result;
    }

    bool CanSpectate(PlayerBase player)
    {
        if(b_AllowAllSpectators)
            return true;

        if(!player)
        {
            Error("Null player in CanSpectate");
            return false;
        }
        PlayerIdentity identity = player.GetIdentity();

        if(!identity)
            return false;

        return CanIdSpectate( identity );
    }

    bool ContainsPlayer(PlayerBase player)
    {
        return (m_Spectators.Find(player) != -1);
    }

    void AddPlayer(PlayerBase player)
    {
        m_Spectators.Insert(player);
        //spin up a thread to handle spectator camera initialization
        GetGame().GameScript.Call(this, "InitSpectatorCamera", player);
    }

    void Update(float delta)
    {
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

            GetGame().ObjectDelete( player ); // this is for network bubble fix

            GetGame().SelectPlayer( identity, NULL ); //null their character

            GetGame().SelectSpectator( identity, "BattleRoyaleCamera", position );

            //activate camera on client side
            GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ActivateSpectatorCamera", new Param1<int>(1), true, identity);

            //Send zone information
            ref BattleRoyaleServer br_server = BattleRoyaleServer.Cast( GetBR() );
            ref BattleRoyaleRound br_round;
            if(Class.CastTo(br_round, br_server.GetCurrentState()))
            {
                if(br_round.IsLocked())
                {
                    //tell the client the current area is now this area
                    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( br_round.GetZone().GetArea() ), true, identity);
                    //tell the client we don't know the next play area
                    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param2<ref BattleRoyalePlayArea, bool>( NULL, false ), true, identity);
                }
                else
                {
                    //-- we are in a round! send zones!
                    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( br_round.GetPreviousZone().GetArea() ), true, identity);
                    //tell the client the next play area
                    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param2<ref BattleRoyalePlayArea, bool>( br_round.GetZone().GetArea(), false ), true, identity);
                }
            }
            ref BattleRoyaleLastRound br_last_round;
            if(Class.CastTo(br_last_round, br_server.GetCurrentState()))
            {
                if(!br_last_round.IsLocked())
                {
                    //tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
                    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", new Param1<ref BattleRoyalePlayArea>( br_last_round.GetPreviousZone().GetArea() ), true, identity);
                    //tell the client the future zone is NULL (no future zone)
                    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param2<ref BattleRoyalePlayArea, bool>( NULL, false ), true, identity);
                }
                //no else case, if the last round locks, there is no circle
            }
        }
        else
        {
            Error("What the fuck? No Player Identity in Spectator Init");
        }
    }
}
