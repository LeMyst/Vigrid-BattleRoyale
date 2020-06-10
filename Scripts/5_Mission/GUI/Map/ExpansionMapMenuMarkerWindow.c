modded class ExpansionMapMenuMarkerWindow {
    //protected ref array<ref ExpansionMapMenuMarkerEntry> m_MarkerIconsEntrys;
    //autoptr array< ref ExpansionMarkerIcon > m_MarkersIconList;
    void ExpansionMapMenuMarkerWindow(Widget parent, ExpansionMapMenu menu, MapWidget mapwidget)
	{
        super.ExpansionMapMenuMarkerWindow(parent,menu,mapwidget); //call base
        //add BR map markers

        ExpansionMarkerIcon circle = new ExpansionMarkerIcon();
        circle.Name = "Circle";
        circle.Path = "BattleRoyale\\GUI\\icons\\marker\\marker_circle.paa"; //TODO: this needs to be a valid PAA file
        m_MarkersIconList.push(circle)
       
        m_MarkerIconsEntrys.clear(); //undo changes made by FillMarkerList
        FillMarkerList(m_MarkersIconList); //call fill marker list
    }
}