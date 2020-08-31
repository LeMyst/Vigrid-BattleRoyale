class ConnectingToNetworkCallback extends BattleRoyaleOnStartCallback
{
	protected ref MainMenu m_MainMenu;
	void ConnectingToNetworkCallback(ref MainMenu menu)
	{
		m_MainMenu = menu;
	}
	override void OnError( int errorCode )
	{
		if(errorCode == 5)
		{
			//some generic error code
		}
		ref RetryNetworkConnectCallback callback = new RetryNetworkConnectCallback( m_MainMenu );
		m_MainMenu.CreatePopup("Failed to connect! Error " + errorCode.ToString(), "Retry", callback);
	}
	override void OnTimeout()
	{
		ref RetryNetworkConnectCallback callback = new RetryNetworkConnectCallback( m_MainMenu );
		m_MainMenu.CreatePopup( DAYZBR_TIMEOUT_MSG, "Retry", callback);
	}
	override void OnSuccess( PlayerData data )
	{
		m_MainMenu.UpdateRegions();
		m_MainMenu.ClosePopup();
		m_MainMenu.GetStats().InitBRStats(); //need to refresh the stats panel once we successfully query for our player data
	}
}