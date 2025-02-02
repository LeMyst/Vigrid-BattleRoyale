#ifndef SERVER
class BattleRoyaleClient: BattleRoyaleBase
{
    protected ref BattleRoyalePlayArea m_CurrentPlayArea;
    protected ref BattleRoyalePlayArea m_FuturePlayArea;
    //protected ref ExpansionMarkerData m_ZoneMarker;
    protected ref Timer m_Timer;

    protected int i_Kills; //TODO: this needs to be done differently (most likely)
    protected bool b_MatchStarted;
    protected int i_SecondsRemaining;

    protected bool b_IsReady;

#ifdef SPECTATOR
    protected ref map<string, ref BattleRoyaleSpectatorMapEntityData> m_SpectatorMapEntityData;
#endif

    protected ref ExpansionServerMarkerData m_ZoneCenterMapMarker;

    void BattleRoyaleClient()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleClient::BattleRoyaleClient");

        b_IsReady = false;
        b_MatchStarted = false;
        i_Kills = 0;
        i_SecondsRemaining = 0;
        m_Timer = new Timer;

#ifdef SPECTATOR
        m_SpectatorMapEntityData = new map<string, ref BattleRoyaleSpectatorMapEntityData>();
#endif

        Init();
    }

    void ~BattleRoyaleClient()
    {
        if ( m_Timer && m_Timer.IsRunning() )
        {
            m_Timer.Stop();
        }

        delete m_Timer;
    }

    void Init()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleClient::Init");

        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetFade", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetInput", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "StartMatch", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", this );
#ifdef SPECTATOR
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ActivateSpectatorCamera", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateEntityHealth", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateMapEntityData", this );
#endif
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "TakeZoneDamage", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetTopPosition", this );
        GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ShowWinScreen", this );

        //m_Timer.Run(1.0, this, "OnSecond", NULL, true); //Call every second
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLaterByName( this, "OnSecond", 1000, true );

		BattleRoyaleUtils.Trace("BattleRoyaleClient::Init - Done");
    }

#ifdef SPECTATOR
    ref map<string, ref BattleRoyaleSpectatorMapEntityData> GetSpectatorMapEntityData()
    {
        return m_SpectatorMapEntityData;
    }
#endif

    //--- note: these return NULL of there is no area referenced for next or current area
    BattleRoyalePlayArea GetPlayArea()
    {
        return m_CurrentPlayArea;
    }

    BattleRoyalePlayArea GetNextArea()
    {
        return m_FuturePlayArea;
    }

    override void Update(float delta)
    {
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
		float distExt;
		float distInt;
		float angle;
		bool isInsideZone;

		// Show distance when we have a white zone
		if( m_FuturePlayArea )
		{
            isInsideZone = GetZoneDistance( m_FuturePlayArea, distExt, distInt, angle );
            gameplay.UpdateZoneDistance( isInsideZone, distExt, distInt, angle );
		}
		// otherwise, show distance to the blue zone
		else if( m_CurrentPlayArea )
		{
			isInsideZone = GetZoneDistance( m_CurrentPlayArea, distExt, distInt, angle );
			gameplay.UpdateZoneDistance( isInsideZone, distExt, distInt, angle );
		// otherwise, hide distance
		} else {
			gameplay.HideDistance();
		}

		// If we have a blue zone, show visual effect when outside of zone
        if( m_CurrentPlayArea )
        {
            GetZoneDistance( m_CurrentPlayArea, distExt, distInt, angle );

            PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
            if (distExt > 0)
            {
                player.QueueAddGlassesEffect(PPERequesterBank.REQ_BATTLEROYALE);
            } else {
                player.QueueRemoveGlassesEffect(PPERequesterBank.REQ_BATTLEROYALE);
            }
        }

#ifdef BR_MINIMAP
        vector camera_pos = GetGame().GetCurrentCameraPosition();
        gameplay.UpdateMiniMap( camera_pos );
#endif
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

    void StartMatch(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        if ( type == CallType.Client )
        {
            OnMatchStarted();
        }
    }

    void AddPlayerKill(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        if ( type == CallType.Client )
        {
            AddPlayerKilled(1); //TODO: maybe we'll eventually store kills on the server & just send that across. Idk we'll figure it out
        }
    }

    void SetCountdownSeconds(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param1<int> data;
        if( !ctx.Read( data ) )
        {
            Error("FAILED TO READ SetCountdownSeconds RPC");
            return;
        }
        if ( type == CallType.Client )
        {
            i_SecondsRemaining =  data.param1;
        }
    }

    void SetPlayerCount(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param2<int, int> data;
        if( !ctx.Read( data ) )
        {
            Error("FAILED TO READ SETPLAYERCOUNT RPC");
            return;
        }
        if ( type == CallType.Client )
        {
            //BattleRoyaleUtils.Trace(string.Format("SetPlayerCount: %1 %2", data.param1, data.param2));
            PlayerCountChanged( data.param1, data.param2 );
        }
    }

    void SetInput(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        BattleRoyaleUtils.Trace("SetInput");
        Param1<bool> data;
        if( !ctx.Read( data ) )
        {
            Error("FAILED TO READ SETINPUT RPC");
            return;
        }
        if ( type == CallType.Client )
        {
            PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
            player.DisableInput( data.param1 );
        }
    }

    void SetFade(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param1<bool> data;
        if( !ctx.Read( data ) )
        {
            Error("FAILED TO READ SETFADE RPC");
            return;
        }
        if ( type == CallType.Client )
        {
            if( data.param1 )
                FadeIn();
            else
                FadeOut();
        }
    }

#ifdef SPECTATOR
    /*
    ADD: Send [Id, Name, Position, Direction]
    UPDATE: Send [Id, Name, Position, Direction]
    DELETE: Send [ID, "", (0,0,0), (0,0,0)]
    */
    void UpdateMapEntityData(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        if ( type == CallType.Client )
        {
            Param4<string, string, vector, vector> data;
            if( !ctx.Read( data ) )
            {
                Error("FAILED TO READ UpdateMapEntityData RPC");
                return;
            }

            if(m_SpectatorMapEntityData.Contains(data.param1))
            {
                if(data.param3 == Vector(0, 0, 0))
                {
                    m_SpectatorMapEntityData.Remove(data.param1);
                }
                else
                {
                    m_SpectatorMapEntityData[data.param1].name = data.param2;
                    m_SpectatorMapEntityData[data.param1].position = data.param3;
                    m_SpectatorMapEntityData[data.param1].direction = data.param4;
                }
            }
            else
            {
                m_SpectatorMapEntityData.Insert(data.param1, new BattleRoyaleSpectatorMapEntityData( data.param2, data.param3, data.param4 ));
            }

        }
    }

    void UpdateEntityHealth(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        PlayerBase pbTarget;
        if ( type == CallType.Client )
        {
            Param2<float, float> data;
            if( !ctx.Read( data ) )
            {
                Error("FAILED TO READ UpdateEntityHealth RPC");
                return;
            }

            if(Class.CastTo( pbTarget, target ))
            {
                pbTarget.UpdateHealthStats( data.param1, data.param2 );
            }
        }
        else
        {
            Error("This is deprecated functionality and shouldn't be called");
        }
    }

    void ActivateSpectatorCamera(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        BattleRoyaleUtils.Trace("Activating Spectator Camera");
        BattleRoyaleCamera br_Camera;
        if ( Class.CastTo( br_Camera, Camera.GetCurrentCamera() ) )
        {
            br_Camera.SetActive( true );

            if ( GetGame().GetPlayer() )
            {
                GetGame().GetPlayer().GetInputController().SetDisabled( true );
            }

            //Gameplay changes
            MissionGameplay mission = MissionGameplay.Cast( GetGame().GetMission() );
            if ( mission )
            {
                BattleRoyaleUtils.Trace("Initializing Spectator in Mission");
                //Enable spectator HUD elements
                mission.InitSpectator();
            }
        }
        else
        {
            Error("Failed to cast camera to BattleRoyaleCamera");
        }
    }
#endif

    void UpdateCurrentPlayArea(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param1<ref BattleRoyalePlayArea> data;
        if( !ctx.Read( data ) )
        {
            Error("FAILED TO READ UpdateCurrentPlayArea RPC");
            return;
        }

        if ( type == CallType.Client )
        {
            m_CurrentPlayArea = data.param1;
        }
    }

    void UpdateFuturePlayArea(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param2<ref BattleRoyalePlayArea, bool> data;
        if( !ctx.Read( data ) )
        {
            Error("FAILED TO READ UpdateFuturePlayArea RPC");
            return;
        }
        if ( type == CallType.Client )
        {
            m_FuturePlayArea = data.param1;
            bool b_ArtillerySound = data.param2;
            if (m_FuturePlayArea)
            {
                UpdateZoneCenterMaker( m_FuturePlayArea.GetCenter() );

                if(b_ArtillerySound)
                {
                    // Artillery_Distant_SoundSet
                    // Artillery_Fall_SoundSet
                    // Artillery_Close_SoundSet
                    ref EffectSound m_ArtySound = SEffectManager.PlaySound("Artillery_Distant_SoundSet", m_FuturePlayArea.GetCenter(), 0.1, 0.1);
                    m_ArtySound.SetAutodestroy(true);
                }
            }
        }
    }

    void TakeZoneDamage(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param1<bool> data;
        if( !ctx.Read( data ) )
        {
            Error("FAILED TO READ TakeZoneDamage RPC");
            return;
        }
        if ( type == CallType.Client )
        {
            if( data.param1 )
            {
                PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
                CachedObjectsParams.PARAM2_FLOAT_FLOAT.param1 = 32;
                CachedObjectsParams.PARAM2_FLOAT_FLOAT.param2 = 0.3;
                player.SpawnDamageDealtEffect2(CachedObjectsParams.PARAM2_FLOAT_FLOAT);
                float shake_strength = Math.InverseLerp(0, 500, Math.RandomIntInclusive(125, 250));
                player.GetCurrentCamera().SpawnCameraShake(shake_strength);
            }
        }
    }

	void SetTopPosition(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ SetTopPosition RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			DayZPlayerImplement player = DayZPlayerImplement.Cast( GetGame().GetPlayer() );
			player.position_top = data.param1;
		}
	}

	void ShowWinScreen(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Widget win_screen_hud = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/win_screen.layout");
		win_screen_hud.Show( true );
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
