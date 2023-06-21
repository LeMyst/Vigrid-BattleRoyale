modded class LoadingScreen
{
    protected autoptr array< ref LoadingScreenBackground > m_Backgrounds;
	static float f_LoadingTime = -1;
	static float f_LoadingTimeStamp = -1;

    void LoadingScreen(DayZGame game)
    {
        Print("Loading screens DayZ-BR");

		m_Backgrounds = LoadingScreenBackgrounds.Get();

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
        m_ImageBackground.LoadMaskTexture("");
        m_ImageLogoMid.Show( false );
        m_ImageLogoCorner.Show( false );
	    UpdateLoadingBackground();
    }

	void UpdateLoadingBackground()
	{
		float loadingTime = f_LoadingTime;
		float tickTime = m_DayZGame.GetTickTime();

		if (f_LoadingTimeStamp < 0)
		{
			f_LoadingTime = 0;
		}
		else
		{
			f_LoadingTime += tickTime - f_LoadingTimeStamp;
		}

		f_LoadingTimeStamp = tickTime;

		//! Show each loading message and screen at least five seconds
		if (loadingTime > -1 && f_LoadingTime < 5)
			return;

		f_LoadingTime = 0;

		LoadingScreenBackground backgrounds = m_Backgrounds[0];

		if (backgrounds)
			m_ImageBackground.LoadImageFile(0, backgrounds.GetRandomPath());
	}

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (IsLoading())
            UpdateLoadingBackground();
    }
}
