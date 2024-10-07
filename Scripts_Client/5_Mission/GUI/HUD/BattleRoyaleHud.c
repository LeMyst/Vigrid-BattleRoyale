#ifndef SERVER
class BattleRoyaleHud
{
    protected Widget m_Root;

    protected Widget m_PlayerCountPanel;
    protected Widget m_GroupCountPanel;
    protected Widget m_ZoneDistancePanel;
    protected Widget m_KillCountPanel;
    protected Widget m_CountdownPanel;

    protected TextWidget m_PlayerTextWidget;
    protected TextWidget m_GroupTextWidget;
    protected TextWidget m_DistanceTextWidget;
    protected TextWidget m_KillTextWidget;
    protected TextWidget m_CountdownTextWidget;
    
    protected ImageWidget m_DistanceZoneArrow;
    protected ImageWidget m_ImageClock;

#ifdef SPECTATOR
    protected ref array<ref BattleRoyaleSpectatorPlayerWidget> m_SpectatorWidgets;
    protected bool show_spectator;
#endif

    protected bool is_shown;

    protected int timeRemaining;

    void BattleRoyaleHud( Widget root )
    {
        m_Root = root;

        Init();
        ShowHud( false );
    }

    protected void Init()
    {
#ifdef SPECTATOR
        m_SpectatorWidgets = new array<ref BattleRoyaleSpectatorPlayerWidget>();
#endif

        m_PlayerCountPanel = Widget.Cast( m_Root.FindAnyWidget( "PlayerCountPanel" ) );
        m_GroupCountPanel = Widget.Cast( m_Root.FindAnyWidget( "GroupsCountPanel" ) );
        m_ZoneDistancePanel = Widget.Cast( m_Root.FindAnyWidget( "ZoneDistancePanel" ) );
        m_KillCountPanel = Widget.Cast( m_Root.FindAnyWidget( "KillCountPanel" ) );
        m_CountdownPanel = Widget.Cast( m_Root.FindAnyWidget( "CountdownPanel" ) );

        m_PlayerTextWidget = TextWidget.Cast( m_PlayerCountPanel.FindAnyWidget( "PlayerText" ) );
        m_GroupTextWidget = TextWidget.Cast( m_GroupCountPanel.FindAnyWidget( "GroupText" ) );
        m_DistanceTextWidget = TextWidget.Cast( m_ZoneDistancePanel.FindAnyWidget( "DistanceText" ) );
        m_KillTextWidget = TextWidget.Cast( m_KillCountPanel.FindAnyWidget( "KillCountText" ) );
        m_CountdownTextWidget = TextWidget.Cast( m_CountdownPanel.FindAnyWidget( "CountdownText" ) );
        
        m_DistanceZoneArrow = ImageWidget.Cast( m_Root.FindAnyWidget( "ZoneIcon" ) );
        m_ImageClock = ImageWidget.Cast( m_Root.FindAnyWidget( "CountdownIcon" ) );

        m_PlayerCountPanel.Show( false );
        m_GroupCountPanel.Show( false );
        m_ZoneDistancePanel.Show( false );
        m_KillCountPanel.Show( false );
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
    }

    void ShowKillCount( bool show )
    {
        m_KillCountPanel.Show( show );
    }

    void ShowCountdown( bool show )
    {
        m_CountdownPanel.Show( show );
    }

    //value control
    void SetDistance(bool isInsideZone, float distExt, float distInt, float angle)
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
            m_DistanceZoneArrow.SetColor(COLOR_EXPANSION_NOTIFICATION_ERROR);
            m_DistanceTextWidget.SetColor(COLOR_EXPANSION_NOTIFICATION_ERROR);

            if ( timeRemaining < (distExt / 8) )
            {
                m_CountdownTextWidget.SetColor(COLOR_EXPANSION_NOTIFICATION_ORANGE);
                m_ImageClock.SetColor(COLOR_EXPANSION_NOTIFICATION_ORANGE);
            }
            else if ( timeRemaining < (distExt / 6) )
            {
                m_CountdownTextWidget.SetColor(COLOR_EXPANSION_NOTIFICATION_ERROR);
                m_ImageClock.SetColor(COLOR_EXPANSION_NOTIFICATION_ERROR);
            }
            else
            {
                m_CountdownTextWidget.SetColor(ARGB(255, 255, 255, 255));
                m_ImageClock.SetColor(ARGB(255, 255, 255, 255));
            }
        }

        m_DistanceZoneArrow.SetRotation( 0, 0, angle );
        if ( isInsideZone )
        	m_DistanceTextWidget.SetText( "safe" );
        else
        	m_DistanceTextWidget.SetText( Math.Ceil(distExt).ToString() + "m");
    }

    void SetCount(int nb_players, int nb_groups)
    {
        if(!m_GroupTextWidget || !m_GroupTextWidget)
        {
            Error("Called SetCount but widget is null!");
            return;
        }

        //BattleRoyaleUtils.Trace(string.Format("SetCount: %1 %2", nb_players, nb_groups));
        
        m_PlayerTextWidget.SetText( nb_players.ToString() );

        if ( nb_groups == -1 )
        {
            m_GroupTextWidget.SetText( "???" );
        }
        else if ( nb_groups == -2 )
		{
            m_GroupCountPanel.Show( false );
            m_GroupTextWidget.Show( false );
		}
        else
        {
            m_GroupTextWidget.SetText( nb_groups.ToString() );
            m_GroupCountPanel.Show( true );
            //m_GroupCountPanel.Show(nb_groups > 0);
        }
    }

    void SetKillCount(int count)
    {
        if(!m_CountdownTextWidget)
        {
            Error("Called SetCountdown but widget is null!");
            return;
        }
        m_KillTextWidget.SetText( count.ToString() );
    }

    void SetCountdown(int value)
    {
        if(!m_CountdownTextWidget)
        {
            Error("Called SetCountdown but widget is null!");
            return;
        }

        timeRemaining = value;

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

#ifdef SPECTATOR
    void Update(float timeslice)
    {
        if(show_spectator)
        {
            array<PlayerBase> players;
            PlayerBase.GetLocalPlayers( players );
            int i;

            //1. iterate over all players, assign them to spectator widgets (in order)
            int index = 0;
            for(i = 0; i < players.Count(); i++)
            {
                //don't render whatever getplayer returns (maybe this will fix the issue with freecam showing the spectator's deleted player on HUD?)
                if(players[i] == PlayerBase.Cast( GetGame().GetPlayer() ) )
                    continue;

                if(index == m_SpectatorWidgets.Count())
                {
                    //not enough spectator widgets! create one!
                    m_SpectatorWidgets.Insert( CreatePlayerWidget( players[i] ) );
                }
                else
                {
                    //widget already exists! update it's player!
                    m_SpectatorWidgets[index].SetPlayer( players[i] );
                }

                //update the widget! (position and stats)
                m_SpectatorWidgets[index].Update(timeslice);

                index++;
            }
            //2. iterate over excess spectator widgets and disable (delete) them.
            for(i = players.Count(); i < m_SpectatorWidgets.Count(); i++)
            {
                m_SpectatorWidgets[i].Delete();
            }
        }
    }

    void InitSpectator()
    {
        show_spectator = true;
    }

    //--- spectator!
    ref BattleRoyaleSpectatorPlayerWidget CreatePlayerWidget(PlayerBase player)
    {
        ref BattleRoyaleSpectatorPlayerWidget player_widget = new BattleRoyaleSpectatorPlayerWidget(m_Root, player);
        return player_widget;
    }
#endif
}
#endif
