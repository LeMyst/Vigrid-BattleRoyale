modded class MissionGameplay
{
    protected Widget m_BattleRoyaleHudRootWidget;
    protected ref BattleRoyaleHud m_BattleRoyaleHud;

#ifdef BR_MINIMAP
    protected ref MapWidget m_MiniMap;
    protected ref CanvasWidget m_MiniMapCanvas;
    protected ref ImageWidget m_UserMarkerImageWidget;
    protected float f_MiniMapScale = 0.1;
    protected bool b_MiniMapShow = true;
    protected SchanaPartyManagerClient party_manager;
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
            m_UserMarkerImageWidget = ImageWidget.Cast( m_BattleRoyaleHudRootWidget.FindAnyWidget( "UserMarker" ) );
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName( this, "UpdateMiniMap", 200, true );

            float canvas_width;
            float canvas_height;
            m_MiniMapCanvas.GetSize(canvas_width, canvas_height);
            Print(canvas_width);
            Print(canvas_height);
#else
            m_BattleRoyaleHudRootWidget = GetGame().GetWorkspace().CreateWidgets("DayZBR-Mod/GUI/layouts/hud/br_hud.layout");
#endif

            m_BattleRoyaleHud = new BattleRoyaleHud( m_BattleRoyaleHudRootWidget );
            m_BattleRoyaleHud.ShowHud( true );
            Print("HUD Initialized");
        }
    }

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

    void UpdatePlayerCount(int nb_players, int nb_groups)
    {
        //BattleRoyaleUtils.Trace(string.Format("UpdatePlayerCount: %1 %2", nb_players, nb_groups));
        m_BattleRoyaleHud.ShowCount( true );
        m_BattleRoyaleHud.SetCount( nb_players, nb_groups );
    }

    void UpdateZoneDistance(float distance)
    {
        m_BattleRoyaleHud.ShowDistance( true );
        m_BattleRoyaleHud.SetDistance( distance );
    }

#ifdef BR_MINIMAP
    void UpdateMiniMap()
    {
        UpdateMiniMap(GetGame().GetCurrentCameraPosition());
    }

    void UpdateMiniMap(vector pos)
    {
        //m_BattleRoyaleHudRootWidget.Show( true );

        if( b_MiniMapShow )
        {
            m_MiniMap.Show( true )
        }
        else
        {
            m_MiniMap.Show( false );
            return;
        }

        if(!m_MiniMap)
            return;

        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        if (!player)
            return;

        m_MiniMap.SetMapPos( Vector(pos[0], 0, pos[2]) );
        m_MiniMap.SetScale( f_MiniMapScale );

        m_MiniMap.ClearUserMarks();
        //m_MiniMap.AddUserMark(pos, "", ARGB(192,0,0,255), "\\dz\\gear\\navigation\\data\\map_bush_ca.paa");

        BattleRoyaleClient brc = BattleRoyaleClient.Cast( m_BattleRoyale );
        if(brc && brc.GetNextArea())
            m_MiniMap.AddUserMark(brc.GetNextArea().GetCenter(), "", ARGB(255,255,0,0), "\\dz\\gear\\navigation\\data\\map_bush_ca.paa");

        if(m_UserMarkerImageWidget)
        {
            //vector rot = player.GetYawPitchRoll();
            //float angle = Math.Round(rot[0]);
            //if (angle < 0)
            //{
            //    angle = 360 + angle;
            //}
            //m_UserMarkerImageWidget.SetRotation(0, 0, angle);


            //float camera_angle = GetGame().GetCurrentCameraDirection().VectorToAngles()[0];
            //camera_angle = Math.NormalizeAngle(camera_angle);

            m_UserMarkerImageWidget.SetRotation( 0, 0, Math.Round( Math.NormalizeAngle( GetGame().GetCurrentCameraDirection().VectorToAngles()[0] ) ), true );
        }

        if(!party_manager)
            party_manager = GetSchanaPartyManagerClient();

        ref map<string, vector> positions = party_manager.GetPositions();
        foreach(string player_id, vector position : positions)
        {
            m_MiniMap.AddUserMark(position, "", ARGB(192,0,0,255), "\\dz\\gear\\navigation\\data\\map_bush_ca.paa");
        }

        // TODO: Temporary placeholders
        //UpdateMiniMapCanvas();
    }

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

    override void OnUpdate( float timeslice )
    {
        super.OnUpdate( timeslice ); //no more using fade out because it causes way to much compatibility issues, instead we'll use widgets

        m_BattleRoyale.Update( timeslice ); //send tick to br client

        m_BattleRoyaleHud.Update( timeslice ); //this is really only used for spectator HUD updates


        if (GetUApi() && !m_UIManager.IsMenuOpen(MENU_CHAT_INPUT)) {
            if (GetUApi().GetInputByID(UADayZBRReadyUp).LocalPress()) {
                BattleRoyaleClient.Cast( m_BattleRoyale ).ReadyUp();
            }
#ifdef BR_MINIMAP
            if (GetUApi().GetInputByID(UADayZBRToggleMiniMap).LocalPress()) {
                b_MiniMapShow = !b_MiniMapShow;
            }

            if (GetUApi().GetInputByID(UADayZBRMiniMapZoomPlus).LocalPress()) {
                if(f_MiniMapScale > 0.1)
                    f_MiniMapScale = f_MiniMapScale - 0.05;
            }

            if (GetUApi().GetInputByID(UADayZBRMiniMapZoomMinus).LocalPress()) {
                if(f_MiniMapScale < 1.0)
                    f_MiniMapScale = f_MiniMapScale + 0.05;
            }
#endif
        }

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
