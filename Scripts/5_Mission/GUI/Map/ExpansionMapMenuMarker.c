modded class ExpansionMapMenuMarker {
	protected CanvasWidget m_Canvas;
	protected RoyaleMapMarkerType m_MarkerType;
	protected float m_ValueA; //A axis value
    protected float m_ValueB; //B axis value

    void ExpansionMapMenuMarker(Widget parent, MapWidget mapwidget, vector position, string name, int color, string icon, ref ExpansionMapMarker marker = NULL)
	{
		//set the marker type
		if(icon == "BattleRoyale\\GUI\\icons\\marker\\marker_circle.paa") //if our icon path is our circle icon
		{
			m_MarkerType = RoyaleMapMarkerType.Circle;
		}
		else if(icon == "BattleRoyale\\GUI\\icons\\marker\\marker_rectangle.paa") //if our icon path is our rectangle icon
		{
			m_MarkerType = RoyaleMapMarkerType.Rectangle;
		}
		else
		{
			m_MarkerType = RoyaleMapMarkerType.Icon;
		}
		
		if(m_MarkerType == RoyaleMapMarkerType.Icon)
		{
			m_Root 				= Widget.Cast( GetGame().GetWorkspace().CreateWidgets( "DayZExpansion/GUI/layouts/map/expansion_map_marker.layout", parent ) );
			m_Icon				= ImageWidget.Cast( m_Root.FindAnyWidget("marker_icon") );
			m_MarkerIcon		= icon;
		}
		else 
		{
			m_Root 				= Widget.Cast( GetGame().GetWorkspace().CreateWidgets( "BattleRoyale/GUI/layouts/map/royale_map_marker.layout", parent ) );
			m_Canvas            = CanvasWidget.Cast( m_Root.FindAnyWidget("marker_canvas") );
		}
		m_Name				= TextWidget.Cast( m_Root.FindAnyWidget("marker_name") );
		m_MarkerButton		= ButtonWidget.Cast( m_Root.FindAnyWidget("marker_button") );
		m_MarkerDragging 	= ButtonWidget.Cast( m_Root.FindAnyWidget("marker_icon_panel") );

		m_MapWidget			= mapwidget;
		m_MarkerPos			= position;
		m_MarkerName		= name;
		m_MarkerColor		= color;
		m_MarkerData		= marker;
		
		m_ValueA = 10;
        m_ValueB = 10;
		
		m_Root.SetHandler( this );

		Init();
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
		
		if(m_Icon)
		{
			if (m_MarkerIcon)
				m_Icon.LoadImageFile(0, m_MarkerIcon);
			
			if (m_MarkerColor)
				m_Icon.SetColor(m_MarkerColor);
		}
		else if(m_Canvas)
		{
			CanvasClear();//-- TODO: find out if this is necessary
			if(m_MarkerType == RoyaleMapMarkerType.Circle)
				CanvasDrawOval(0,0,m_ValueA, m_ValueB, 1, m_MarkerColor); //TODO: find out if canvas rendering is 0 based on canvas position
			else if(m_MarkerType == RoyaleMapMarkerType.Rectangle)
				CanvasDrawRectangle(0,0,m_ValueA, m_ValueB,0,1,m_MarkerColor); 
		}		
		if (m_MarkerColor)
			m_Name.SetColor(m_MarkerColor);
		
		if (m_MarkerName)
			m_Name.SetText(m_MarkerName);
		
		if (m_MarkerPos && !m_Dragging)
		{
			vector mapPos = m_MapWidget.MapToScreen(m_MarkerPos);
			m_Root.SetPos(mapPos[0], mapPos[1], true);
		}
	}
	
	
	void SetMarkerSize(float A, float B) 
    {
		if(!m_Canvas)
			return;
		
        m_ValueA = A;
        m_ValueB = B;
    }
	void CanvasClear() 
    {
		if(m_Canvas)
			m_Canvas.Clear();
    }
    void CanvasDrawCircle(float cx, float cy, float r, float thickness, int color) 
    {
		if(!m_Canvas)
			return;
		
        CanvasDrawOval(cx, cy, r, r, thickness, color);
    }
    void CanvasDrawOval(float cx, float cy, float a, float b, float thickness, int color)
    {
		if(!m_Canvas)
			return;
		
        for(int i = 0; i < 360;i++)
        {
            float x1 = cx + (a * Math.Cos(i*Math.DEG2RAD));
            float y1 = cy + (b * Math.Sin(i*Math.DEG2RAD));

            float x2 = cx + (a * Math.Cos((i+1)*Math.DEG2RAD));
            float y2 = cy + (b * Math.Sin((i+1)*Math.DEG2RAD));

            m_Canvas.DrawLine(x1, y1, x2, y2, thickness, color);
        }
    }
    void CanvasDrawRectangle(float cx, float cy, float w, float h, float rotation, float thickness, int color)
    {
		if(!m_Canvas)
			return;
		
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