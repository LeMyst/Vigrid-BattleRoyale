
//this is a special case, when the matchmake api returns "WAIT", this object is returned.
class MatchmakingWaitResult extends ServerData
{
	void MatchmakingWaitResult()
	{
		name = "WAIT";
		connection = "127.0.0.1:2302";
		query_port = "27016";
		last_started = 0;
		matches = new array<string>;
		region = "any";
		_id = "0000000000000000000000000";
		locked = true;
	}
	override bool CanConnect()
	{
		return false;
	}
}