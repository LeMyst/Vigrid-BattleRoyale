//this is the MPInterrupt UI
#ifndef SERVER
modded class InGameMenu
{
    override Widget Init()
    {
        Widget layoutRoot = super.Init();

        //BR logo
        if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
            Error("Failed to load imageset image");

        SetServerInfoVisibility( false ); //Don't ever show what server you're on for DayZBR

        // Change respawn button label to spectate
        ButtonSetText(m_RespawnButton, "#STR_BR_MENU_SPECTATE");

        return layoutRoot;
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
		m_RestartButton.Show( false );  // Hide the restart button
		m_FeedbackButton.Show( false ); // Hide the feedback button
	}
}
#endif