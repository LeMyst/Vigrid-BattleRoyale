modded class LoadingScreen
{
    protected autoptr array< ref LoadingScreenBackground > m_Backgrounds;
	static float f_LoadingTime = -1;
	static float f_LoadingTimeStamp = -1;

    void LoadingScreen(DayZGame game)
    {
        BattleRoyaleUtils.Trace("Loading screens DayZ-BR");

		m_Backgrounds = LoadingScreenBackgrounds.Get();

        m_ImageLogoMid.Show( false );
        m_ImageLogoCorner.Show( false );
        m_ModdedWarning.Show( false );
    }

    override void Show()
    {
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

		LoadingScreenBackground backgrounds = m_Backgrounds[1]; // TODO: override default map

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
