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
    protected bool use_api;

    protected ref RestApi m_Rest;
    protected ref RestContext m_ClientContext;
    protected ref RestContext m_ServerContext;
    
    protected ref PlayerData m_PlayerData; //player object (should only ever be set once per instance)
    protected ref ServerData m_ServerData;
    protected ref RegionData m_RegionData;

    void BattleRoyaleAPI()
    {
        BattleRoyaleAPIData api_settings = BattleRoyaleConfig.GetConfig().GetApiData();
        server_private_key = api_settings.api_key; 
        rest_api_endpoint = api_settings.endpoint; 
        use_api = api_settings.use_api;
    }
    //--- public functions

    bool ShouldUseApi()
    {
        return use_api;
    }

    void SetCurrentPlayer( ref PlayerData data)
    {
        m_PlayerData = data;
    }
    PlayerData GetCurrentPlayer()
    {
        return m_PlayerData;
    }
    void SetRegionData( ref RegionData data)
    {
        m_RegionData = data;
    }
    RegionData GetRegionData()
    {
        return m_RegionData;
    }
    ServerData GetCurrentServer()
    {
        return m_ServerData;
    }
    bool HasPurchase(string shop_flag)
    {
        if(m_PlayerData)
        {
            return (m_PlayerData.shop_purchases.Find( shop_flag ) != -1);
        }
        return false;
    }

    //server startup request
    ServerData RequestServerStart()
    {
        BattleRoyaleServerData p_ServerSettings = BattleRoyaleConfig.GetConfig().GetServerData();
        if(!p_ServerSettings)
        {
            Error("BattleRoyaleAPI::RequestServerStart() => ERROR: p_ServerSettings = NULL");
            return NULL;
        }
        string query_port = p_ServerSettings.query_port.ToString();
        string ip_address = p_ServerSettings.ip_address;
        string request = "onstart/" + query_port;
        if(ip_address != "127.0.0.1")
        {
            request += "/" + ip_address + "/admin_only"; //admin_only is hard coded into web API
        }
        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Server, request);
        Print("BattleRoyaleAPI::RequestServerStart() => WEB RESULT: " + result);
        if(result == "invalid")
        {
            Error("BattleRoyaleAPI::RequestServerStart() => ERROR: INVALID REQUEST RESPONSE");
            return NULL;
        }
        if(result == "")
		{
			Error("BattleRoyaleAPI::RequestServerStart() => ERROR: web_result = NULL");
			return NULL;
		}

        JsonSerializer m_Serializer = new JsonSerializer;
		string error;
		if(!m_Serializer.ReadFromString( m_ServerData, result, error ))
		{
            m_ServerData = NULL; //bad request make sure our stored value is null
			Print("BattleRoyaleAPI::RequestServerStart() => JSON Failed To Parse!");
			Error(error);
			return NULL;
		}

        return m_ServerData;
    }

    void ServerFinish(string winner_name)
    {
        BattleRoyaleServerData p_ServerSettings = BattleRoyaleConfig.GetConfig().GetServerData();
        if(!p_ServerSettings)
        {
            Error("BattleRoyaleAPI::ServerFinish() => ERROR: p_ServerSettings = NULL");
            return;
        }
        string query_port = p_ServerSettings.query_port.ToString();
        string ip_address = p_ServerSettings.ip_address;

        string request = "onfinish/" + Encode(winner_name) + "/" + query_port;
        if(ip_address != "127.0.0.1")
        {
            request += "/" + ip_address + "/admin_only"; //admin_only is hard coded into web API
        }

        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Server, request);
        Print("BattleRoyaleAPI::ServerFinish() => WEB RESULT: " + result);
    }
    string SubmitMatchData(ref MatchData data_object)
    {
        string request = "matchsubmit/" + GetCurrentServer()._id;
        //post our JSON data
        RestContext context = GetContext(BattleRoyaleAPIContextType.Server);
        if(!context)
        {
            Error("BattleRoyaleAPI::SendRequest_Sync() => GetContext() RETURNED NULL!");
            return "";
        }

        string request_body = data_object.GetJSON();
        Print(request);
        Print(request_body);

        string result = context.POST_now(request, request_body);
        
        Print("BattleRoyaleAPI::SubmitMatchData() => WEB RESULT: " + result);
        return result;
    }

    void ServerSetLock(bool value)
    {
        int req_data = 0;
        if(value)
            req_data = 1;

        BattleRoyaleServerData p_ServerSettings = BattleRoyaleConfig.GetConfig().GetServerData();
        if(!p_ServerSettings)
        {
            Error("BattleRoyaleAPI::ServerSetLock() => ERROR: p_ServerSettings = NULL");
            return;
        }

        string query_port = p_ServerSettings.query_port.ToString();
        string ip_address = p_ServerSettings.ip_address;

        string request = "setlock/" + req_data.ToString() + "/" + query_port;
        if(ip_address != "127.0.0.1")
        {
            request += "/" + ip_address + "/admin_only"; //admin_only is hard coded into web API
        }

        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Server, request);
        Print("BattleRoyaleAPI::ServerSetLock() => WEB RESULT: " + result);
    }

    //get server info by ID
    ServerData GetServer(string server_id)
    {
        //https://dayzbr.dev/client/server/5efbdb35b9d68fe5ee8d5689
        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Client, "server/" + server_id);
        Print("BattleRoyaleAPI::GetServer() => WEB RESULT: " + result);
        if(result == "")
		{
			Error("BattleRoyaleAPI::GetServer() => ERROR: web_result = NULL");
			return NULL;
		}
        
        JsonSerializer m_Serializer = new JsonSerializer;
		ServerData p_ServerWebData;
		string error;
		if(!m_Serializer.ReadFromString( p_ServerWebData, result, error ))
		{
			Print("BattleRoyaleAPI::GetServer() => JSON Failed To Parse!");
			Error(error);
			return NULL;
		}

        return p_ServerWebData;
    }

    //request player simple data (likely will not be used)
    PlayerData GetPlayer(string SteamID)
    {
        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Client, "player/" + SteamID);
        Print("BattleRoyaleAPI::GetPlayer() => WEB RESULT: " + result);
        if(result == "")
		{
			Error("BattleRoyaleAPI::GetPlayer() => ERROR: result = NULL");
			return NULL;
		}
        JsonSerializer m_Serializer = new JsonSerializer;
        PlayerData m_PlayerWebData;
		string error;
		
		if(!m_Serializer.ReadFromString( m_PlayerWebData, result, error ))
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
        string result = SendRequest_Sync(BattleRoyaleAPIContextType.Client, "start/" + SteamID + "/" + Encode(Name));
        Print("BattleRoyaleAPI::RequestStart() => WEB RESULT: " + result);
        if(result == "")
		{
			Error("BattleRoyaleAPI::RequestStart() => ERROR: web_result = NULL");
			return NULL;
		}
		
		JsonSerializer m_Serializer = new JsonSerializer;
		string error;
		
		if(!m_Serializer.ReadFromString( m_PlayerData, result, error ))
		{
            m_PlayerData = NULL; //failed to parse, make sure our object is null
			Print("BattleRoyaleAPI::RequestStart() => JSON Failed To Parse!");
			Error(error);
			return NULL;
		}

		return m_PlayerData;
	}


    void RequestStartAsync(string SteamID, string Name, ref BattleRoyaleOnStartCallback callback)
    {
        Print("RequestStartAsync()");
        Print(SteamID);
        Print(Name);

        ref OnStartCallback_REST rest_callback = new OnStartCallback_REST( callback );
        SendRequest_Async(BattleRoyaleAPIContextType.Client, "start/" + SteamID + "/" + Encode(Name), rest_callback);
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
		ServerData p_ServerWebData;
		string error;
		if(!m_Serializer.ReadFromString( p_ServerWebData, result, error ))
		{
			Print("BattleRoyaleAPI::RequestMatchmake() => JSON Failed To Parse!");
			Error(error);
			return NULL;
		}

        return p_ServerWebData;
	}
    void RequestMatchmakeAsync(PlayerData m_webplayer, ref BattleRoyaleMatchmakeCallback callback, string region = "any")
    {
        Print("RequestMatchmakeAsync()");
        Print(region);

        if(!m_webplayer)
        {
            Error("BattleRoyaleAPI::RequestMatchmake() => m_webplayer is NULL");
            callback.OnError( DAYZBR_NETWORK_ERRORCODE_WEBPLAYER_NULL_RESULT );
            return;
        }
        Print(m_webplayer._id);
        ref MatchmakeCallback_REST rest_callback = new MatchmakeCallback_REST( callback );
		SendRequest_Async(BattleRoyaleAPIContextType.Client, "matchmake/" + m_webplayer._id + "/" + region, rest_callback);
    }

    RestContext GetContext(BattleRoyaleAPIContextType context_type = BattleRoyaleAPIContextType.Client)
    {
        Print("BattleRoyale: Requesting API Context...");
        if(context_type == BattleRoyaleAPIContextType.Client)
            return GetClientContext();
        else
            return GetServerContext();
    }

    string Encode(string message)
    {
        string encoded = "";

        for(int i = 0; i < message.Length(); i++)
        {
            string char = message.Get(i);
            int ascii = char.ToAscii(char); //? what the fuck? devs? Hello? DEVS?!
            encoded += "%" + ToHex(ascii);
        }

        encoded = message;

        return encoded;
    }

    //ascii decimal to hex string 00 01 02 ... FD FE FF
    protected string ToHex(int ascii)
    {
        if(ascii > 255)
            return "FF"; //ascii doesn't support characters > 255
        
        array<string> characters = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"}
        //determine first character 
        int first = ascii % 16; //0-15
        int last = Math.Floor(ascii / 16); //just incase it casts it to a float, we'll convert it back to an int after flooring

        return (characters.Get(first) + characters.Get(last));
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
    protected void SendRequest_Async(BattleRoyaleAPIContextType context_type, string request, RestCallback callback)
    {
        Print("SendRequest_Async()");
        Print(context_type);
        Print(request);

        RestContext context = GetContext(context_type);
        if(!context)
        {
            Error("BattleRoyaleAPI::SendRequest_Sync() => GetContext() RETURNED NULL!");
            callback.OnError( DAYZBR_NETWORK_ERRORCODE_CONTEXT_NULL_RESULT );
            return;
        }
        context.GET(callback, request);
    }

    protected void InitRestApi()
    {
        Print("BattleRoyale: Initializing Rest API");
        m_Rest = GetRestApi();
    }
    protected void InitClientContext()
    {
        Print("BattleRoyale: Initializing Client Context");
        //if(!m_Rest)
            InitRestApi();
        
        m_ClientContext = m_Rest.GetRestContext( rest_api_endpoint + "/client/" );
    }
    protected void InitServerContext()
    {
        Print("BattleRoyale: Initializing Server Context");
        //if(!m_Rest)
            InitRestApi();
        
        m_ServerContext = m_Rest.GetRestContext( rest_api_endpoint + "/server/" + server_private_key + "/" );
    }
    protected RestContext GetClientContext()
    {
        Print("BattleRoyale: Grabbing Client Context");  //TODO: (client crash is caused immediately after this)
        //if(!m_ClientContext)
            InitClientContext();

        return m_ClientContext;
    }
    protected RestContext GetServerContext()
    {
        Print("BattleRoyale: Grabbing Server Context");
       // if(!m_ServerContext)
            InitServerContext();

        return m_ServerContext;
    }
    
}
