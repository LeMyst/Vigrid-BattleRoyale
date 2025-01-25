#ifndef SERVER
modded class ExpansionMapMenu
{
    protected ref BattleRoyaleClient m_BattleRoyaleClient;
    protected ref BattleRoyaleMapMarkerZone m_CurrentZone;
    protected ref BattleRoyaleMapMarkerZone m_NextZone;

#ifdef SPECTATOR
    protected ref array<ref BattleRoyaleMapMarkerPlayerArrow> m_SpectatorPlayerMarkers;
#endif
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

#ifdef SPECTATOR
    //this updates players within the network bubble
    void UpdatePlayers()
    {
        //ensure objects exist
        if(!m_SpectatorPlayerMarkers)
        {
            m_SpectatorPlayerMarkers = new array<ref BattleRoyaleMapMarkerPlayerArrow>();
        }

        //get local players
        array<PlayerBase> players;
        PlayerBase.GetLocalPlayers( players );
        int i;
        int index = 0;

        array<string> local_identities = new array<string>();

        //create new markers if necessary
        for(i = 0; i < players.Count(); i++)
        {
            if(players[i] && players[i].GetIdentity())
            {
                local_identities.Insert( players[i].GetIdentity().GetId() ); //insert this players identity (so we know not to render the networked data on the map for this client)
            }

            //skip our player in the list (this is a bugfix => our player doesn't actually exist but game thinks it does)
            if(players[i] == PlayerBase.Cast( GetGame().GetPlayer() ) )
                continue;

            if(index == m_SpectatorPlayerMarkers.Count())
            {
                BattleRoyaleUtils.Trace("Spectator Map: Inserting new local player marker");
                m_SpectatorPlayerMarkers.Insert( new BattleRoyaleMapMarkerPlayerArrow( layoutRoot, m_MapWidget ) );
            }

            //update player (if necessary)
            m_SpectatorPlayerMarkers[index].SetPlayer( players[i] );

            //if player does not exist in marker list, then insert it
            if(m_Markers.Find(m_SpectatorPlayerMarkers[index]) == -1)
            {
                m_Markers.Insert( m_SpectatorPlayerMarkers[index] );
            }

            index++;
        }

        //delete old markers if unnecessary
        for(i = players.Count(); i < m_SpectatorPlayerMarkers.Count(); i++)
        {
            BattleRoyaleUtils.Trace("Spectator Map: Deleting local player marker");
            m_SpectatorPlayerMarkers[i].SetPlayer(null); //null out player
            m_Markers.RemoveItem( m_SpectatorPlayerMarkers[i] ); //remove from render list
        }

        if(!m_NetworkPlayerMarkers)
        {
            m_NetworkPlayerMarkers = new map<string, ref BattleRoyaleMapMarkerPlayerArrow>();
        }

        ref BattleRoyaleClient brc = BattleRoyaleClient.Cast( GetBR() );
        ref map<string, ref BattleRoyaleSpectatorMapEntityData> map_entity_data = brc.GetSpectatorMapEntityData();

        for(i = 0; i < map_entity_data.Count(); i++)
        {
            string Id = map_entity_data.GetKey(i);
            ref BattleRoyaleSpectatorMapEntityData data = map_entity_data.GetElement(i);
            if(local_identities.Find(Id) == -1)
            {
                //RENDER!
                //create
                if ( !m_NetworkPlayerMarkers.Contains( Id ) )
                {
                    m_NetworkPlayerMarkers.Insert( Id, new BattleRoyaleMapMarkerPlayerArrow( layoutRoot, m_MapWidget ) );
                }

                //update
                m_NetworkPlayerMarkers[Id].SetName( data.name );
                m_NetworkPlayerMarkers[Id].SetEntityPosition( data.position );
                m_NetworkPlayerMarkers[Id].SetEntityDirection( data.direction );

                //insert
                if ( m_Markers.Find(m_NetworkPlayerMarkers[Id]) == -1 )
                {
                    m_Markers.Insert( m_NetworkPlayerMarkers[Id] );
                }
            }
            else
            {
                //UNRENDER!
                if ( m_NetworkPlayerMarkers.Contains( Id ) )
                {
                    m_Markers.RemoveItem( m_NetworkPlayerMarkers[Id] );
                }
            }
        }
    }
#endif

    //ensure BR markers are rendering correct
    override void Update( float timeslice )
    {
        //BattleRoyaleUtils.Trace("Updating Zones...");
        UpdateZones();

#ifdef SPECTATOR
        //spectator markers
        if ( GetGame() && GetGame().GetMission() )
        {
            MissionGameplay gameplay;
            if ( Class.CastTo(gameplay, GetGame().GetMission()) )
            {
                if ( gameplay.IsInSpectator() )
                {
                    UpdatePlayers();
                }
            }
        }
#endif

        super.Update( timeslice );
    }
}
#endif
