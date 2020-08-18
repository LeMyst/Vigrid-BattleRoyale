/*
This class had to be heavily modified 
to make adding custom marker data types easy to interact with


*/
static string MARKERDATA_TYPE_PERSONAL = "PERSONAL";
static string MARKERDATA_TYPE_SERVER = "SERVER";
static string MARKERDATA_TYPE_PARTY = "PARTY";

modded class ExpansionMarkerClientData
{
    ref map<string, ref MarkerClientData> m_ClientData;

    void ExpansionMarkerClientData()
	{
        m_ClientData = new map<string, ref MarkerClientData>();
    }
    void ~ExpansionMarkerClientData()
	{
        delete m_ClientData;
    }
    void Init() //Modders can override this to insert their own marker data types
    {
        m_ClientData.Insert( MARKERDATA_TYPE_PERSONAL, new PersonnalMarkerClientData );
        m_ClientData.Insert( MARKERDATA_TYPE_SERVER, new ServerMarkerClientData );
        m_ClientData.Insert( MARKERDATA_TYPE_PARTY, new PartyMarkerClientData );
    }
    ref MarkerClientData GetMarkerData(string type)
    {
        return m_ClientData.Get( type );
    }

    
    //========== PERSONAL ==========
    override int PersonalCount()
    {
        return GetMarkerData( MARKERDATA_TYPE_PERSONAL ).Count();
    }
    override ref ExpansionMarkerData PersonalGet( int idx )
    {
        return GetMarkerData( MARKERDATA_TYPE_PERSONAL ).Get( idx );
    }
    override void PersonalInsert( ref ExpansionMarkerData data)
    {
        GetMarkerData( MARKERDATA_TYPE_PERSONAL ).Insert( data );
    }
    override bool PersonalRemove( int idx )
	{
		return GetMarkerData( MARKERDATA_TYPE_PERSONAL ).Remove( idx );
	}
    override bool PersonalRemove( string uid )
	{
        return GetMarkerData( MARKERDATA_TYPE_PERSONAL ).Remove( uid );
    }
    override array< ref ExpansionMarkerData > PersonalGet()
	{
        //-- this functionality only applies to personal markers, so we need to cast
        PersonnalMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PERSONAL ) ))
        {
            return m_PMCD.GetArray();
        }
		return NULL;
	}
    //============ PARTY ============
    override int PartyCount() 
    {
        return GetMarkerData( MARKERDATA_TYPE_PARTY ).Count();
    }
    override ref ExpansionMarkerData PartyGet( int i )
    {
        return GetMarkerData( MARKERDATA_TYPE_PARTY ).Get( i );
    }
    override ref ExpansionMarkerData PartyGet( string uid )
	{
        return GetMarkerData( MARKERDATA_TYPE_PARTY ).Get( uid );
    }
    override int PartySetVisibility( string uid, int vis )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.SetVisibility( uid, vis );
        }
		return 0;
    }
    override int PartyRemoveVisibility( string uid, int vis )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.RemoveVisibility( uid, vis );
        }
		return 0;
    }
    override void PartyInsert( ref ExpansionMarkerData data )
    {
        GetMarkerData( MARKERDATA_TYPE_PARTY ).Insert( data );
    }
    override void PartyUpdate( ref ExpansionMarkerData data )
    {
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.Update( data );
        }
    }
    override bool PartyRemove( string uid )
    {
        return GetMarkerData( MARKERDATA_TYPE_PARTY ).Remove( uid );
    }
    override int PartyPlayerCount()
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.PlayerCount();
        }
		return 0;
    }
    override ref ExpansionPlayerMarkerData PartyPlayerGet( int i )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.PlayerGet( i );
        }
		return NULL;
    }
    override ref ExpansionPlayerMarkerData PartyPlayerGet( string uid )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.PlayerGet( uid );
        }
		return NULL;
    }
    override int PartyPlayerSetVisibility( string uid, int vis )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.PlayerSetVisibility( uid, vis );
        }
		return 0;
    }
    override int PartyPlayerRemoveVisibility( string uid, int vis )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.PlayerRemoveVisibility( uid, vis );
        }
		return 0;
    }
    override int PartyQuickCount()
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.QuickCount();
        }
		return 0;
    }
    override ref ExpansionPartyQuickMarkerData PartyQuickGet( int i )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.QuickGet( i );
        }
		return NULL;
    }
    override ref ExpansionPartyQuickMarkerData PartyQuickGet( string uid )
	{
        PartyMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_PARTY ) ))
        {
            return m_PMCD.QuickGet( uid );
        }
		return NULL;
    }
    //============ SERVER ==================
    override int ServerCount()
    {
        return GetMarkerData( MARKERDATA_TYPE_SERVER ).Count();
    }
    override ref ExpansionMarkerData ServerGet( int idx )
    {
        return GetMarkerData( MARKERDATA_TYPE_SERVER ).Get(idx);
    }
    override ref ExpansionMarkerData ServerGet( string uid )
    {
        return GetMarkerData( MARKERDATA_TYPE_SERVER ).Get(uid);
    }
    override int ServerSetVisibility( string uid, int vis )
	{
        ServerMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_SERVER ) ))
        {
            return m_PMCD.SetVisibility( uid, vis );
        }
		return 0;
    }
    override int ServerRemoveVisibility( string uid, int vis )
	{
        ServerMarkerClientData m_PMCD; 
        if(Class.CastTo( m_PMCD , GetMarkerData( MARKERDATA_TYPE_SERVER ) ))
        {
            return m_PMCD.RemoveVisibility( uid, vis );
        }
		return 0;
    }
    //===== OTHER METHODS ===

    override void OnStoreSave( ParamsWriteContext file )
    {
        file.Write( m_IP );
		file.Write( m_Port );

        array<ref MarkerClientData> all_data = m_ClientData.GetValueArray();
        for(int i = 0; i < all_data.Count();i++)
        {
            ref MarkerClientData marker_data = all_data.Get( i );
            marker_data.OnStoreSave( file );
        }
    }
    override bool OnStoreLoad( ParamsReadContext ctx, int version )
    {
        if ( Expansion_Assert_False( ctx.Read( m_IP ), "[" + this + "] Failed reading m_IP" ) )
			return false;
		if ( Expansion_Assert_False( ctx.Read( m_Port ), "[" + this + "] Failed reading m_Port" ) )
			return false;

        
        array<ref MarkerClientData> all_data = m_ClientData.GetValueArray();
        for(int i = 0; i < all_data.Count();i++)
        {
            ref MarkerClientData marker_data = all_data.Get( i );
            if(!marker_data.OnStoreLoad( ctx, version ))
                return false;
        }

        return true;
    }

    override void OnRefresh()
    {
        array<ref MarkerClientData> all_data = m_ClientData.GetValueArray();
        for(int i = 0; i < all_data.Count();i++)
        {
            ref MarkerClientData marker_data = all_data.Get(i);

            marker_data.Refresh();
        }
    }

    ref array< ExpansionMarkerData > GetAll()
    {
        ref array< ExpansionMarkerData > markers = new array< ExpansionMarkerData >();
        
        array<ref MarkerClientData> all_data = m_ClientData.GetValueArray();
        for(int i = 0; i < all_data.Count();i++)
        {
            ref MarkerClientData marker_data = all_data.Get(i);

            markers.InsertAll(marker_data.ToArray());
        }

        return markers;
    }
}


//this acts like an interface, defining common methods that we'll be using (note that we can add unique methods in each one, but a cast would be required to access that data)
class MarkerClientData
{
    int Count() { return 0; }
    ref ExpansionMarkerData Get( int idx ) { return NULL; }
    ref ExpansionMarkerData Get( int uid ) { return NULL; }
    void Insert( ref ExpansionMarkerData data ) { return; }
    bool Remove( int idx ) { return false; } 
    bool Remove( string uid ) { return false; } 
    void Refresh() { return; }
    array< ref ExpansionMarkerData > GetArray() { return new array< ref ExpansionMarkerData >(); }
    void OnStoreSave( ParamsWriteContext file ) { return; }
    bool OnStoreLoad( ParamsReadContext ctx, int version ) { return true; }
}
class PersonnalMarkerClientData extends MarkerClientData
{
    int m_NextPersonalUID;
    ref array< ref ExpansionMarkerData > m_PersonalMarkers;

    void PersonnalMarkerClientData()
    {
        m_NextPersonalUID = -1;
        m_PersonalMarkers = new array< ref ExpansionMarkerData >();
    }
    void ~PersonnalMarkerClientData()
    {
        delete m_PersonalMarkers;
    }

    //--- interface methods (note that some are not implemented * i only did those that were in use *)
    override void OnStoreSave( ParamsWriteContext file ) 
    {
        //Note: it is the job of the caller to ensure the context cursor is correct
        file.Write( m_NextPersonalUID );
		file.Write( m_PersonalMarkers.Count() );
		for ( int i = 0; i < m_PersonalMarkers.Count(); ++i )
		{
			file.Write( m_PersonalMarkers[i].GetUID() );
			m_PersonalMarkers[i].OnStoreSave( file );
		}
    }
    override bool OnStoreLoad( ParamsReadContext ctx, int version )
    {
        //Note: it is the job of the caller to ensure the context cursor is correct
        int count = 0;
		int index = 0;
		string uid = "";

        if ( version >= 8 )
		{
			if ( Expansion_Assert_False( ctx.Read( m_NextPersonalUID ), "[" + this + "] Failed reading m_NextPersonalUID" ) )
				return false;
		}
        if ( !Expansion_Assert_False( ctx.Read( count ), "[" + this + "] Failed reading personal count" ) )
		{
			for ( index = 0; index < count; ++index ) 
			{
				if ( Expansion_Assert_False( ctx.Read( uid ), "[" + this + "] Failed reading uid" ) )
					return false;

				ExpansionMarkerData newMarker = ExpansionMarkerData.Create( ExpansionMapMarkerType.PERSONAL, uid );
				if ( !newMarker.OnStoreLoad( ctx, version ) )
					return false;
				
				m_PersonalMarkers.Insert( newMarker );
			}
		} else
		{
			return false;
		}

        return true;
    }
    
    override int Count()
    {
        return m_PersonalMarkers.Count();
    }
    override ref ExpansionMarkerData Get( int idx )
    {
        return m_PersonalMarkers[idx];
    }
    override void Insert( ref ExpansionMarkerData data ) 
    {
        m_NextPersonalUID++;
        data.SetUID("" + m_NextPersonalUID);
        m_PersonalMarkers.Insert( data );
    }
    override bool Remove( int idx )
    {
        delete m_PersonalMarkers[idx];
        m_PersonalMarkers.Remove( idx );
        return true;
    }
    override bool Remove( string uid )
    {
        for ( int i = 0; i < m_PersonalMarkers.Count(); ++i )
		{
			if ( m_PersonalMarkers[i].GetUID() == uid )
			{
				delete m_PersonalMarkers[i];
				m_PersonalMarkers.Remove( i );
				return true;
			}
		}
		return false;
    }
    override array< ref ExpansionMarkerData > GetArray()
    {
        return m_PersonalMarkers; 
    }
    override void Refresh()
    {
        set< string > uids = new set< string >();
		int count_uids = 0;
		int count_uids_prev = 0;

		int index = m_PersonalMarkers.Count() - 1;
		int count_neg = 0;
		while ( index >= m_PersonalMarkers.Count() )
		{
			count_uids_prev = uids.Count();
			uids.Insert( m_PersonalMarkers[index].GetUID() );
			count_uids = uids.Count();

			if ( count_uids_prev != count_uids )
			{
				count_neg--;
			} else
			{
				m_PersonalMarkers.RemoveOrdered( index );
			}

			index = m_PersonalMarkers.Count() - 1 - count_neg;
		}
    }
}
class PartyMarkerClientData extends MarkerClientData
{
    protected ExpansionPartyModule m_PartyModule;
    ref array< ref ExpansionMarkerClientInfo > m_MarkerInfo_Party;
    ref array< ref ExpansionMarkerClientInfo > m_MarkerInfo_PartyPlayers;
    protected int m_PartyUID;

    void PartyMarkerClientData()
    {
        Class.CastTo( m_PartyModule, GetModuleManager().GetModule( ExpansionPartyModule ) );
        m_MarkerInfo_Party = new array< ref ExpansionMarkerClientInfo >();
        m_MarkerInfo_PartyPlayers = new array< ref ExpansionMarkerClientInfo >();
        //TODO: onstoresave / onstoreload
    }
    void ~PartyMarkerClientData()
    {
        
    }
    //--- unique methods
    int SetVisibility( string uid, int vis )
    {
        for ( int i = 0; i < m_MarkerInfo_Party.Count(); ++i )
		{
			if ( m_MarkerInfo_Party[i].GetUID() == uid )
			{
				return m_MarkerInfo_Party[i].SetVisibility( vis );
			}
		}

		return 0;
    }
    int RemoveVisibility( string uid, int vis )
    {
        for ( int i = 0; i < m_MarkerInfo_Party.Count(); ++i )
		{
			if ( m_MarkerInfo_Party[i].GetUID() == uid )
			{
				return m_MarkerInfo_Party[i].RemoveVisibility( vis );
			}
		}

		return 0;
    }
    void Update( ref ExpansionMarkerData data )
    {
        m_PartyModule.UpdateMarker( data );
    }
    int PlayerCount()
    {
        if ( !m_PartyModule || !m_PartyModule.GetParty() )
			return 0;

		return m_PartyModule.GetParty().GetPlayers().Count();
    }
    ref ExpansionPlayerMarkerData PlayerGet( int i )
    {
        return m_PartyModule.GetParty().GetPlayers()[i].Marker;
    }
    ref ExpansionPlayerMarkerData PlayerGet( string uid )
    {
        return m_PartyModule.GetParty().GetPlayer( uid ).Marker;
    }
    int PlayerSetVisibility( string uid, int vis )
    {
        for ( int i = 0; i < m_MarkerInfo_PartyPlayers.Count(); ++i )
		{
			if ( m_MarkerInfo_PartyPlayers[i].GetUID() == uid )
			{
				return m_MarkerInfo_PartyPlayers[i].SetVisibility( vis );
			}
		}

		return 0;
    }
    int PlayerRemoveVisibility( string uid, int vis )
	{
		for ( int i = 0; i < m_MarkerInfo_PartyPlayers.Count(); ++i )
		{
			if ( m_MarkerInfo_PartyPlayers[i].GetUID() == uid )
			{
				return m_MarkerInfo_PartyPlayers[i].RemoveVisibility( vis );
			}
		}

		return 0;
	}
    ref ExpansionPartyQuickMarkerData QuickGet( int i )
    {
        return m_PartyModule.GetParty().GetPlayers()[i].QuickMarker;
    }
    ref ExpansionPartyQuickMarkerData QuickGet( string uid )
	{
		return m_PartyModule.GetParty().GetPlayer( uid ).QuickMarker;
	}
    int QuickCount()
	{
		if ( !m_PartyModule || !m_PartyModule.GetParty() )
			return 0;

		return m_PartyModule.GetParty().GetPlayers().Count();
	}


    //--- interface methods
    override int Count()
    {
        if ( !m_PartyModule || !m_PartyModule.GetParty() )
			return 0;

        return m_PartyModule.GetParty().GetAllMarkers().Count();
    }
    override ref ExpansionMarkerData Get( int idx )
    {
        return m_PartyModule.GetParty().GetAllMarkers()[idx];
    }
    override ref ExpansionMarkerData Get( int uid )
    {
        array< ref ExpansionMarkerData > markers = m_PartyModule.GetParty().GetAllMarkers();
		for ( int i = 0; i < markers.Count(); ++i )
		{
			if ( markers[i].GetUID() == uid )
			{
				return markers[i];
			}
		}

		return NULL;
    }
    override void Insert( ref ExpansionMarkerData data ) 
    {
        m_PartyModule.CreateMarker( data );
    }
    override bool Remove( string uid ) 
    { 
        if ( m_PartyModule )
		{
			m_PartyModule.DeleteMarker( uid );
			return true;
		}

		return false;
    } 
    override void Refresh()
    {
        if ( !m_PartyModule || !m_PartyModule.HasParty() )
		{
			m_MarkerInfo_Party.Clear();
			m_MarkerInfo_PartyPlayers.Clear();
			
			return;
		}

        ExpansionMarkerData data;
		ref ExpansionMarkerClientInfo info;

		ExpansionPartyData party = m_PartyModule.GetParty();

		int count = m_MarkerInfo_Party.Count();
		int index = 0;

		for ( index = 0; index < count; ++index )
		{
			data = Get( m_MarkerInfo_Party[index].GetUID() );
			if ( data )
			{
				data.SetVisibility( m_MarkerInfo_Party[index].GetVisibility() );
			}
		}

		count = m_MarkerInfo_PartyPlayers.Count();

		for ( index = 0; index < count; ++index )
		{
			data = PlayerGet( m_MarkerInfo_PartyPlayers[index].GetUID() );
			if ( data )
			{
				data.SetVisibility( m_MarkerInfo_PartyPlayers[index].GetVisibility() );
			}
		}
		
		m_MarkerInfo_Party.Clear();
		m_MarkerInfo_PartyPlayers.Clear();

		array< ref ExpansionMarkerData > markers = party.GetAllMarkers();

		count = markers.Count();
		for ( index = 0; index < count; ++index )
		{
			info = new ExpansionMarkerClientInfo( markers[index].GetUID() );
			m_MarkerInfo_Party.Insert( info );
		}

		array< ref ExpansionPartyPlayerData > players = party.GetPlayers();

		count = players.Count();
		for ( index = 0; index < count; ++index )
		{
			info = new ExpansionMarkerClientInfo( players[index].UID );
			m_MarkerInfo_PartyPlayers.Insert( info );
		}
    }
    override array< ref ExpansionMarkerData > GetArray()
    {
        ref array< ExpansionMarkerData > markers = new array< ExpansionMarkerData >();
        ExpansionMarkerData marker;
        int i = 0;
        for ( i = 0; i < Count(); ++i )
		{
			marker = Get( i );
			if ( marker )
				markers.Insert( marker );
		}

        PlayerBase localPlayer = PlayerBase.Cast( GetGame().GetPlayer() );
		string localUid = "";
		if ( localPlayer )
			localUid = localPlayer.GetIdentityUID();

        for ( i = 0; i < PlayerCount(); ++i )
		{
			marker = PlayerGet( i );
			if ( marker && marker.GetUID() != localUid )
				markers.Insert( marker );
		}

        for ( i = 0; i < QuickCount(); ++i )
		{
			marker = QuickGet( i );
			if ( marker )
				markers.Insert( marker );
		}

        return markers;
    }

    override void OnStoreSave( ParamsWriteContext file ) 
    {
        //Note: it is the job of the caller to ensure the context cursor is correct
        file.Write( m_PartyUID );
		file.Write( m_MarkerInfo_Party.Count() );
		for ( i = 0; i < m_MarkerInfo_Party.Count(); ++i )
		{
			m_MarkerInfo_Party[i].OnStoreSave( file );
		}
        file.Write( m_MarkerInfo_PartyPlayers.Count() );
		for ( i = 0; i < m_MarkerInfo_PartyPlayers.Count(); ++i )
		{
			m_MarkerInfo_PartyPlayers[i].OnStoreSave( file );
		}
    }
    override bool OnStoreLoad( ParamsReadContext ctx, int version )
    {
        //Note: it is the job of the caller to ensure the context cursor is correct
        int count = 0;
		int index = 0;
		string uid = "";
        ExpansionMarkerClientInfo newMarkerInfo;

        if ( Expansion_Assert_False( ctx.Read( m_PartyUID ), "[" + this + "] Failed reading m_PartyUID" ) )
			return false;

        if ( !Expansion_Assert_False( ctx.Read( count ), "[" + this + "] Failed reading party count" ) )
		{
			for ( index = 0; index < count; ++index ) 
			{
				newMarkerInfo = new ExpansionMarkerClientInfo();
				if ( !newMarkerInfo.OnStoreLoad( ctx, version ) )
					return false;
				
				m_MarkerInfo_Party.Insert( newMarkerInfo );
			}
		} else
		{
			return false;
		}

        if ( !Expansion_Assert_False( ctx.Read( count ), "[" + this + "] Failed reading party player count" ) )
		{
			for ( index = 0; index < count; ++index ) 
			{
				newMarkerInfo = new ExpansionMarkerClientInfo();
				if ( !newMarkerInfo.OnStoreLoad( ctx, version ) )
					return false;
				
				m_MarkerInfo_PartyPlayers.Insert( newMarkerInfo );
			}
		} else
		{
			return false;
		}

        return true;
    }
}
class ServerMarkerClientData extends MarkerClientData
{
    ref array< ref ExpansionMarkerClientInfo > m_MarkerInfo_Server;

    void ServerMarkerClientData()
    {
        m_MarkerInfo_Server = new array< ref ExpansionMarkerClientInfo >();
        //TODO: onstore/onsave
    }
    void ~ServerMarkerClientData()
    {
        
    }
    //--- unique methods
    int SetVisibility( string uid, int vis )
    {
        for ( int i = 0; i < m_MarkerInfo_Server.Count(); ++i )
		{
			if ( m_MarkerInfo_Server[i].GetUID() == uid )
			{
				return m_MarkerInfo_Server[i].SetVisibility( vis );
			}
		}

		return 0;
    }
    int RemoveVisibility( string uid, int vis )
    {
        for ( int i = 0; i < m_MarkerInfo_Server.Count(); ++i )
		{
			if ( m_MarkerInfo_Server[i].GetUID() == uid )
			{
				return m_MarkerInfo_Server[i].RemoveVisibility( vis );
			}
		}

		return 0;
    }

    //--- interface methods
    override int Count()
    {
        return GetExpansionSettings().GetMap().ServerMarkers.Count();
    }
    override ref ExpansionMarkerData Get( int idx )
    {
        return GetExpansionSettings().GetMap().ServerMarkers[ idx ];
    }
    override ref ExpansionMarkerData Get( int uid )
    {
        array< ref ExpansionMarkerData > markers = GetExpansionSettings().GetMap().ServerMarkers;
		for ( int i = 0; i < markers.Count(); ++i )
		{
			if ( markers[i].GetUID() == uid )
			{
				return markers[i];
			}
		}

		return NULL;
    }
    override void Refresh()
    {

		int count = m_MarkerInfo_Server.Count();
		int index = 0;

		for ( index = 0; index < count; ++index )
		{
			ExpansionMarkerData data = Get( m_MarkerInfo_Server[index].GetUID() );
			if ( data )
			{
				data.SetVisibility( m_MarkerInfo_Server[index].GetVisibility() );
			}
		}

		m_MarkerInfo_Server.Clear();

		array< ref ExpansionMarkerData > markers = GetExpansionSettings().GetMap().ServerMarkers;

		count = markers.Count();
		for ( index = 0; index < count; ++index )
		{
			ref ExpansionMarkerClientInfo info = new ExpansionMarkerClientInfo( markers[index].GetUID() );
			m_MarkerInfo_Server.Insert( info );
		}
    }
    override array< ref ExpansionMarkerData > GetArray()
    {
        ref array< ExpansionMarkerData > markers = new array< ExpansionMarkerData >();
        ExpansionMarkerData marker;
        int i = 0;

        for ( i = 0; i < Count(); ++i )
        {
            marker = Get( i );
            if ( marker )
                markers.Insert( marker );
        }

        return markers;
    }

    override void OnStoreSave( ParamsWriteContext file ) 
    {
        //Note: it is the job of the caller to ensure the context cursor is correct
        file.Write( m_MarkerInfo_Server.Count() );
		for ( i = 0; i < m_MarkerInfo_Server.Count(); ++i )
		{
			m_MarkerInfo_Server[i].OnStoreSave( file );
		}
    }
    override bool OnStoreLoad( ParamsReadContext ctx, int version )
    {
        //Note: it is the job of the caller to ensure the context cursor is correct
        int count = 0;
		int index = 0;
		string uid = "";
        ExpansionMarkerClientInfo newMarkerInfo;

        if ( !Expansion_Assert_False( ctx.Read( count ), "[" + this + "] Failed reading server count" ) )
		{
			for ( index = 0; index < count; ++index ) 
			{
				newMarkerInfo = new ExpansionMarkerClientInfo();
				if ( !newMarkerInfo.OnStoreLoad( ctx, version ) )
					return false;
				
				m_MarkerInfo_Server.Insert( newMarkerInfo );
			}
		} else
		{
			return false;
		}

        return true;
    }

}