class BattleRoyaleAPI {
    static ref BattleRoyaleAPI m_Instance;
    static BattleRoyaleAPI GetAPI()
    {
        if(!m_Instance)
        {
            Print("INITIALIZING BATTLE ROYALE WEB API");
            m_Instance = new BattleRoyaleAPI;
        }
        return m_Instance;
    }

    protected string server_private_key;
    protected string bans_api_key;
    protected string rest_api_endpoint;
    protected bool use_api;

    protected ref PlayerData m_PlayerData; //player object (should only ever be set once per instance)
    protected ref ServerData m_ServerData;
    protected ref RegionData m_RegionData;

    void BattleRoyaleAPI()
    {
        BattleRoyaleAPIData api_settings = BattleRoyaleConfig.GetConfig().GetApiData();
        server_private_key = api_settings.api_key; 
        bans_api_key = api_settings.bans_api_key;
        rest_api_endpoint = api_settings.endpoint; 
        use_api = api_settings.use_api;
    }

    bool ShouldUseApi()
    {
        return use_api;
    }
    void SetCurrentPlayer( ref PlayerData data)
    {
        m_PlayerData = data;
    }
    ref PlayerData GetCurrentPlayer()
    {
        return m_PlayerData;
    }
    void SetRegionData( ref RegionData data)
    {
        m_RegionData = data;
    }
    ref RegionData GetRegionData()
    {
        return m_RegionData;
    }
    ref ServerData GetCurrentServer()
    {
        return m_ServerData;
    }
    bool HasPurchase(string shop_flag)
    {
        Print("Checking API for purchase flag " + shop_flag);
        if(m_PlayerData)
        {
            return (m_PlayerData.shop_purchases.Find( shop_flag ) != -1);
        }
        return false;
    }

    ref ServerData RequestServerStart() {
        Print("requesting server start...");
        BattleRoyaleServerData p_ServerSettings = BattleRoyaleConfig.GetConfig().GetServerData();
        if(!p_ServerSettings)
        {
            Error("p_ServerSettings = NULL");
            return NULL;
        }
        ref OnStartRequest req = new OnStartRequest;
        req.query_port = p_ServerSettings.query_port.ToString();
        req.server_ip = p_ServerSettings.ip_address;
        req.server_version = BATTLEROYALE_VERSION;
        
        ref OnStartResponse res = HttpPostRequestOnStart.SendSync(rest_api_endpoint,"/server/" + server_private_key + "/onstart", req);

        if(res == null) {
            Error("OnStartResponse is null");
            return NULL;
        }
        if(!res.success) {
            Error(res.error);
            return NULL;
        }
        if(res.data == null) {
            Error("Response data field is null")
            return NULL;
        }
        this.m_ServerData = res.data;
        return res.data;
    }

    void ServerFinish(string winner_name) {
        Print("requesting server finish...");
        BattleRoyaleServerData p_ServerSettings = BattleRoyaleConfig.GetConfig().GetServerData();
        if(!p_ServerSettings)
        {
            Error("p_ServerSettings = NULL");
            return;
        }
        ref OnFinishRequest req = new OnFinishRequest;
        req.winner = winner_name;
        req.query_port = p_ServerSettings.query_port.ToString();
        req.server_ip = p_ServerSettings.ip_address;
        ref OnFinishResponse res = HttpPostRequestOnFinish.SendSync(rest_api_endpoint, "/server/" + server_private_key + "/onfinish", req);
        
        if(res == null) {
            Error("OnStartResponse is null");
            return;
        }
        if(!res.success) {
            Error(res.error);
        }
    }   

    void SubmitMatchData(ref MatchData data_object) {
        Print("submitting match data...");
        ref SubmitMatchRequest req = new SubmitMatchRequest;
        req.server_id = GetCurrentServer()._id;
        req.match_data = BRMatch.Cast( data_object );
        ref SubmitMatchResponse res = HttpPostRequestSubmitMatch.SendSync(rest_api_endpoint, "/data/" + server_private_key + "/matchsubmit", req);
        if(res == null) {
            Error("OnStartResponse is null");
            return;
        }
        if(!res.success) {
            Error(res.error);
        }
    }

    void ServerSetLock(bool value) {
        Print("updating lock state...");
        BattleRoyaleServerData p_ServerSettings = BattleRoyaleConfig.GetConfig().GetServerData();
        if(!p_ServerSettings)
        {
            Error("p_ServerSettings = NULL");
            return;
        }
        ref SetLockRequest req = new SetLockRequest;
        req.lock = value;
        req.query_port = p_ServerSettings.query_port.ToString();
        req.server_ip = p_ServerSettings.ip_address;
        ref SetLockResponse res = HttpPostRequestSetLock.SendSync(rest_api_endpoint, "/server/" + server_private_key + "/setlock", req);
        if(res == null) {
            Error("OnStartResponse is null");
            return;
        }
        if(!res.success) {
            Error(res.error);
        }
    }
    ref ServerData GetServer(string server_id) {
        Print("getting server by id...");
        ref GetServerRequest req = new GetServerRequest;
        req.id = server_id;

        ref GetServerResponse res = HttpPostRequestGetServer.SendSync(rest_api_endpoint, "/client/server", req);
        if(res == null) {
            Error("OnStartResponse is null");
            return NULL;
        }
        if(!res.success) {
            Error(res.error);
            return NULL;
        }
        return res.data;
    }
    ref PlayerData GetPlayer(string SteamID) {
        Print("getting player by id...");
        ref GetPlayerRequest req = new GetPlayerRequest;
        req.id = SteamID;

        ref GetPlayerResponse res = HttpPostRequestGetPlayer.SendSync(rest_api_endpoint, "/client/player", req);
        if(res == null) {
            Error("OnStartResponse is null");
            return NULL;
        }
        if(!res.success) {
            Error(res.error);
            return NULL;
        }
        return res.data;
    }
    void RequestStartAsync(string SteamID, string Name, Class inst, string callback) {
        Print("requesting async start...");
        ref StartRequest req = new StartRequest;
        req.steamid = SteamID;
        req.name = Name;

        Print(rest_api_endpoint);
        Print("/client/start");

        HttpPostRequestStartClient.SendAsync(rest_api_endpoint, "/client/start", req, new HttpAsyncCallback(inst, callback));
    }
    void RequestMatchmakeAsync(ref PlayerData m_webplayer, Class inst, string callback, string region = "any") {
        Print("requesting async matchmake...");
        if(!m_webplayer) {
            Error("player data is null")

            ref Param2<ref ClientMatchMakeResponse, string> params = new Param2<ref ClientMatchMakeResponse, string>(null, DAYZBR_NETWORK_ERRORCODE_WEBPLAYER_NULL);
            GetGame().GameScript.CallFunctionParams(inst, callback, NULL, params);
            return;
        }
        
        ref MatchMakeRequest req = new MatchMakeRequest;
        req.id = m_webplayer._id;
        req.region = region;
        req.version = BATTLEROYALE_VERSION;

        HttpPostRequestMatchmake.SendAsync(rest_api_endpoint, "/client/matchmake", req, new HttpAsyncCallback(inst, callback));
    }
}