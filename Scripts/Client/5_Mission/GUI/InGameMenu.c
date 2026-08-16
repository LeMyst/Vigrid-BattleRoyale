//this is the MPInterrupt UI
#ifndef SERVER
modded class InGameMenu
{
    override Widget Init()
    {
        Widget result = super.Init();

        //--- BR logo. Resolved locally rather than through Expansion's m_Logo member: that member is
        //--- declared on Expansion's modded InGameMenu, so reading it made this file impossible to
        //--- compile without Expansion loaded - and it was only ever a cached lookup of the VANILLA
        //--- widget "dayz_logo" (day_z_ingamemenu.layout). Finding it ourselves costs one call, works
        //--- with Expansion and without, and needs no #ifdef.
        //---
        //--- Order still holds when Expansion IS loaded: super.Init() above is where it sets its own
        //--- logo, so this overwrite lands after it exactly as it did before.
        ImageWidget logo = ImageWidget.Cast( layoutRoot.FindAnyWidget("dayz_logo") );
        if ( logo )
        {
            if(!logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
                Error("Failed to load imageset image");
        }

        SetServerInfoVisibility( false ); //Don't ever show what server you're on for DayZBR

        HideExpansionNewsFeed();

        return result;
    }

    //--- Expansion builds its news feed in InGameMenu.Init() and parents it to layoutRoot. Its
    //--- default links are placeholder Discord/Twitter entries pointing at google.com, and the
    //--- default text is "CHANGE ME". None of it belongs on a DayZBR server, so drop the panel.
    //---
    //--- ⚠️ THE WHOLE BODY IS GUARDED, and the old comment here claimed the opposite - that reaching
    //--- the feed through m_NewsFeed rather than by naming the ExpansionNewsFeed type kept this off
    //--- Expansion. It does not: m_NewsFeed is itself declared on Expansion's modded InGameMenu, so
    //--- the member is exactly as hard a dependency as the type would have been. Without the guard
    //--- this file simply does not compile when Expansion is absent - measured, "Can't find variable
    //--- 'm_NewsFeed'", and it took the whole Mission module down with it.
    //---
    //--- DZ_Expansion is Expansion's CfgMods CLASS NAME, which the engine auto-defines for every
    //--- loaded mod - there is no defines[] entry to look for, and Expansion declares none. The same
    //--- mechanism is why DabsFramework and JM_CommunityFramework are defined.
    protected void HideExpansionNewsFeed()
    {
#ifdef DZ_Expansion
        if ( m_NewsFeed )
            m_NewsFeed.GetLayoutRoot().Show( false );
#endif
    }

    //--- Expansion_OnGeneralSettingsUpdated() runs again whenever the server pushes its general
    //--- settings, which is after Init() on a fresh connect. Today it only ever hides the feed, so
    //--- this override is insurance against that changing rather than a fix for a live re-show.
    //---
    //--- The override itself has to be inside the guard, not just its body: overriding a method that
    //--- does not exist is an error in its own right ("marked as override, but there is no function
    //--- with this name in the base class"), whatever the body does.
#ifdef DZ_Expansion
    override void Expansion_OnGeneralSettingsUpdated()
    {
        super.Expansion_OnGeneralSettingsUpdated();

        HideExpansionNewsFeed();
    }
#endif

    override protected void SetGameVersion()
    {
        TextWidget version_widget = TextWidget.Cast( layoutRoot.FindAnyWidget("version") );

        string version;
        GetGame().GetVersion( version );

        if ( GetDayZGame() )
        {
            //--- The Expansion segment is optional: GetExpansionClientVersion() is declared on
            //--- Expansion Core's modded DayZGame, so calling it unguarded made this file - and with
            //--- it the whole Mission module - fail to compile whenever Expansion was absent.
            //--- DZ_Expansion_Core is Core's CfgMods class name; see HideExpansionNewsFeed above.
            string version_line = "Client #main_menu_version" + " " + version;
#ifdef DZ_Expansion_Core
            version_line = version_line + " | " + GetDayZGame().GetExpansionClientVersion();
#endif
            version_line = version_line + " | " + GetDayZGame().GetBattleRoyaleClientVersion();
            version_widget.SetText( version_line );
        }
        else
        {
            version_widget.SetText( "DayZ SA #main_menu_version" + " " + version );
        }
    }

	override protected void UpdateGUI()
	{
		super.UpdateGUI();
		m_RespawnButton.Show( false );  // Hide the respawn button
		m_RestartButton.Show( false );  // Hide the restart button

		//--- Vanilla hides Continue unless the player is ALIVE (ingamemenu.c:335), and a spectator
		//--- never is - so without this the menu has no ordinary way out.
		BattleRoyaleClient br_client = BattleRoyaleClient.Cast( GetBR() );
		if( br_client && br_client.IsSpectating() )
			m_ContinueButton.Show( true );
	}

	/**
	 *  Defence in depth. The respawn button is already hidden, but vanilla's OnClick_Respawn calls
	 *  GameRespawn() -> g_Game.RespawnPlayer(), which would drop a fresh, untracked, zone-immune
	 *  character into a running match. Deliberately does NOT chain super.
	 */
	override protected void OnClick_Respawn()
	{
		BattleRoyaleUtils.Trace("InGameMenu: respawn suppressed");
	}
}
#endif