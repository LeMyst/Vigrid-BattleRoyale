class RetryMatchmakeCallback extends PopupButtonCallback
{
    protected ref MainMenu m_MainMenu;
	void RetryNetworkConnectCallback(ref MainMenu menu)
	{
		m_MainMenu = menu;
	} 
	override void OnButtonClick()
	{
		m_MainMenu.Play();
	}
}