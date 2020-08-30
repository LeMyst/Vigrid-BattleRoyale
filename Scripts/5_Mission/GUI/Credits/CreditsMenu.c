modded class CreditsMenu
{
    override Widget Init()
	{
        layoutRoot = super.Init();

        if(!m_Logo.LoadImageFile( 0, "set:battleroyale_gui image:DayZBRLogo_White" ))
			Error("Failed to load imageset image");

        

        return layoutRoot;
    }
}