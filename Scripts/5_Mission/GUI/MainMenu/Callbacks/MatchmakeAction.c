

class MatchmakeAction {
    private bool cancelled;
    private ref MainMenu m_MainMenu;

    void MatchmakeAction(ref MainMenu menu) {
        this.cancelled = false;
        this.m_MainMenu = menu;
    }

    void Cancel() {
        //if we're waiting 10 seconds between calls, we need to kick that action out of the callqueue
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove( this.RepeatRequest );
        this.cancelled = true; //cancel this so when we get a response from any current post request, we ignore it.
    }
    void RepeatRequest()
    {
        BattleRoyaleAPI api = BattleRoyaleAPI.GetAPI();
        api.RequestMatchmakeAsync(api.GetCurrentPlayer(), this, "OnMatchmakeComplete", m_MainMenu.GetSelectedRegion());
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
				m_MainMenu.CreatePopup("Failed to connect! Error JSON_PARSE_FAIL", "Close", onclick, "Retry", onretry);
			} else if(error_msg == DAYZBR_NETWORK_ERRORCODE_WEBPLAYER_NULL) {
				m_MainMenu. CreatePopup("Failed to connect! Error WEBPLAYER_NULL", "Close", onclick, "Retry", onretry);
			} else if(error_msg == DAYZBR_NETWORK_ERRORCODE_FILE) {
				m_MainMenu.CreatePopup("Failed to connect! Error FILE", "Close", onclick, "Retry", onretry);
			} else {
				m_MainMenu.CreatePopup("Failed to connect! Error " + error_msg, "Close", onclick, "Retry", onretry);
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
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.RepeatRequest, 10*1000, false );
            return;          
        }

        ref ServerData p_ServerData = mm_res.server;

        if(!p_ServerData.CanConnect()) {

            //invalid server recieved, wait 10 seconds and run another matchmake
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.RepeatRequest, 10*1000, false );
            return;
        }

        //connect to target server!
        string ip_addr = p_ServerData.GetIP();
		int port = p_ServerData.GetPort();

        if(ip_addr == "") {
            //wait is 0, but server returned is invalid.
            m_MainMenu.CreatePopup( "DayZBR Mod is Out Of Date! No Servers Found!", "Close", onclick, "Retry", onretry);
            return;
        }

        if(!p_ServerData.IsMatchingVersion())
		{
            //mismatched version match made (weird!) lets notify the player.
            m_MainMenu.CreatePopup( "VERSION MISMATCH WITH MATCHMAKED SERVER", "Close", onclick, "Retry", onretry);
            return;
        }

        int connect_code = GetGame().Connect(m_MainMenu, ip_addr, port, BATTLEROYALE_SERVER_PASSWORD);

        if(connect_code != 0)
		{
            string message = ErrorModuleHandler.GetClientMessageByCode(connect_code, "");
			Print(p_ServerData.connection);
			Error("BattleRoyale: Failed to connect to server (" + connect_code.ToString() + ")");
		    m_MainMenu.CreatePopup(DAYZBR_FAILED_TO_CONNECT_MSG + message, "Close", onclick, "Retry", onretry);
            return;
		}
        
		m_MainMenu.CreatePopup(DAYZBR_CONNECTING_TO_SERVER_MSG,"Close", onclick);
    }
}