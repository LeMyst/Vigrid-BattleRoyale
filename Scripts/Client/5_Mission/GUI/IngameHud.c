#ifndef SERVER
modded class IngameHud
{
	protected bool b_BrVisibilityOverride;

	void IngameHud()
	{
		b_BrVisibilityOverride = false;
	}

#ifdef SPECTATOR
	void BR_HIDE()
	{
		Show( false );

		b_BrVisibilityOverride = true;
	}
#endif

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
			}
		}
	}
}
#endif
