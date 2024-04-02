#ifdef SERVER
class JATParties
{
	string jwt_token;
	array<ref set<string>> parties

	void JATParties( string in_token, array<ref set<string>> in_parties )
	{
		jwt_token = in_token;
		parties = in_parties;
	};
};
