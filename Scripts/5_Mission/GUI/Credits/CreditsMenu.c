modded class CreditsMenu
{
    override Widget Init()
	{
        layoutRoot = super.Init();

        if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
			Error("Failed to load imageset image");

        return layoutRoot;
    }
}
