#ifdef SERVER
class JATMods
{
	string jwt_token;
	string server_password;
	map<string, string> mods;

	void JATMods( string in_token, string in_password, map<string, string> in_mods )
	{
		jwt_token = in_token;
		server_password = in_password;
		mods = in_mods;
	};
};
