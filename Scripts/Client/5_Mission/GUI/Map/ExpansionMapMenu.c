#ifndef SERVER
modded class ExpansionMapMenu
{
    protected ref BattleRoyaleClient m_BattleRoyaleClient;
    protected ref BattleRoyaleMapMarkerZone m_CurrentZone;
    protected ref BattleRoyaleMapMarkerZone m_NextZone;
    protected ref array<ref BattleRoyaleMapMarkerZone> m_HotZones;

    protected ref map<string, ref BattleRoyaleMapMarkerPlayerArrow> m_NetworkPlayerMarkers;

    override Widget Init()
    {
        BattleRoyaleBase DayZBR = GetBR();
        if(!Class.CastTo(m_BattleRoyaleClient, DayZBR))
        {
            Error("Failed to cast DayZBR to BattleRoyaleClient!");
        }

        super.Init();

        m_HotZones = new array<ref BattleRoyaleMapMarkerZone>();

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

        // Update Hot Zones
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if (br_rpc && br_rpc.hot_zone_centers && br_rpc.hot_zone_centers.Count() > 0)
        {
            int needed_hot_zones = br_rpc.hot_zone_centers.Count();

            if (m_HotZones.Count() < needed_hot_zones)
            {
                int zones_to_create = needed_hot_zones - m_HotZones.Count();
                int hz_idx;
                for (hz_idx = 0; hz_idx < zones_to_create; hz_idx++)
                {
                    BattleRoyaleMapMarkerZone hot_zone_marker = new BattleRoyaleMapMarkerZone( layoutRoot, m_MapWidget );
                    hot_zone_marker.SetColor(ARGB(255, 255, 0, 0));
                    hot_zone_marker.SetThickness(2);
                    m_HotZones.Insert(hot_zone_marker);
                    m_Markers.Insert(hot_zone_marker);
                    BattleRoyaleUtils.Trace("Creating Hot Zone Map Marker!");
                }
            }

            int hz_index;
            for (hz_index = 0; hz_index < br_rpc.hot_zone_centers.Count(); hz_index++)
            {
                center = br_rpc.hot_zone_centers.Get(hz_index);
                radius = br_rpc.hot_zone_radii.Get(hz_index);

                BattleRoyaleMapMarkerZone hot_zone = m_HotZones.Get(hz_index);
                hot_zone.SetPosition(center);
                hot_zone.SetSize_A(radius);
                hot_zone.SetSize_B(radius);
            }
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
