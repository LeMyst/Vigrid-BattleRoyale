modded class LoadingScreen
{
    void LoadingScreen(DayZGame game)
    {
        Print("Loading screens DayZ-BR");

        /*JsonFileLoader< ref array< ref ExpansionLoadingScreenBackground > >.JsonLoadFile( DAYZBR_LOADING_SCREENS_PATH, m_Backgrounds );
        JsonFileLoader< ref array< ref ExpansionLoadingScreenMessageData > >.JsonLoadFile( DAYZBR_LOADING_MESSAGES_PATH, m_MessageJson );

        m_ImageLogoMid.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE );
        m_ImageLogoCorner.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE );
        m_ModdedWarning.SetText( BATTLEROYALE_LOADING_MODDED_MESSAGE );
        float x;
        float y;
        m_ModdedWarning.GetPos(x, y);
        m_ModdedWarning.SetPos( x, y + 5 ); //add a buffer of 5 pixels
        m_ProgressLoading.SetColor( DAYZBR_LOADING_BAR_COLOR );*/

        m_ImageLogoMid.Show( false );
        m_ImageLogoCorner.Show( false );
        m_ModdedWarning.Show( false );
    }

    override void Show()
    {
        /*m_Backgrounds.Clear();
        JsonFileLoader< ref array< ref ExpansionLoadingScreenBackground > >.JsonLoadFile( DAYZBR_LOADING_SCREENS_PATH, m_Backgrounds );

        m_MessageJson.Clear();
        JsonFileLoader< ref array< ref ExpansionLoadingScreenMessageData > >.JsonLoadFile( DAYZBR_LOADING_MESSAGES_PATH, m_MessageJson );*/

        super.Show();
        m_ImageLogoMid.Show( false );
        m_ImageLogoCorner.Show( false );
    }
}
