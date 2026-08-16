#ifndef SERVER
class BattleRoyaleHud
{
    protected Widget m_Root;

    protected Widget m_PlayerCountPanel;
    protected Widget m_GroupCountPanel;
    protected Widget m_ZoneDistancePanel;
    protected Widget m_KillCountPanel;
    protected Widget m_AudienceCountPanel;
    protected Widget m_CountdownPanel;

    protected TextWidget m_PlayerTextWidget;
    protected TextWidget m_GroupTextWidget;
    protected TextWidget m_DistanceTextWidget;
    protected TextWidget m_KillTextWidget;
    protected TextWidget m_AudienceTextWidget;
    protected TextWidget m_CountdownTextWidget;
    
    protected ImageWidget m_DistanceZoneArrow;
    protected ImageWidget m_ImageClock;

    protected bool is_shown;

    void BattleRoyaleHud( Widget root )
    {
        m_Root = root;

        Init();
        ShowHud( false );
    }

    protected void Init()
    {
        m_PlayerCountPanel = Widget.Cast( m_Root.FindAnyWidget( "PlayerCountPanel" ) );
        m_GroupCountPanel = Widget.Cast( m_Root.FindAnyWidget( "GroupsCountPanel" ) );
        m_ZoneDistancePanel = Widget.Cast( m_Root.FindAnyWidget( "ZoneDistancePanel" ) );
        m_KillCountPanel = Widget.Cast( m_Root.FindAnyWidget( "KillCountPanel" ) );
        m_AudienceCountPanel = Widget.Cast( m_Root.FindAnyWidget( "AudienceCountPanel" ) );
        m_CountdownPanel = Widget.Cast( m_Root.FindAnyWidget( "CountdownPanel" ) );

        m_PlayerTextWidget = TextWidget.Cast( m_PlayerCountPanel.FindAnyWidget( "PlayerText" ) );
        m_GroupTextWidget = TextWidget.Cast( m_GroupCountPanel.FindAnyWidget( "GroupText" ) );
        m_DistanceTextWidget = TextWidget.Cast( m_ZoneDistancePanel.FindAnyWidget( "DistanceText" ) );
        m_KillTextWidget = TextWidget.Cast( m_KillCountPanel.FindAnyWidget( "KillCountText" ) );
        m_AudienceTextWidget = TextWidget.Cast( m_AudienceCountPanel.FindAnyWidget( "AudienceText" ) );
        m_CountdownTextWidget = TextWidget.Cast( m_CountdownPanel.FindAnyWidget( "CountdownText" ) );
        
        m_DistanceZoneArrow = ImageWidget.Cast( m_Root.FindAnyWidget( "ZoneIcon" ) );
        m_ImageClock = ImageWidget.Cast( m_Root.FindAnyWidget( "CountdownIcon" ) );

        m_PlayerCountPanel.Show( false );
        m_GroupCountPanel.Show( false );
        m_ZoneDistancePanel.Show( false );
        m_KillCountPanel.Show( false );
        m_AudienceCountPanel.Show( false );
        m_CountdownPanel.Show( false );
    }

    bool Shown()
    {
        return is_shown;
    }

    void ShowHud( bool show )
    {
        is_shown = show;
        m_Root.Show( show );
    }

    //show/hide control
    void ShowCount( bool show )
    {
        m_PlayerCountPanel.Show( show );
    }

    void ShowDistance( bool show )
    {
        m_ZoneDistancePanel.Show( show );

        //--- The clock and its icon live in CountdownPanel but are coloured from SetDistance, which
        //--- only runs while this panel is up. Without this, a clock that went red stays red for the
        //--- rest of the session once the distance readout goes away - 7_BattleRoyaleLastRound clears
        //--- both circles at LockFinalZone while a countdown is still running, which is exactly that.
        //---
        //--- Guarded, unlike the panel above: HideDistance() runs EVERY FRAME in the lobby, so an
        //--- unresolved widget here would be a per-frame null dereference rather than a one-off.
        if ( !show && m_CountdownTextWidget && m_ImageClock )
        {
            m_CountdownTextWidget.SetColor(ARGB(255, 255, 255, 255));
            m_ImageClock.SetColor(ARGB(255, 255, 255, 255));
        }
    }

    void ShowKillCount( bool show )
    {
        m_KillCountPanel.Show( show );
    }

    //! How many people are watching this player. Hidden outright at zero rather than showing a "0",
    //! same as the kill count above - nobody watching is the normal state and does not need a row.
    void ShowAudienceCount( bool show )
    {
        m_AudienceCountPanel.Show( show );
    }

    void ShowCountdown( bool show )
    {
        m_CountdownPanel.Show( show );
    }

    //value control
    /**
     *  secondsToZone is the deadline for being INSIDE the circle the arrow points at, which is not
     *  always the number printed on the clock - see BattleRoyaleClient.Update and
     *  BattleRoyaleState.SendCountdown.
     *
     *  Passed in rather than read off a member the way `timeRemaining` used to be: SetCountdown only
     *  ran on an edge, from a different call path, and ran AFTER this in the same frame - so the
     *  colour was keyed to a stale value nobody could see was stale.
     */
    void SetDistance(bool isInsideZone, float distExt, float distInt, float angle, int secondsToZone)
    {
        if(!m_DistanceTextWidget)
        {
            Error("Called SetDistance but widget is null!");
            return;
        }

        if ( isInsideZone )
        {
            m_DistanceZoneArrow.SetColor(ARGB(255, 255, 255, 255));
            m_DistanceTextWidget.SetColor(ARGB(255, 255, 255, 255));
            m_CountdownTextWidget.SetColor(ARGB(255, 255, 255, 255));
            m_ImageClock.SetColor(ARGB(255, 255, 255, 255));
        }
        else
        {
            m_DistanceZoneArrow.SetColor(BR_COLOR_ALERT);
            m_DistanceTextWidget.SetColor(BR_COLOR_ALERT);

			// Calculate speed needed to reach the zone in time (m/s)
			float speedNeededToReachZone = 0;

			if (secondsToZone > 0)
			{
				speedNeededToReachZone = distExt / secondsToZone;
			}
			else
			{
				speedNeededToReachZone = 99999; // Infinite speed needed if no time
			}

			//--- Gated so the string is not even BUILT at production log levels. This runs once per
			//--- frame for as long as a player is outside the circle, which used to mean the tail end
			//--- of a round and now means the whole pre-lock phase too - measured at ~60 lines/second
			//--- per client, and string.Format runs before Debug() can reject it.
			if (BattleRoyaleUtils.CheckLogLevel(BattleRoyaleUtils.DEBUG))
				BattleRoyaleUtils.Debug(string.Format("SetDistance: distExt=%1 secondsToZone=%2 speedNeededToReachZone=%3", distExt, secondsToZone, speedNeededToReachZone));

			// Convert m/min thresholds to m/s for comparison (divide by 60)
			float fastThreshold = 400.0 / 60.0;    // 6.67 m/s
			float mediumThreshold = 240.0 / 60.0;  // 4.0 m/s

			if (speedNeededToReachZone > fastThreshold)
			{
				// Need to move faster than 400m/min (6.67m/s) - RED (impossible)
				m_CountdownTextWidget.SetColor(BR_COLOR_ALERT);
				m_ImageClock.SetColor(BR_COLOR_ALERT);
			}
			else if (speedNeededToReachZone > mediumThreshold)
			{
				// Need to move between 240m/min and 400m/min (4-6.67m/s) - ORANGE (difficult)
				m_CountdownTextWidget.SetColor(BR_COLOR_WARN);
				m_ImageClock.SetColor(BR_COLOR_WARN);
			}
			else
			{
				// Need to move less than 240m/min (4m/s) - WHITE (feasible)
				m_CountdownTextWidget.SetColor(ARGB(255, 255, 255, 255));
				m_ImageClock.SetColor(ARGB(255, 255, 255, 255));
			}
        }

        m_DistanceZoneArrow.SetRotation( 0, 0, angle );
        if ( isInsideZone )
        	m_DistanceTextWidget.SetText( "#STR_BR_SAFE" );
        else
        	m_DistanceTextWidget.SetText( Math.Ceil(distExt).ToString() + "m");
    }

    void SetCount(int nb_players, int nb_groups)
    {
        //--- Both widgets, not m_GroupTextWidget twice: the player one is dereferenced unconditionally
        //--- below, so a layout that failed to resolve it reached a null call rather than this guard.
        if(!m_PlayerTextWidget || !m_GroupTextWidget)
        {
            Error("Called SetCount but widget is null!");
            return;
        }

        //BattleRoyaleUtils.Trace(string.Format("SetCount: %1 %2", nb_players, nb_groups));

        m_PlayerTextWidget.SetText( nb_players.ToString() );

        if ( nb_groups == BR_HUD_GROUPS_CONCEALED )
        {
            //--- Endgame concealment: the count exists but is deliberately hidden.
            m_GroupTextWidget.SetText( "???" );
            m_GroupCountPanel.Show( true );
            m_GroupTextWidget.Show( true );
        }
        else if ( nb_groups == BR_HUD_GROUPS_NONE )
		{
            //--- No groups in play, so a group count would be meaningless.
            m_GroupCountPanel.Show( false );
            m_GroupTextWidget.Show( false );
		}
        else
        {
            m_GroupTextWidget.SetText( nb_groups.ToString() );
            m_GroupCountPanel.Show( true );
            //--- Show the text widget again too: the -2 branch above hides it, and nothing else
            //--- ever restored it, so a single -2 left the panel blank for the rest of the session.
            m_GroupTextWidget.Show( true );
        }
    }

    void SetKillCount(int count)
    {
        //--- The widget this method actually writes to. It used to guard m_CountdownTextWidget and
        //--- report "Called SetCountdown", then dereference m_KillTextWidget unguarded - a
        //--- copy-paste that made the guard test the wrong widget in both directions.
        if(!m_KillTextWidget)
        {
            Error("Called SetKillCount but widget is null!");
            return;
        }
        m_KillTextWidget.SetText( count.ToString() );
    }

    void SetAudienceCount(int count)
    {
        if(!m_AudienceTextWidget)
        {
            Error("Called SetAudienceCount but widget is null!");
            return;
        }
        m_AudienceTextWidget.SetText( count.ToString() );
    }

    void SetCountdown(int value)
    {
        if(!m_CountdownTextWidget)
        {
            Error("Called SetCountdown but widget is null!");
            return;
        }

        int seconds = (value % 60);
        string second_string = seconds.ToString();
        if(seconds < 10)
            second_string = "0" + second_string;

        int minutes = (value / 60);
        string minute_string = minutes.ToString();
        if(minutes < 10)
            minute_string = "0" + minute_string;

        string display_str = minute_string + ":" + second_string;

        m_CountdownTextWidget.SetText( display_str );
    }
}
