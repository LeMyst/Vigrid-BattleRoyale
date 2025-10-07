#ifndef SERVER
modded class ExpansionMapMenu
{
    protected ref BattleRoyaleClient m_BattleRoyaleClient;
    protected ref BattleRoyaleMapMarkerZone m_CurrentZone;
    protected ref BattleRoyaleMapMarkerZone m_NextZone;

    protected ref map<string, ref BattleRoyaleMapMarkerPlayerArrow> m_NetworkPlayerMarkers;

    override Widget Init()
    {
        BattleRoyaleBase DayZBR = GetBR();
        if(!Class.CastTo(m_BattleRoyaleClient, DayZBR))
        {
            Error("Failed to cast DayZBR to BattleRoyaleClient!");
        }

        super.Init();

        BattleRoyaleUtils.Trace("Map Init! Updating Zones...");
        UpdateZones();

        return layoutRoot;
    }

    void UpdateZones()
    {
        float radius;
        vector center;

        BattleRoyalePlayArea current_playarea = m_BattleRoyaleClient.GetPlayArea();
        if(current_playarea)
        {
            if(!m_CurrentZone)
            {
                m_CurrentZone = new BattleRoyaleMapMarkerZone( layoutRoot, m_MapWidget );
                m_CurrentZone.SetThickness(2);
                m_CurrentZone.SetColor(ARGB(255, 0, 0, 255));
                BattleRoyaleUtils.Trace("Creating Current Zone Map Marker!");
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
                BattleRoyaleUtils.Trace("Creating Next Zone Map Marker!");
                m_Markers.Insert( m_NextZone );
            }
            center = next_playarea.GetCenter();
            radius = next_playarea.GetRadius();
            m_NextZone.SetPosition(center);
            m_NextZone.SetSize_A(radius);
            m_NextZone.SetSize_B(radius);
        }
    }

    ref array<vector> GetPlayerPositions

    //ensure BR markers are rendering correct
    override void Update( float timeslice )
    {
        //BattleRoyaleUtils.Trace("Updating Zones...");
        UpdateZones();
        super.Update( timeslice );
    }
}
#endif
