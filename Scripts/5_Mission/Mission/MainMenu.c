#ifndef SERVER
modded class MainMenu
{
    override void Play()
    {
        // TODO: Translate this
        GetGame().GetUIManager().ShowDialog("CLOSE YOUR GAME", "Please close your game and connect using your launcher.", 0, DBT_OK, DBB_NONE, DMT_INFO, this);
    }

    override void OpenMenuServerBrowser()
    {

    }
};
#endif
