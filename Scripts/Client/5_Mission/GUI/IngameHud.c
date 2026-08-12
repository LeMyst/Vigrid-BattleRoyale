#ifndef SERVER
modded class IngameHud
{
	protected bool b_BrVisibilityOverride;

	void IngameHud()
	{
		b_BrVisibilityOverride = false;
	}

	override void Show( bool show )
	{
		if(b_BrVisibilityOverride)
			return;

		super.Show( show );
	}

	override void Update( float timeslice )
	{
		super.Update( timeslice );

		// Show player tags only if the match is not started
		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
		if( !br_rpc.match_started )
		{
			RefreshPlayerTags();
			ShowPlayerTag(timeslice);
			if ( m_CurrentTaggedPlayer && m_CurrentTaggedPlayer.GetIdentity() )
			{
				Widget m_TagFrame = m_PlayerTag.FindAnyWidget( "TagFrame" );
				m_TagFrame.SetSize( 300, 25 );

				//--- Vanilla's ShowPlayerTag just wrote GetPlainName() into the tag, and on the client
				//--- that is still the launcher name - the correction only exists on the server. Repaint
				//--- it here, after super has run, rather than reimplementing the tag's own fade and
				//--- placement logic. Falls through to the same string when nothing was resolved.
				if ( m_PlayerTagText )
					m_PlayerTagText.SetText( br_rpc.ResolveDisplayName( m_CurrentTaggedPlayer.GetIdentity() ) );
			}
		}
	}
}
#endif
