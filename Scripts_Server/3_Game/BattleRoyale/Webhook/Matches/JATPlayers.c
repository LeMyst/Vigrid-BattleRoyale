#ifdef SERVER
class JATPlayers
{
	string jwt_token;
	map<string, string> players

	void JATPlayers( string in_token, map<string, string> in_players )
	{
		jwt_token = in_token;
		players = in_players;
	};
};
