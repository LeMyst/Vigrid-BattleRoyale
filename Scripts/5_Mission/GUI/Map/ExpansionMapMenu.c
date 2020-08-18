modded class ExpansionMapMenu {
    protected ref BattleRoyaleClient m_BattleRoyaleClient;
    override Widget Init()
    {
        BattleRoyaleBase DayZBR = GetBR();
        if(!Class.CastTo(m_BattleRoyaleClient, DayZBR))
        {
            Error("Failed to cast DayZBR to BattleRoyaleClient!");
        } 

        return super.Init();
    }
    override void LoadPersonnalMarkers()
	{
        super.LoadPersonnalMarkers();

        if(m_BattleRoyaleClient)
        {
            BattleRoyalePlayArea current_zone = m_BattleRoyaleClient.GetPlayArea();
            BattleRoyalePlayArea next_zone = m_BattleRoyaleClient.GetNextArea();

            string name;
            int icon;
            vector pos;
            int color;
            bool is3D;
            ExpansionMapMarker newMarker;
            ExpansionMapMenuMarker mapMarker;

            if(current_zone)
            {
                

                name = " Current Zone";
                icon = ExpansionMarkerIcons.GetCircleIndex();
                pos = current_zone.GetCenter();
                color = ARGB(255, 0, 125, 255);
                is3D = false;
                
                newMarker = m_MarkerModule.CreateClientMarker(name, icon, pos, color, false, is3D);
                mapMarker = new ExpansionMapMenuMarker(layoutRoot, m_MapWidget, pos, name, color, ExpansionMarkerIcons.GetMarkerPath(icon), newMarker);
                
                mapMarker.SetMarkerSize(current_zone.GetRadius(), current_zone.GetRadius());
                
                m_MapMarkers.Insert(mapMarker);
                m_MapSavedMarkers.Insert(newMarker);

                Print("===== Map Render Marker Current Zone =====");
                Print(current_zone.GetCenter());
                Print(current_zone.GetRadius());
            }

            if(next_zone)
            {


                name = " Next Zone";
                icon = ExpansionMarkerIcons.GetCircleIndex();
                pos = next_zone.GetCenter();
                color = ARGB(255, 0, 255, 0);
                is3D = false;
                
                newMarker = m_MarkerModule.CreateClientMarker(name, icon, pos, color, false, is3D);
                mapMarker = new ExpansionMapMenuMarker(layoutRoot, m_MapWidget, pos, name, color, ExpansionMarkerIcons.GetMarkerPath(icon), newMarker);
                
                mapMarker.SetMarkerSize(next_zone.GetRadius(), next_zone.GetRadius());
                mapMarker.LockMarker(true); //prevent dragging and editing
                
                m_MapMarkers.Insert(mapMarker);
                m_MapSavedMarkers.Insert(newMarker);

                Print("===== Map Render Marker Next Zone =====");
                Print(next_zone.GetCenter());
                Print(next_zone.GetRadius());
            }
        }

        
    }
    override void RemovePersonalMarker(string name)
    {
        //--- can't delete dayzbr marker
        if(name == " Current Zone")
        {
            return;
        }
        if(name == " Next Zone")
        {
            return;
        }
        super.RemovePersonalMarker(name);
    }
}