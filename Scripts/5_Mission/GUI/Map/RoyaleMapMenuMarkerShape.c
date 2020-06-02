class RoyaleMapMenuMarkerShape extends ExpansionMapMenuMarker {

    protected CanvasWidget m_Canvas;
    protected int m_Shape; //TODO: convert this to an enum
    protected float m_ValueA; //A axis value
    protected float m_ValueB; //B axis value

    void RoyaleMapMenuMarkerShape(Widget parent, MapWidget mapwidget, vector position, string name, int color, string icon, ref ExpansionMapMarker marker = NULL)
    {
        //--- the base class will call overridable constructor (so dumb)
    }
    override void OverridableConstructor(Widget parent, MapWidget mapwidget, vector position, string name, int color, string icon, ref ExpansionMapMarker marker = NULL) 
    {
        m_Root 				= Widget.Cast( GetGame().GetWorkspace().CreateWidgets( "BattleRoyale/GUI/layouts/map/royale_map_marker.layout", parent ) );

		m_Name				= TextWidget.Cast( m_Root.FindAnyWidget("marker_name") );
		//m_Icon				= ImageWidget.Cast( m_Root.FindAnyWidget("marker_icon") );
        m_Canvas            = CanvasWidget.Cast( m_Root.FindAnyWidget("marker_canvas") );
		m_MarkerButton		= ButtonWidget.Cast( m_Root.FindAnyWidget("marker_button") );
		m_MarkerDragging 	= ButtonWidget.Cast( m_Root.FindAnyWidget("marker_icon_panel") );

		m_MapWidget			= mapwidget;
		m_MarkerPos			= position;
		m_MarkerName		= name;
		m_MarkerIcon		= "shape"; //this will let our map marker rendering know we are a shape
		m_MarkerColor		= color;
		m_MarkerData		= marker;
		m_ValueA = 10;
        m_ValueB = 10;
        m_Shape = 0; //TODO: change this to an enum

		m_Root.SetHandler( this );
    }

    void SetMarkerSize(float A, float B) 
    {
        m_ValueA = A;
        m_ValueB = B;
    }

    override void Update( float timeslice )
	{
        //TODO: draw shape to canvace
        //X size is A
        //Y size is B
        //center of shape is position
        //color is m_MarkerColor
        //text color is m_MarkerColor
        //set text (if given a text value)



		if (m_MarkerIcon)
			m_Icon.LoadImageFile(0, m_MarkerIcon);
		
		if (m_MarkerColor)
			m_Icon.SetColor(m_MarkerColor);
			m_Name.SetColor(m_MarkerColor);
		
		if (m_MarkerName)
			m_Name.SetText(m_MarkerName);
		
		if (m_MarkerPos && !m_Dragging)
		{
			vector mapPos = m_MapWidget.MapToScreen(m_MarkerPos);
			m_Root.SetPos(mapPos[0], mapPos[1], true);
		}
	}






    //--- everything in these functions will be in screen space
    void CanvasClear() 
    {
        m_Canvas.Clear();
    }
    void CanvasDrawCircle(float cx, float cy, float r, float thickness, int color) 
    {
        CanvasDrawOval(cx, cy, r, r, thickness, color);
    }
    void CanvasDrawOval(float cx, float cy, float a, float b, float thickness, int color)
    {
        for(int i = 0; i < 360;i++)
        {
            float x1 = cx + (a * cos(i*0.0174532925));
            float y1 = cy + (b * sin(i*0.0174532925));

            float x2 = cx + (a * cos((i+1)*0.0174532925)); //PI/180 = 0.0174532925
            float y2 = cy + (b * sin((i+1)*0.0174532925));

            m_Canvas.DrawLine(x1, y1, x2, y2, thickness, color);
        }
    }
    void CanvasDrawRectangle(float cx, float cy, float w, float h, float rotation, float thickness, int color)
    {
        float xMin = cx - (w/2); //TODO: rotation calculation
        float yMin = cy - (h/2); //TODO: rotation calculation
        float xMax = cx + (w/2); //TODO: rotation calculation
        float yMax = cy + (h/2); //TODO: rotation calculation
        m_Canvas.DrawLine(xMin, yMin, xMin, yMax, thickness, color); //left line
        m_Canvas.DrawLine(xMin, yMin, xMax, yMin, thickness, color); //bottom line
        m_Canvas.DrawLine(xMax, yMin, xMax, yMax, thickness, color); //right line
        m_Canvas.DrawLine(xMin, yMax, xMax, yMax, thickness, color); //top line
    }
}