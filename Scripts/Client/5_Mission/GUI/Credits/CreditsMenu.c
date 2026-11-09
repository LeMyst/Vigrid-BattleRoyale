#ifndef SERVER
modded class CreditsMenu
{
    override Widget Init()
    {
        layoutRoot = super.Init();

        if(!m_Logo.LoadImageFile( 0, BATTLEROYALE_LOGO_IMAGE ))
            BattleRoyaleUtils.Warn("CreditsMenu: failed to load the logo image");

        return layoutRoot;
    }
}
#endif