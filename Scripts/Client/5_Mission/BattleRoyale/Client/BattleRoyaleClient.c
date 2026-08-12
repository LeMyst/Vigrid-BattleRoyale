#ifndef SERVER
class BattleRoyaleClient: BattleRoyaleBase
{
    protected ref BattleRoyalePlayArea m_CurrentPlayArea;
    protected ref BattleRoyalePlayArea m_FuturePlayArea;

    protected int i_Kills; //TODO: this needs to be done differently (most likely)
    protected bool b_MatchStarted;
    protected int i_SecondsRemaining;

    protected bool b_IsReady;

#ifdef EXPANSION_MAP_ZONES
    protected ref ExpansionServerMarkerData m_ZoneCenterMapMarker;
#endif

    protected ref BattleRoyaleSpeakingList m_SpeakingList;

    void BattleRoyaleClient()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleClient::BattleRoyaleClient");

        b_IsReady = false;
        b_MatchStarted = false;
        i_Kills = 0;
        i_SecondsRemaining = 0;

        Init();
    }

    void ~BattleRoyaleClient()
    {
    	BattleRoyaleUtils.Trace("BattleRoyaleClient::~BattleRoyaleClient");

#ifdef VIGRID_MAP
        // The map addon's zone state is static and outlives this object, so without this the
        // previous match's circles would still be drawn after a server change.
        VigridMapAPI.ClearZones();
#endif
    }

    void Init()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleClient::Init");

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLaterByName( this, "OnSecond", 1000, true );

		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
		br_rpc.Reset();

		m_SpeakingList = new BattleRoyaleSpeakingList();

		BattleRoyaleUtils.Trace("BattleRoyaleClient::Init - Done");
    }

    //--- note: these return NULL of there is no area referenced for next or current area
    BattleRoyalePlayArea GetPlayArea()
    {
        return m_CurrentPlayArea;
    }

    BattleRoyalePlayArea GetNextArea()
    {
        return m_FuturePlayArea;
    }

	// To track changes
    bool br_previous_fade_state = false;
    bool br_previous_input_state = false;
    vector br_previous_current_play_area_center;
    float br_previous_current_play_area_radius;
    vector br_previous_future_play_area_center;
    float br_previous_future_play_area_radius;
    bool br_previous_win_screen = false;
    int br_previous_countdown = 0;

    override void Update(float delta)
    {
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );

		float distExt;
		float distInt;
		float angle;
		bool isInsideZone;

		// First check if player is outside current play area
		if (m_CurrentPlayArea)
		{
			// Check if player is inside current play area and get distExt, distInt and angle
			isInsideZone = GetZoneDistance(m_CurrentPlayArea, distExt, distInt, angle);

			// If outside current play area, show distance to it
			if (!isInsideZone)
			{
				gameplay.UpdateZoneDistance(isInsideZone, distExt, distInt, angle);
			}
			// Player is inside current play area, check if future play area exists
			else if (m_FuturePlayArea)
			{
				isInsideZone = GetZoneDistance(m_FuturePlayArea, distExt, distInt, angle);
				gameplay.UpdateZoneDistance(isInsideZone, distExt, distInt, angle);
			}
			// Player inside current area but no future area
			else
			{
				gameplay.UpdateZoneDistance(true, 0, distInt, angle);
			}
		}
		// No current play area
		else
		{
			gameplay.HideDistance();
		}

		// If we have a blue zone, show visual effect when outside of zone
        if( m_CurrentPlayArea )
        {
            GetZoneDistance( m_CurrentPlayArea, distExt, distInt, angle );

            if (distExt > 0)
            {
                player.QueueAddGlassesEffect(PPERequesterBank.REQ_BATTLEROYALE);
            }
            else
            {
                player.QueueRemoveGlassesEffect(PPERequesterBank.REQ_BATTLEROYALE);
            }
        }

#ifdef BR_MINIMAP
        vector camera_pos = GetGame().GetCurrentCameraPosition();
        gameplay.UpdateMiniMap( camera_pos );
#endif
		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();

		if( br_rpc )
		{
			// Update player and group remaining count
			PlayerCountChanged( br_rpc.nb_players, br_rpc.nb_groups );

			// Fade in/out effect
			if( br_previous_fade_state != br_rpc.fade_state )
			{
				if( br_rpc.fade_state )
				{
					FadeIn();
				}
				else
				{
					FadeOut();
				}
				br_previous_fade_state = br_rpc.fade_state;
			}

			// Input enable/disable
			if( br_previous_input_state != br_rpc.input_state )
			{
				player.DisableInput( br_rpc.input_state );
				br_previous_input_state = br_rpc.input_state;
			}

			// Update player kill count
			gameplay.UpdateKillCount( br_rpc.player_kills );

			// Update match start state
			if( br_rpc.match_started && !b_MatchStarted )
			{
				OnMatchStarted();
			}

			// Update countdown timer and zone distance
			if ( br_previous_countdown != br_rpc.countdown_seconds )
			{
				i_SecondsRemaining = br_rpc.countdown_seconds;
				gameplay.UpdateCountdownTimer( i_SecondsRemaining );
				br_previous_countdown = br_rpc.countdown_seconds;
			}

			// Update current play area. Diffed like the future area below, and for the same
			// reason: without the diff this allocated a fresh BattleRoyalePlayArea every frame
			// for the entire match.
			if ( br_previous_current_play_area_center != br_rpc.current_play_area_center || br_previous_current_play_area_radius != br_rpc.current_play_area_radius )
			{
				// "0 0 0" with a zero radius is the server deliberately clearing the area, not an
				// absent update - 7_BattleRoyaleLastRound sends exactly that. Treating it as
				// "nothing to do" left the final circle on the client for ever.
				if ( br_rpc.current_play_area_center != "0 0 0" && br_rpc.current_play_area_radius != 0.0 )
					m_CurrentPlayArea = new BattleRoyalePlayArea( br_rpc.current_play_area_center, br_rpc.current_play_area_radius );
				else
					m_CurrentPlayArea = NULL;

				br_previous_current_play_area_center = br_rpc.current_play_area_center;
				br_previous_current_play_area_radius = br_rpc.current_play_area_radius;
			}

			// Update future play area
			if ( br_previous_future_play_area_center != br_rpc.future_play_area_center || br_previous_future_play_area_radius != br_rpc.future_play_area_radius )
			{
				if ( br_rpc.future_play_area_center && br_rpc.future_play_area_radius )
				{
					m_FuturePlayArea = new BattleRoyalePlayArea( br_rpc.future_play_area_center, br_rpc.future_play_area_radius );

#ifdef EXPANSION_MAP_ZONES
					UpdateZoneCenterMaker( br_rpc.future_play_area_center );
#endif

					if ( br_rpc.b_ArtillerySound )
					{
						ref EffectSound m_ArtySound = SEffectManager.PlaySound("Artillery_Distant_SoundSet", m_FuturePlayArea.GetCenter(), 0.1, 0.1);
						m_ArtySound.SetAutodestroy(true);
					}
				}
				br_previous_future_play_area_center = br_rpc.future_play_area_center;
				br_previous_future_play_area_radius = br_rpc.future_play_area_radius;
			}

#ifdef VIGRID_MAP
			// Hand the two circles to the map addon. Push rather than pull: the addon may not
			// reference a BattleRoyale* symbol, so it cannot come and fetch these itself.
			// Called unconditionally - VigridMapAPI diffs internally and only does work when a
			// circle has actually moved, so this costs four comparisons on a normal frame.
			vector map_current_center = "0 0 0";
			float map_current_radius = 0;
			vector map_next_center = "0 0 0";
			float map_next_radius = 0;

			if ( m_CurrentPlayArea )
			{
				map_current_center = m_CurrentPlayArea.GetCenter();
				map_current_radius = m_CurrentPlayArea.GetRadius();
			}

			if ( m_FuturePlayArea )
			{
				map_next_center = m_FuturePlayArea.GetCenter();
				map_next_radius = m_FuturePlayArea.GetRadius();
			}

			VigridMapAPI.SetZones( map_current_center, map_current_radius, map_next_center, map_next_radius );
#endif

			// Set top position
			if ( player )
			{
				player.position_top = br_rpc.top_position;
			}

			// Update the list of players currently speaking
			if( m_SpeakingList )
			{
				bool show_speaking = br_rpc.speaking_list_enabled;
				if( b_MatchStarted && !br_rpc.speaking_list_during_match )
					show_speaking = false;

				m_SpeakingList.Update( show_speaking );
			}

			// Show the winner screen
			if( br_rpc.winner_screen && !br_previous_win_screen )
			{
				Widget win_screen_hud = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/win_screen.layout");
				win_screen_hud.Show( true );
				br_previous_win_screen = true;
			}
		}
    }

#ifdef EXPANSION_MAP_ZONES
    //! Red dot at the next zone centre, on DayZ Expansion's map and in its 3D marker layer.
    //! Superseded by Extra/Map/, which draws both without needing Expansion at all.
    protected void UpdateZoneCenterMaker(vector center)
    {
        if (!m_ZoneCenterMapMarker)
        {
            m_ZoneCenterMapMarker = new ExpansionServerMarkerData("ServerMarker_Zone_Center");
            m_ZoneCenterMapMarker.Set3D(true);
            m_ZoneCenterMapMarker.SetName("Center");
            m_ZoneCenterMapMarker.SetIconName("Map Marker");
            m_ZoneCenterMapMarker.SetColor(ARGB(255, 255, 0, 0));
            m_ZoneCenterMapMarker.SetVisibility(EXPANSION_MARKER_VIS_WORLD | EXPANSION_MARKER_VIS_MAP);
            GetExpansionSettings().GetMap().AddServerMarker(m_ZoneCenterMapMarker);
        }

        m_ZoneCenterMapMarker.SetPosition( center + "0 5 0" );
    }
#endif

    protected bool GetZoneDistance(BattleRoyalePlayArea play_area, out float distExt, out float distInt, out float angle)
    {
        vector center = play_area.GetCenter();
        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        vector playerpos = player.GetPosition();

        //2d distance check
        center[1] = 0;
        playerpos[1] = 0;
        float distance_from_center = vector.Distance(center, playerpos);
        distExt = distance_from_center - play_area.GetRadius();
        distInt = Math.AbsFloat(distance_from_center);
        vector playerdir = vector.Direction(playerpos, center);
		angle = Math.NormalizeAngle(360 - ( GetGame().GetCurrentCameraDirection().VectorToAngles()[0] - playerdir.VectorToAngles()[0] ) );

        return distExt < 0;
    }

    protected void PlayerCountChanged(int nb_players, int nb_groups)
    {
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        if (gameplay)
        {
            //BattleRoyaleUtils.Trace(string.Format("PlayerCountChanged: %1 %2", nb_players, nb_groups));
            gameplay.UpdatePlayerCount( nb_players, nb_groups );
        }
    }

    protected void FadeIn()
    {
        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        BattleRoyaleUtils.Trace("BattleRoyale: FADE IN!");

        PPERequesterBank.GetRequester(PPERequester_BurlapSackEffects).Start();
        player.SetInventorySoftLock(true);
        player.SetMasterAttenuation("BurlapSackAttenuation");
    }

    protected void FadeOut()
    {
        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        BattleRoyaleUtils.Trace("BattleRoyale: FADE OUT!");

        PPERequesterBank.GetRequester(PPERequester_BurlapSackEffects).Stop();
        player.SetInventorySoftLock(false);
        player.SetMasterAttenuation("");
    }

    protected void OnSecond()
    {
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        if(i_SecondsRemaining > 0)
        {
            i_SecondsRemaining--;
            gameplay.UpdateCountdownTimer(i_SecondsRemaining);
        }
        else
        {
            gameplay.HideCountdownTimer();
        }
    }

    protected void AddPlayerKilled(int increase)
    {
        i_Kills += increase;
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        gameplay.UpdateKillCount(i_Kills);
    }

    protected void OnMatchStarted()
    {
        if(b_MatchStarted)
        {
            Error("Match started already but received another RPC?");
        }
        VONManager.GetInstance().SetMaxVolume( VoiceLevelShout );
        //VONManager.GetInstance().EnableVoice( true );
        b_MatchStarted = true;
    }

    void ReadyUp()
    {
        //if(b_IsReady)
        //    return; //already ready!

        b_IsReady = true; //this only runs once

        ref Param1<bool> ready_state = new Param1<bool>( true );  //perhaps this can be made togglable?
        //--- No target: the server resolves the subject from the RPC sender identity. Sending one
        //--- would be ignored, and inviting it back is how the ready-up-anyone exploit worked.
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", ready_state, false );

    }

    void Unstuck()
    {
        //--- No target - see ReadyUp above.
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerUnstuck", NULL, true );

    }

    //--- There was a ReportSteamName() here, sending BiosUser.GetName() to the server as a fallback
    //--- for players the Steam Web API cannot answer for. It was removed after measurement:
    //--- on PC, BiosUser.GetName() returns the *profile* name, not the Steam persona. Two local
    //--- clients on one Steam account (persona "ANTHOxY") reported "Client_A" and "Survivor" -
    //--- their -name= launcher values. So the client has no persona name to offer, and this could
    //--- only ever have echoed back the placeholder it was meant to replace.

    //! Ask the server for one leaderboard ladder. The server rate-limits this per player, so the
    //! menu is free to call it on show, on every tab switch, and on a poll while it is open.
    void RequestLeaderboard( int board )
    {
        ref Param1<int> requested_board = new Param1<int>( board );
        //--- No target - see ReadyUp above.
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestLeaderboard", requested_board, true );
    }

    void StartMatch(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if ( type == CallType.Client )
        {
            OnMatchStarted();
        }
    }

    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        //unused
    }

    override void OnPlayerKilled(PlayerBase killed, Object killer)
    {
        //unused
    }
}
#endif
