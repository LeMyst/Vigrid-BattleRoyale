class BattleRoyaleMarkerClientData extends BattleRoyaleMarkerClientData
{
    ref array< ref ExpansionMarkerData > m_BattleRoyaleMarkers;
    int m_NextBattleRoyaleUID;

    void BattleRoyaleMarkerClientData()
    {
        m_NextBattleRoyaleUID = -1;
        m_BattleRoyaleMarkers = new array< ref ExpansionMarkerData >();
    }
    void ~PersonnalMarkerClientData()
    {
        delete m_PersonalMarkers;
    }

    //note: we are not override onStoreSave/onStoreLoad because we will not be saving these markers
    override int Count()
    {
        return m_BattleRoyaleMarkers.Count();
    }
    override ref ExpansionMarkerData Get( int idx )
    {
        return m_BattleRoyaleMarkers[idx];
    }
    override void Insert( ref ExpansionMarkerData data ) 
    {
        m_NextBattleRoyaleUID++;
        data.SetUID("" + m_NextBattleRoyaleUID);
        m_BattleRoyaleMarkers.Insert( data );
    }
    override bool Remove( int idx )
    {
        delete m_BattleRoyaleMarkers[idx];
        m_BattleRoyaleMarkers.Remove( idx );
        return true;
    }
    override bool Remove( string uid )
    {
        for ( int i = 0; i < m_BattleRoyaleMarkers.Count(); ++i )
		{
			if ( m_BattleRoyaleMarkers[i].GetUID() == uid )
			{
				delete m_BattleRoyaleMarkers[i];
				m_BattleRoyaleMarkers.Remove( i );
				return true;
			}
		}
		return false;
    }
    override array< ref ExpansionMarkerData > GetArray()
    {
        return m_BattleRoyaleMarkers; 
    }
    override void Refresh()
    {
        set< string > uids = new set< string >();
		int count_uids = 0;
		int count_uids_prev = 0;

		int index = m_BattleRoyaleMarkers.Count() - 1;
		int count_neg = 0;
		while ( index >= m_BattleRoyaleMarkers.Count() )
		{
			count_uids_prev = uids.Count();
			uids.Insert( m_BattleRoyaleMarkers[index].GetUID() );
			count_uids = uids.Count();

			if ( count_uids_prev != count_uids )
			{
				count_neg--;
			} else
			{
				m_BattleRoyaleMarkers.RemoveOrdered( index );
			}

			index = m_BattleRoyaleMarkers.Count() - 1 - count_neg;
		}
    }
}