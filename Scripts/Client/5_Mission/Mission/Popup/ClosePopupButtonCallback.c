#ifndef SERVER
class ClosePopupButtonCallback extends PopupButtonCallback
{
	protected ref MainMenu m_MainMenu;

	void ClosePopupButtonCallback(MainMenu menu)
	{
		m_MainMenu = menu;
	}

	override void OnButtonClick()
	{
		m_MainMenu.ClosePopup();
	}
}