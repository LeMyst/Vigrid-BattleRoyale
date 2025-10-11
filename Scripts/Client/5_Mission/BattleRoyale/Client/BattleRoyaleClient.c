#ifndef SERVER
class BattleRoyaleClient: BattleRoyaleBase
{
    protected ref BattleRoyalePlayArea m_CurrentPlayArea;
    protected ref BattleRoyalePlayArea m_FuturePlayArea;

    protected int i_Kills; //TODO: this needs to be done differently (most likely)
    protected bool b_MatchStarted;
    protected int i_SecondsRemaining;

    protected bool b_IsReady;

    protected ref ExpansionServerMarkerData m_ZoneCenterMapMarker;

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
    }

    void Init()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleClient::Init");

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLaterByName( this, "OnSecond", 1000, true );

		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
		br_rpc.Reset();

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "InitSpectate", this );

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SyncInvisibility", this );

		BattleRoyaleUtils.Trace("BattleRoyaleClient::Init - Done");
    }

	void InitSpectate(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		BattleRoyaleUtils.Trace("InitSpectate");
		Param1<Object> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ INIT_SPECTATE RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			BattleRoyaleUtils.Trace("InitSpectate : " + data.param1);
			if ( data.param1 != null )
			{
				BattleRoyaleUtils.Trace("Target for spectate: " + data.param1.GetPosition());
				if (GetGame().GetPlayer() != null)
				{
					BattleRoyaleUtils.Trace("Deleting original player for spectate");

					// TODO: Remove the Shock effect from the player if needed in future.

					GetGame().ObjectDelete(GetGame().GetPlayer());
					GetGame().SelectPlayer(null, null);
				}
				else
				{
					BattleRoyaleUtils.Trace("Original player for spectate is null");
				}

				BattleRoyaleUtils.Trace("Starting spectate mode");
				BRVPPSpectateCam cam = BRVPPSpectateCam.Cast(GetGame().CreateObject( "BRVPPSpectateCam", data.param1.GetPosition(), true ));
				cam.SetTargetObj( PlayerBase.Cast(data.param1) );  // Set the target to follow
				cam.SetActive(true);
			}
			else
			{
				Error("InitSpectate - No target for spectate");
			}
		}
		else
		{
			Error("InitSpectate - Not Client");
		}
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

			// Update current play area
			if ( br_rpc.current_play_area_center != "0 0 0" && br_rpc.current_play_area_radius != 0.0 )
				m_CurrentPlayArea = new BattleRoyalePlayArea( br_rpc.current_play_area_center, br_rpc.current_play_area_radius );

			// Update future play area
			if ( br_previous_future_play_area_center != br_rpc.future_play_area_center || br_previous_future_play_area_radius != br_rpc.future_play_area_radius )
			{
				if ( br_rpc.future_play_area_center && br_rpc.future_play_area_radius )
				{
					m_FuturePlayArea = new BattleRoyalePlayArea( br_rpc.future_play_area_center, br_rpc.future_play_area_radius );

					UpdateZoneCenterMaker( br_rpc.future_play_area_center );

					if ( br_rpc.b_ArtillerySound )
					{
						ref EffectSound m_ArtySound = SEffectManager.PlaySound("Artillery_Distant_SoundSet", m_FuturePlayArea.GetCenter(), 0.1, 0.1);
						m_ArtySound.SetAutodestroy(true);
					}
				}
				br_previous_future_play_area_center = br_rpc.future_play_area_center;
				br_previous_future_play_area_radius = br_rpc.future_play_area_radius;
			}

			// Set top position
			if ( player )
			{
				player.position_top = br_rpc.top_position;
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

        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        ref Param1<bool> ready_state = new Param1<bool>( true );  //perhaps this can be made togglable?
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", ready_state, false , NULL, player);

    }

    void Unstuck()
    {
        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerUnstuck", NULL, true , NULL, player);

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

    void SyncInvisibility(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SYNC_INVISIBILITY RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			bool is_enabled = data.param1;
			BattleRoyaleUtils.Trace("SyncInvisibility: " + is_enabled);
			PlayerBase player;
			if (Class.CastTo(player, target))
			{
				const int FLAGS_VISIBLE = EntityFlags.VISIBLE|EntityFlags.SOLID|EntityFlags.ACTIVE;
				player.m_isInvisible = data.param1;
				if (is_enabled)
				{
					player.ClearFlags(FLAGS_VISIBLE, true);
					dBodySetInteractionLayer(player, PhxInteractionLayers.NOCOLLISION);
					player.Update();
					player.DisableSimulation(is_enabled);
				}
				else
				{
					player.SetFlags(FLAGS_VISIBLE, true);
					dBodySetInteractionLayer(player, PhxInteractionLayers.CHARACTER);
				}

				player.SetInvisible(is_enabled);
			}
		}
	}
}
#endif
