modded class LoadingScreen
{
    //TODO: move this to a constant perhaps?
    static ref array<string> GetLoadingScreens()
    {
        ref array<string> result = new array<string>();

        result.Insert( BATTLEROYALE_LOADING_SCREENS_PATH + "br_loading_1.edds" );
        result.Insert( BATTLEROYALE_LOADING_SCREENS_PATH + "br_loading_2.edds" );

        return result;
    }

    void LoadingScreen(DayZGame game)
	{
        //DayZExpansion should be called first

        if(!m_ImageLogoMid.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
			Error("Failed to load imageset image");

        if(!m_ImageLogoCorner.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
			Error("Failed to load imageset image");

        m_ModdedWarning.SetText( BATTLEROYALE_LOADING_MODDED_MESSAGE );


        float x;
        float y;
        m_ModdedWarning.GetPos(x, y);
        m_ModdedWarning.SetPos( x, y + 5 ); //add a buffer of 5 pixels


        /*
        int i;
        ref ExpansionLoadingScreenBackground background;

        //TODO: config these up maybe?
        
        //these loading screens are *very* bad :)
        array<string> m_BadPaths = {
            "DayZExpansion/GUI/textures/loading_screens/loading_screen_11_co.edds",
            "DayZExpansion/GUI/textures/loading_screens/loading_screen_7_co.edds"
        };
        array<string> m_NewPaths = {
            BATTLEROYALE_LOADING_SCREENS_PATH + "br_loading_1.edds",
            BATTLEROYALE_LOADING_SCREENS_PATH + "br_loading_2.edds"
        };
        
        //delete files defined in m_BadPaths
        array<ref ExpansionLoadingScreenBackground> m_DeleteThese = new array<ref ExpansionLoadingScreenBackground>();
        for(i = 0; i < m_Backgrounds.Count(); i++)
        {
            background = m_Backgrounds[i];

            if(m_BadPaths.Find( background.Path ) != -1)
            {
                m_DeleteThese.Insert(background);
            }
        }
        for(i = 0; i < m_DeleteThese.Count(); i++)
        {
            m_Backgrounds.RemoveItem(m_DeleteThese[i]);
        }
        
        //add new paths
        for(i = 0; i < m_NewPaths.Count(); i++)
        {
            background = new ExpansionLoadingScreenBackground( m_NewPaths[i] );

            m_Backgrounds.Insert( background );
        }
        
        //JsonFileLoader< ref array< ref ExpansionLoadingScreenBackground > >.JsonLoadFile( "DayZExpansion/Scripts/Data/LoadingImages.json", m_Backgrounds );
        */
    }

    override void Show()
    {
        m_Backgrounds.Clear(); 
        ref array<string> loading_screens = LoadingScreen.GetLoadingScreens();
        for(int i = 0; i < loading_screens.Count(); i++)
        {
            m_Backgrounds.Insert(new ExpansionLoadingScreenBackground( loading_screens[i] ));
        }
        super.Show();
    }

    
}