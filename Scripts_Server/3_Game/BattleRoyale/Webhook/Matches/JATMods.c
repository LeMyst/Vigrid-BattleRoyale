#ifdef SERVER
class JATMods
{
	string jwt_token;
	map<string, string> mods;

	void JATMods( string in_token, map<string, string> in_mods )
	{
		jwt_token = in_token;
		mods = in_mods;
	};
};
