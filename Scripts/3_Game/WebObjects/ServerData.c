


class ServerData 
{
	//here is an example of the json data we expect to parse into this class
	/*
	{
		"_id":"5efcf6ec4ffec2570de122ab",
		"name":"Official DayZ Expansion Server",
		"connection":"78.46.109.81:2302",
		"query_port":"27016",
		"last_started":1593639481,
		"matches":[],
		"region":"eu"
	}
	*/
	string name;
	string connection;
	string query_port;
	int last_started;
	ref array<string> matches;
	string region;
	string _id;
	int locked; //booleans are stored as int's with 0 == false and  1 == true

	
	//methods used for easy connecting
	string GetIP()
	{
		TStringArray parts = new TStringArray;
		connection.Split(":",parts);
		
		return parts.Get(0);
	}
	int GetPort()
	{
		TStringArray parts = new TStringArray;
		connection.Split(":",parts);

		return parts.Get(1).ToInt();
	}
	bool CanConnect()
	{
		return (locked == 0);
	}
}
