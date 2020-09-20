class BattleRoyaleSpectatorPlayerWidget
{
    protected PlayerBase m_Player;
    protected Widget m_ParentWidget;

    protected Widget m_Root;
    protected Widget m_Panel;
    protected Widget m_CharacterPanel;
    protected Widget m_StatsPanel;
    protected TextWidget m_Name;
    protected TextWidget m_Health;
    protected TextWidget m_Kills;
    protected ProgressBarWidget m_HealthProgressBar;

    protected SpectatorRenderMode m_RenderMode;
    protected bool m_Shown;


    void BattleRoyaleSpectatorPlayerWidget(Widget root, PlayerBase player)
    {
        m_ParentWidget = root;

        m_Shown = true;

        m_RenderMode = SpectatorRenderMode.info;

        Init( player );
    }
    void Init(PlayerBase player)
    {
        m_Root = GetGame().GetWorkspace().CreateWidgets("BattleRoyale/GUI/layouts/hud/spectator/player.layout", m_ParentWidget);

        m_Panel = Widget.Cast( m_Root.FindAnyWidget( "InfoPanel" ) );

        m_StatsPanel = Widget.Cast( m_Panel.FindAnyWidget( "StatsPanel" ) );
        m_CharacterPanel = Widget.Cast( m_Panel.FindAnyWidget( "CharacterPanel" ) );

        m_Name = TextWidget.Cast( m_CharacterPanel.FindAnyWidget( "Name" ) );
        m_Health = TextWidget.Cast( m_StatsPanel.FindAnyWidget( "HPTextBar" ) );
        m_Kills = TextWidget.Cast( m_StatsPanel.FindAnyWidget( "KillsTextBar" ) );
        m_HealthProgressBar = ProgressBarWidget.Cast( m_StatsPanel.FindAnyWidget( "HPProgressBar" ) );
        
        
        SetPlayer( player );
    }

    void SetPlayer(PlayerBase player)
    {
        m_Player = player;

        if(m_Player && m_Player.GetIdentity())
        {
            SetName(m_Player.GetIdentity().GetName());
        }
        else
        {
            Error("Failed to find player or identity when initialize spectator icons")
        }

        SetShow( true );
    }
    PlayerBase GetPlayer()
    {
        return m_Player;
    }

    void Delete()
    {
        m_Player = null;
        SetShow( false );
    }

    void SetRenderMode(SpectatorRenderMode mode)
    {
        m_RenderMode = mode;
    }

    void SetShow( bool show )
    {
        m_Shown = show;
        m_Root.Show( show );
    }

    void SetName(string name)
    {
        m_Name.SetText(name);
    }
    void SetHealthText(string health)
    {
        m_Health.SetText( health );
    }
    void SetKillsText(string kills)
    {
        m_Kills.SetText( kills );
    }
    void SetHealthProgress(float value)
    {
        if(value < 0)
            value = 0;
        if(value > 100)
            value = 100;

        m_HealthProgressBar.SetCurrent( value );
    }
    void Update(float timeslice)
    {
        if(!m_Player || (m_Player.GetPlayerState() != EPlayerStates.ALIVE))
        {
            //this icon will be cleaned up in the BRHud class
            SetShow( false );
            return;
        }

        if(UpdatePosition())
        {
            if(m_RenderMode == SpectatorRenderMode.info)
            {
                UpdateStats();
            }
        }
    }
    void UpdateStats()
    {
        if(m_Player)
        {
            float health = m_Player.GetHealth01("", "Health") * 100; //0-100 for HP
            float blood = m_Player.GetHealth("","Blood"); //0-max for blood
            int blood_rounded = Math.Round( blood );
            int health_rounded = Math.Round( health );
            string text = health_rounded.ToString() + " HP | " + blood_rounded.ToString() + " Blood";

            SetHealthText( text );
            SetHealthProgress(health);

            //TODO: get player kills from the server (requires a rework on kills stats storing)
            SetKillsText(m_Player.GetIdentity().GetPlainId()); //temp debugging for plain ids
        }
    }
    bool UpdatePosition()
    {

        vector position = m_Player.GetBonePositionWS( m_Player.GetBoneIndexByName( "Head" ) );
        position = position + Vector(0, 1, 0);//shift hud above head a bit

        vector screen_pos = GetGame().GetScreenPos( position );

        float x = screen_pos[0];
        float y = screen_pos[1];
        float distance = screen_pos[2];
        if(distance > 500)
        {
            SetShow( false );
            return false;
        }
        else
        {
            SetShow( true );
        }
        
        float width;
        float height;
        m_Panel.GetSize( width, height );

        x = x + (width / 2); //center along width
        y = y - height; //align bottom of panel

        if(m_RenderMode == SpectatorRenderMode.names)
        {
            //remove invisible stats panel from y
            m_StatsPanel.GetSize( width, height );
            y = y + height;
        }

        m_Panel.SetPos(x, y, true);

        //TODO: perhaps scale the widget based on distance (so players can get a proper depth perception?)
        //ooo another idea would be to fade out based on distance (between 350 and 500m fade from 100% to 0% visibility)
        //TODO: set name text to include a distance from camera (helps with depth perception)

        return true;
    }
}

enum SpectatorRenderMode
{
    names,
    info
}