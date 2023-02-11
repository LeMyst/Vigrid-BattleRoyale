class CancelMatchmakingCallback extends PopupButtonCallback
{
	protected ref MainMenu m_MainMenu;
	protected ref MatchmakeAction m_Matchmaking;
	void CancelMatchmakingCallback(ref MainMenu menu, ref MatchmakeAction action)
	{
		m_MainMenu = menu;
		m_Matchmaking = action;
	}
	override void OnButtonClick()
	{
		Print("CANCELING MATCHMAKING!");
		m_Matchmaking.Cancel();
		m_MainMenu.ClosePopup();
	}
}
