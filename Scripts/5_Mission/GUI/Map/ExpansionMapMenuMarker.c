modded class ExpansionMapMenuMarker {

    void ExpansionMapMenuMarker(Widget parent, MapWidget mapwidget, vector position, string name, int color, string icon, ref ExpansionMapMarker marker = NULL)
	{
        OverridableConstructor(parent, mapwidget, position, name, color, icon, marker);
	}

    void OverridableConstructor(Widget parent, MapWidget mapwidget, vector position, string name, int color, string icon, ref ExpansionMapMarker marker = NULL) 
    {
        m_Root 				= Widget.Cast( GetGame().GetWorkspace().CreateWidgets( "DayZExpansion/GUI/layouts/map/expansion_map_marker.layout", parent ) );

		m_Name				= TextWidget.Cast( m_Root.FindAnyWidget("marker_name") );
		m_Icon				= ImageWidget.Cast( m_Root.FindAnyWidget("marker_icon") );
		m_MarkerButton		= ButtonWidget.Cast( m_Root.FindAnyWidget("marker_button") );
		m_MarkerDragging 	= ButtonWidget.Cast( m_Root.FindAnyWidget("marker_icon_panel") );

		m_MapWidget			= mapwidget;
		m_MarkerPos			= position;
		m_MarkerName		= name;
		m_MarkerIcon		= icon;
		m_MarkerColor		= color;
		m_MarkerData		= marker;
		
		m_Root.SetHandler( this );

		Init();
    }

}