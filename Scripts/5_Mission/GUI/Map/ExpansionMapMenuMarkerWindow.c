
modded class ExpansionMapMenuMarkerWindow {
    //protected ref array<ref ExpansionMapMenuMarkerEntry> m_MarkerIconsEntrys;
    //autoptr array< ref ExpansionMarkerIcon > m_MarkersIconList;

	override void Init()
	{
		super.Init(); //call base (and init base markers)
		
		//add BR map markers

        ExpansionMarkerIcon circle = new ExpansionMarkerIcon();
        
		//because we had to mod expansion marker icons to include circles, we just added a few functions so *this* setup uses the strings based in that file
		circle.Name = ExpansionMarkerIcons.GetCircleName();
        circle.Path = ExpansionMarkerIcons.GetCirclePath();
		
		Print("Adding Circle Icon");
		Print(circle.Name);
		Print(circle.Path);
        
		m_MarkersIconList.Insert(circle);
		
       
        m_MarkerIconsEntrys.Clear(); //undo changes made by FillMarkerList
		
		
        FillMarkerList(m_MarkersIconList); //call fill marker list
	}
	
	
	override void SetMarkerSelection(ref ExpansionMapMenuMarkerEntry data)
	{
		super.SetMarkerSelection(data);
		Print("Updating Selected Marker");
		Print(data.GetMarkerName());
		Print(data.GetMarkerIcon());
		Print(data.GetMarkerIconIndex());
	}
	override void CreateMarker()
	{
		Print("Create Marker");
		if (m_CurrentSelectedMarker)
		{
			m_MarkerIconIndex = m_CurrentSelectedMarker.GetMarkerIconIndex();
			if(m_MarkerIconIndex <= 0)
			{
				Error("DayZBR: GetMarkerIconIndex() Returned < 1");
			}
		}
		else
		{
			Error("DayZBR: m_CurrentSelectedMarker IS NULL");
		}
		super.CreateMarker();
	}
	
}