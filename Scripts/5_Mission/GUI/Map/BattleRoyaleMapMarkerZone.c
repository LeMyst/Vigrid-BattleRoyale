//--- circle/oval marker 
class BattleRoyaleMapMarkerZone extends ExpansionMapWidgetBase
{
    protected float f_SizeA;
    protected float f_SizeB;
    protected float f_Thickness;
    protected int i_Color;

    protected Widget m_Frame;
    protected CanvasWidget m_Canvas;

    void BattleRoyaleMapMarkerZone( Widget parent, MapWidget mapWidget, bool autoInit = true )
	{
        Print("BattleRoyaleMapMarkerZone::Constructor()");
    }
    void ~BattleRoyaleMapMarkerZone()
    {
        Print("BattleRoyaleMapMarkerZone::Deconstructor()");
    }

    protected override void OnInit( Widget layoutRoot )
    {
        Class.CastTo( m_Frame, layoutRoot.FindAnyWidget( "marker_frame" ) );
        Class.CastTo( m_Canvas, layoutRoot.FindAnyWidget( "marker_canvas" ) );

        if(!m_Frame)
        {
            Error("[BattleRoyaleMapMarkerZone] marker_frame not found!");
            return;
        }
        if(!m_Canvas)
        {
            Error("[BattleRoyaleMapMarkerZone] marker_canvas not found!");
            return;
        }

        m_Canvas.Show( true ); //ensure canvas is visible

        m_Frame.SetColor( ARGB( 0, 0, 0, 0 ) );
        SetColor(ARGB(255,255,255,255));
        SetSize_A(150);
        SetSize_B(150);
        SetThickness(2);
        vector pos = "1000 10 1000";
        SetPosition(pos);
    }
    override void Update( float pDt )
	{
        super.Update(pDt);

        Show();

        m_Canvas.Clear();
        
        vector m_edgePos_A = GetPosition();
        m_edgePos_A[0] = m_edgePos_A[0] + f_SizeA;

        vector m_edgePos_B = GetPosition();
        m_edgePos_B[2] = m_edgePos_B[2] + f_SizeB;

        vector mapPos_edge_A = GetMapWidget().MapToScreen(m_edgePos_A);				
        vector mapPos_edge_B = GetMapWidget().MapToScreen(m_edgePos_B);
        vector mapPos_center = GetMapWidget().MapToScreen(GetPosition());

        float distance_A = vector.Distance(mapPos_center,mapPos_edge_A);
        float distance_B = vector.Distance(mapPos_center,mapPos_edge_B);

        float canvas_width;
        float canvas_height;
        m_Canvas.GetSize(canvas_width,canvas_height);
        float center_x = canvas_width / 2.0;
        float center_y = canvas_height / 2.0;

        if(((center_x + distance_A) > canvas_width) || ((center_y + distance_B) > canvas_height))
        {
            Print("Resizing Zone Marker!");
            //Resize to fit
            float new_x = (center_x + distance_A) + 2;
            float new_y = (center_y + distance_B) + 2;
            float new_size = Math.Max(Math.Max(new_x,new_y), 34.0);

            m_Canvas.SetSize(new_size,new_size);
        }

        float cx = center_x;
        float cy = center_y;
        float a = distance_A;
        float b = distance_B;

        for(int i = 0; i < 360;i++)
        {
            float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
            float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

            float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
            float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

            m_Canvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);
        }
    }



    override string GetLayoutPath()
    {
        return "BattleRoyale/GUI/layouts/map/royale_map_marker.layout";// custom BR widget structure
    }
    int GetColor()
    {
        return i_Color;
    }
    void SetColor(int color)
    {
        i_Color = color;
    }
    float GetSize_A()
    {
        return f_SizeA;
    }
    void SetSize_A(float value)
    {
        f_SizeA = value;
    }
    float GetSize_B()
    {
        return f_SizeB;
    }
    void SetSize_B(float value)
    {
        f_SizeB = value;
    }
    float GetThickness()
    {
        return f_Thickness;
    }
    void SetThickness(float value)
    {
        f_Thickness = value;
    }
    //GetPosition() -> returns world position
    //SetPosition(vector position) --> update position in world coordinates

}