modded class DayZGame {
	protected RestContext m_ClientContext;
	protected ref PlayerData m_PlayerData;
	
	string GetBattleRoyaleClientVersion() 
	{
		return "0.1.b";
	}
	
	PlayerData GetWebPlayer() 
	{
		Print("Requested Player Data");
		if(!m_PlayerData)
			Error("Requested Null Data");
		
		return m_PlayerData;
	}
	void SetWebPlayer(ref PlayerData new_data)
	{
		Print("Simple Data Set");
		Print(new_data.name);
		m_PlayerData = new_data;
	}
	
	string RequestStart(string SteamID, string Name)
	{
		if(!GetClientContext())
		{
			Error("Failed to initialize web client context");
			return "";
		}
		
		return GetClientContext().GET_now("start/" + SteamID + "/" + Name);
	}
	string RequestMatchmake(string region = "any")
	{
		if(!GetClientContext())
		{
			Error("Failed to initialize web client context");
			return "";
		}
		if(!GetWebPlayer())
		{
			Error("No player object exists.");
			return "";
		}
		
		return GetClientContext().GET_now("matchmake/" + GetWebPlayer()._id + "/" + region);
	}
	RestContext GetClientContext() 
	{
		if(!m_ClientContext)
		{
			Print("Creating new Client Context");
			RestApi p_Rest = GetRestApi();
			m_ClientContext = p_Rest.GetRestContext("https://dayzbr.dev/client/");
		}
		return m_ClientContext;
	}
}


