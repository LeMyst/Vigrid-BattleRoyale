#ifndef SERVER
modded class LoadingScreen
{
    protected autoptr array< ref LoadingScreenBackground > m_Backgrounds;
	static float f_LoadingTime = -1;
	static float f_LoadingTimeStamp = -1;

    void LoadingScreen(DayZGame game)
    {
        BattleRoyaleUtils.Trace("Loading screens DayZ-BR");

		m_Backgrounds = LoadingScreenBackgrounds.Get();

        m_ImageLogoMid.Show( false );  // Hide the DayZ logo
        m_ImageLogoCorner.Show( false );  // Hide the Bohemia Interactive logo
        m_ModdedWarning.Show( false );  // Hide the modded warning
    }

    override void Show()
    {
        super.Show();

        m_ImageBackground.LoadMaskTexture("");  // Hide the mask texture
        m_ImageLogoMid.Show( false );  // Hide the DayZ logo
        m_ImageLogoCorner.Show( false );  // Hide the Bohemia Interactive logo
        
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

		// Get current world name
		LoadingScreenBackground backgrounds = null;
		LoadingScreenBackground defaultBackgrounds = null;

		string worldNameLower = GetGame().GetWorldName();
		worldNameLower.ToLower();

		// Find background for current map or default
		for (int i = 0; i < m_Backgrounds.Count(); i++)
		{
			LoadingScreenBackground current = m_Backgrounds[i];
			if (current)
			{
				string mapNameLower = current.MapName;
				mapNameLower.ToLower();
				if (mapNameLower == worldNameLower)
				{
					backgrounds = current;
					break;
				}

				if (mapNameLower == "default")
				{
					defaultBackgrounds = current;
				}
			}
		}

		// If no matching background found, use default
		if (!backgrounds)
		{
			backgrounds = defaultBackgrounds;
		}

		if (backgrounds)
			m_ImageBackground.LoadImageFile(0, backgrounds.GetRandomPath());
		else
			m_ImageBackground.LoadImageFile(0, "Vigrid-BattleRoyale/GUI/textures/loading_screens/br_loading_1.edds");
	}

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

		// Switch to another background image
        if (IsLoading())
            UpdateLoadingBackground();
    }
}
#endif
