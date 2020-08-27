class BattleRoyaleHud 
{
    protected Widget m_Root;

    protected Widget m_PlayerCountPanel;
    protected Widget m_ZoneDistancePanel;
    
    protected TextWidget m_CountTextWidget;
    protected TextWidget m_DistanceTextWidget;

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
        m_ZoneDistancePanel = Widget.Cast( m_Root.FindAnyWidget( "ZoneDistancePanel" ) );

        m_CountTextWidget = TextWidget.Cast( m_PlayerCountPanel.FindAnyWidget( "CountText" ) );
        m_DistanceTextWidget = TextWidget.Cast( m_PlayerCountPanel.FindAnyWidget( "DistanceText" ) );

        m_PlayerCountPanel.Show( false );
        m_ZoneDistancePanel.Show( false );
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
}