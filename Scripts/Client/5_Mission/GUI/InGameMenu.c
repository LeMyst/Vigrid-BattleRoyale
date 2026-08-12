//this is the MPInterrupt UI
#ifndef SERVER
modded class InGameMenu
{
    override Widget Init()
    {
        Widget result = super.Init();

        //BR logo
        if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
            Error("Failed to load imageset image");

        SetServerInfoVisibility( false ); //Don't ever show what server you're on for DayZBR

        HideExpansionNewsFeed();

        return result;
    }

    //--- Expansion builds its news feed in InGameMenu.Init() and parents it to layoutRoot. Its
    //--- default links are placeholder Discord/Twitter entries pointing at google.com, and the
    //--- default text is "CHANGE ME". None of it belongs on a DayZBR server, so drop the panel.
    //--- Deliberately reached through m_NewsFeed rather than by widget name: naming the
    //--- ExpansionNewsFeed type here would be a hard compile dependency, the member is not.
    protected void HideExpansionNewsFeed()
    {
        if ( m_NewsFeed )
            m_NewsFeed.GetLayoutRoot().Show( false );
    }

    //--- Expansion_OnGeneralSettingsUpdated() runs again whenever the server pushes its general
    //--- settings, which is after Init() on a fresh connect. Today it only ever hides the feed, so
    //--- this override is insurance against that changing rather than a fix for a live re-show.
    override void Expansion_OnGeneralSettingsUpdated()
    {
        super.Expansion_OnGeneralSettingsUpdated();

        HideExpansionNewsFeed();
    }

    override protected void SetGameVersion()
    {
        TextWidget version_widget = TextWidget.Cast( layoutRoot.FindAnyWidget("version") );

        string version;
        GetGame().GetVersion( version );

        if ( GetDayZGame() )
        {
            string expansion_version = GetDayZGame().GetExpansionClientVersion();
            string dayzbr_version = GetDayZGame().GetBattleRoyaleClientVersion();
            version_widget.SetText( "Client #main_menu_version" + " " + version + " | " + expansion_version + " | " + dayzbr_version );
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
	}
}
#endif