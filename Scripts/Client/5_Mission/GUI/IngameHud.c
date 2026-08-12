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

		BR_SuppressPointTag();
	}

	/**
	 *  Keep the "point at somebody to read their name" tag out of the way while the mod is drawing
	 *  its own names over every head.
	 *
	 *  THIS USED TO BE THE FEATURE RATHER THAN ITS SUPPRESSION. The block here called vanilla's
	 *  RefreshPlayerTags / ShowPlayerTag - which ship #ifdef PLATFORM_PS4 and never run on PC
	 *  otherwise - whenever the match had not started, and repainted the text with the resolved
	 *  name. BattleRoyaleLobbyTags replaces it: a name over every non-teammate, which is the shape
	 *  the question "who is in this lobby" actually has.
	 *
	 *  WHAT IS HIDDEN IS THE ROOT, m_PlayerTag, and that choice is what makes this work against DayZ
	 *  Expansion's NameTags addon too. Expansion's modded IngameHud drives the same vanilla-owned
	 *  root and text widgets and parents its own icon inside them, so hiding the root takes the icon
	 *  with it - a hidden parent is not drawn whatever happens to its children. Nothing in vanilla or
	 *  in Expansion ever calls Show(true) on that root; they fade the text alpha instead. So this
	 *  wins regardless of which addon's Update runs first in the modded-class chain, which matters
	 *  because that order is not something this mod's requiredAddons pins down.
	 *
	 *  A no-op when neither is loaded: m_PlayerTag is created lazily by whoever draws the tag, so on
	 *  a server without Expansion it simply stays NULL.
	 */
	protected void BR_SuppressPointTag()
	{
		if ( !m_PlayerTag )
			return;

		//--- Show(true) on the other side of the branch rather than a one-way hide, so the tag comes
		//--- back for the match. Both calls are per-frame and idempotent; the engine no-ops a Show
		//--- that changes nothing.
		m_PlayerTag.Show( !BR_ModDrawsItsOwnNames() );
	}

	//! The mission cast is done here rather than through GetBR(), whose failure path calls the global
	//! Error() - and that halts the script VM. This runs every frame, including the teardown frames
	//! where the cast legitimately comes back NULL.
	protected bool BR_ModDrawsItsOwnNames()
	{
		MissionBaseWorld world = MissionBaseWorld.Cast( GetGame().GetMission() );
		if ( !world )
			return false;

		BattleRoyaleClient client = BattleRoyaleClient.Cast( world.GetBattleRoyale() );
		if ( !client )
			return false;

		return client.IsShowingOwnNameTags();
	}
}
#endif
