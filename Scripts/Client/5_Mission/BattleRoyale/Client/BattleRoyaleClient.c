#ifndef SERVER
class BattleRoyaleClient: BattleRoyaleBase
{
    protected ref BattleRoyalePlayArea m_CurrentPlayArea;
    protected ref BattleRoyalePlayArea m_FuturePlayArea;

    protected int i_Kills; //TODO: this needs to be done differently (most likely)
    protected bool b_MatchStarted;
    protected int i_SecondsRemaining;

    protected bool b_IsReady;

    //--- One-shot: we have told the server we finished loading. See SendLoadedInOnce().
    protected bool b_SentLoadedIn;

#ifdef EXPANSION_MAP_ZONES
    protected ref ExpansionServerMarkerData m_ZoneCenterMapMarker;
#endif

    protected ref BattleRoyaleSpeakingList m_SpeakingList;

    //--- Set from the SetSpectateTarget / EndSpectate edge in Update(). Read through IsSpectating().
    protected bool b_Spectating;

    //--- Whether the out-of-zone tint is currently running via the requester directly, which is the
    //--- spectator path. Edge-tracked because PPERequesterBase.Stop() walks the whole request
    //--- structure and queues a manager pass - it must not be called every frame. The LIVING player
    //--- path does not use this: it goes through the vanilla glasses queue, which self-corrects each
    //--- tick and is shared with real glasses and NVG effects.
    protected bool b_TintActive;

    //--- Cheap signature of the set of post-process requesters currently running, so
    //--- SuppressCorpsePostProcess can notice a CHANGE without building a string every frame.
    protected int m_PPESignature;
    protected int m_PPENextLogMs;

    void BattleRoyaleClient()
    {
        BattleRoyaleUtils.Trace("BattleRoyaleClient::BattleRoyaleClient");

        b_IsReady = false;
        b_SentLoadedIn = false;
        b_MatchStarted = false;
        b_Spectating = false;
        b_TintActive = false;
        m_PPESignature = -1;
        m_PPENextLogMs = 0;
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

#ifdef DIAG_DEVELOPER
		//--- The diag bag is static and outlives a server change, exactly like br_rpc.
		BattleRoyaleDiag.Reset();
#endif

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

#ifdef DIAG_DEVELOPER
    //--- Diag menu bookkeeping. Counters are compared against BattleRoyaleDiag's monotonic request
    //--- counters; the zone values exist so the fake circles are rebuilt on an edge rather than
    //--- allocated fresh every frame.
    protected float br_diag_zone_radius = -1;
    protected float br_diag_zone_next_radius = -1;
    protected vector br_diag_zone_origin;
    protected int br_diag_req_spawn_menu = 0;
    protected int br_diag_req_leaderboard = 0;

    /**
     *  Replace the live play areas with synthetic circles pinned near the player.
     *
     *  Feeds the HUD distance arrow and, through the single VigridMapAPI.SetZones call below, the
     *  map's rings - so both can be looked at in an offline session with no match running.
     *
     *  The centre is captured once, on the frame the toggle goes on, and not updated afterwards. A
     *  circle that follows the player would read "distance 0" for ever and test nothing.
     */
    protected void BR_DiagApplyZones()
    {
        if ( !BattleRoyaleDiag.zones_fake )
        {
            //--- Marks "not currently faking", which is also what makes the next enable re-centre.
            br_diag_zone_radius = -1;
            return;
        }

        bool rebuild = false;
        if ( br_diag_zone_radius != BattleRoyaleDiag.zone_radius )
            rebuild = true;
        if ( br_diag_zone_next_radius != BattleRoyaleDiag.zone_next_radius )
            rebuild = true;

        if ( !rebuild )
            return;

        if ( br_diag_zone_radius < 0 )
        {
            PlayerBase local_player = PlayerBase.Cast( GetGame().GetPlayer() );
            if ( local_player )
                br_diag_zone_origin = local_player.GetPosition();
        }

        //--- Next circle offset inside the current one rather than concentric: two circles sharing a
        //--- centre hide every bug in how the pair is drawn and how the arrow picks between them.
        vector diag_next_center = br_diag_zone_origin;
        diag_next_center[0] = diag_next_center[0] + (BattleRoyaleDiag.zone_radius * 0.4);

        m_CurrentPlayArea = new BattleRoyalePlayArea( br_diag_zone_origin, BattleRoyaleDiag.zone_radius );
        m_FuturePlayArea = new BattleRoyalePlayArea( diag_next_center, BattleRoyaleDiag.zone_next_radius );

        br_diag_zone_radius = BattleRoyaleDiag.zone_radius;
        br_diag_zone_next_radius = BattleRoyaleDiag.zone_next_radius;
    }

    //! Drain the diag menu's one-shot requests. Counters, so a press is never lost or double-fired.
    protected void BR_DiagHandleRequests()
    {
        if ( br_diag_req_spawn_menu != BattleRoyaleDiag.req_open_spawn_menu )
        {
            br_diag_req_spawn_menu = BattleRoyaleDiag.req_open_spawn_menu;
#ifdef DIAG
            MissionGameplay diag_gameplay = MissionGameplay.Cast( GetGame().GetMission() );
            if ( diag_gameplay )
                diag_gameplay.BR_DiagOpenSpawnSelection();
#endif
        }

        if ( br_diag_req_leaderboard != BattleRoyaleDiag.req_open_leaderboard )
        {
            br_diag_req_leaderboard = BattleRoyaleDiag.req_open_leaderboard;

            //--- Parentless, on a cleared stack, for the same reason ShowSpawnSelection is.
            //--- Qualified: the bare GetUIManager() that MissionGameplay uses is a method on
            //--- Mission, and this class is not one.
            GetGame().GetUIManager().CloseAll();
            GetGame().GetUIManager().EnterScriptedMenu( MENU_BR_LEADERBOARD, NULL );
        }
    }
#endif
    /**
     *  True once the server has put this client into spectate.
     *
     *  ASK THIS, NOT GetGame().GetPlayer(). It was assumed for most of this feature's life that
     *  SelectSpectator nulls the local player - it does not. Measured live at 20:41:49 while
     *  spectating: "out-of-zone tint=1 (spectating=1, player=1)". GetPlayer() keeps handing back the
     *  CORPSE, which is a perfectly valid PlayerBase that simply is not being simulated or updated
     *  any more.
     *
     *  That makes "is there a player object" a silently wrong test for "is this an ordinary living
     *  client", and it cost the out-of-zone tint: the effect was queued onto the corpse, whose queue
     *  nothing drains. Every site that cares about the difference - GetReferencePosition,
     *  GetSubjectPosition, ApplyOutOfZoneTint - tests this instead.
     */
    bool IsSpectating()
    {
        return b_Spectating;
    }

    /**
     *  The position anything on-screen should be measured FROM.
     *
     *  While spectating that is the camera, not the corpse - a spectator following someone across the
     *  map would otherwise be told the zone distance from wherever their body fell, forever. The
     *  camera fallback also covers the ordinary NULL-player windows (load, teardown, AbortMission),
     *  which is why this is safe to route every caller through unconditionally.
     */
    vector GetReferencePosition()
    {
        PlayerBase local_player = PlayerBase.Cast( GetGame().GetPlayer() );

        if( IsSpectating() )
            return GetGame().GetCurrentCameraPosition();

        if( local_player )
            return local_player.GetPosition();

        return GetGame().GetCurrentCameraPosition();
    }

    /**
     *  Turn the out-of-zone red tint on or off.
     *
     *  Two paths:
     *
     *  ALIVE - the vanilla glasses queue, unchanged and called every frame. That queue is shared
     *  with real glasses and NVG effects and drains itself each tick, so it self-corrects; edge
     *  tracking it would fight those other effects.
     *
     *  SPECTATING - the requester directly. This is exactly what the queue does when it drains
     *  (P:/scripts/4_world/entities/manbase/playerbase.c:9594-9601 calls
     *  PPERequesterBank.GetRequester(id).Start() / .Stop()), just without needing the player.
     *  Edge-tracked: Stop() walks the whole request structure and queues a manager pass, so it must
     *  not run per frame.
     *
     *  THE CHOICE IS MADE ON IsSpectating(), NOT on whether a PlayerBase happens to be reachable.
     *  That distinction is the whole bug this shape fixes. GetGame().GetPlayer() is supposed to go
     *  NULL once SelectSpectator runs, but the tint silently never appeared while spectating even
     *  though the evidence says the verdict was right: at 20:23:33 the server had pushed a current
     *  play area of radius 70 centred <10979.5, 13197.1> and the watched player was sitting at
     *  <10943, 12957.1>, 243 m out - 173 m past the boundary. Every input was correct and nothing
     *  rendered.
     *
     *  The one thing that explains that exactly is this method taking the queue branch with the
     *  CORPSE as local_player. Nothing drains a corpse's m_ProcessAddGlassesEffects - the drain runs
     *  from the live player's update - so the request would sit in an array forever, with no error
     *  anywhere and no way to tell from a log. Keying on the mode instead makes it not matter
     *  whether the engine hands a dead player back or NULL: while spectating, the corpse's queue is
     *  the wrong place either way.
     */
    protected void ApplyOutOfZoneTint(bool active, PlayerBase local_player)
    {
        if( !IsSpectating() && local_player )
        {
            if( active )
                local_player.QueueAddGlassesEffect( PPERequesterBank.REQ_BATTLEROYALE );
            else
                local_player.QueueRemoveGlassesEffect( PPERequesterBank.REQ_BATTLEROYALE );

            return;
        }

        if( active == b_TintActive )
            return;

        b_TintActive = active;

        //--- Edge-triggered, so it is cheap, and it is the one line that separates the two ways this
        //--- can fail: no line at all means the VERDICT never flipped (subject, play area or
        //--- GetSubjectPosition), a line with active=1 and a screen that stays clean means the
        //--- REQUESTER ran and did not render. Those look identical from the player's chair.
        BattleRoyaleUtils.Trace(string.Format("[Spectate] out-of-zone tint=%1 (spectating=%2, player=%3)", active, IsSpectating(), local_player != NULL));

        if( active )
            PPERequesterBank.GetRequester( PPERequester_BattleRoyale ).Start();
        else
            PPERequesterBank.GetRequester( PPERequester_BattleRoyale ).Stop();
    }

    /**
     *  The entity whose situation the screen is describing - the player being watched.
     *
     *  NULL when there is nobody: not spectating, or spectating in ORBIT mode, where the camera
     *  circles the final circle with no target at all.
     */
    protected Object ResolveSpectateSubject(BattleRoyaleRPC br_rpc)
    {
        if( !br_rpc )
            return NULL;
        if( !IsSpectating() )
            return NULL;
        if( br_rpc.spectate_mode != BR_SPECTATE_MODE_FOLLOW )
            return NULL;

        Object subject = br_rpc.spectate_target_obj;

        //--- Same proximity fallback the camera uses when CF could not marshal the entity - shared
        //--- deliberately, so the camera and the zone logic can never disagree about who is being
        //--- watched.
        if( !subject )
            subject = FindLocalPlayerNear( br_rpc.spectate_target_pos );

        return subject;
    }

    /**
     *  The position the ZONE should be evaluated against, and whether there is anyone to evaluate.
     *
     *  Deliberately a different answer from GetReferencePosition(), which returns the CAMERA while
     *  spectating. The camera is right for anything about the view; the subject is right for
     *  anything about the zone. A camera trailing 3.5 m behind its target - or pulled somewhere else
     *  entirely by the collision trace - can sit on the far side of the boundary from the player it
     *  is watching, and it is the player who matters.
     *
     *  Returns false when there is no subject: ORBIT mode, or no local player. Callers treat that as
     *  "nothing is out of the zone" rather than substituting the camera - the orbit sits 120 m out
     *  from the small final circle, so keying the tint to it would mean a permanently red screen.
     */
    protected bool GetSubjectPosition(out vector position)
    {
        position = vector.Zero;

        if( IsSpectating() )
        {
            BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
            if( !br_rpc )
                return false;
            if( br_rpc.spectate_mode != BR_SPECTATE_MODE_FOLLOW )
                return false;

            Object subject = ResolveSpectateSubject( br_rpc );
            if( subject )
            {
                position = subject.GetPosition();
                return true;
            }

            //--- No entity latched - the target is outside our network bubble. The server-pushed
            //--- position is only refreshed at 1 Hz, so the verdict can lag by up to a second here;
            //--- it corrects itself the moment the entity streams back in.
            if( br_rpc.spectate_target_uid == "" )
                return false;

            position = br_rpc.spectate_target_pos;
            return true;
        }

        PlayerBase local_player = PlayerBase.Cast( GetGame().GetPlayer() );
        if( !local_player )
            return false;

        position = local_player.GetPosition();
        return true;
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

    /**
     *  Announce, exactly once, that this client has finished loading and is controlling its
     *  character.
     *
     *  The server cannot work this out for itself. Vanilla calls MissionServer.InvokeOnConnect from
     *  the ClientNew path, which runs the moment the character is created - while the client is
     *  still loading the world - and for a brand new character ClientReadyEventTypeID never fires at
     *  all (measured 2026-08-10: ClientPrepare to ClientNew alone took 20 s, and no ready event was
     *  logged in an entire eight minute run). So a grace period measured from the connect event is
     *  mostly loading screen.
     *
     *  IsPlayerSelected() is the condition rather than a mere non-null player: the entity exists
     *  client-side before OnSelectPlayer runs, and it is that call which marks the player as
     *  controlling their character.
     */
    protected void SendLoadedInOnce(PlayerBase player)
    {
        if(b_SentLoadedIn)
            return;

        if(!player || !player.IsPlayerSelected())
            return;

        b_SentLoadedIn = true;
        BattleRoyaleUtils.Trace("BattleRoyaleClient::SendLoadedInOnce - reporting loaded in");
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerLoadedIn", new Param1<bool>( true ), true );
    }

    override void Update(float delta)
    {
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );

        //--- Tell the server we are actually in the world. Only the late-join kick uses this, but it
        //--- is sent unconditionally because the client has no idea whether it joined late.
        SendLoadedInOnce( player );

		float distExt;
		float distInt;
		float angle;
		bool isInsideZone;

#ifdef DIAG_DEVELOPER
		// Diag menu, drained first so a forced value cannot be overwritten later in the same frame.
		BR_DiagApplyZones();
		BR_DiagHandleRequests();
#endif
		//--- Everything zone-related describes the SUBJECT: the local player normally, the player
		//--- being watched while spectating. Falls back to the reference position (the camera) when
		//--- there is no subject, so orbit mode still reports something sensible instead of nothing.
		vector subject_pos;
		bool has_subject = GetSubjectPosition( subject_pos );
		if( !has_subject )
			subject_pos = GetReferencePosition();

		// First check if player is outside current play area
		if (m_CurrentPlayArea)
		{
			// Check if player is inside current play area and get distExt, distInt and angle
			isInsideZone = GetZoneDistanceFrom(m_CurrentPlayArea, subject_pos, distExt, distInt, angle);

			// If outside current play area, show distance to it
			if (!isInsideZone)
			{
				gameplay.UpdateZoneDistance(isInsideZone, distExt, distInt, angle);
			}
			// Player is inside current play area, check if future play area exists
			else if (m_FuturePlayArea)
			{
				isInsideZone = GetZoneDistanceFrom(m_FuturePlayArea, subject_pos, distExt, distInt, angle);
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

		// If we have a blue zone, show visual effect when outside of zone.
		//--- Keyed to the SUBJECT, not the camera. While spectating, the tint says "the player you
		//--- are watching is outside the circle" - a camera that swings across the boundary behind a
		//--- target who is safely inside must not tint, and a target who runs out must, even though
		//--- the camera trails them.
		bool want_tint = false;
        if( m_CurrentPlayArea && has_subject )
        {
            GetZoneDistanceFrom( m_CurrentPlayArea, subject_pos, distExt, distInt, angle );
            want_tint = distExt > 0;
        }

        ApplyOutOfZoneTint( want_tint, player );

#ifdef BR_MINIMAP
        vector camera_pos = GetGame().GetCurrentCameraPosition();
        gameplay.UpdateMiniMap( camera_pos );
#endif
		BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();

		if( br_rpc )
		{
			//--- Everything the HUD shows, taken from the wire unless the diag menu is forcing it.
			//--- Resolved once, up front, so there is exactly one place the two sources meet.
			int hud_players = br_rpc.nb_players;
			int hud_groups = br_rpc.nb_groups;
			int hud_kills = br_rpc.player_kills;
			int hud_countdown = br_rpc.countdown_seconds;

#ifdef DIAG_DEVELOPER
			if ( BattleRoyaleDiag.hud_force )
			{
				hud_players = BattleRoyaleDiag.hud_players;
				hud_groups = BattleRoyaleDiag.hud_groups;
				hud_kills = BattleRoyaleDiag.hud_kills;
				hud_countdown = BattleRoyaleDiag.hud_countdown;
			}
#endif

			// Update player and group remaining count
			PlayerCountChanged( hud_players, hud_groups );

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
			if( br_previous_input_state != br_rpc.input_state && player )
			{
				player.DisableInput( br_rpc.input_state );
				br_previous_input_state = br_rpc.input_state;
			}

			// Update player kill count
			gameplay.UpdateKillCount( hud_kills );

			// Update match start state
			if( br_rpc.match_started && !b_MatchStarted )
			{
				OnMatchStarted();
			}

			// Update countdown timer and zone distance
			bool countdown_changed = ( br_previous_countdown != hud_countdown );
#ifdef DIAG_DEVELOPER
			//--- Re-assert every frame while forcing, or OnSecond ticks the forced value down to
			//--- zero and hides the widget a minute after it was set.
			if ( BattleRoyaleDiag.hud_force )
				countdown_changed = true;
#endif

			if ( countdown_changed )
			{
				i_SecondsRemaining = hud_countdown;
				gameplay.UpdateCountdownTimer( i_SecondsRemaining );
				br_previous_countdown = hud_countdown;
			}

			//--- False while the diag menu owns the circles, so the wire cannot overwrite them.
			bool zones_from_server = true;
#ifdef DIAG_DEVELOPER
			if ( BattleRoyaleDiag.zones_fake )
				zones_from_server = false;
#endif

			// Update current play area. Diffed like the future area below, and for the same
			// reason: without the diff this allocated a fresh BattleRoyalePlayArea every frame
			// for the entire match.
			if ( zones_from_server && ( br_previous_current_play_area_center != br_rpc.current_play_area_center || br_previous_current_play_area_radius != br_rpc.current_play_area_radius ) )
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
			if ( zones_from_server && ( br_previous_future_play_area_center != br_rpc.future_play_area_center || br_previous_future_play_area_radius != br_rpc.future_play_area_radius ) )
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

			// Spectating
			UpdateSpectate( br_rpc );

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
        //--- GetExpansionSettings().GetMap() is declared by @DayZ-Expansion-Navigation, which is
        //--- optional - and unlike a missing TYPE, a missing METHOD is invisible to any grep for
        //--- Expansion class names, so this one only surfaced as the SECOND compile failure after
        //--- Navigation was dropped. Without the map there is nowhere for a server marker to show,
        //--- so the whole body is skipped rather than just the AddServerMarker call.
#ifdef EXPANSIONMODNAVIGATION
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
#endif
    }
#endif

    /**
     *  Is `from` inside `play_area`, and how far out / in / at what bearing?
     *
     *  The origin is always passed in explicitly. It used to be taken from the local player, then
     *  from GetReferencePosition() - but neither is right for a spectator, whose zone readouts have
     *  to describe the player being WATCHED rather than the corpse or the camera. See
     *  GetSubjectPosition().
     */
    protected bool GetZoneDistanceFrom(BattleRoyalePlayArea play_area, vector from, out float distExt, out float distInt, out float angle)
    {
        vector center = play_area.GetCenter();
        vector playerpos = from;

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

    /**
     *  Drive the spectator camera from the fields BattleRoyaleRPC latched.
     *
     *  Called every frame. Everything here tolerates being run repeatedly with unchanged values,
     *  which is what makes the server's 1 Hz keepalive safe.
     */
    protected void UpdateSpectate(BattleRoyaleRPC br_rpc)
    {
        if( !br_rpc )
            return;

        //--- Drive the death screen from here. UIScriptedMenu.Update() is engine-driven and fires
        //--- only ONCE for that menu, so it cannot run its own countdown or hold its own cursor.
        //--- This method, by contrast, is called every frame from MissionGameplay.OnUpdate, outside
        //--- vanilla's m_LifeState == ALIVE gate - which is the whole reason it still ticks for a
        //--- dead player.
        //--- Not FindMenu: it walks GetMenu()/GetParentMenu() and so goes blind the moment anything
        //--- nulls the engine's current-menu pointer, which is exactly the failure this whole screen
        //--- was suffering from. The static reference cannot go stale that way.
        DeathScreenMenu death_menu = DeathScreenMenu.GetInstance();
        if( death_menu )
            death_menu.Tick();

        //--- Enter once, on the rising edge. b_SpectateEntered is what guarantees the input-focus
        //--- release below happens EXACTLY once: ChangeGameFocus is an additive counter, so a second
        //--- release would leave the game permanently unfocused.
        if( br_rpc.spectate_active && !b_Spectating )
        {
            b_Spectating = true;
            EnterSpectate();
        }

        if( !b_Spectating )
            return;

        //--- Every frame, not once at entry - see the method.
        SuppressCorpsePostProcess();

        //--- The match ended (EndSpectate). The camera deliberately stays where it is, but the
        //--- server has stopped pushing, so the last target position is frozen - clear the tint
        //--- rather than leaving the screen stuck red on a stale verdict.
        if( !br_rpc.spectate_active )
            ApplyOutOfZoneTint( false, NULL );

        //--- Before the camera, because it does not depend on one existing.
        UpdatePartyViewpoint( br_rpc );

        BattleRoyaleSpectatorCamera camera = BattleRoyaleSpectatorCamera.GetInstance();
        if( !camera )
            return;

        //--- Shared with the zone logic, so the camera and the tint always agree on the subject.
        Object target = ResolveSpectateSubject( br_rpc );

        camera.SetTarget( target, br_rpc.spectate_target_uid, br_rpc.spectate_target_pos, br_rpc.spectate_mode );
    }

    /**
     *  Hold down every gameplay post-process effect for as long as this client is spectating.
     *
     *  EnterSpectate()'s StopAllEffects sweep is a ONE-SHOT, and the body goes on re-Start()ing
     *  effects after it - PPERequester_UnconEffects from OnUnconsciousUpdate (playerbase.c:3652) and
     *  PPERequester_DeathDarkening (playerbase.c:8577) are both per-frame re-asserts. So whether the
     *  sweep sticks was a race against how fast the player pressed Spectate: entries at 6 s, 9 s and
     *  20 s after death came up clean, an entry at 3 s did not.
     *
     *  THIS IS DELIBERATELY GENERIC, over the whole requester bank, rather than naming the classes
     *  it expects. Naming them was tried and was wrong: the halo survived a build that held down
     *  UnconEffects and DeathDarkening specifically, and the log proved neither was ever running -
     *  so it is some third effect. Guessing which one twice was enough. Iterating the bank cannot
     *  be wrong about the set, and the trace below reports what was actually found.
     *
     *  Two exclusions, both deliberate:
     *    - PPERequester_BattleRoyale is the out-of-zone tint, which this class is actively driving.
     *      Stopping it here would fight ApplyOutOfZoneTint and desync b_TintActive.
     *    - anything outside GAMEPLAY_EFFECTS, because MENU_EFFECTS owns the escape-menu blur and the
     *      spawn-selection menu, and a spectator can still legitimately open those.
     *
     *  Cost is a bank walk per frame - IsRequesterRunning() is a plain bool read
     *  (pperequestplatformsbase.c:53) and the ~40 ids are contiguous from 0 to m_lastID
     *  (pperequesterbank.c:184). The string is built only when the running SET changes, which is why
     *  the signature exists: a per-frame concat would allocate for nothing.
     */
    protected void SuppressCorpsePostProcess()
    {
        int signature = 0;
        int i = 0;
        PPERequesterBase requester;

        //--- PASS 1: observe only. Nothing is stopped here.
        //---
        //--- The first version of this folded the scan and the Stop into one loop and then rebuilt
        //--- the name list afterwards - which reported the state AFTER suppression, so every single
        //--- line came out "<none>". It faithfully logged what it had just finished destroying. The
        //--- names have to be read before anything is stopped, hence two passes.
        for( i = 0; i <= PPERequesterBank.m_lastID; i++ )
        {
            requester = PPERequesterBank.GetRequester( i );
            if( !requester )
                continue;
            if( !requester.IsRequesterRunning() )
                continue;

            signature = signature * 31 + i;
        }

        //--- Only speak when the running SET changes, and never more than once every
        //--- BR_SPECTATE_PPE_LOG_MS. Without the throttle a corpse re-asserting an effect every
        //--- frame makes this oscillate stopped/running and log on every single flip - which it did,
        //--- fifteen times in one second.
        if( signature != m_PPESignature && GetGame().GetTime() >= m_PPENextLogMs )
        {
            m_PPESignature = signature;
            m_PPENextLogMs = GetGame().GetTime() + BR_SPECTATE_PPE_LOG_MS;
            LogRunningPostProcess();
        }

        //--- PASS 2: suppress.
        for( i = 0; i <= PPERequesterBank.m_lastID; i++ )
        {
            requester = PPERequesterBank.GetRequester( i );
            if( !requester )
                continue;
            if( !requester.IsRequesterRunning() )
                continue;

            //--- Never touch the tint: this class drives it, and b_TintActive would go out of sync.
            if( requester.Type() == PPERequester_BattleRoyale )
                continue;
            //--- MENU_EFFECTS owns the escape-menu blur and the spawn-selection menu, both of which
            //--- a spectator can still legitimately open.
            if( (requester.GetCategoryMask() & PPERequesterCategory.GAMEPLAY_EFFECTS) == 0 )
                continue;

            requester.Stop();
        }
    }

    //! Name every running requester. Called from pass 1, BEFORE anything has been stopped.
    protected void LogRunningPostProcess()
    {
        string running = "";

        for( int i = 0; i <= PPERequesterBank.m_lastID; i++ )
        {
            PPERequesterBase requester = PPERequesterBank.GetRequester( i );
            if( !requester )
                continue;
            if( !requester.IsRequesterRunning() )
                continue;

            running = running + requester.Type().ToString() + " ";
        }

        if( running == "" )
            running = "<none>";

        BattleRoyaleUtils.Trace("[Spectate] running post-process: " + running);
    }

    /**
     *  Tell the party addon to measure its distances from the player being watched.
     *
     *  The party HUD and nametags measure from GetGame().GetPlayer(), which while spectating is the
     *  CORPSE - so every teammate distance was reported from wherever this player fell, frozen for
     *  the rest of the match. On the nametags that is worse than a wrong number: the same origin
     *  drives nametag_max_distance culling and the alpha fade, so a teammate standing next to the
     *  person being watched could be faded out or culled outright.
     *
     *  Goes through VigridPartyClientAPI and nothing else. Party must not reference a BattleRoyale*
     *  symbol - that rule is what keeps a later extraction into a standalone @Vigrid-Party mod a
     *  build-plumbing job - so the addon is told a position and a uid, and knows nothing about
     *  spectating, matches or why the origin moved.
     *
     *  The uid may name somebody who is not in the party at all: the chain follows killers, who are
     *  usually enemies. That is handled on the far side - no roster row matches, so no row has its
     *  distance suppressed, which is exactly right.
     */
    protected void UpdatePartyViewpoint(BattleRoyaleRPC br_rpc)
    {
#ifdef VIGRID_PARTY
        vector subject_pos;

        //--- ORBIT mode has no subject, and GetSubjectPosition says so - fall back to the local
        //--- player rather than pinning every distance to the centre of the final circle.
        if( IsSpectating() && GetSubjectPosition( subject_pos ) )
            VigridPartyClientAPI.SetHudViewpoint( subject_pos, br_rpc.spectate_target_uid );
        else
            VigridPartyClientAPI.ClearHudViewpoint();
#endif
    }

    //! Nearest local player entity within BR_SPECTATE_LATCH_RADIUS of `position`, or NULL.
    protected Object FindLocalPlayerNear(vector position)
    {
        array<PlayerBase> local_players;
        //--- Already exists for exactly this kind of lookup - see the s_LocalPlayers block in
        //--- Scripts/Client/4_World/Entities/ManBase/PlayerBase.c.
        PlayerBase.GetLocalPlayers( local_players );
        if( !local_players )
            return NULL;

        PlayerBase best = NULL;
        float best_distance = BR_SPECTATE_LATCH_RADIUS;

        for( int i = 0; i < local_players.Count(); i++ )
        {
            PlayerBase candidate = local_players.Get(i);
            if( !candidate )
                continue;

            float distance = vector.Distance( candidate.GetPosition(), position );
            if( distance > best_distance )
                continue;

            best_distance = distance;
            best = candidate;
        }

        return best;
    }

    /**
     *  Undo what SimulateDeath(true) did, so the spectator can actually see and hear the match.
     *
     *  Vanilla only ever reverses these in MissionGameplay.OnPlayerRespawned, which never runs for
     *  someone who is staying dead. Runs exactly once per match.
     */
    protected void EnterSpectate()
    {
        BattleRoyaleUtils.Info("[Spectate] entry undo: fade / audio / input / hud");

        //--- 1. Close the death screen, and clear the engine fade in case the menu failed to open and
        //--- ShowDeadScreen fell back to it. ScreenFadeOut is harmless when nothing is faded.
        if( GetGame().GetUIManager() )
        {
            GetGame().GetUIManager().ScreenFadeOut( BR_SPECTATE_FADE_SECONDS );

            //--- Not FindMenu: it walks GetMenu()/GetParentMenu() and so goes blind the moment anything
            //--- nulls the engine's current-menu pointer, which is exactly the failure this whole
            //--- screen was suffering from. The static reference cannot go stale that way.
            DeathScreenMenu death_menu = DeathScreenMenu.GetInstance();
            if( death_menu )
                death_menu.Close();
        }

        //--- 2. Restore the five sound buses death zeroed. Verbatim from
        //--- MissionGameplay.OnPlayerRespawned (missiongameplay.c:1626-1630) - the only place vanilla
        //--- puts them back. Without this the spectator is completely deaf.
        //--- The m_volume_* fields live on DayZGame, not CGame, so this has to go through g_Game.
        if( g_Game && g_Game.GetSoundScene() )
        {
            g_Game.GetSoundScene().SetSoundVolume( g_Game.m_volume_sound, 1 );
            g_Game.GetSoundScene().SetSpeechExVolume( g_Game.m_volume_speechEX, 1 );
            g_Game.GetSoundScene().SetMusicVolume( g_Game.m_volume_music, 1 );
            g_Game.GetSoundScene().SetVOIPVolume( g_Game.m_volume_VOIP, 1 );
            g_Game.GetSoundScene().SetRadioVolume( g_Game.m_volume_radio, 1 );
        }

        //--- 3. Hand input back to the game. This mirrors LockControls(false)'s else branch
        //--- (dayzplayerimplement.c:874). SimulateDeath called LockControls(true) exactly once, so
        //--- exactly one release balances it.
        if( GetGame().GetInput() )
        {
            GetGame().GetInput().ChangeGameFocus( -1, INPUT_DEVICE_MOUSE );
            GetGame().GetInput().ChangeGameFocus( -1, INPUT_DEVICE_KEYBOARD );
            GetGame().GetInput().ChangeGameFocus( -1, INPUT_DEVICE_GAMEPAD );
        }

        if( GetGame().GetUIManager() )
            GetGame().GetUIManager().ShowUICursor( false );

        //--- 4. Hide the vanilla HUD. Vanilla only ever shows it while ALIVE, so nothing hides it on
        //--- death and a spectator would watch the whole match behind a corpse's zeroed vitals.
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        if( gameplay )
            gameplay.SetVanillaHudVisible( false );

        //--- 5. Clear leftover post-process. All of this is manager-side, so none of it needs a
        //--- PlayerBase - which matters, because SelectSpectator has already nulled
        //--- GetGame().GetPlayer() by the time we get here.
        PPEffects.ResetDOFOverride();

        //--- THE BLACK HALO. Dying while unconscious leaves PPERequester_UnconEffects running
        //--- forever: OnUnconsciousUpdate re-Starts it every frame with a vignette value, and
        //--- PlayerBase.CommandHandler only ever reaches OnUnconsciousStop() through an IsAlive()
        //--- branch - which a dead player never takes. Nothing in SimulateDeath or ShowDeadScreen
        //--- clears it either, so the spectator watches the whole match through a black ring.
        //---
        //--- GAMEPLAY_EFFECTS rather than ALL, deliberately: MENU_EFFECTS owns the escape-menu blur
        //--- and the spawn-selection menu, and a spectator can still open those. This one call also
        //--- subsumes the two Stop()s that used to be here, plus death darkening, tunnel vision,
        //--- blood loss, pain blur and every glasses/NVG requester - none of which a spectator wants.
        //---
        //--- NOT PPEffects.ResetVignettes(): that whole class is marked deprecated in favour of
        //--- PPEManager, and it writes the glow material directly, which races the requester layer.
        if( PPEManagerStatic.GetPPEManager() )
            PPEManagerStatic.GetPPEManager().StopAllEffects( PPERequesterCategory.GAMEPLAY_EFFECTS );

        //--- That sweep includes PPERequester_BattleRoyale, so the tint is definitely off now.
        //--- Say so, or the edge tracker would believe a stale "on" and never re-Start it.
        b_TintActive = false;

        //--- ...and the sweep above is a one-shot, which the body can undo on any later frame.
        //--- SuppressCorpsePostProcess takes over from here. Force the signature to a value the scan
        //--- cannot produce, so its first pass always reports what is running - that first line is
        //--- the whole diagnostic for anything the sweep failed to clear.
        m_PPESignature = -1;
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

        //--- The post-process is player-free and always runs; the two inventory/audio calls need a
        //--- character. A spectator has none, and EnterSpectate() restores the audio buses and stops
        //--- this requester by class anyway, so an unpaired FadeIn is not left hanging.
        PPERequesterBank.GetRequester(PPERequester_BurlapSackEffects).Start();

        if( player )
        {
            player.SetInventorySoftLock(true);
            player.SetMasterAttenuation("BurlapSackAttenuation");
        }
    }

    protected void FadeOut()
    {
        PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        BattleRoyaleUtils.Trace("BattleRoyale: FADE OUT!");

        PPERequesterBank.GetRequester(PPERequester_BurlapSackEffects).Stop();

        if( player )
        {
            player.SetInventorySoftLock(false);
            player.SetMasterAttenuation("");
        }
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
