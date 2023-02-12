class RetryMatchmakeCallback extends PopupButtonCallback
{
    protected ref MainMenu m_MainMenu;
    void RetryMatchmakeCallback(ref MainMenu menu)
    {
        m_MainMenu = menu;
    }
    override void OnButtonClick()
    {
        m_MainMenu.Play();
    }
}
