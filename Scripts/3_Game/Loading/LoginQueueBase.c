modded class LoginQueueBase
{
    void LoginQueueBase()
    {
        m_Backgrounds.Clear();
        JsonFileLoader< ref array< ref ExpansionLoadingScreenBackground > >.JsonLoadFile( DAYZBR_LOADING_SCREENS_PATH, m_Backgrounds );
    };
}
