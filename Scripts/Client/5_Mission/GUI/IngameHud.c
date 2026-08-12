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

	//--- The one hook that covers both paths: vanilla Init() calls this once at startup, and
	//--- RespawnDialogue calls it again after a respawn. Everything below is idempotent, so running
	//--- twice is harmless.
	override void InitBadgesAndNotifiers()
	{
		super.InitBadgesAndNotifiers();

		BR_HideSurvivalNotifiers();
	}

	//--- Hide the thirst, hunger and temperature notifiers, plus the divider that separates them from
	//--- Blood, and close the gap that leaves in front of the badge group. See
	//--- BR_HIDE_SURVIVAL_NOTIFIERS in BattleRoyaleConstants.c for why they are dead weight here.
	//---
	//--- Each Show(false) targets the PARENT panel, never the Icon<Name> image inside it: vanilla
	//--- unconditionally Show(true)s those icons right above us in super, and DisplayTendency keeps
	//--- tinting them. A hidden parent is not drawn whatever happens to its children, so nothing has
	//--- to be fought frame by frame. Nothing in P:\scripts ever shows these panels themselves.
	protected void BR_HideSurvivalNotifiers()
	{
		if ( !BR_HIDE_SURVIVAL_NOTIFIERS )
			return;

		if ( !m_Notifiers || !m_HudPanelWidget )
			return;

		Widget w;

		w = m_Notifiers.FindAnyWidget( "Thirsty" );
		if ( w )
			w.Show( false );

		w = m_Notifiers.FindAnyWidget( "Hungry" );
		if ( w )
			w.Show( false );

		//--- Takes the heat-buffer arrows and the temperature readout with it, both being children.
		w = m_Notifiers.FindAnyWidget( "Temperature" );
		if ( w )
			w.Show( false );

		//--- Referenced by no vanilla script at all - only BadgeNotifierDivider is managed, by
		//--- IngameHudVisibility's NO_BADGE flag - so one Show(false) settles it for good.
		w = m_Notifiers.FindAnyWidget( "NotifierDivider" );
		if ( w )
			w.Show( false );

		//--- Absolute x, and the widget's own y, so a second pass after a respawn is a no-op rather
		//--- than a second shift.
		float pos_x;
		float pos_y;

		w = m_HudPanelWidget.FindAnyWidget( "BadgesSpacer" );
		if ( w )
		{
			w.GetPos( pos_x, pos_y );
			w.SetPos( BR_HUD_BADGES_SPACER_X, pos_y );
		}

		w = m_HudPanelWidget.FindAnyWidget( "BadgesPanel" );
		if ( w )
		{
			w.GetPos( pos_x, pos_y );
			w.SetPos( BR_HUD_BADGES_PANEL_X, pos_y );
		}
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
