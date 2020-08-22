modded class ExpansionMapMenu {
    protected ref BattleRoyaleClient m_BattleRoyaleClient;
    protected ref BattleRoyaleMapMarkerZone m_CurrentZone;
    protected ref BattleRoyaleMapMarkerZone m_NextZone;

    
    protected ref BattleRoyaleMapMarkerZone m_DebugZone;
    protected ref BattleRoyaleMapMarkerZone m_DebugZone2;

    override Widget Init()
    {
        BattleRoyaleBase DayZBR = GetBR();
        if(!Class.CastTo(m_BattleRoyaleClient, DayZBR))
        {
            Error("Failed to cast DayZBR to BattleRoyaleClient!");
        } 

        super.Init();

        Print("Map Init! Updating Zones...");
        UpdateZones();

        return layoutRoot;
    }

    void UpdateZones()
    {
        float radius;
        vector center;

        /*
        //--- debug markers (on screen always)
        if(!m_DebugZone)
        {
            //should default to 1000 0 1000 w/ a radius of 150, and a thickness of 2
            m_DebugZone = new BattleRoyaleMapMarkerZone( layoutRoot, m_MapWidget );
            m_DebugZone.SetColor( ARGB(255, 255, 0, 0) );
            m_DebugZone.SetZoneType(ZoneType.Lined); //CrossHatch | Lined
            Print("Creating Debug Zone Marker!");
            m_Markers.Insert( m_DebugZone );
        }

        if(!m_DebugZone2)
        {
            //should default to 1000 0 1000 w/ a radius of 150, and a thickness of 2
            m_DebugZone2 = new BattleRoyaleMapMarkerZone( layoutRoot, m_MapWidget );
            vector pos = "2000 10 2000";
            m_DebugZone2.SetPosition(pos);
            m_DebugZone2.SetColor( ARGB(255, 255, 255, 0) );
            m_DebugZone2.SetZoneType(ZoneType.CrossHatch); //CrossHatch | Lined
            Print("Creating Debug Zone Marker #2!");
            m_Markers.Insert( m_DebugZone2 );
        }
        */

        BattleRoyalePlayArea current_playarea = m_BattleRoyaleClient.GetPlayArea();
        if(current_playarea)
        {
            if(!m_CurrentZone)
            {
                m_CurrentZone = new BattleRoyaleMapMarkerZone( layoutRoot, m_MapWidget );
                m_CurrentZone.SetThickness(2);
                m_CurrentZone.SetColor(ARGB(255, 0, 0, 255));
                Print("Creating Current Zone Map Marker!");
                m_Markers.Insert( m_CurrentZone );
            }
            center = current_playarea.GetCenter();
            radius = current_playarea.GetRadius();
            m_CurrentZone.SetPosition(center);
            m_CurrentZone.SetSize_A(radius);
            m_CurrentZone.SetSize_B(radius);
        }

        BattleRoyalePlayArea next_playarea = m_BattleRoyaleClient.GetNextArea();
        if(next_playarea)
        {
            if(!m_NextZone)
            {
                m_NextZone = new BattleRoyaleMapMarkerZone( layoutRoot, m_MapWidget );
                m_NextZone.SetColor(ARGB(255, 255, 255, 255));
                m_NextZone.SetThickness(2);
                Print("Creating Next Zone Map Marker!");
                m_Markers.Insert( m_NextZone );
            }
            center = next_playarea.GetCenter();
            radius = next_playarea.GetRadius();
            m_NextZone.SetPosition(center);
            m_NextZone.SetSize_A(radius);
            m_NextZone.SetSize_B(radius);
        }
    }

    //ensure BR markers are rendering correct
    override void Update( float timeslice )
	{
        UpdateZones(); //TODO: this shouldn't exist, instead we should call UpdateZones from BattleRoyaleClient (when we recieve our payload data with new zone info)

        super.Update( timeslice );
    }
}