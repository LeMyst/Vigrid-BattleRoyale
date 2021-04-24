

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
class HttpPostRequest<Class T1, Class T2> extends RestCallback {
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
        ref BetterEye_PostRequest<T1,T2> rest_callback = new BetterEye_PostRequest<T1,T2>(callback);
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