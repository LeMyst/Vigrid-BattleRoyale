#ifdef SERVER
class AutoLockWebhook
{
	protected string s_ServerURL;
	protected string s_ServerIP;
	protected int i_ServerPort;
	protected string s_ServerRconPassword;
	protected int i_TryLeft = 3;

	void AutoLockWebhook(string url, string ip, int port, string rcon_password)
	{
		BattleRoyaleUtils.Trace("AutoLockWebhook()");

		s_ServerURL = url;
		s_ServerIP = ip;
		i_ServerPort = port;
		s_ServerRconPassword = rcon_password;
	};

	void LockServer()
	{
		BattleRoyaleUtils.Trace("AutoLockWebhook().LockServer()");

		if( i_TryLeft <= 0 )
		{
			BattleRoyaleUtils.Trace("AutoLockWebhook: Too much try, giving up.");
			return;
		}

		HttpArguments arguments = {
			new HttpArgument("version", "1"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		i_TryLeft = i_TryLeft - 1;

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(s_ServerURL);
		ctx.SetHeader("application/json");
		ctx.POST(new AutoLockCallback( s_ServerURL, s_ServerIP, i_ServerPort, s_ServerRconPassword, i_TryLeft ), arguments.ToQuery(string.Format("autolock/%1/%2", s_ServerIP, i_ServerPort)), "{\"rcon_password\":\"" + s_ServerRconPassword + "\"}");
	};

	void SetTryLeft(int try_left)
	{
		i_TryLeft = try_left;
	};
};

class AutoLockCallback: RestCallback
{
	protected string s_ServerURL;
	protected string s_ServerIP;
	protected int i_ServerPort;
	protected string s_ServerRconPassword;
	protected int i_TryLeft;

	void AutoLockCallback(string url, string ip, int port, string rcon_password, int try_left)
	{
        BattleRoyaleUtils.Trace("AutoLockCallback() " + try_left);

		s_ServerURL = url;
		s_ServerIP = ip;
		i_ServerPort = port;
		s_ServerRconPassword = rcon_password;
		i_TryLeft = try_left;
	}

	override void OnError( int errorCode )
	{
		BattleRoyaleUtils.Trace(" !!! OnError(): " + errorCode);

		AutoLockWebhook serverWebhook = new AutoLockWebhook( s_ServerURL, s_ServerIP, i_ServerPort, s_ServerRconPassword );
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.LockServer();
	};

	override void OnTimeout()
	{
		BattleRoyaleUtils.Trace(" !!! OnTimeout() ");

		AutoLockWebhook serverWebhook = new AutoLockWebhook( s_ServerURL, s_ServerIP, i_ServerPort, s_ServerRconPassword );
		serverWebhook.SetTryLeft( i_TryLeft );
		serverWebhook.LockServer();
	};

	override void OnSuccess( string data, int dataSize )
	{
		BattleRoyaleUtils.Trace(" !!! OnSuccess() size=" + dataSize );
		if( dataSize > 0 )
			BattleRoyaleUtils.Trace(data);
	};
};
