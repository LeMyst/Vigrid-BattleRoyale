class RetryNetworkConnectCallback extends PopupButtonCallback
{
    protected ref MainMenu m_MainMenu;
	void RetryNetworkConnectCallback(ref MainMenu menu)
	{
		m_MainMenu = menu;
	} 
	override void OnButtonClick()
	{
		m_MainMenu.OnStart(true); //true parameter ensures we force reconnect (incase we're in a bugged state with an invalid player object)
	}
}