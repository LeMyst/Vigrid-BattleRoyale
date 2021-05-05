/*

//example code that *should* work despite bugs in 1_12


class PostData {
    string RequestToJson() { return "{}"; }
    string ResponseToJson() { return "{}"; }
    void ParseRequest(string json) { Error("invalid call to postdata-parserequest"); }
    void ParseResponse(string json) { Error("invalid call to postdata-parserequest"); }
}
class HttpPostData<Class T1, Class T2> extends PostData {
    T1 request;
    T2 response;

    override string RequestToJson() {
        return JsonFileLoader<T1>.JsonMakeData(this.request);
    }
    override string ResponseToJson() {
        return JsonFileLoader<T2>.JsonMakeData(this.response);
    }
    override void ParseRequest(string json) {
        T1 result;
        JsonFileLoader<T1>.JsonLoadData(json, result);
        this.request = result;
    }
    override void ParseResponse(string json) {
        T2 result;
        JsonFileLoader<T2>.JsonLoadData(json, result);
        this.response = result;
    }
}
class HttpCallback {
    ref Class c_Inst;
    string s_Func;

    void HttpCallback(ref Class inst, string function) {
        this.c_Inst = inst;
        this.s_Func = function;
    }
}
class HttpPostRequest extends RestCallback {
    static ref PostData SendSync(string server, string function, ref PostData data) {
        string jsondata = data.RequestToJson();
        
        RestContext ctx = GetRestApi().GetRestContext(server);
        string outdata = ctx.POST_now(function, jsondata);

        data.ParseResponse(outdata);
        return data;
    }
    static void SendAsync(string server, string function, ref PostData data, ref HttpCallback callback = null) {
        string jsondata = data.RequestToJson();

        ref HttpPostRequest rest_callback = new HttpPostRequest(data, callback); //pass data in here because we'll need to use this object reference to parse response

        RestContext ctx = GetRestApi().GetRestContext(server);
        ctx.POST(rest_callback, function, jsondata); //post asynchronously
    }

    private ref HttpCallback callback;
    private ref PostData data;

    void HttpPostRequest( ref PostData _data, ref HttpCallback _callback = null ) {
        this.callback = _callback;
        this.data = _data;
    }

    void RunCallback(string error_msg)
    {
        if(this.callback == null) return;

        ref Param2<ref PostData, string> params = new Param2<ref PostData, string>(data, error_msg);
        GetGame().GameScript.CallFunctionParams(this.callback.c_Inst, this.callback.s_Func, NULL, params);
    }
    override void OnError( int errorCode )
    {
        RunCallback(errorCode.ToString());
    }
    override void OnTimeout()
    {
        RunCallback("timeout");
    }
    override void OnFileCreated( string fileName, int dataSize )
    {
        RunCallback("file created?");
    }
    override void OnSuccess( string _data, int dataSize )
    {
        if(!this.data) {
            RunCallback("internal error");
            return;
        }
        if( dataSize > 0 )
        {
            this.data.ParseResponse(_data);
            RunCallback("");
        }
        RunCallback("no data received");
    }
}


class TestPost_Struct {
    string value;
}
void TestPost() {
    ref TestPost_Struct struc = new TestPost_Struct;
    struc.value = "";

    HttpPostData<TestPost_Struct,TestPost_Struct> data = new HttpPostData<TestPost_Struct,TestPost_Struct>;
    data.request = struc;

    data = HttpPostRequest.SendSync("http://dayzbr.dev","/client/test_endpoint", data);

    ref TestPost_Struct res = data.response;
    Print(res.value);
}
*/