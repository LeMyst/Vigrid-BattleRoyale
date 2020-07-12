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
    protected string rest_api_endpoint;

    protected ref RestApi m_Rest;
    protected ref RestContext m_ClientContext;
    protected ref RestContext m_ServerContext;
    
    protected ref PlayerData m_PlayerData; //player object (should only ever be set once per instance)

    void BattleRoyaleAPI()
    {
        BattleRoyaleAPIData api_settings = BattleRoyaleConfig.GetConfig().GetApiData();
        server_private_key = api_settings.api_key; 
        rest_api_endpoint = api_settings.endpoint; 
    }
    //--- public functions


    PlayerData GetCurrentPlayer()
    {
        return m_PlayerData;
    }


    //request player simple data
    PlayerData GetPlayer(string SteamID)
    {
        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Client, "player/" + SteamID);
        Print("BattleRoyaleAPI::GetPlayer() => WEB RESULT: " + result);
        if(result == "")
		{
			Error("BattleRoyaleAPI::GetPlayer() => ERROR: web_result = NULL");
			return NULL;
		}
        JsonSerializer m_Serializer = new JsonSerializer;
        PlayerData m_PlayerWebData;
		string error;
		
		if(!m_Serializer.ReadFromString( m_PlayerWebData, web_result, error ))
		{
			Print("BattleRoyaleAPI::RequestStart() => JSON Failed To Parse!");
			Error(error);
			return NULL;
		}

		return m_PlayerWebData;
    }

    //client onstart api
    PlayerData RequestStart(string SteamID, string Name)
	{
        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Client, "start/" + SteamID + "/" + Name);
        Print("BattleRoyaleAPI::RequestStart() => WEB RESULT: " + result);
        if(result == "")
		{
			Error("BattleRoyaleAPI::RequestStart() => ERROR: web_result = NULL");
			return NULL;
		}
		
		JsonSerializer m_Serializer = new JsonSerializer;
		string error;
		
		if(!m_Serializer.ReadFromString( m_PlayerData, web_result, error ))
		{
            m_PlayerData = NULL; //failed to parse, make sure our object is null
			Print("BattleRoyaleAPI::RequestStart() => JSON Failed To Parse!");
			Error(error);
			return NULL;
		}

		return m_PlayerData;
	}

    //request matchmake
    ServerData RequestMatchmake(PlayerData m_webplayer, string region = "any")
	{
        if(!m_webplayer)
        {
            Error("BattleRoyaleAPI::RequestMatchmake() => m_webplayer is NULL");
            return NULL;
        }
		string result = SendRequest_Sync(BattleRoyaleAPIContextType.Client, "matchmake/" + m_webplayer._id + "/" + region);
        Print("BattleRoyaleAPI::RequestMatchmake() => WEB RESULT: " + result);

        if(result == "")
		{
			Error("BattleRoyaleAPI::RequestMatchmake() => ERROR: web_result = NULL");
			return NULL;
		}

        if(result.IndexOf("WAIT") == 0)
        {
            //-- wait request
            Print("BattleRoyaleAPI::RequestMatchmake() => RETURND `WAIT` VALUE");
            MatchmakingWaitResult return_value = new MatchmakingWaitResult;
            return return_value;
        }

		JsonSerializer m_Serializer = new JsonSerializer;
		ServerData p_ServerData;
		string error;
		if(!m_Serializer.ReadFromString( p_ServerData, result, error ))
		{
			Print("BattleRoyaleAPI::RequestMatchmake() => JSON Failed To Parse!");
			Error(error);
			return NULL;
		}

        return p_ServerData;
	}

    RestContext GetContext(BattleRoyaleAPIContextType context_type = BattleRoyaleAPIContextType.Client)
    {
        Print("BattleRoyale: Requesting API Context...");
        if(context_type == BattleRoyaleAPIContextType.Client)
            return GetClientContext();
        else
            return GetServerContext();
    }


    //--- private functions
    protected string SendRequest_Sync(BattleRoyaleAPIContextType context_type, string request)
    {
        RestContext context = GetContext(context_type);
        if(!context)
        {
            Error("BattleRoyaleAPI::SendRequest_Sync() => GetContext() RETURNED NULL!");
            return "";
        }
        return context.GET_now(request);
    }
    protected void InitRestApi()
    {
        Print("BattleRoyale: Initializing Rest API");
        m_Rest = GetRestApi();
    }
    protected void InitClientContext()
    {
        Print("BattleRoyale: Initializing Client Context");
        if(!m_Rest)
            InitRestApi();
        
        m_ClientContext = m_Rest.GetRestContext( rest_api_endpoint + "/client/" );
    }
    protected void InitServerContext()
    {
        Print("BattleRoyale: Initializing Server Context");
        if(!m_Rest)
            InitRestApi();
        
        m_ServerContext = m_Rest.GetRestContext( rest_api_endpoint + "/server/" + server_private_key + "/" );
    }
    protected RestContext GetClientContext()
    {
        Print("BattleRoyale: Grabbing Client Context");
        if(!m_ClientContext)
            InitClientContext();

        return m_ClientContext;
    }
    protected RestContext GetServerContext()
    {
        Print("BattleRoyale: Grabbing Server Context");
        if(!m_ServerContext)
            InitServerContext();

        return m_ServerContext;
    }
    
}
