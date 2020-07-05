
modded class ExpansionMapMenuMarkerWindow {
    //protected ref array<ref ExpansionMapMenuMarkerEntry> m_MarkerIconsEntrys;
    //autoptr array< ref ExpansionMarkerIcon > m_MarkersIconList;

	override void Init()
	{
		super.Init(); //call base (and init base markers)
		
		//add BR map markers

        ExpansionMarkerIcon circle = new ExpansionMarkerIcon();
        circle.Name = "Circle";
        circle.Path = "BattleRoyale\\GUI\\icons\\marker\\marker_circle.paa"; //TODO: this needs to be a valid PAA file
        m_MarkersIconList.Insert(circle)
       
        m_MarkerIconsEntrys.Clear(); //undo changes made by FillMarkerList
        FillMarkerList(m_MarkersIconList); //call fill marker list
	}
}