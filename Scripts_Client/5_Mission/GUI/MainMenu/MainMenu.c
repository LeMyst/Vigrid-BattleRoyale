#ifndef SERVER
modded class MainMenu
{
    protected ImageWidget m_Logo;

    override Widget Init()
    {
        Print("INITIALIZING BR IN ONLINE MODE");

        super.Init(); // this calls dayz expansion init

        m_Logo = ImageWidget.Cast( layoutRoot.FindAnyWidget( "dayz_logo" ) );
        if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
            Error("Failed to load imageset image");

        //m_NewsfeedPanel.Show( false ); //--- don't show dayz expansion news feed

        m_ChooseServer.Show( false ); // Don't show choose server
        m_TutorialButton.Show( false ); // Don't show tutorial button
        //m_MessageButton.Show( false ); // Don't show credits button
        ref array<string> funny_strings = {
			"Bro, it's DayZ but with Battle Royale stuff!",
			"Welcome to the wild world of DayZ Battle Royale, where the only certainty is uncertainty.",
			"Thou whom read this message, S1Q.",
			"Next time, try to aim better...",
			"Just so you know, you're allergic to bullets.",
			"You Miss 100% Of The Shots You Don't Take.",
			"I should have improved the mod instead of writing this sentence.",
			"Remember, bushes aren't bulletproof.",
			"If at first, you don't succeed, respawn and try again.",
			"Pro tip: Running in a straight line isn't the best strategy.",
			"Warning: May cause sudden addiction to adrenaline and respawn buttons.",
			"Just a friendly reminder: camping in this game is for tents, not players.",
			"Remember, it's not about how many kills you get, but how stylishly you avoid death.",
			"Embrace the chaos, but don't forget to reload.",
			"Just when you thought it was safe to loot...",
			"In this game, karma has a faster firing rate than any weapon."
        };
        m_ModdedWarning.SetText(funny_strings.GetRandomElement());
        //m_ModdedWarning.Show( false ); // Hide modded message

        string version;
        GetGame().GetVersion( version );

        if ( GetDayZGame() )
        {
            string expansion_version = GetDayZGame().GetExpansionClientVersion();
            string dayzbr_version = GetDayZGame().GetBattleRoyaleClientVersion();
            m_Version.SetText( "Client #main_menu_version" + " " + version + " | " + expansion_version + " | " + dayzbr_version );
        }
        else
        {
            m_Version.SetText( "DayZ SA #main_menu_version" + " " + version );
        }

        if(OnStart())
        {
            Print("You are running a developer build of DayZBR");
        }
        else
        {
            Error("Something Went Wrong! BR Failed To Start?!");
        }

        m_LastPlayedTooltip.Show( false );// ensure last played is not shown

        return layoutRoot;
    }

    bool OnStart(bool force_restart = false)
    {
        Print("DAYZ BATTLE ROYALE INIT");
        BiosUserManager p_UserManager = GetGame().GetUserManager();
        if(!p_UserManager)
        {
            Error("DBR ERROR: p_UserManager = NULL");
            return false;
        }

        BiosUser p_User = p_UserManager.GetTitleInitiator();
        if(!p_User)
        {
            Error("DBR ERROR: p_User = NULL");
            return false;
        }

        return true;
    }

    override void LoadMods()
    {
        super.LoadMods(); //initialize like normal

        //No BR mod found? ya just delete the lists....
        if( m_ModsSimple )
            delete m_ModsSimple;
        
        if( m_ModsDetailed )
            delete m_ModsDetailed;
    }

    private void LoadBRModEntry(ref ModInfo dbr_mod_info)
    {
        if( m_ModsSimple )
            delete m_ModsSimple;

        ref array<ref ModInfo> modArray = new array<ref ModInfo>;
        modArray.Insert(dbr_mod_info);
        m_ModsSimple = new ModsMenuSimple(modArray, layoutRoot.FindAnyWidget("ModsSimple"), m_ModsDetailed);
    }
}
#endif
