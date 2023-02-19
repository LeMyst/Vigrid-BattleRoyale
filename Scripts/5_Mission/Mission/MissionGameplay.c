modded class MissionGameplay
{
    protected Widget m_BattleRoyaleHudRootWidget;
    protected ref BattleRoyaleHud m_BattleRoyaleHud;
#ifdef BR_MINIMAP
    protected ref MapWidget m_MiniMap;
    protected ref CanvasWidget m_MiniMapCanvas;
#endif

    protected bool is_spectator;

    void MissionGameplay()
    {
        m_BattleRoyaleHudRootWidget = null;
        is_spectator = false;
    }

    override void OnInit()
    {
        super.OnInit();

        m_BattleRoyale = new BattleRoyaleClient;

        InitBRhud();
    }

    void InitBRhud()
    {
        Print("Initializing BattleRoyale HUD");
        if(!m_BattleRoyaleHudRootWidget)
        {
#ifdef BR_MINIMAP
            m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("DayZBR-Mod/GUI/layouts/hud/br_hud_minimap.layout");
            m_MiniMap = MapWidget.Cast(m_BattleRoyaleHudRootWidget.FindAnyWidget("MiniMap"));
            m_MiniMapCanvas = CanvasWidget.Cast(m_BattleRoyaleHudRootWidget.FindAnyWidget("CanvasMiniMap"));
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName( this, "UpdateMiniMap", 150, true );
#else
            m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("DayZBR-Mod/GUI/layouts/hud/br_hud.layout");
#endif

            m_BattleRoyaleHud = new BattleRoyaleHud( m_BattleRoyaleHudRootWidget );
            m_BattleRoyaleHud.ShowHud( true );
            Print("HUD Initialized");
        }
    }

#ifdef BR_MINIMAP
    void UpdateMiniMap()
    {
        UpdateMiniMap(GetGame().GetCurrentCameraPosition());
    }
#endif

    bool IsInSpectator()
    {
        return is_spectator;
    }

    void InitSpectator()
    {
        Print("Initializing Spectator HUD");
        m_BattleRoyaleHud.InitSpectator();

        is_spectator = true;

        HideHud();
    }

    void UpdateKillCount(int count)
    {
        m_BattleRoyaleHud.ShowKillCount( true );
        m_BattleRoyaleHud.SetKillCount( count );
    }

    void HideCountdownTimer()
    {
        m_BattleRoyaleHud.ShowCountdown( false );
    }

    void UpdateCountdownTimer(int seconds)
    {
        m_BattleRoyaleHud.ShowCountdown( true );
        m_BattleRoyaleHud.SetCountdown( seconds );
    }

    void UpdatePlayerCount(int count)
    {
        if(count == 0)
        {
            m_BattleRoyaleHud.ShowCount( false );
            return;
        }

        m_BattleRoyaleHud.ShowCount( true );
        m_BattleRoyaleHud.SetCount( count );
    }

    void UpdateZoneDistance(float distance)
    {
        m_BattleRoyaleHud.ShowDistance( true );
        m_BattleRoyaleHud.SetDistance( distance );
    }

#ifdef BR_MINIMAP
    void UpdateMiniMap(vector pos)
    {
        if(!m_MiniMap)
            return;

        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        if (!player)
            return;

        m_BattleRoyaleHudRootWidget.Show(true);

        m_MiniMap.SetMapPos( Vector(pos[0], 0, pos[2]) );
        m_MiniMap.SetScale( 0.1 );

        m_MiniMap.ClearUserMarks();
        m_MiniMap.AddUserMark(player.GetPosition(), "", ARGB(255,255,255,255), "\\dz\\gear\\navigation\\data\\map_tree_ca.paa");

        if (m_MiniMapCanvas)
        {
            Print("Update MiniMap Canvas!");
            vector spawn_point = "14829.2 0 14572.3";

            m_MiniMapCanvas.Clear();

            vector m_edgePos_A = spawn_point;
            m_edgePos_A[0] = m_edgePos_A[0] + 50;

            vector m_edgePos_B = spawn_point;
            m_edgePos_B[2] = m_edgePos_B[2] + 50;

            vector mapPos_edge_A = m_MiniMap.MapToScreen(m_edgePos_A);
            vector mapPos_edge_B = m_MiniMap.MapToScreen(m_edgePos_B);
            vector mapPos_center = m_MiniMap.MapToScreen(player.GetPosition());

            float distance_A = vector.Distance(mapPos_center, mapPos_edge_A);
            float distance_B = vector.Distance(mapPos_center, mapPos_edge_B);

            float canvas_width;
            float canvas_height;
            m_MiniMapCanvas.GetSize(canvas_width, canvas_height);
            float center_x = canvas_width / 2.0;
            float center_y = canvas_height / 2.0;

            RenderOval(center_x, center_y, distance_A, distance_B);
        }
        /*float width;
        float height;
        m_MiniMapCanvas.GetSize(width, height);
        Print(string.Format("GetSize %1 %2", width, height))
        m_MiniMapCanvas.GetScreenPos(width, height);
        Print(string.Format("GetScreenPos %1 %2", width, height))
        m_MiniMapCanvas.GetScreenSize(width, height);
        Print(string.Format("GetScreenSize %1 %2", width, height))
        m_MiniMapCanvas.DrawLine(0, 0, width, height, 2, ARGB(255, 0, 0, 255));*/
    }

    //TODO: temporary place
    void RenderOval(float cx, float cy, float a, float b)
    {
        for(int i = 0; i < 360; i++)
        {
            Print("RenderOval "+cx+" "+cy+" "+a+" "+b);
            float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
            float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

            float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
            float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

            m_MiniMapCanvas.DrawLine(x1, y1, x2, y2, 2, ARGB(255, 0, 0, 255));
        }
    }
#endif

    //TODO: move this into modded keybinds systems
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);

        if( key == KeyCode.KC_F1 )
        {
            BattleRoyaleClient.Cast( m_BattleRoyale ).ReadyUp();
        }
    }

    override void OnUpdate( float timeslice )
    {
        super.OnUpdate( timeslice ); //no more using fade out because it causes way to much compatibility issues, instead we'll use widgets

        m_BattleRoyale.Update( timeslice ); //send tick to br client

        m_BattleRoyaleHud.Update( timeslice ); //this is really only used for spectator HUD updates

        if(is_spectator)
        {
            HideHud();
        }
    }

    void HideHud()
    {
        IngameHud hud = IngameHud.Cast( GetHud() );
        if ( hud )
        {
            hud.BR_HIDE();
        }
    }
}
