

class MatchmakeAction {
    private bool cancelled;
    private ref MainMenu m_MainMenu;

    void MatchmakeAction(ref MainMenu menu) {
        this.cancelled = false;
        this.m_MainMenu = menu;
    }

    void Cancel() {
        //if we're waiting 10 seconds between calls, we need to kick that action out of the callqueue
        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove( api.RequestMatchmakeAsync );
        this.cancelled = true; //cancel this so when we get a response from any current post request, we ignore it.
    }
    void OnMatchmakeComplete(ref ClientMatchMakeResponse res, string error_msg) {
        if(this.cancelled) return;
        ref ClosePopupButtonCallback onclick = new ClosePopupButtonCallback( m_MainMenu );
        ref RetryMatchmakeCallback onretry = new RetryMatchmakeCallback( m_MainMenu );
        
        if(!res) {
            Error("res null!");


            

            if(error_msg == DAYZBR_NETWORK_ERRORCODE_TIMEOUT) {
				m_MainMenu.CreatePopup( DAYZBR_TIMEOUT_MSG, "Close", onclick, "Retry", onretry);
			} else if(error_msg == DAYZBR_NETWORK_ERRORCODE_JSON_PARSE_FAIL) {
				CreatePopup("Failed to connect! Error JSON_PARSE_FAIL", "Close", onclick, "Retry", onretry);
			} else if(error_msg == DAYZBR_NETWORK_ERRORCODE_WEBPLAYER_NULL) {
				m_MainMenu. CreatePopup("Failed to connect! Error WEBPLAYER_NULL", "Close", onclick, "Retry", onretry);
			} else if(error_msg == DAYZBR_NETWORK_ERRORCODE_FILE) {
				m_MainMenu.CreatePopup("Failed to connect! Error FILE", "Close", onclick, "Retry", onretry);
			} else {
				m_MainMenu.CreatePopup("Failed to connect! Error " + errorCode.ToString(), "Close", onclick, "Retry", onretry);
			}

            
            //some error occured

            return;
        }
        if(!res.success) {
            Error(res.error);
            m_MainMenu.CreatePopup("Failed to connect! Internal Server Error", "Close", onclick, "Retry", onretry);
            return;
        }
        if(!res.data) {
            Error("data null!");
            m_MainMenu.CreatePopup("Failed to connect! No Data Received", "Close", onclick, "Retry", onretry);
            return;
        }

        ref MatchMakingResponse mm_res = res.data;
        if(mm_res.wait) {
            //wait! run another matchmake in 10 seconds!
            BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
            PlayerData p_PlayerWebData = api.GetCurrentPlayer();
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( api.RequestMatchmakeAsync, 10*1000, false, p_PlayerWebData, this.OnMatchmakeComplete,  m_MainMenu.GetSelectedRegion());
            return;          
        }

        ref ServerData p_ServerData = mm_res.server;

        if(!p_ServerData.CanConnect()) {

            //invalid server recieved, wait 10 seconds and run another matchmake
            BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
            PlayerData p_PlayerWebData = api.GetCurrentPlayer();
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( api.RequestMatchmakeAsync, 10*1000, false, p_PlayerWebData, this.OnMatchmakeComplete,  m_MainMenu.GetSelectedRegion());
            return;
        }

        //connect to target server!
        string ip_addr = p_ServerData.GetIP();
		int port = p_ServerData.GetPort();
        if(!p_ServerData.IsMatchingVersion())
		{
            //mismatched version match made (weird!) lets notify the player.
            m_MainMenu.CreatePopup( "VERSION MISMATCH WITH MATCHMAKED SERVER", "Close", onclick, "Retry", onretry);
            return;
        }

        if(!GetGame().Connect(m_MainMenu, ip_addr, port, BATTLEROYALE_SERVER_PASSWORD))
		{
			Print(p_ServerData.connection);
			Error("BattleRoyale: Failed to connect to server");	
		    m_MainMenu.CreatePopup(DAYZBR_FAILED_TO_CONNECT_MSG, "Close", onclick, "Retry", onretry);
            return;
		}
        
		m_MainMenu.CreatePopup(DAYZBR_CONNECTING_TO_SERVER_MSG,"Close", onclick);
    }
}