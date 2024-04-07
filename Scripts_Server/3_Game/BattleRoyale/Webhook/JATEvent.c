#ifdef SERVER
class JATEvent
{
	string jwt_token;
	string event_name;
	string json_data;

	void JATEvent( string in_token, string in_event_name, string in_json_data )
	{
		jwt_token = in_token;
		event_name = in_event_name;
		json_data = in_json_data;
	};
};
