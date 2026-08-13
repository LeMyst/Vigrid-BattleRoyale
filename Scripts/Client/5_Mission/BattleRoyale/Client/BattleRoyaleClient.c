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

    //--- The admin spectator's floating player tags. Built lazily on first use, so an ordinary
    //--- player never creates the widget tree at all.
    protected ref BattleRoyaleSpectatorTags m_SpectatorTags;

    //--- Names over the heads of non-teammates while the lobby is running. Also built lazily, and
    //--- kept afterwards rather than torn down: it costs one hidden panel for the rest of the match.
    protected ref BattleRoyaleLobbyTags m_LobbyTags;

    //--- Set from the SetSpectateTarget / EndSpectate edge in Update(). Read through IsSpectating().
    protected bool b_Spectating;

    //--- Mirrored from BattleRoyaleRPC.is_admin, which the server pushes once on connect.
    //--- PRESENTATION ONLY. It decides whether the admin keys put a packet on the wire and whether
    //--- the death screen offers its third button; it is never an authorization decision, because
    //--- every admin RPC is re-checked against admins_steamid64 server-side.
    protected bool b_IsAdmin;

    //--- Did the DEATH path lock input before this client started spectating?
    //---
    //--- THE FOCUS COUNTER IS ADDITIVE AND THIS IS WHAT KEEPS IT BALANCED. EnterSpectate ends with
    //--- three ChangeGameFocus(-1) calls that exist to undo the LockControls(true) SimulateDeath
    //--- performed. An ADMIN entering spectate alive never ran SimulateDeath, so those three
    //--- releases would drive the counter NEGATIVE - and a negative focus counter breaks input with
    //--- no error, no log line and nothing on screen to say what happened.
    protected bool b_DeathLocked;

    //--- Is the skeleton overlay on at this admin's request? OURS, not a mirror of COT's own flag -
    //--- the drawing runs from UpdateSkeletonOverlay rather than from JMESPModule.OnUpdate.
    protected bool b_SkeletonOverlay;
    protected int m_NextSkeletonDiagMs;

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
        b_IsAdmin = false;
        b_DeathLocked = false;
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

        // The GUI call queue lives for the whole process, so without this OnSecond keeps firing
        // once a second against a mission that no longer exists, for the rest of the session.
        // GetGame() is checked because a destructor can run on the way out - BattleRoyaleUtils
        // guards its own client-side Chat() call for the same reason.
        if ( GetGame() )
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).RemoveByName( this, "OnSecond" );

#ifdef VIGRID_MAP
        // The map addon's zone state is static and outlives this object, so without this the
        // previous match's circles would still be drawn after a server change.
        VigridMapAPI.ClearZones();
        VigridMapAPI.ClearHotZones();
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
    protected int br_diag_req_death_screen = 0;

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

        if ( br_diag_req_death_screen != BattleRoyaleDiag.req_open_death_screen )
        {
            br_diag_req_death_screen = BattleRoyaleDiag.req_open_death_screen;

            GetGame().GetUIManager().CloseAll();
            GetGame().GetUIManager().EnterScriptedMenu( MENU_BR_DEAD, NULL );
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

		// Nothing below this line is useful without the mission, and MissionGameplay.OnUpdate calls
		// us unconditionally - including on the teardown frames where the cast comes back NULL.
		if ( !gameplay )
			return;

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

			// Fade in/out effect. Both halves touch the player, so the edge is held rather than
			// consumed while there isn't one - advancing br_previous_* on a playerless frame would
			// swallow the transition for the rest of the match. Same for the input edge below.
			if( br_previous_fade_state != br_rpc.fade_state && player )
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
				// Same clear payload as the current area above, and the same reason to test it
				// explicitly - the old test used the vector as a boolean and had no else, so the
				// next-circle indicator outlived the match it belonged to.
				if ( br_rpc.future_play_area_center != "0 0 0" && br_rpc.future_play_area_radius != 0.0 )
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
				else
				{
					m_FuturePlayArea = NULL;
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

			// Same push, same deal: the addon diffs internally, so calling this every frame costs a
			// count comparison on a normal frame and a walk only when the pair actually changed.
			VigridMapAPI.SetHotZones( br_rpc.hot_zone_centers, br_rpc.hot_zone_radii );
#endif

			// Set top position
			if ( player )
			{
				player.position_top = br_rpc.top_position;
			}

			// Spectating
			UpdateSpectate( br_rpc );

			// Names over the heads of everyone in the lobby
			UpdateLobbyTags( br_rpc );

			// Bone skeletons for the admin spectator. Returns on one bool when off.
			UpdateSkeletonOverlay();

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

        //--- Mirrored every frame rather than latched on an edge: SetAdminFlag arrives once, and this
        //--- is a plain copy of it, so there is no edge to miss and nothing to get out of step.
        b_IsAdmin = br_rpc.is_admin;

        //--- Latched, NOT mirrored, and the difference matters. ShowDeadScreen sets death_locked and
        //--- nothing ever clears it, so a plain copy would re-arm the flag after EnterSpectate had
        //--- already consumed it - and an admin who died, spectated, respawned and spectated again
        //--- would release a focus lock that no longer exists. Rising edge only.
        if( br_rpc.death_locked )
        {
            b_DeathLocked = true;
            br_rpc.death_locked = false;
        }

        //--- Enter once, on the rising edge. b_Spectating is what guarantees the input-focus release
        //--- inside EnterSpectate happens EXACTLY once: ChangeGameFocus is an additive counter, so a
        //--- second release would leave the game permanently unfocused.
        if( br_rpc.spectate_active && !b_Spectating )
        {
            b_Spectating = true;
            EnterSpectate();
        }

        //--- Falling edge, and the test for it is "did the engine hand us back a LIVING player".
        //---
        //--- Both cases clear spectate_active, so the flag alone cannot tell them apart:
        //---   match ended  - an ordinary spectator keeps the camera on purpose (see EndSpectate in
        //---                  BattleRoyaleRPC), and GetPlayer() still hands back their CORPSE, which
        //---                  is not alive - so this branch correctly does not fire.
        //---   admin left   - the server ran SelectPlayer(identity, body) first, so GetPlayer() is a
        //---                  live character and everything spectate changed has to be put back.
        //---
        //--- Using IsAlive rather than "is there a player object" is the whole point: GetPlayer()
        //--- does NOT go NULL while spectating, which is the trap documented on IsSpectating().
        PlayerBase returned_body = PlayerBase.Cast( GetGame().GetPlayer() );
        if( !br_rpc.spectate_active && b_Spectating && returned_body && returned_body.IsAlive() )
        {
            b_Spectating = false;
            LeaveSpectate();
            return;
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

        UpdateSpectatorTags( br_rpc );

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
        //--- ADMIN ONLY. An admin draws their own name tags over every player and their own roster
        //--- data, so leaving Party's HUD up stacks two labels over each character and shows a party
        //--- panel for a party they are not currently playing in. An ORDINARY spectator keeps theirs:
        //--- following a teammate with the party HUD live is the existing, wanted behaviour.
        VigridPartyClientAPI.SetHudSuppressed( b_IsAdmin && IsSpectating() );

        vector subject_pos;

        //--- ORBIT mode has no subject, and GetSubjectPosition says so - fall back to the local
        //--- player rather than pinning every distance to the centre of the final circle.
        if( IsSpectating() && GetSubjectPosition( subject_pos ) )
            VigridPartyClientAPI.SetHudViewpoint( subject_pos, br_rpc.spectate_target_uid );
        else
            VigridPartyClientAPI.ClearHudViewpoint();
#endif
    }

    /**
     *  Drive the admin overlay: a name, health bar, distance and kill count over every living player.
     *
     *  Built on first use rather than in the constructor, so an ordinary player - who will never see
     *  this - never creates the widget tree. Once built it is kept: an admin toggles in and out of
     *  spectate repeatedly, and tearing the pool down each time would rebuild it seconds later.
     *
     *  Origin is the CAMERA, not a body. For a flying admin "how far away is that" can only sensibly
     *  mean "from where I am looking", and GetReferencePosition already answers exactly that while
     *  spectating.
     */
    protected void UpdateSpectatorTags(BattleRoyaleRPC br_rpc)
    {
        bool wanted = b_IsAdmin && IsSpectating() && br_rpc.spectate_active;

        if( !m_SpectatorTags )
        {
            if( !wanted )
                return;

            m_SpectatorTags = new BattleRoyaleSpectatorTags();
        }

        m_SpectatorTags.Update( wanted, GetReferencePosition(), br_rpc.spectate_target_uid );
    }

    /**
     *  Drive the lobby name tags: a name over every living non-teammate while the players are still
     *  gathered before the match.
     *
     *  The phase test is lobby_phase, NOT !match_started, and the difference is the whole reason
     *  that fact exists. match_started is a one-way latch set by a broadcast, so a client that
     *  connected after it was sent - an admin joining mid-match, everyone else being kicked - never
     *  receives it and reads false for the rest of the session. That is exactly how the old
     *  point-at-somebody tag ended up live for an admin in a running match.
     *
     *  Suppressed while spectating as well. It cannot currently coincide - no state both allows
     *  spectating and counts as the lobby - but if one ever did, an admin would be looking at two
     *  overlays naming the same characters.
     */
    protected void UpdateLobbyTags(BattleRoyaleRPC br_rpc)
    {
        bool wanted = BR_LOBBY_TAGS_ENABLED && br_rpc.lobby_phase && !IsSpectating();

        if( !m_LobbyTags )
        {
            //--- Nothing is built until the first frame that actually wants tags, so a client that
            //--- connects mid-match never creates the widget tree at all.
            if( !wanted )
                return;

            m_LobbyTags = new BattleRoyaleLobbyTags();
        }

        m_LobbyTags.Update( wanted );
    }

    /**
     *  Is the mod currently drawing its own names over players' heads?
     *
     *  Read by the modded IngameHud, which uses it to keep the "point at somebody" tag - vanilla's,
     *  or DayZ Expansion's if that addon is loaded - out of the way while we are naming everyone
     *  anyway. Two labels for one character is the stacking that had to be fixed once already for
     *  the party tags.
     */
    bool IsShowingOwnNameTags()
    {
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc )
            return false;

        if( BR_LOBBY_TAGS_ENABLED && br_rpc.lobby_phase && !IsSpectating() )
            return true;

        //--- The admin overlay. An ORDINARY spectator is deliberately not covered: they have no name
        //--- overlay of their own, so there is nothing for the point tag to collide with.
        if( b_IsAdmin && IsSpectating() && br_rpc.spectate_active )
            return true;

        return false;
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
        //---
        //--- GATED ON b_DeathLocked, AND THAT GATE IS LOAD-BEARING. ChangeGameFocus is an ADDITIVE
        //--- counter. An admin entering spectate alive never went through SimulateDeath and so has
        //--- no lock outstanding - releasing anyway would drive the counter negative, and a negative
        //--- focus counter breaks input with no error, no log line and nothing on screen to explain
        //--- it. The ledger this has to balance is written out in DeathScreenMenu.c:10-22.
        if( b_DeathLocked && GetGame().GetInput() )
        {
            GetGame().GetInput().ChangeGameFocus( -1, INPUT_DEVICE_MOUSE );
            GetGame().GetInput().ChangeGameFocus( -1, INPUT_DEVICE_KEYBOARD );
            GetGame().GetInput().ChangeGameFocus( -1, INPUT_DEVICE_GAMEPAD );

            //--- Consumed. The release has happened, so a later LeaveSpectate must not repeat it.
            b_DeathLocked = false;
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

    /**
     *  Put back what EnterSpectate changed, for an admin who has been handed a living body.
     *
     *  Only ever runs on that path - an ordinary spectator's session ends with the match and keeps
     *  its camera, so there is nothing to restore and nowhere to restore it to.
     *
     *  DELIBERATELY NOT A MIRROR IMAGE OF EnterSpectate, and each omission is a decision:
     *
     *    - No ChangeGameFocus. EnterSpectate's three releases are gated on b_DeathLocked and are
     *      consumed there; an admin who entered alive never had a lock, so there is nothing to
     *      re-take. Adding a symmetric-looking +1 here is exactly how the counter goes wrong.
     *    - No sound-bus restore. Those were never zeroed for a living admin, and re-setting them
     *      from g_Game.m_volume_* would be a no-op at best.
     *    - StopAllEffects is not undone. The body's own modifiers re-assert whatever it should have
     *      within a tick or two, and there is no "restore the effects I stopped" API to call.
     */
    protected void LeaveSpectate()
    {
        BattleRoyaleUtils.Info("[Spectate] leaving: hud / tint / camera state");

        //--- The vanilla HUD went away in EnterSpectate step 4 and nothing else brings it back:
        //--- vanilla only shows it on respawn, which is not a path this player took.
        MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
        if( gameplay )
            gameplay.SetVanillaHudVisible( true );

        //--- The spectator tint is driven through the requester directly and is edge-tracked, so it
        //--- would stay on for a living player whose zone verdict is now computed a different way.
        ApplyOutOfZoneTint( false, NULL );

        //--- Force the post-process scan to report on its next pass rather than trusting a signature
        //--- taken while a corpse was re-asserting effects.
        m_PPESignature = -1;

        //--- Hidden HERE rather than left to UpdateSpectatorTags, because UpdateSpectate returns
        //--- immediately after calling this and never reaches it again - the tags would stay painted
        //--- over a living player for the rest of the match. The pool itself is kept: an admin
        //--- toggles in and out repeatedly and would only rebuild it seconds later.
        if( m_SpectatorTags )
            m_SpectatorTags.Update( false, vector.Zero, "" );

        //--- Same reason: UpdatePartyViewpoint is above the early return, so without this the party
        //--- HUD would stay suppressed for the rest of the match once an admin left the camera.
#ifdef VIGRID_PARTY
        VigridPartyClientAPI.SetHudSuppressed( false );
#endif

        //--- And the skeletons, which COT would otherwise keep drawing over living players for the
        //--- rest of the match - an admin back in their own body with an x-ray view of everyone is
        //--- the exact thing the non-participant rule exists to prevent. Only touched when it is on,
        //--- so an admin who never pressed F6 does not pay for a COT lookup on every exit.
        if( b_SkeletonOverlay )
            SetSkeletonOverlay( false );

        if( GetGame().GetUIManager() )
            GetGame().GetUIManager().ShowUICursor( false );
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
        if ( !gameplay )
            return;

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
        if ( !gameplay )
            return;

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

    //------------------------------------------------------------------------------------------
    //--- Admin spectate, client half. All four are requests: the server owns the session, the
    //--- target and the mode, and pushes them back through SetSpectateTarget. Nothing here changes
    //--- what the camera does - it only asks.
    //---
    //--- The b_IsAdmin gate is presentation, not security. It stops an ordinary player's F3 from
    //--- putting a packet on the wire at all; the server re-checks every one of these against
    //--- admins_steamid64 regardless, so a client that lies about the flag achieves nothing.
    //------------------------------------------------------------------------------------------

    //! F3. Enter or leave admin spectate, respawning first when dead. What the press means is
    //! resolved server-side from the admin's actual situation - see BattleRoyaleSpectators.AdminToggle.
    void AdminSpectateToggle()
    {
        if( !b_IsAdmin )
            return;

        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateToggle", NULL, true );
    }

    //! F5. Flip between following a player and flying the free camera.
    void AdminSpectateCycleMode()
    {
        if( !b_IsAdmin )
            return;
        if( !IsSpectating() )
            return;

        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc )
            return;

        //--- Asked for against the mode the SERVER last pushed, not a locally-toggled one, so a
        //--- dropped or reordered reply cannot leave the two disagreeing about which mode we are in.
        int wanted = BR_SPECTATE_MODE_FREE;
        if( br_rpc.spectate_mode == BR_SPECTATE_MODE_FREE )
            wanted = BR_SPECTATE_MODE_FOLLOW;

        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateMode", new Param1<int>( wanted ), true );
    }

    //! Left / Right arrow. Step through the living players. Works in FREE mode too - the target is
    //! still tracked there, for the overlay highlight and so that flipping back to FOLLOW lands
    //! somewhere deliberate.
    void AdminSpectateCycle( int direction )
    {
        if( !b_IsAdmin )
            return;
        if( !IsSpectating() )
            return;

        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateCycle", new Param1<int>( direction ), true );
    }

    /**
     *  F6. Draw a bone skeleton over every player, or stop.
     *
     *  THE ONLY ADMIN KEY WITH NO SERVER HALF. The other four ask the server to change a session it
     *  owns; this one changes what this client renders and nothing else, so there is no state to
     *  replicate and nothing to authorise on the wire.
     *
     *  IT IS COT'S RENDERER, CALLED, NOT COPIED. Community-Online-Tools is already a hard dependency
     *  and JMESPModule.SetDrawPlayerSkeletonsEnabled is a public method on it; its OnUpdate runs on
     *  every client all the time and returns immediately unless the flag is set, and the canvas it
     *  needs is created unconditionally from COT's own MissionGameplay.OnInit. So the whole feature
     *  is this call. That also settles the licence question that made porting the code a non-starter:
     *  COT is CC BY-SA 4.0 and this repo is DSPL-SA, which adds restrictions BY-SA forbids adding to
     *  adapted material - but calling a published API is interoperation, not adaptation.
     *
     *  COT's ESPRadius is deliberately left alone. It is the admin's own COT setting, it survives
     *  this session, and quietly rewriting another mod's configuration to suit ours is not ours to do.
     */
    void AdminSpectateToggleSkeleton()
    {
        if( !b_IsAdmin )
            return;
        if( !IsSpectating() )
            return;

        SetSkeletonOverlay( !b_SkeletonOverlay );
    }

    /**
     *  Apply the skeleton flag and say what actually happened.
     *
     *  THE READ-BACK IS THE POINT. COT's setter opens with `if (!HasAccess()) return;` - a silent
     *  refusal for anyone without its ESP.View permission - so asking for skeletons without that
     *  permission would look exactly like a dead key. Reading the flag back turns that into a
     *  message. This mod has been bitten by silent no-ops often enough to be worth the extra call.
     */
    protected void SetSkeletonOverlay( bool enabled )
    {
#ifdef JM_COT
        JMESPModule esp;
        if( !CF_Modules<JMESPModule>.Get( esp ) )
        {
            BattleRoyaleUtils.Warn("[Spectate] JMESPModule not available - no skeleton overlay");
            NotifyLocal( "#STR_BR_SPECTATE_SKELETON_DENIED" );
            return;
        }

        //--- The canvas is COT's, created unconditionally from its MissionGameplay.OnInit. Asking it
        //--- to build one is harmless if it already has (CreateCanvas is guarded internally).
        if( !esp.m_ESPCanvas )
            esp.CreateCanvas();

        b_SkeletonOverlay = enabled;

        //--- Clear on the way out. Nothing else will: COT's own loop early-returns on a flag we
        //--- deliberately never set, so a canvas we stopped refreshing would keep its last frame
        //--- painted over the world for good.
        if( !enabled )
            ClearSkeletonCanvas( esp );

        BattleRoyaleUtils.Info("[Spectate] Skeleton overlay " + enabled);

        if( enabled )
            NotifyLocal( "#STR_BR_SPECTATE_SKELETON_ON" );
        else
            NotifyLocal( "#STR_BR_SPECTATE_SKELETON_OFF" );
#else
        //--- Build without COT. Nothing to draw with, so say so rather than leaving a dead key.
        b_SkeletonOverlay = false;
        NotifyLocal( "#STR_BR_SPECTATE_SKELETON_DENIED" );
#endif
    }

#ifdef JM_COT
    /**
     *  A red skeleton over a corpse, drawn straight onto the canvas.
     *
     *  WHY THIS IS NOT JMESPSkeleton.Draw. That method takes no colour: it picks one from
     *  human.GetHealthLevel(), and a dead body is STATE_RUINED, which COT paints 0xFF232323 -
     *  near-black, and useless for the one thing corpse markers are for. JMESPCanvas.DrawLine does
     *  take a colour and does its own projection and bounds check, so the corpse pass uses that
     *  directly. Living players still go through COT's renderer, whose health colouring is worth
     *  having and which is already proven.
     *
     *  ⚠️ THE BONE CHAIN IS DERIVED FROM VANILLA'S RIG, NOT COPIED FROM COT'S LIMB TABLE, and that
     *  distinction is the licence. COT is CC BY-SA 4.0 against this repo's DSPL-SA: *calling*
     *  JMESPSkeleton.Draw is interoperation and carries no obligation, but transcribing its
     *  s_Limbs array would be adaptation. These pairs come from the bone names vanilla itself
     *  registers per damage zone in BleedingSourcesManagerBase.Init
     *  (P:\scripts\4_world\classes\bleedingsources\bleedingsourcesmanagerbase.c:23-61), walked in
     *  rig order. It shows: this chain carries the shoulders and the full spine, which COT's does
     *  not, and stops at the feet rather than the finger bones.
     */
    protected void DrawCorpseSkeleton( PlayerBase body, JMESPCanvas canvas )
    {
        //--- Spine, head to pelvis.
        DrawBone( body, canvas, "Head", "Neck" );
        DrawBone( body, canvas, "Neck", "Spine3" );
        DrawBone( body, canvas, "Spine3", "Spine2" );
        DrawBone( body, canvas, "Spine2", "Spine1" );
        DrawBone( body, canvas, "Spine1", "Spine" );
        DrawBone( body, canvas, "Spine", "Pelvis" );

        //--- Arms.
        DrawBone( body, canvas, "Neck", "LeftShoulder" );
        DrawBone( body, canvas, "LeftShoulder", "LeftArm" );
        DrawBone( body, canvas, "LeftArm", "LeftForeArm" );
        DrawBone( body, canvas, "LeftForeArm", "LeftForeArmRoll" );

        DrawBone( body, canvas, "Neck", "RightShoulder" );
        DrawBone( body, canvas, "RightShoulder", "RightArm" );
        DrawBone( body, canvas, "RightArm", "RightForeArm" );
        DrawBone( body, canvas, "RightForeArm", "RightForeArmRoll" );

        //--- Legs.
        DrawBone( body, canvas, "Pelvis", "LeftUpLeg" );
        DrawBone( body, canvas, "LeftUpLeg", "LeftLeg" );
        DrawBone( body, canvas, "LeftLeg", "LeftFoot" );
        DrawBone( body, canvas, "LeftFoot", "LeftToeBase" );

        DrawBone( body, canvas, "Pelvis", "RightUpLeg" );
        DrawBone( body, canvas, "RightUpLeg", "RightLeg" );
        DrawBone( body, canvas, "RightLeg", "RightFoot" );
        DrawBone( body, canvas, "RightFoot", "RightToeBase" );
    }

    //! One limb. A bone that will not resolve is skipped rather than drawn from the origin, which is
    //! what an unchecked -1 index would produce - a line shooting off to the map corner.
    protected void DrawBone( PlayerBase body, JMESPCanvas canvas, string from_bone, string to_bone )
    {
        int from_index = body.GetBoneIndexByName( from_bone );
        if( from_index == -1 )
            return;

        int to_index = body.GetBoneIndexByName( to_bone );
        if( to_index == -1 )
            return;

        vector from_pos = body.GetBonePositionWS( from_index );
        vector to_pos = body.GetBonePositionWS( to_index );

        //--- DrawLine takes WORLD positions and does its own projection and off-screen rejection.
        canvas.DrawLine( from_pos, to_pos, BR_SPECTATE_SKELETON_CORPSE_THICKNESS, BR_SPECTATE_SKELETON_CORPSE_COLOUR );
    }

    protected void ClearSkeletonCanvas( JMESPModule esp )
    {
        if( !esp.m_ESPCanvas )
            return;
        if( !esp.m_ESPCanvas.HasCanvas() )
            return;

        esp.m_ESPCanvas.Clear();
    }
#endif

    /**
     *  Draw the skeletons, once per frame, from our own loop.
     *
     *  ⚠️ WHY THIS DOES NOT USE COT'S OWN RENDERER LOOP, having tried twice. The first build set
     *  JMESPModule.SetDrawPlayerSkeletonsEnabled(true) and nothing appeared; the log proved the flag
     *  was set, the permission granted and the canvas present, so the only thing left was that
     *  JMESPModule.OnUpdate - which is what actually draws - was never being ticked. The second
     *  build added esp.EnableUpdate() and STILL nothing appeared.
     *
     *  So the loop is ours. JMESPSkeleton.Draw is a public static taking a Human, a canvas and a
     *  line width; it reads bone positions in world space and writes into the canvas. Calling it
     *  directly needs nothing enabled and nothing ticking, and it is the same published-API call as
     *  before as far as the licence goes - COT is CC BY-SA 4.0, and using its API is interoperation,
     *  not adaptation.
     *
     *  Two things come with owning the loop, both of them wanted:
     *    - RANGE IS OURS. COT culls at ESPRadius, default 200 m and an admin's own COT setting - well
     *      inside where a spectating admin watches from, and a strong candidate for why even the
     *      EnableUpdate build showed nothing.
     *    - CLEARING IS OURS. A CanvasWidget keeps what was drawn until something clears it, so the
     *      Clear() must happen every frame BEFORE the redraw, and again when the overlay is turned
     *      off or the session ends.
     */
    protected void UpdateSkeletonOverlay()
    {
#ifdef JM_COT
        if( !b_SkeletonOverlay )
            return;

        //--- Belt and braces against a session that ended without LeaveSpectate: an admin back in
        //--- their body with skeletons still painted is the x-ray view the whole feature is gated to
        //--- prevent.
        if( !IsSpectating() )
        {
            SetSkeletonOverlay( false );
            return;
        }

        JMESPModule esp;
        if( !CF_Modules<JMESPModule>.Get( esp ) )
            return;
        if( !esp.m_ESPCanvas )
            return;
        if( !esp.m_ESPCanvas.HasCanvas() )
            return;

        esp.m_ESPCanvas.Clear();

        if( !ClientData.m_PlayerBaseList )
            return;

        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc )
            return;

        int count = ClientData.m_PlayerBaseList.Count();
        int drawn = 0;
        int not_playing = 0;
        int corpses = 0;

        for( int i = 0; i < count; i++ )
        {
            PlayerBase other = PlayerBase.Cast( ClientData.m_PlayerBaseList.Get( i ) );
            if( !other )
                continue;

            /**
             *  THE LIVING AND THE DEAD ARE FILTERED DIFFERENTLY, because only one of them can be
             *  checked against a roster.
             *
             *  ⚠️ LIVING: MATCH PARTICIPANTS ONLY, and this is what keeps the admin's OWN body out.
             *  The anchor body the camera carries under itself is created server-side and reaches
             *  this client as a REMOTE entity - the connection's selected object is the spectator
             *  camera, not the body - so ClientData.m_PlayerBaseList holds it like anybody else and
             *  it was getting a skeleton of its own. Comparing against GetGame().GetPlayer() does
             *  not help precisely because that inequality is why it was inserted. admin_uids is the
             *  server's match roster, pushed for the overlay tags, and a non-participant admin is
             *  absent from it by construction.
             *
             *  DEAD: DRAWN UNCONDITIONALLY, to find bodies after a fight. They cannot be roster-
             *  checked at all - admin_uids carries only the LIVING, so a corpse fails that test by
             *  definition - and there is nothing to gate them on anyway: a corpse is somewhere a
             *  fight happened, which is exactly the thing worth seeing.
             */
            bool is_alive = other.IsAlive();

            if( is_alive )
            {
                PlayerIdentity subject = other.GetIdentity();
                if( !subject )
                {
                    not_playing++;
                    continue;
                }

                string uid = subject.GetPlainId();

                //--- On its own line ahead of the test: a container read nested inside a call
                //--- argument has a measured aliasing defect in this codebase.
                int roster_index = br_rpc.admin_uids.Find( uid );
                if( roster_index == -1 )
                {
                    not_playing++;
                    continue;
                }
            }
            else
            {
                if( !BR_SPECTATE_SKELETON_CORPSES )
                    continue;

                corpses++;
            }

            //--- z is depth along the view axis, which is what a range cull wants, and its SIGN is
            //--- the behind-camera test. Drawing a behind-camera subject would mirror it to the
            //--- wrong side of the screen.
            vector screen_pos = GetGame().GetScreenPosRelative( other.GetPosition() );
            if( screen_pos[2] < 0 )
                continue;
            if( screen_pos[2] > BR_SPECTATE_SKELETON_RANGE_M )
                continue;

            //--- Living go through COT's renderer, which colours by health level. Corpses go through
            //--- ours, because that renderer has no colour parameter and would paint them near-black.
            if( is_alive )
                JMESPSkeleton.Draw( other, esp.m_ESPCanvas, BR_SPECTATE_SKELETON_THICKNESS );
            else
                DrawCorpseSkeleton( other, esp.m_ESPCanvas );

            drawn++;
        }

        ReportSkeletonFunnel( count, not_playing, corpses, drawn );
#endif
    }

    //! One throttled line: how many replicated players there were and how many got a skeleton. The
    //! lobby tags needed exactly this to stop the guessing, and this feature has now cost two builds
    //! to blind reasoning.
    protected void ReportSkeletonFunnel( int population, int not_playing, int corpses, int drawn )
    {
        int now = GetGame().GetTime();
        if( now < m_NextSkeletonDiagMs )
            return;

        m_NextSkeletonDiagMs = now + BR_LOBBY_TAG_DIAG_MS;

        //--- notplaying counts LIVING entities that are replicated but not on the match roster -
        //--- normally just the admin's own anchor body, so a figure above 1 is worth a look. corpses
        //--- is how many of `drawn` were bodies rather than players.
        BattleRoyaleUtils.Debug("[Spectate] skeletons population=" + population + " notplaying=" + not_playing + " corpses=" + corpses + " drawn=" + drawn);
    }

    /**
     *  A notification raised by this client for itself.
     *
     *  Every other notification in this mod is a server push through MessagePlayerUntranslated, and
     *  that is still the rule for anything the server decides. This one exists because the skeleton
     *  toggle never reaches the server, so there is nobody else to raise it. Same widget, icon and
     *  duration as the RPC path - see BattleRoyaleRPC.NotificationMessage and DAYZBR_MSG_TIME, which
     *  is what MessagePlayerUntranslated passes - just without the wire.
     *
     *  The key is passed with its leading '#' and handed straight to the widget, unlike the server
     *  path, which ships a bare key and localises it on arrival. Nothing crosses a stage boundary
     *  here, so there is no reason to take the string apart and put it back together.
     */
    protected void NotifyLocal( string key )
    {
        ExpansionNotification( DAYZBR_MSG_TITLE, key, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, DAYZBR_MSG_TIME ).Create();
    }

    //! Does this client believe it is an admin? Pushed once by the server on connect. Used to gate
    //! the keys above and the death screen's admin button - never as an authorization decision.
    bool IsAdmin()
    {
        return b_IsAdmin;
    }

    //--- There was a ReportSteamName() here, sending BiosUser.GetName() to the server as a fallback
    //--- for players the Steam Web API cannot answer for. It was removed after measurement:
    //--- on PC, BiosUser.GetName() returns the *profile* name, not the Steam persona. Two local
    //--- clients on one Steam account (persona "ANTHOxY") reported "Client_A" and "Survivor" -
    //--- their -name= launcher values. So the client has no persona name to offer, and this could
    //--- only ever have echoed back the placeholder it was meant to replace.

    /**
     *  Ask the server for one leaderboard ladder. The server rate-limits this per player, so the
     *  menu is free to call it on show, on every tab switch, and on a poll while it is open.
     *
     *  ⚠ NEVER pass BR_LEADERBOARD_BOARD_LASTMATCH. BattleRoyaleLeaderboard.ServeRequest treats
     *  anything that is not GROUP as SOLO, so board 2 would be answered with the solo ladder tagged
     *  as solo while still consuming this player's cooldown - the tab would simply appear dead. The
     *  last-match tab has its own request below.
     */
    void RequestLeaderboard( int board )
    {
        ref Param1<int> requested_board = new Param1<int>( board );
        //--- No target - see ReadyUp above.
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestLeaderboard", requested_board, true );
    }

    //! Ask the server for the PREVIOUS match: the standings table and this player's own recap.
    //! No payload - the server resolves the actor from the RPC sender.
    void RequestLastMatch()
    {
        GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestLastMatch", NULL, true );
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
