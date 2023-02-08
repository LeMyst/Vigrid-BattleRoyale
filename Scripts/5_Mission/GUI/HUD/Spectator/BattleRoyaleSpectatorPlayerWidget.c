class BattleRoyaleSpectatorPlayerWidget
{
    protected PlayerBase m_Player;
    protected Widget m_ParentWidget;

    protected Widget m_Root;
    protected Widget m_Panel;
    protected Widget m_CharacterPanel;
    protected Widget m_StatsPanel;
    protected TextWidget m_BRName;
    protected TextWidget m_Health;
    protected TextWidget m_Kills;
    protected ProgressBarWidget m_HealthProgressBar;

    protected SpectatorRenderMode m_RenderMode;
    protected bool m_Shown;

    protected float panel_max_w = 256;
    protected float panel_max_h = 96;

    protected float panel_min_w = 64;
    protected float panel_min_h = 24;

    float scale_max_dist = 10;
    float scale_min_dist = 100;

    protected float render_max_distance = 500;
    protected float fade_max_distance = 400;

    void BattleRoyaleSpectatorPlayerWidget(Widget root, PlayerBase player)
    {
        m_ParentWidget = root;

        m_Shown = true;

        m_RenderMode = SpectatorRenderMode.info;

        Init( player );
    }

    void Init(PlayerBase player)
    {
        m_Root = GetGame().GetWorkspace().CreateWidgets("DayZBR-Mod/GUI/layouts/hud/spectator/player.layout", m_ParentWidget);

        m_Panel = Widget.Cast( m_Root.FindAnyWidget( "InfoPanel" ) );

        m_StatsPanel = Widget.Cast( m_Panel.FindAnyWidget( "StatsPanel" ) );
        m_CharacterPanel = Widget.Cast( m_Panel.FindAnyWidget( "CharacterPanel" ) );

        m_BRName = TextWidget.Cast( m_CharacterPanel.FindAnyWidget( "Name" ) );
        m_Health = TextWidget.Cast( m_StatsPanel.FindAnyWidget( "HPTextBar" ) );
        m_Kills = TextWidget.Cast( m_StatsPanel.FindAnyWidget( "KillsTextBar" ) );
        m_HealthProgressBar = ProgressBarWidget.Cast( m_StatsPanel.FindAnyWidget( "HPProgressBar" ) );
        
        
        SetPlayer( player );
    }

    void SetPlayer(PlayerBase player)
    {
        m_Player = player;

        if(m_Player)
        {
            if(m_Player.GetIdentity())
            {
                SetName(m_Player.GetIdentity().GetName());
            }
            else
            {
                Error("Failed to find identity when initialize spectator icons");
            }
        }
        else
        {
            Error("Failed to find player when initialize spectator icons");
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
        m_BRName.SetText(name);
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
        //TODO: need another sanity check here (some elements get gliched and stay on the Hud (I think because of our new render distance settings in config))
    }

    void UpdateStats()
    {
        if(m_Player)
        {
            if(m_Player.health_percent == -1 || m_Player.blood_percent == -1)
            {
                m_Player.UpdateHealthStats(1, 1);
                //request these stats from the server
                Print("Unknown player stats! Requesting stats from the server!");
                GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestEntityHealthUpdate", new Param1<int>( 1 ), true, NULL, m_Player);
            }
            float health = m_Player.health_percent * 100; //0-100 for HP
            float blood = m_Player.blood_percent * 100; //0-max for blood
            int blood_rounded = Math.Round( blood );
            int health_rounded = Math.Round( health );
            string text = health_rounded.ToString() + " HP | " + blood_rounded.ToString() + " Blood";

            SetHealthText( text );
            SetHealthProgress(health);

            EntityAI item_in_hands = m_Player.GetHumanInventory().GetEntityInHands();
            string display_text = "";
            if(item_in_hands)
            {
                //get display name for item in hands
                display_text = item_in_hands.GetDisplayName();
            }
            //TODO: get player kills from the server (requires a rework on kills stats storing)
            SetKillsText( display_text ); 
        }
    }
    bool UpdatePosition()
    {

        vector position = m_Player.GetBonePositionWS( m_Player.GetBoneIndexByName( "Head" ) );
        position = position + Vector(0, 1, 0);//shift hud above head a bit

        Print("Player Pos");
        Print(position);

        vector screen_pos = GetGame().GetScreenPos( position );

        Print("Screen Pos Player");
        Print(screen_pos);

        float x = screen_pos[0];
        float y = screen_pos[1];
        float distance = screen_pos[2];
        if(distance > 500 || distance < 0)
        {
            SetShow( false );
            return false;
        }
        else
        {
            SetShow( true );
        }
        
        //--- set fade
        float root_alpha = 1;
        float fd_1 = distance - fade_max_distance;
        if(fd_1 > 0)
        {
            float max_fade = render_max_distance - fade_max_distance;
            root_alpha = 1 - (fd_1 / max_fade);
        }
        m_Panel.SetAlpha( root_alpha );
        
        //--- set scale
        float w_delta = panel_max_w - panel_min_w;
        float h_delta = panel_max_h - panel_min_h;
        
        float new_w = panel_max_w;
        float new_h = panel_max_h;
        if(distance > scale_max_dist)
        {
            if(distance > scale_min_dist)
            {
                new_w = panel_min_w;
                new_h = panel_min_h;
            }
            else
            {
                //some subscale
                //1. figure out how far from 40m (max) we are as a percent where 100% is == 40m and 0% is == 300m
                float delta_distance = scale_min_dist - scale_max_dist; //260m distance check that we need to clamp our % to

                float adjusted_dist = distance - scale_max_dist; //get our distance from 40m limit (we know its between 0 and 260 now)
                float percent = 1 - (adjusted_dist / delta_distance);  //this gives us the value we want

                //adjust our deltas, shrinking them so they approach 0 as our percent approaches 0
                w_delta = w_delta * percent;
                h_delta = h_delta * percent;

                //if we are at 0%, we want new_w to == panel_min_w, so w_delta will be 0 so we add w_delta to min_w
                new_w = panel_min_w + w_delta;
                new_h = panel_min_h + h_delta;

            }
        }
        m_Panel.SetSize(new_w, new_h, true);


        float width;
        float height;
        m_Panel.GetSize( width, height );

        x = x - (width / 2); //center along width
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
