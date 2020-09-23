class IgnoreNetworkConnectCallback extends PopupButtonCallback
{
    protected ref MainMenu m_MainMenu;
	void IgnoreNetworkConnectCallback(ref MainMenu menu)
	{
		m_MainMenu = menu;
	} 
	override void OnButtonClick()
	{
		m_MainMenu.OfflineInit();
		m_MainMenu.ClosePopup();
        m_MainMenu.GetStats().InitOffline();
	}
}