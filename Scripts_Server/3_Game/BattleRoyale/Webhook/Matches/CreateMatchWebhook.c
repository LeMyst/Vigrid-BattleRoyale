#ifdef SERVER
class CreateMatchWebhook
{
	protected string s_ServerToken;

	void CreateMatchWebhook(string server_token)
	{
		Print("CreateMatchWebhook()");

		s_ServerToken = server_token;
	};

	string getMatchUUID()
	{
		Print("CreateMatchWebhook().getMatchUUID()");

		HttpArguments arguments = {
			new HttpArgument("version", "1"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		JSONAuthToken jsonAuthToken = new JSONAuthToken( s_ServerToken );

		string jatString;
		string jatError;
		JsonFileLoader<JSONAuthToken>.MakeData(jsonAuthToken, jatString, jatError);

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
        string result = ctx.POST_now( arguments.ToQuery("matches"), jatString);
        Print("CreateMatchWebhook().getMatchUUID() result: " + result);

        return result;
	};
};
