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

        //--- Battle Royale logo in place of the DayZ one. The image is the imageset's, whose
        //--- INTERNAL name is "battleroyale_gui" rather than the filename dayzbr_gui - see
        //--- BATTLEROYALE_LOGO_IMAGE. Loaded once here; the texture survives every later Show().
        //--- The corner slot stays hidden: it is the Bohemia logo's, and vanilla Show() only ever
        //--- passes false to it anyway.
        m_ImageLogoMid.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE );
        m_ImageLogoMid.Show( true );
        m_ImageLogoCorner.Show( false );  // Hide the Bohemia Interactive logo

        //--- Replace vanilla's "#str_modded_version_warning0" with our own, and show it. Vanilla
        //--- gates this on g_Game.ReportModded(), which it evaluates in a constructor where g_Game
        //--- is still null (see UpdateLoadingBackground below) - so the widget is effectively left
        //--- at the layout's `visible 0` and we have to turn it on ourselves.
        //---
        //--- The original code also nudged it down by five pixels with GetPos/SetPos. That is NOT
        //--- restored: declared widget geometry is scaled by viewport/1920 while SetPos takes real
        //--- screen pixels, and mixing the two silently misplaces things. The declared spot (pixel
        //--- 100,286, directly under the DayZ logo this mod hides) is fine as it is.
        m_ModdedWarning.SetText( BATTLEROYALE_LOADING_MODDED_MESSAGE );
        m_ModdedWarning.Show( true );

        //--- Drop the background's alpha mask so the loading art is shown whole. The engine answers
        //--- an empty path with `RESOURCES (E): Bad texture name ''` plus
        //--- `GUI (E): ImageWidget::AlphaMaskTexture can't load ''`, and this used to run from
        //--- Show(), i.e. once per loading screen. It is ONCE PER PROCESS here: m_ImageBackground is
        //--- resolved once at dayzgame.c:739 and never re-created, and no vanilla path reloads its
        //--- mask texture - so a single clear holds for the session.
        //---
        //--- Vanilla's SetMaskProgress(0.0) is NOT a substitute, and it looks like one. Vanilla
        //--- Show() already calls it (dayzgame.c:854, and ours runs super first), and ShowEx hands
        //--- m_ImageBackground to ProgressAsync.SetUserData (:774) so the engine drives mask
        //--- progress for the whole load - any one-shot value is overwritten within the frame.
        m_ImageBackground.LoadMaskTexture("");
    }

    override void Show()
    {
        super.Show();

        //--- Re-assert after super, which hides the mid logo on the main-menu loading screen and
        //--- shows it everywhere else (dayzgame.c:863-876). We want the BR logo in both cases, so
        //--- this is unconditional rather than a mirror of vanilla's branch.
        m_ImageLogoMid.Show( true );
        m_ImageLogoCorner.Show( false );  // Hide the Bohemia Interactive logo
        
	    UpdateLoadingBackground();
    }

	void UpdateLoadingBackground()
	{
		//--- The first call comes from inside the DayZGame constructor: dayzgame.c:1065 does
		//--- `m_loading.ShowEx(this)`, and CreateGame only assigns g_Game once that constructor
		//--- has returned (game.c:5). GetGame() is therefore still null here, and GetWorldName()
		//--- below threw "NULL pointer to instance" on every single client launch. It survived
		//--- unnoticed because clients run with -newErrorsAreWarnings, which downgrades the
		//--- exception - the loading screen just silently kept the vanilla background.
		//---
		//--- Bail before touching the static timing state, so the next call still behaves as the
		//--- first one and picks a background immediately rather than waiting out the 5 s hold.
		//--- OnUpdate calls this again on the very next frame while IsLoading(), so nothing is lost.
		if (!GetGame())
			return;
		if (!m_DayZGame)
			return;
		if (!m_Backgrounds)
			return;

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
