//--- circle/oval marker 

enum ZoneType
{
    Circle = 1,
    CrossHatch = 2, 
    Lined = 3
}

class BattleRoyaleMapMarkerZone extends ExpansionMapWidgetBase
{
    protected float f_SizeA;
    protected float f_SizeB;
    protected float f_Thickness;
    protected int i_Color;

    protected CanvasWidget m_BRCanvas;
    protected ZoneType e_ZoneType;

    void BattleRoyaleMapMarkerZone( Widget parent, MapWidget mapWidget, bool autoInit = true )
	{
        Print("BattleRoyaleMapMarkerZone::Constructor()");
        SetIgnorePointer(true);
        //! Do NOT show by default, otherwise it'll pop up on the load screen in the top left corner at the end of the loading progress
        Hide();
    }

    void ~BattleRoyaleMapMarkerZone()
    {
        Print("BattleRoyaleMapMarkerZone::Deconstructor()");
    }

    protected override void OnInit( Widget layoutRoot )
    {
        super.OnInit( layoutRoot );

        Class.CastTo( m_BRCanvas, layoutRoot.FindAnyWidget( "marker_canvas" ) );

        if(!m_BRCanvas)
        {
            Error("[BattleRoyaleMapMarkerZone] marker_canvas not found!");
            return;
        }

        m_Name.Show( false ); //--- the marker name is for debugging

        m_BRCanvas.Show( true ); //ensure canvas is visible

        SetColor( ARGB(255,255,255,255) );
        SetSize_A(150);
        SetSize_B(150);
        SetThickness(2);
        vector pos = "1000 10 1000";
        SetPosition(pos);
        SetZoneType(ZoneType.Circle);
    }

    override void Update( float pDt )
	{
        super.Update(pDt);

        Show();

        m_BRCanvas.Clear();

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
        m_BRCanvas.GetSize(canvas_width, canvas_height);
        float center_x = canvas_width / 2.0;
        float center_y = canvas_height / 2.0;

        if(((center_x + distance_A) > canvas_width) || ((center_y + distance_B) > canvas_height))
        {
            //Resize to fit
            float new_x = (center_x + distance_A) + 2;
            float new_y = (center_y + distance_B) + 2;
            float new_size = Math.Max(Math.Max(new_x,new_y), 32.0);

            m_BRCanvas.SetSize(new_size,new_size);

            Print("Resize Marker");
            Print(canvas_width);
            Print(canvas_height);
            Print(new_x);
            Print(new_y);
            Print(new_size);
            Print(f_Thickness);
            Print(i_Color);
            Print(center_x);
            Print(center_y);
            Print(distance_A);
            Print(distance_B);
        }

        if(f_Thickness <= 0)
        {
            Error("Thickness for marker invalid!");
            return;
        }

        if(e_ZoneType == ZoneType.Circle)
        {
            RenderOval(center_x, center_y, distance_A, distance_B);
        }
        else if(e_ZoneType == ZoneType.Lined)
        {
            RenderLined(center_x, center_y, distance_A, distance_B);
        }
        else
        {
            RenderCrosshatched(center_x, center_y, distance_A, distance_B);
        }
    }

    void RenderOval(float cx, float cy, float a, float b)
    {
        for(int i = 0; i < 360;i++)
        {
            float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
            float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

            float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
            float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

            m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);
        }
    }

    void RenderCrosshatched(float cx, float cy, float a, float b)
    {
        int ep1 = 45;
        int ep2 = 45;

        int ep3 = 135;
        int ep4 = 135;

        for(int i = 0; i < 180; i++)
        {
            //--- render circle first half
            float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
            float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

            float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
            float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

            m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);

            //--- render circle second half
            x1 = cx + (a * Math.Cos(-1*i*Math.DEG2RAD));
            y1 = cy + (b * Math.Sin(-1*i*Math.DEG2RAD));

            x2 = cx + (a * Math.Cos(-1*(i+1)*Math.DEG2RAD));
            y2 = cy + (b * Math.Sin(-1*(i+1)*Math.DEG2RAD));

            m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);

            //--- render line between EP1 and EP2 (skipping 4 degrees each step)
            if((ep1 != ep2) && ((i % 4) == 0))
            {
                x1 = cx + (a * Math.Cos(ep1*Math.DEG2RAD));
                y1 = cy + (b * Math.Sin(ep1*Math.DEG2RAD));

                x2 = cx + (a * Math.Cos(ep2*Math.DEG2RAD));
                y2 = cy + (b * Math.Sin(ep2*Math.DEG2RAD));

                m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);
            }
            if((ep3 != ep4) && ((i % 4) == 0))
            {
                x1 = cx + (a * Math.Cos(ep3*Math.DEG2RAD));
                y1 = cy + (b * Math.Sin(ep3*Math.DEG2RAD));

                x2 = cx + (a * Math.Cos(ep4*Math.DEG2RAD));
                y2 = cy + (b * Math.Sin(ep4*Math.DEG2RAD));

                m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);
            }

            ep1++;
            ep2--;
            ep3++;
            ep4--;
        }
    }

    void RenderLined(float cx, float cy, float a, float b)
    {
        int ep1 = 45;
        int ep2 = 45;

        for(int i = 0; i < 180; i++)
        {
            //--- render circle first half
            float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
            float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

            float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
            float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

            m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);

            //--- render circle second half
            x1 = cx + (a * Math.Cos(-1*i*Math.DEG2RAD));
            y1 = cy + (b * Math.Sin(-1*i*Math.DEG2RAD));

            x2 = cx + (a * Math.Cos(-1*(i+1)*Math.DEG2RAD));
            y2 = cy + (b * Math.Sin(-1*(i+1)*Math.DEG2RAD));

            m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);

            //--- render line between EP1 and EP2 (skipping 4 degrees each step)
            if((ep1 != ep2) && ((i % 4) == 0))
            {
                x1 = cx + (a * Math.Cos(ep1*Math.DEG2RAD));
                y1 = cy + (b * Math.Sin(ep1*Math.DEG2RAD));

                x2 = cx + (a * Math.Cos(ep2*Math.DEG2RAD));
                y2 = cy + (b * Math.Sin(ep2*Math.DEG2RAD));

                m_BRCanvas.DrawLine(x1, y1, x2, y2, f_Thickness, i_Color);
            }

            ep1++;
            ep2--;
        }
    }

    ZoneType GetZoneType()
    {
        return e_ZoneType;
    }

    void SetZoneType( ZoneType type )
    {
        e_ZoneType = type;
    }

    override string GetLayoutPath()
    {
        return "DayZBR-Mod/GUI/layouts/map/royale_map_marker.layout"; // custom BR widget structure
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
