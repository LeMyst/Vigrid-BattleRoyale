#ifdef SERVER
class CreateMatchWebhook
{
	protected string s_ServerToken;

	void CreateMatchWebhook(string server_token)
	{
		BattleRoyaleUtils.Trace("CreateMatchWebhook()");

		s_ServerToken = server_token;
	};

	string getMatchUUID()
	{
		BattleRoyaleUtils.Trace("CreateMatchWebhook().getMatchUUID()");

		HttpArguments arguments = {
			new HttpArgument("version", "1"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		map<string, string> mods = new map<string, string>; // TODO: Dummy mods
		ref array<ref ModInfo> modList = new array<ref ModInfo>();
		GetGame().GetModInfos(modList);

		for (int i = 0; i < modList.Count(); i++)
		{
			ModInfo modInfo = modList.Get(i);
			mods.Insert(modInfo.GetName(), modInfo.GetVersion());
		}

		JATMods jatMods = new JATMods( s_ServerToken, mods );

		string jatString;
		string jatError;
		JsonFileLoader<JATMods>.MakeData(jatMods, jatString, jatError);

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext("https://api.vigrid.ovh/");
		string result = ctx.POST_now( arguments.ToQuery("matches"), jatString);
		BattleRoyaleUtils.Trace("CreateMatchWebhook().getMatchUUID() result: " + result);

		return result;
	};
};
