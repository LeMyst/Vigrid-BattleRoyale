#ifndef SERVER
class MatchMakingWebhook
{
	void MatchMakingWebhook()
	{
		BattleRoyaleUtils.Trace("MatchMakingWebhook()");
	};

	array<string> searchGame( array<ref ModInfo> mod_list )
	{
		BattleRoyaleUtils.Trace("MatchMakingWebhook().searchGame()");

        array<string> a_Result = new array<string>;

		HttpArguments arguments = {
			new HttpArgument("version", "2"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		MM_PlayerInfo playerInfo = new MM_PlayerInfo( mod_list );

		string mmString;
		string mmError;
		JsonFileLoader<MM_PlayerInfo>.MakeData(playerInfo, mmString, mmError);

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
        string result = ctx.POST_now( arguments.ToQuery("matches/matchmaking"), mmString );
        BattleRoyaleUtils.Trace("MatchMakingWebhook().searchGame() result: " + result);

        // Split the result into an array
        if( result.Length() > 0 )
        {
            result.Split(";", a_Result);
        }

        // At this point, we should have an array with the following format:
        // [0] = "ip"
        // [1] = "port"
        // [2] = "password"

        return a_Result;
	};

	bool isMatchMakingAvailable( string ip, string port )
	{
		BattleRoyaleUtils.Trace("MatchMakingWebhook().isMatchMakingAvailable()");

		HttpArguments arguments = {
			new HttpArgument("version", "2"),
			new HttpArgument("tick", GetGame().GetTickTime().ToString())
		};

		RestApi restApi = GetRestApi();
		RestContext ctx = restApi.GetRestContext(BATTLEROYALE_API_ENDPOINT);
		string result = ctx.GET_now( arguments.ToQuery(string.Format("matches/matchmaking/%1:%2", ip, port)) );
		BattleRoyaleUtils.Trace("MatchMakingWebhook().isMatchMakingAvailable() result: " + result);

		return result == "true";
	};
};

class MM_PlayerInfo
{
    ref map<string, string> mod_list = new map<string, string>;
    
    void MM_PlayerInfo( array<ref ModInfo> in_mod_list )
    {
        for( int i = 0; i < in_mod_list.Count(); i++ )
        {
            string mod_name = in_mod_list[i].GetName();
            string mod_version = in_mod_list[i].GetVersion();

            mod_list.Insert( mod_name, mod_version );
        }
    };
};