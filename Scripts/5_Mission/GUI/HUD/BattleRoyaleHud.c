class BattleRoyaleHud 
{
    protected Widget m_Root;

    protected Widget m_PlayerCountPanel;
    protected Widget m_ZoneDistancePanel;
    protected Widget m_KillCountPanel;
    protected Widget m_CountdownPanel;
    
    protected TextWidget m_CountTextWidget;
    protected TextWidget m_DistanceTextWidget;
    protected TextWidget m_KillTextWidget;
    protected TextWidget m_CountdownTextWidget;

    protected ref array<ref BattleRoyaleSpectatorPlayerWidget> m_SpectatorWidgets;
    protected bool show_spectator;    

    protected bool is_shown;

    void BattleRoyaleHud( Widget root )
    {
        m_Root = root;
        
        Init();
        ShowHud( false );
    }


    protected void Init()
    {
        m_SpectatorWidgets = new array<ref BattleRoyaleSpectatorPlayerWidget>();

        m_PlayerCountPanel = Widget.Cast( m_Root.FindAnyWidget( "PlayerCountPanel" ) );
        m_ZoneDistancePanel = Widget.Cast( m_Root.FindAnyWidget( "ZoneDistancePanel" ) );
        m_KillCountPanel = Widget.Cast( m_Root.FindAnyWidget( "KillCountPanel" ) );
        m_CountdownPanel = Widget.Cast( m_Root.FindAnyWidget( "CountdownPanel" ) );

        m_CountTextWidget = TextWidget.Cast( m_PlayerCountPanel.FindAnyWidget( "CountText" ) );
        m_DistanceTextWidget = TextWidget.Cast( m_ZoneDistancePanel.FindAnyWidget( "DistanceText" ) );
        m_KillTextWidget = TextWidget.Cast( m_KillCountPanel.FindAnyWidget( "KillCountText" ) );
        m_CountdownTextWidget = TextWidget.Cast( m_CountdownPanel.FindAnyWidget( "CountdownText" ) );

        m_PlayerCountPanel.Show( false );
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
    void SetDistance(float value)
    {
        if(!m_DistanceTextWidget)
        {
            Error("Called SetDistance but widget is null!");
            return;
        }
        if(value <= 0)
        {
            m_DistanceTextWidget.SetText( "safe" );
        }
        else
        {
            m_DistanceTextWidget.SetText( Math.Round(value).ToString() + "m");//2000m for example
        }
    }
    void SetCount(int count)
    {
        if(!m_CountTextWidget)
        {
            Error("Called SetCount but widget is null!");
            return;
        }

        m_CountTextWidget.SetText( count.ToString() );   
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
        int seconds = (value % 60);
        int minutes = (value / 60); //int conversion should auto floor this

        string minute_string = minutes.ToString();
        string second_string = seconds.ToString();
        if(seconds < 10)
        {
            second_string = "0" + second_string;
        }

        //don't display 0 minutes
        string display_str = second_string;
        if(minutes > 0)
        {
            display_str = minute_string + ":" + display_str;
        }

        m_CountdownTextWidget.SetText( display_str );
    }

    void Update(float timeslice)
    {
        if(show_spectator)
        {
            array<PlayerBase> players;
            PlayerBase.GetLocalPlayers( players ); //This doesn't seem to work on client.... wtf
            int i;

            //1. iterate over all players, assign them to spectator widgets (in order)
            int index = 0;
            for(i = 0; i < players.Count(); i++)
            {
                //don't render whatever getplayer returns (maybe this will fix the issue with freecam showing the spectator's deleted player on HUD?)
                if(players[i] == GetGame().GetPlayer() )
                    continue;

                if(index == m_SpectatorWidgets.Count())
                {
                    //not enough spectator widgets! create one!
                    Print("Spectator: Creating Player Info Widget");
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
                Print("Spectator: Deleting Player Info Widget");
                m_SpectatorWidgets[i].Delete();
            }
        }
    }
    void InitSpectator()
    {
        Print("Showing Spectator Hud");
        show_spectator = true;
    }

    //--- spectator!
    ref BattleRoyaleSpectatorPlayerWidget CreatePlayerWidget(PlayerBase player)
    {
        ref BattleRoyaleSpectatorPlayerWidget player_widget = new BattleRoyaleSpectatorPlayerWidget(m_Root, player);
        return player_widget;
    }
}