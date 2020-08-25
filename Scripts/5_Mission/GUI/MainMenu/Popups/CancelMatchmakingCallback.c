class CancelMatchmakingCallback extends PopupButtonCallback
{
	protected ref MainMenu m_MainMenu;
	protected ref MatchmakeCallback m_Matchmaking;
	void CancelMatchmakingCallback(ref MainMenu menu, ref MatchmakeCallback callback)
	{
		m_MainMenu = menu;
		m_Matchmaking = callback;
	} 
	override void OnButtonClick()
	{
		Print("CANCELING MATCHMAKING!");
		m_Matchmaking.Cancel();
		m_MainMenu.ClosePopup();
	}
}