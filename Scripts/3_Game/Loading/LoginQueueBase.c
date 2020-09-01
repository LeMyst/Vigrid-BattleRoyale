modded class LoginQueueBase
{
    override Widget Init()
    {
        m_Backgrounds.Clear(); 
        ref array<string> loading_screens = LoadingScreen.GetLoadingScreens();
        for(int i = 0; i < loading_screens.Count(); i++)
        {
            m_Backgrounds.Insert(new ExpansionLoadingScreenBackground( loading_screens[i] ));
        }
        return super.Init();
    }
}