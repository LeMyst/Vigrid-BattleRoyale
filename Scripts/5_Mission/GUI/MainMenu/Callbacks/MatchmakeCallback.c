
class MatchmakeCallback extends BattleRoyaleMatchmakeCallback
{
	protected bool is_canceled;

	protected ref MainMenu m_MainMenu;
	void Cancel()
	{
        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove( api.RequestMatchmakeAsync ); //in case we cancel during the request downtime
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
        ref RetryMatchmakeCallback onretry = new RetryMatchmakeCallback( m_MainMenu );
		m_MainMenu.CreatePopup("Failed to connect! Error " + errorCode.ToString(), "Close", onclick, "Retry", onretry);
	}
	override void OnTimeout()
	{
		if(is_canceled)
		{
			return;
		}
		ref ClosePopupButtonCallback onclick = new ClosePopupButtonCallback( m_MainMenu );
        ref RetryMatchmakeCallback onretry = new RetryMatchmakeCallback( m_MainMenu );
		m_MainMenu.CreatePopup( DAYZBR_TIMEOUT_MSG, "Close", onclick, "Retry", onretry);
	}
	override void OnSuccess( ref ServerData data )
	{
		ref ServerData p_ServerData = data;
        ref ClosePopupButtonCallback onclick;
        ref RetryMatchmakeCallback onretry;

		if(is_canceled) 
		{
			return;
		}
		if(p_ServerData == NULL)
		{
			Error("BattleRoyale: ServerData is NULL, cannot matchmake");
			onclick = new ClosePopupButtonCallback( m_MainMenu );
            onretry = new RetryMatchmakeCallback( m_MainMenu );
		    m_MainMenu.CreatePopup( DAYZBR_NULL_RESPONSE_MSG, "Close", onclick, "Retry", onretry);
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
		
		if(!p_ServerData.IsMatchingVersion())
		{
			Error("Matchmaking returned a server with a mismatched version.");
			onclick = new ClosePopupButtonCallback( m_MainMenu );
            onretry = new RetryMatchmakeCallback( m_MainMenu );
		    m_MainMenu.CreatePopup( "VERSION MISMATCH WITH MATCHMAKED SERVER", "Close", onclick, "Retry", onretry);
			return;
		}

		if(!GetGame().Connect(m_MainMenu, ip_addr, port, BATTLEROYALE_SERVER_PASSWORD))
		{
			Print(p_ServerData.connection);
			Error("BattleRoyale: Failed to connect to server");	
            onclick = new ClosePopupButtonCallback( m_MainMenu );
            onretry = new RetryMatchmakeCallback( m_MainMenu );
		    m_MainMenu.CreatePopup(DAYZBR_FAILED_TO_CONNECT_MSG, "Close", onclick, "Retry", onretry);
            return;
		}

        //Note: the close button is only for beta testing (dayz's sweet "Connect" feature sometimes takes a while for the server handshake to complete)
        onclick = new ClosePopupButtonCallback( m_MainMenu );
		m_MainMenu.CreatePopup(DAYZBR_CONNECTING_TO_SERVER_MSG,"Close", onclick);
	}
}