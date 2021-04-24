
// DayZ 1.12 broke template definitions for RestCallback / Managed objects.
// so instead, below we defined individual HttpPostRequest objects for each req/res pair....

/*
class HttpPostRequest<Class T1, Class T2> extends RestMiddleman {
    static T2 SendSync(string server, string function, T1 data)
    {
        string jsondata = JsonFileLoader<T1>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        T2 result;//convert output to T2 object
        JsonFileLoader<T2>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, T1 data, func callback = null)
    {
        string jsondata = JsonFileLoader<T1>.JsonMakeData(data);
        ref HttpPostRequest<T1,T2> rest_callback = new HttpPostRequest<T1,T2>(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequest(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(T2 result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            T2 result;
            JsonFileLoader<T2>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}
*/
//BR does not use Get Requests
/*
class HttpGetRequest<Class T> extends RestCallback {
    static T SendSync(string server, string function) {
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.GET_now(function);
        T result;
        JsonFileLoader<T>.JsonLoadData(outdata, result);
        return result;
    }
    static void SendAsync(string server, string function, func callback = null) {
        ref HttpGetRequest<T> rest_callback = new HttpGetRequest<T>(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.GET(rest_callback, function);
    }

    private func cb;
    void HttpGetRequest(func callback) {
        this.cb = callback;
    }

    void RunCallback(T result, string error_msg) {
        if(this.cb == null) return;
        //really jank way of running a "func" with parameters
        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, "timeout");
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            T result;
            JsonFileLoader<T>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, "failed to parse");
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, "file created?!");
    }
}
*/

//typedef HttpPostRequest<OnStartRequest, OnStartResponse> HttpPostRequestOnStart;
class HttpPostRequestOnStart extends RestCallback {
    static OnStartResponse SendSync(string server, string function, OnStartRequest data)
    {
        string jsondata = JsonFileLoader<OnStartRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        OnStartResponse result;//convert output to T2 object
        JsonFileLoader<OnStartResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, OnStartRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<OnStartRequest>.JsonMakeData(data);
        ref HttpPostRequestOnStart rest_callback = new HttpPostRequestOnStart(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestOnStart(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(OnStartResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            OnStartResponse result;
            JsonFileLoader<OnStartResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}

//typedef HttpPostRequest<OnFinishRequest, OnFinishResponse> HttpPostRequestOnFinish;
class HttpPostRequestOnFinish extends RestCallback {
    static OnFinishResponse SendSync(string server, string function, OnFinishRequest data)
    {
        string jsondata = JsonFileLoader<OnFinishRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        OnFinishResponse result;//convert output to T2 object
        JsonFileLoader<OnFinishResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, OnFinishRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<OnFinishRequest>.JsonMakeData(data);
        ref HttpPostRequestOnFinish rest_callback = new HttpPostRequestOnFinish(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestOnFinish(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(OnFinishResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            OnFinishResponse result;
            JsonFileLoader<OnFinishResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}

//typedef HttpPostRequest<SubmitMatchRequest, SubmitMatchResponse> HttpPostRequestSubmitMatch;
class HttpPostRequestSubmitMatch extends RestCallback {
    static SubmitMatchResponse SendSync(string server, string function, SubmitMatchRequest data)
    {
        string jsondata = JsonFileLoader<SubmitMatchRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        SubmitMatchResponse result;//convert output to T2 object
        JsonFileLoader<SubmitMatchResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, SubmitMatchRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<SubmitMatchRequest>.JsonMakeData(data);
        ref HttpPostRequestSubmitMatch rest_callback = new HttpPostRequestSubmitMatch(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestSubmitMatch(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(SubmitMatchResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            SubmitMatchResponse result;
            JsonFileLoader<SubmitMatchResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}

//typedef HttpPostRequest<SetLockRequest, SetLockResponse> HttpPostRequestSetLock;
class HttpPostRequestSetLock extends RestCallback {
    static SetLockResponse SendSync(string server, string function, SetLockRequest data)
    {
        string jsondata = JsonFileLoader<SetLockRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        SetLockResponse result;//convert output to T2 object
        JsonFileLoader<SetLockResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, SetLockRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<SetLockRequest>.JsonMakeData(data);
        ref HttpPostRequestSetLock rest_callback = new HttpPostRequestSetLock(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestSetLock(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(SetLockResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            SetLockResponse result;
            JsonFileLoader<SetLockResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}

//typedef HttpPostRequest<GetServerRequest, GetServerResponse> HttpPostRequestGetServer;
class HttpPostRequestGetServer extends RestCallback {
    static GetServerResponse SendSync(string server, string function, GetServerRequest data)
    {
        string jsondata = JsonFileLoader<GetServerRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        GetServerResponse result;//convert output to T2 object
        JsonFileLoader<GetServerResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, GetServerRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<GetServerRequest>.JsonMakeData(data);
        ref HttpPostRequestGetServer rest_callback = new HttpPostRequestGetServer(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestGetServer(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(GetServerResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            GetServerResponse result;
            JsonFileLoader<GetServerResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}

//typedef HttpPostRequest<GetPlayerRequest, GetPlayerResponse> HttpPostRequestGetPlayer;
class HttpPostRequestGetPlayer extends RestCallback {
    static GetPlayerResponse SendSync(string server, string function, GetPlayerRequest data)
    {
        string jsondata = JsonFileLoader<GetPlayerRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        GetPlayerResponse result;//convert output to T2 object
        JsonFileLoader<GetPlayerResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, GetPlayerRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<GetPlayerRequest>.JsonMakeData(data);
        ref HttpPostRequestGetPlayer rest_callback = new HttpPostRequestGetPlayer(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestGetPlayer(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(GetPlayerResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            GetPlayerResponse result;
            JsonFileLoader<GetPlayerResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}

//typedef HttpPostRequest<StartRequest,ClientStartResponse> HttpPostRequestStartClient;
class HttpPostRequestStartClient extends RestCallback {
    static ClientStartResponse SendSync(string server, string function, StartRequest data)
    {
        string jsondata = JsonFileLoader<StartRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        ClientStartResponse result;//convert output to T2 object
        JsonFileLoader<ClientStartResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, StartRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<StartRequest>.JsonMakeData(data);
        ref HttpPostRequestStartClient rest_callback = new HttpPostRequestStartClient(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestStartClient(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(ClientStartResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            ClientStartResponse result;
            JsonFileLoader<ClientStartResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}

//typedef HttpPostRequest<MatchMakeRequest, ClientMatchMakeResponse> HttpPostRequestMatchmake;
class HttpPostRequestMatchmake extends RestCallback {
    static ClientMatchMakeResponse SendSync(string server, string function, MatchMakeRequest data)
    {
        string jsondata = JsonFileLoader<MatchMakeRequest>.JsonMakeData(data); //convert data to json
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata); //post request synchronous
        ClientMatchMakeResponse result;//convert output to T2 object
        JsonFileLoader<ClientMatchMakeResponse>.JsonLoadData(outdata, result);
        return result;//return out object
    }
    static void SendAsync(string server, string function, MatchMakeRequest data, func callback = null)
    {
        string jsondata = JsonFileLoader<MatchMakeRequest>.JsonMakeData(data);
        ref HttpPostRequestMatchmake rest_callback = new HttpPostRequestMatchmake(callback);
        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private func cb;
    void HttpPostRequestMatchmake(func callback)
    {
        this.cb = callback;
    }
    void RunCallback(ClientMatchMakeResponse result, string error_msg)
    {
        if(this.cb == null) return;

        ScriptCallQueue callQueue = new ScriptCallQueue;
        callQueue.Call(this.cb, result, error_msg);
        callQueue.Tick(1000);
    }
    override void OnError( int errorCode )
    {
        RunCallback(null, errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_TIMEOUT);
    }
    override void OnSuccess( string data, int dataSize )
    {
        if( dataSize > 0 )
        {
            ClientMatchMakeResponse result;
            JsonFileLoader<ClientMatchMakeResponse>.JsonLoadData(data, result);
            if(result)
            {
                RunCallback(result, "");
                return;
            }
        }
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL);
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback(null, DAYZBR_NETWORK_ERRORCODE_FILE);
    }
}
