#ifndef SERVER
modded class IngameHud
{
    protected bool b_BrVisibilityOverride;

    void IngameHud()
    {
        b_BrVisibilityOverride = false;
    }

    void BR_HIDE()
    {
        Show( false );

        b_BrVisibilityOverride = true;
    }

    override void Show( bool show )
    {
        if(b_BrVisibilityOverride)
            return;

        super.Show( show );
    }

#ifdef EXPANSIONMODNAMETAGS
	override protected bool HandleCurrentTaggedPlayer(float timeslice)
	{
		bool result = super.HandleCurrentTaggedPlayer(timeslice);

		DayZPlayerImplement localPlayer = DayZPlayerImplement.Cast(GetGame().GetPlayer());
		if ( localPlayer.Expansion_IsInSafeZone() )
		{
			if (m_IsFriendly || m_IsMember)
			{
				m_PlayerTagText.SetColor(ARGB( 255, 0, 190, 255 ));
			}
			else
			{
				m_PlayerTagText.SetColor( COLOR_EXPANSION_NOTIFICATION_INFO );
			}
		}

		return result;
	}
#endif
}
#endif
