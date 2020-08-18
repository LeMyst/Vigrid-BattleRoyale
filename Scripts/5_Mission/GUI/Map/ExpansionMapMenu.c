modded class ExpansionMapMenu 
{

    protected ref map< string, ShapeMapMarker > m_BattleRoyaleMarkers;

    override Widget Init()
	{
        m_BattleRoyaleMarkers = new map< string, ShapeMapMarker >
        return super.Init();
    }

    override void UpdateMarkers()
    {
        UpdateBattleRoyaleMarkers();
        super.UpdateMarkers();
    }

    void UpdateBattleRoyaleMarkers()
    {
        if ( !m_MarkerModule )
			return;

        
    }
}