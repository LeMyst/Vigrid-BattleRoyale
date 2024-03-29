#ifdef SERVER
class JSONAuthToken
{
	string jwt_token;

	void JSONAuthToken( string token )
	{
		jwt_token = token;
	};
};
