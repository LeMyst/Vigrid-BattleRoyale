
class MatchmakeCallback extends BattleRoyaleMatchmakeCallback
{
	protected bool is_canceled;

	protected ref MainMenu m_MainMenu;
	void Cancel()
	{
		is_canceled = true;
	}
	void MatchmakeCallback(ref MainMenu menu)
	{
		is_canceled = false;
		m_MainMenu = menu;
	}
	override void OnError( int errorCode )
	{
		if(is_canceled)
		{
			return;
		}
		ref ClosePopupButtonCallback onclick = new ClosePopupButtonCallback( m_MainMenu );
		m_MainMenu.CreatePopup("Failed to connect! Error " + errorCode.ToString(), "Close", onclick);
	}
	override void OnTimeout()
	{
		if(is_canceled)
		{
			return;
		}
		ref ClosePopupButtonCallback onclick = new ClosePopupButtonCallback( m_MainMenu );
		m_MainMenu.CreatePopup("Failed to connect! Timed out!", "Close", onclick);
	}
	override void OnSuccess( ref ServerData data )
	{
		ref ServerData p_ServerData = data;

		if(is_canceled) 
		{
			return;
		}
		if(p_ServerData == NULL)
		{
			Error("BattleRoyale: ServerData is NULL, cannot matchmake");
			m_MainMenu.ClosePopup();
			m_MainMenu.OpenMenuServerBrowser();
			return;
		}

		if(!p_ServerData.CanConnect())
		{
			Print("No viable match found... keep searching");

            //--- get our player data 
            BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
		    PlayerData p_PlayerWebData = api.GetCurrentPlayer();

            //submit another request in 10 sec
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( api.RequestMatchmakeAsync, 10*1000, false, p_PlayerWebData, this,  m_MainMenu.GetSelectedRegion());
            //api.RequestMatchmakeAsync(p_PlayerWebData, this, m_MainMenu.GetSelectedRegion());
			return;
		}

		string ip_addr = p_ServerData.GetIP();
		int port = p_ServerData.GetPort();
		
		if(!GetGame().Connect(m_MainMenu, ip_addr, port, "DayZBR_Beta"))
		{
			Print(p_ServerData.connection);
			Error("BattleRoyale: Failed to connect to server");
			m_MainMenu.OpenMenuServerBrowser();
		}

		m_MainMenu.ClosePopup();
	}
}