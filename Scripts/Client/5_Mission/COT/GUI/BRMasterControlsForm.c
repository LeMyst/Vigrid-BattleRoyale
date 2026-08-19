#ifdef JM_COT
class BRMasterControlsForm: JMFormBase
{
    private UIActionScroller m_sclr_MainActions;
    private Widget m_ActionsWrapper;
    private BRMasterControlsModule m_Module;

#ifndef SERVER
    //--- Status readout, repainted from the last snapshot the server answered.
    private UIActionText m_txt_State;
    private UIActionText m_txt_Population;
    private UIActionText m_txt_Countdown;
    private UIActionText m_txt_Circle;
    private UIActionText m_txt_Seed;

    //--- Lobby start gate. Two lines because one carries too many fields to read at a glance - and
    //--- because a single concatenation of this many terms is exactly the shape that trips
    //--- EnfusionScript's "Formula too complex".
    private UIActionText m_txt_LobbyGateA;
    private UIActionText m_txt_LobbyGateB;
    private UIActionCheckbox m_chk_HoldLobby;
    private UIActionButton m_btn_StartNow;

    //--- Player operations.
    private UIActionText m_txt_Selected;
    private UIActionText m_txt_SelectedStats;
    private UIActionButton m_btn_Ready;
    private UIActionButton m_btn_Remove;
    private UIActionButton m_btn_Add;
    private UIActionButton m_btn_Unstuck;
    private UIActionButton m_btn_TpCircle;
    private UIActionButton m_btn_CancelKick;

    //--- Scoreboard.
    private ref array<UIActionText> m_ScoreRows;
    private UIActionText m_txt_ScoreMore;

    //--- Announcements.
    private UIActionEditableText m_edit_Announce;
    private UIActionSlider m_sld_AnnounceSeconds;

    //--- Zone table.
    private UIActionText m_txt_ZoneHeader;
    private ref array<UIActionText> m_ZoneRows;

    //--- GetGame().GetTime() of the next poll of each feed, and the sequence number of the snapshot
    //--- currently painted. Polling and painting are separate clocks: the poll is a network cost paid
    //--- on a timer, the repaint is only worth doing when something arrived.
    private int m_NextStatusPollMs;
    private int m_NextRosterPollMs;
    private int m_PaintedStatusSeq;
    private int m_PaintedRosterSeq;
    private int m_PaintedZoneSeq;

    //--- Whether the one-shot zone request has gone out for this showing.
    private bool m_ZoneRequested;

    //--- ⚠️ The selection is held as a UID, never as a row index. The roster is rebuilt server-side
    //--- every poll and its order follows the connected population, so an index silently comes to
    //--- mean a different player the moment anybody joins or leaves - and an admin action aimed at
    //--- "the selected player" would then hit somebody else. The index is derived from the uid at
    //--- the moment it is needed.
    private string m_SelectedUid;
#endif

    protected override bool SetModule( JMRenderableModuleBase mdl )
    {
        return Class.CastTo( m_Module, mdl );
    }

#ifndef SERVER
    override void OnInit()
    {
        m_ScoreRows = new array<UIActionText>();
        m_ZoneRows = new array<UIActionText>();

        m_sclr_MainActions = UIActionManager.CreateScroller( layoutRoot.FindAnyWidget( "panel" ) );
        m_ActionsWrapper = m_sclr_MainActions.GetContentWidget();

        //--- MATCH STATUS -------------------------------------------------------------------------
        Widget wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 5, 1 );
            m_txt_State      = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_STATE", "-" );
            m_txt_Population = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_PLAYERS", "-" );
            m_txt_Countdown  = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_COUNTDOWN", "-" );
            m_txt_Circle     = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_CIRCLE", "-" );
            m_txt_Seed       = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_SEED", "-" );

        //--- LOBBY -------------------------------------------------------------------------------
        //--- The two gate lines are the same facts BR_DiagLogGate() prints and which are otherwise
        //--- only reachable under DIAG_DEVELOPER, i.e. never on a live server.
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_LOBBY" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 2, 1 );
            m_txt_LobbyGateA = UIActionManager.CreateText( wrapper, "#STR_BR_COT_LOBBY_GATE", "-" );
            m_txt_LobbyGateB = UIActionManager.CreateText( wrapper, " ", "-" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 3 );
            m_chk_HoldLobby = UIActionManager.CreateCheckbox( wrapper, "#STR_BR_COT_LOBBY_HOLD", this, "HoldLobby_Changed", false );
            m_btn_StartNow  = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_LOBBY_STARTNOW", this, "StartNow_Clicked" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_READYALL", this, "ReadyAll_Clicked" );

        //--- PLAYERS ------------------------------------------------------------------------------
        //--- Prev/Next over the roster rather than a dropdown: UIActionSelectBox.SetSelections
        //--- CONSTRUCTS a new OptionSelectorMultistate on every call, so refreshing the option list
        //--- at poll rate would rebuild its widgets twice a second.
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_PLAYERS" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 2 );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_PREV", this, "PlayerPrev_Clicked" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_NEXT", this, "PlayerNext_Clicked" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 2, 1 );
            m_txt_Selected      = UIActionManager.CreateText( wrapper, "#STR_BR_COT_PLAYER_SELECTED", "-" );
            m_txt_SelectedStats = UIActionManager.CreateText( wrapper, " ", "-" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 3 );
            m_btn_Ready      = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_READY", this, "PlayerReady_Clicked" );
            m_btn_Unstuck    = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_UNSTUCK", this, "PlayerUnstuck_Clicked" );
            m_btn_TpCircle   = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_TPCIRCLE", this, "PlayerTpCircle_Clicked" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 3 );
            m_btn_Remove     = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_REMOVE", this, "PlayerRemove_Clicked" );
            m_btn_Add        = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_ADD", this, "PlayerAdd_Clicked" );
            m_btn_CancelKick = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PLAYER_KEEPJOIN", this, "PlayerCancelKick_Clicked" );

        //--- SCOREBOARD ---------------------------------------------------------------------------
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_SCOREBOARD" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, BR_ADMIN_SCOREBOARD_ROWS + 1, 1 );
            for ( int s = 0; s < BR_ADMIN_SCOREBOARD_ROWS; s++ )
                m_ScoreRows.Insert( UIActionManager.CreateText( wrapper, " ", "" ) );

            m_txt_ScoreMore = UIActionManager.CreateText( wrapper, " ", "" );

        //--- ANNOUNCE ----------------------------------------------------------------------------
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_ANNOUNCE" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 2, 1 );
            m_edit_Announce = UIActionManager.CreateEditableText( wrapper, "#STR_BR_COT_ANNOUNCE_TEXT", this, "", "" );
            m_sld_AnnounceSeconds = UIActionManager.CreateSlider( wrapper, "#STR_BR_COT_ANNOUNCE_SECONDS", BR_ANNOUNCE_MIN_SECONDS, BR_ANNOUNCE_MAX_SECONDS, this, "" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 2 );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_ANNOUNCE_ALL", this, "AnnounceAll_Clicked" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_ANNOUNCE_ONE", this, "AnnounceOne_Clicked" );

        //--- ZONES -------------------------------------------------------------------------------
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_ZONES" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, BR_ADMIN_ZONE_ROWS + 1, 1 );
            m_txt_ZoneHeader = UIActionManager.CreateText( wrapper, " ", "-" );

            for ( int z = 0; z < BR_ADMIN_ZONE_ROWS; z++ )
                m_ZoneRows.Insert( UIActionManager.CreateText( wrapper, " ", "" ) );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 3 );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_ZONE_LOCKNOW", this, "ZoneLockNow_Clicked" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_ZONE_SELFTEST", this, "ZoneSelfTest_Clicked" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_ZONE_REFRESH", this, "ZoneRefresh_Clicked" );

        //--- STATE MACHINE -----------------------------------------------------------------------
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 4 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATEMACHINE" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_NEXTSTATE", this, "StateMachine_Next" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_PAUSE", this, "StateMachine_Pause" );
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_RESUME", this, "StateMachine_Resume" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 4 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_EVENTMANAGER" );
#ifdef EXPANSIONMODMISSIONS
            UIActionManager.CreateButton( wrapper, "#STR_BR_COT_SPAWNAIRDROP", this, "SpawnAirdrop" );
#endif

        //--- No DIAG block here on purpose. "Add Player" / "Add Group" used to live here, but
        //--- BattleRoyaleCOTStateMachineRPC.ReservedAddFakePlayer / .ReservedAddFakeGroup have no
        //--- case in the server switch, so both buttons sent an RPC into the void. Fake players and
        //--- fake parties now come from the diag menu instead - see PluginDiagMenu.c.

        m_sclr_MainActions.UpdateScroller();
    }

    override void OnShow()
    {
        //--- Ask immediately rather than waiting out an interval, so the panel is populated by the
        //--- time the operator has finished opening it.
        m_NextStatusPollMs = 0;
        m_NextRosterPollMs = 0;
        m_PaintedStatusSeq = -1;
        m_PaintedRosterSeq = -1;
        m_PaintedZoneSeq = -1;
        m_ZoneRequested = false;
    }

    override void OnHide()
    {
    }

    /**
     *  Driven per frame by JMWindowBase.Update.
     *
     *  Three feeds on three different clocks, because they change at three different rates: the
     *  status every second, the roster every two, and the zone chain ONCE - it is generated at boot
     *  and never changes afterwards, so polling it would be pure waste.
     */
    override void Update()
    {
        super.Update();

        if ( !m_Module )
            return;

        if ( !IsVisible() )
            return;

        int now = GetGame().GetTime();

        if ( now >= m_NextStatusPollMs )
        {
            m_NextStatusPollMs = now + BR_ADMIN_STATUS_INTERVAL_MS;
            m_Module.RequestStatus();
        }

        if ( now >= m_NextRosterPollMs )
        {
            m_NextRosterPollMs = now + BR_ADMIN_ROSTER_INTERVAL_MS;
            m_Module.RequestRoster();
        }

        if ( !m_ZoneRequested )
        {
            m_ZoneRequested = true;
            m_Module.RequestZoneTable();
        }

        int status_seq = m_Module.GetStatusSeq();
        if ( status_seq != m_PaintedStatusSeq )
        {
            m_PaintedStatusSeq = status_seq;
            Repaint( m_Module.GetStatus() );
        }

        int roster_seq = m_Module.GetRosterSeq();
        if ( roster_seq != m_PaintedRosterSeq )
        {
            m_PaintedRosterSeq = roster_seq;
            RepaintRoster( m_Module.GetRoster() );
        }

        int zone_seq = m_Module.GetZoneSeq();
        if ( zone_seq != m_PaintedZoneSeq )
        {
            m_PaintedZoneSeq = zone_seq;
            RepaintZones( m_Module.GetZoneTable() );
        }
    }

    //--- mm:ss, or a dash when nothing is counting down. BR_COUNTDOWN_NONE rather than 0 is the
    //--- "no countdown" sentinel, because a countdown legitimately reaches 0.
    private string FormatCountdown( int ms )
    {
        if ( ms == BR_COUNTDOWN_NONE )
            return "-";

        int total_seconds = ms / 1000;
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;

        string seconds_text = seconds.ToString();
        if ( seconds < 10 )
            seconds_text = "0" + seconds_text;

        return minutes.ToString() + ":" + seconds_text;
    }

    private string FormatBool( bool value )
    {
        if ( value )
            return Widget.TranslateString( "#STR_BR_COT_YES" );

        return Widget.TranslateString( "#STR_BR_COT_NO" );
    }

    /**
     *  ⚠️ Every line here is built in STEPS rather than as one concatenation. EnfusionScript caps how
     *  complex a single expression may be - around ten concatenated terms is rejected with
     *  "Formula too complex", which is a hard compile error that packs fine and only surfaces when
     *  the game loads the module. These status lines are exactly that shape.
     */
    private void Repaint( BattleRoyaleAdminStatus status )
    {
        if ( !status )
            return;

        string state_line = status.state_name;
        state_line += "  [" + (status.state_index + 1).ToString();
        state_line += "/" + status.state_count.ToString() + "]";
        if ( status.state_paused )
            state_line += "  " + Widget.TranslateString( "#STR_BR_COT_STATUS_PAUSED" );
        m_txt_State.SetText( state_line );

        string pop_line = status.players_alive.ToString() + " alive";

        //--- Shown only when the party manager can actually answer. With it disabled GetGroupCount()
        //--- returns one group per player - a number always identical to players_alive - and putting
        //--- that under a group heading is worse than omitting it (#158).
        if ( status.groups_alive != BR_ADMIN_GROUPS_UNKNOWN )
            pop_line += " / " + status.groups_alive.ToString() + " groups";

        pop_line += " / " + status.connected.ToString() + " connected";
        pop_line += " / " + status.spectators.ToString() + " spectating";
        m_txt_Population.SetText( pop_line );

        m_txt_Countdown.SetText( FormatCountdown( status.countdown_ms ) );

        string circle_line = "-";
        if ( status.current_radius > 0 || status.future_radius > 0 )
        {
            circle_line = "now " + Math.Round( status.current_radius ).ToString() + " m";
            circle_line += " / next " + Math.Round( status.future_radius ).ToString() + " m";
            if ( status.zone_locked )
                circle_line += "  " + Widget.TranslateString( "#STR_BR_COT_STATUS_LOCKED" );
        }
        m_txt_Circle.SetText( circle_line );

        m_txt_Seed.SetText( status.generation_seed.ToString() );

        RepaintLobby( status );
    }

    private void RepaintLobby( BattleRoyaleAdminStatus status )
    {
        //--- Outside the lobby every one of these fields is meaningless rather than merely zero, so
        //--- the block says so instead of rendering a row of zeroes that look like real readings.
        if ( !status.lobby_phase )
        {
            m_txt_LobbyGateA.SetText( Widget.TranslateString( "#STR_BR_COT_LOBBY_OVER" ) );
            m_txt_LobbyGateB.SetText( "" );
            m_chk_HoldLobby.SetEnabled( false );
            m_btn_StartNow.SetEnabled( false );
            return;
        }

        string gate_a = "loaded " + status.lobby_loaded.ToString();
        gate_a += "/" + status.lobby_minimum.ToString();
        gate_a += "  players " + status.lobby_players.ToString();
        gate_a += "  loading " + status.lobby_not_loaded.ToString();
        gate_a += "  uptime " + status.lobby_uptime_s.ToString();
        gate_a += "/" + status.lobby_min_wait_s.ToString() + "s";
        m_txt_LobbyGateA.SetText( gate_a );

        string gate_b = "full " + FormatBool( status.lobby_full );
        gate_b += "  not-full wait " + FormatBool( status.lobby_notfull_ok );
        if ( status.lobby_notfull_left_s > 0 )
            gate_b += " (" + status.lobby_notfull_left_s.ToString() + "s)";
        gate_b += "  vote " + FormatBool( status.lobby_vote_system );
        gate_b += "  ready " + status.lobby_ready_count.ToString();
        gate_b += "  load hold " + FormatBool( status.lobby_load_hold );
        m_txt_LobbyGateB.SetText( gate_b );

        m_chk_HoldLobby.SetEnabled( true );
        m_chk_HoldLobby.SetChecked( status.lobby_held );

        //--- Disabled rather than allowed-and-refused. The one hard refusal is the group-count floor,
        //--- and a match force-started under it ends on its very first tick - so this is a button
        //--- whose only possible outcome is an instantly-finished match, and the honest thing is to
        //--- not offer it. Every other gate an admin may legitimately overrule and this stays live.
        m_btn_StartNow.SetEnabled( status.lobby_can_start_now );
    }

    //--- Index of the selected uid in the current roster, or -1. Derived on demand rather than
    //--- stored - see the note on m_SelectedUid.
    private int FindSelectedIndex( BattleRoyaleAdminRoster roster )
    {
        if ( !roster || !roster.rows )
            return -1;

        for ( int i = 0; i < roster.rows.Count(); i++ )
        {
            BattleRoyaleAdminRosterRow row = roster.rows.Get( i );
            if ( row && row.uid == m_SelectedUid )
                return i;
        }

        return -1;
    }

    private void StepSelection( int direction )
    {
        BattleRoyaleAdminRoster roster = m_Module.GetRoster();
        if ( !roster || !roster.rows || roster.rows.Count() == 0 )
            return;

        int index = FindSelectedIndex( roster );

        //--- A selection that no longer exists - the player left - lands on the first row rather than
        //--- staying pointed at nobody.
        if ( index < 0 )
            index = 0;
        else
            index = index + direction;

        if ( index < 0 )
            index = roster.rows.Count() - 1;
        if ( index >= roster.rows.Count() )
            index = 0;

        BattleRoyaleAdminRosterRow row = roster.rows.Get( index );
        if ( row )
            m_SelectedUid = row.uid;

        RepaintRoster( roster );
    }

    private void RepaintRoster( BattleRoyaleAdminRoster roster )
    {
        if ( !roster )
            return;

        //--- "No reply yet" and "an empty server" must look different, or a panel that never got an
        //--- answer reads as a server everybody left.
        if ( !roster.valid )
        {
            m_txt_Selected.SetText( Widget.TranslateString( "#STR_BR_COT_PLAYER_WAITING" ) );
            m_txt_SelectedStats.SetText( "" );
            SetPlayerActionsEnabled( false );
            return;
        }

        RepaintScoreboard( roster );

        int index = FindSelectedIndex( roster );
        if ( index < 0 && roster.rows.Count() > 0 )
        {
            BattleRoyaleAdminRosterRow first = roster.rows.Get( 0 );
            if ( first )
            {
                m_SelectedUid = first.uid;
                index = 0;
            }
        }

        if ( index < 0 )
        {
            m_txt_Selected.SetText( Widget.TranslateString( "#STR_BR_COT_PLAYER_NONE" ) );
            m_txt_SelectedStats.SetText( "" );
            SetPlayerActionsEnabled( false );
            return;
        }

        BattleRoyaleAdminRosterRow row = roster.rows.Get( index );
        if ( !row )
            return;

        string header = row.name;
        header += "   (" + (index + 1).ToString() + "/" + roster.rows.Count().ToString() + ")";
        if ( roster.truncated )
            header += "  " + Widget.TranslateString( "#STR_BR_COT_PLAYER_TRUNCATED" );
        m_txt_Selected.SetText( header );

        string detail = "";
        if ( row.spectating )
            detail += "spectating  ";
        else if ( row.alive )
            detail += "alive  ";
        else if ( row.in_state )
            detail += "dead  ";
        else
            detail += "not in match  ";

        if ( row.group != BR_ADMIN_GROUPS_UNKNOWN )
            detail += "team " + row.group.ToString() + "  ";

        detail += "ready " + FormatBool( row.ready ) + "  ";
        detail += "loaded " + FormatBool( row.loaded );

        if ( roster.scoring )
        {
            detail += "  kills " + row.kills.ToString();
            detail += "  dmg " + row.damage.ToString();
        }

        //--- The one field that is an alarm rather than a stat: this player is on a disconnect
        //--- countdown, which until now was invisible from anywhere.
        if ( row.late_join_seconds >= 0 )
            detail += "  KICK IN " + row.late_join_seconds.ToString() + "s";

        m_txt_SelectedStats.SetText( detail );

        SetPlayerActionsEnabled( true );

        //--- Per-action gating, so a button is never offered for a state it cannot apply to. Each of
        //--- these has a server-side refusal behind it as well - the panel works off a snapshot that
        //--- is up to a poll old, so it can be wrong for a moment and must not be the only guard.
        m_btn_CancelKick.SetEnabled( row.late_join_seconds >= 0 );
        m_btn_Remove.SetEnabled( row.in_state );
        m_btn_Add.SetEnabled( !row.in_state );

        //--- ⚠️ Only a player the lobby actually holds may be readied. Marking somebody off the
        //--- roster ready produced "1/1 ready" with the real player not ready: the ready count and
        //--- the denominator were drawn from two different sets of people. The server refuses it in
        //--- ReadyUp too - this just stops the button inviting it.
        m_btn_Ready.SetEnabled( row.in_state && !row.ready );
    }

    private void SetPlayerActionsEnabled( bool enabled )
    {
        m_btn_Ready.SetEnabled( enabled );
        m_btn_Remove.SetEnabled( enabled );
        m_btn_Add.SetEnabled( enabled );
        m_btn_Unstuck.SetEnabled( enabled );
        m_btn_TpCircle.SetEnabled( enabled );
        m_btn_CancelKick.SetEnabled( enabled );
    }

    /**
     *  Leaders first: finished placements ascending, then the living by kills.
     *
     *  Written out by hand because EnfusionScript's `string` exposes no comparison and
     *  `array<T>.Sort` reorders in place - and the roster must NOT be reordered, since the player
     *  selector indexes it. This builds an order of indices instead, exactly as the party menu's
     *  BuildOnlineOrder does and for the same reason.
     */
    private void RepaintScoreboard( BattleRoyaleAdminRoster roster )
    {
        if ( !roster.scoring )
        {
            for ( int blank = 0; blank < m_ScoreRows.Count(); blank++ )
                m_ScoreRows.Get( blank ).SetText( "" );

            m_txt_ScoreMore.SetText( Widget.TranslateString( "#STR_BR_COT_SCOREBOARD_IDLE" ) );
            return;
        }

        array<int> order = new array<int>();
        for ( int seed = 0; seed < roster.rows.Count(); seed++ )
            order.Insert( seed );

        //--- Insertion sort. The population is at most a few dozen and this runs on a 2 s edge, so
        //--- the simplest correct thing is the right thing.
        for ( int i = 1; i < order.Count(); i++ )
        {
            int held = order.Get( i );
            int j = i - 1;

            while ( j >= 0 )
            {
                if ( !ScoreBefore( roster, held, order.Get( j ) ) )
                    break;

                order.Set( j + 1, order.Get( j ) );
                j--;
            }

            order.Set( j + 1, held );
        }

        int shown = 0;
        for ( int slot = 0; slot < m_ScoreRows.Count(); slot++ )
        {
            if ( slot >= order.Count() )
            {
                m_ScoreRows.Get( slot ).SetText( "" );
                continue;
            }

            BattleRoyaleAdminRosterRow row = roster.rows.Get( order.Get( slot ) );
            if ( !row )
                continue;

            string line = "";
            if ( row.place > 0 )
                line += "#" + row.place.ToString() + "  ";
            else
                line += "--  ";

            line += row.name;
            line += "   k " + row.kills.ToString();
            line += "  dmg " + row.damage.ToString();
            line += "  hits " + row.hits.ToString();

            m_ScoreRows.Get( slot ).SetText( line );
            shown++;
        }

        int omitted = order.Count() - shown;
        if ( omitted > 0 )
            m_txt_ScoreMore.SetText( "+ " + omitted.ToString() + Widget.TranslateString( "#STR_BR_COT_SCOREBOARD_MORE" ) );
        else
            m_txt_ScoreMore.SetText( "" );
    }

    //--- Should row `a` sort above row `b`? A finished placement outranks anyone still playing, and
    //--- placements are ascending because #1 is the winner.
    private bool ScoreBefore( BattleRoyaleAdminRoster roster, int a, int b )
    {
        BattleRoyaleAdminRosterRow ra = roster.rows.Get( a );
        BattleRoyaleAdminRosterRow rb = roster.rows.Get( b );
        if ( !ra || !rb )
            return false;

        if ( ra.place > 0 && rb.place > 0 )
            return (ra.place < rb.place);

        if ( ra.place > 0 )
            return true;

        if ( rb.place > 0 )
            return false;

        if ( ra.kills != rb.kills )
            return (ra.kills > rb.kills);

        return (ra.damage > rb.damage);
    }

    private void RepaintZones( BattleRoyaleAdminZoneTable table )
    {
        if ( !table || !table.valid )
        {
            m_txt_ZoneHeader.SetText( Widget.TranslateString( "#STR_BR_COT_ZONES_NONE" ) );
            return;
        }

        string header = "seed " + table.generation_seed.ToString();
        header += "  zones " + table.num_zones.ToString();
        header += "  derived " + FormatBool( table.derive_timers );
        header += "  flex " + FormatBool( table.allow_flex );
        m_txt_ZoneHeader.SetText( header );

        for ( int slot = 0; slot < m_ZoneRows.Count(); slot++ )
        {
            if ( slot >= table.rows.Count() )
            {
                m_ZoneRows.Get( slot ).SetText( "" );
                continue;
            }

            BattleRoyaleAdminZoneRow row = table.rows.Get( slot );
            if ( !row )
                continue;

            string line = row.play_order.ToString() + ".";
            line += "  r " + Math.Round( row.radius ).ToString() + " m";
            line += "  t " + row.timer_s.ToString() + "s";

            if ( row.offset_s > 0 )
                line += " +" + row.offset_s.ToString();

            //--- Shown whether or not the setting is on: this column is what an operator consults to
            //--- decide whether to turn it on, so hiding it when it is off defeats the purpose.
            line += "  geo " + row.derived_timer_s.ToString() + "s";

            if ( row.growth_m > 0 )
                line += "  grew " + Math.Round( row.growth_m ).ToString() + " m";

            if ( row.current )
                line += "   <<";
            else if ( row.skipped )
                line += "   (skipped)";

            m_ZoneRows.Get( slot ).SetText( line );
        }
    }

    //--- The selected player's uid, or "" when there is no usable selection. Every action button
    //--- goes through this rather than reading m_SelectedUid directly, so a stale selection cannot
    //--- send an action naming somebody who has left.
    private string SelectedUid()
    {
        BattleRoyaleAdminRoster roster = m_Module.GetRoster();
        if ( FindSelectedIndex( roster ) < 0 )
            return "";

        return m_SelectedUid;
    }

    private float AnnounceSeconds()
    {
        if ( !m_sld_AnnounceSeconds )
            return BR_ANNOUNCE_DEFAULT_SECONDS;

        return m_sld_AnnounceSeconds.GetCurrent();
    }

    /**
     *  ⚠️ UIActionCheckbox fires CLICK, not CHANGE - it calls CallEvent( UIEvent.CLICK ) for both the
     *  box and its label button (UIActionCheckbox.OnClick). Listening for CHANGE here compiles, draws
     *  a working-looking checkbox and silently never sends anything.
     *
     *  IsChecked() already reads the NEW state by the time this runs, so the local box flips
     *  immediately and the next status reply either confirms it or - if the server refused, or the
     *  lobby closed underneath us - puts it back. That reconciliation is why the handler does not
     *  need to know whether the action succeeded.
     */
    void HoldLobby_Changed(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        UIActionCheckbox checkbox = UIActionCheckbox.Cast( action );
        if ( !checkbox )
            return;

        int held = 0;
        if ( checkbox.IsChecked() )
            held = 1;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.LOBBY_SET_HOLD, held );
    }

    void StartNow_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.LOBBY_START_NOW );
    }

    void ReadyAll_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.PLAYER_READY_ALL );
    }

    void PlayerPrev_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        StepSelection( -1 );
    }

    void PlayerNext_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        StepSelection( 1 );
    }

    void PlayerReady_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.PLAYER_READY, 0, 0, SelectedUid() );
    }

    void PlayerRemove_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.PLAYER_REMOVE, 0, 0, SelectedUid() );
    }

    void PlayerAdd_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.PLAYER_ADD, 0, 0, SelectedUid() );
    }

    void PlayerUnstuck_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.PLAYER_UNSTUCK, 0, 0, SelectedUid() );
    }

    void PlayerTpCircle_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.PLAYER_TP_CIRCLE, 0, 0, SelectedUid() );
    }

    void PlayerCancelKick_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.PLAYER_EXEMPT_LATEJOIN, 0, 0, SelectedUid() );
    }

    void AnnounceAll_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;
        if ( !m_edit_Announce )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.ANNOUNCE_ALL, 0, AnnounceSeconds(), "", m_edit_Announce.GetText() );
    }

    void AnnounceOne_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;
        if ( !m_edit_Announce )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.ANNOUNCE_PLAYER, 0, AnnounceSeconds(), SelectedUid(), m_edit_Announce.GetText() );
    }

    void ZoneLockNow_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.ZONE_LOCK_NOW );
    }

    void ZoneSelfTest_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.SendAdminAction( BattleRoyaleAdminAction.ZONE_SELFTEST, BR_ADMIN_SELFTEST_RUNS );
    }

    void ZoneRefresh_Clicked(UIEvent eid, UIActionBase action)
    {
        if ( eid != UIEvent.CLICK )
            return;

        m_Module.RequestZoneTable();
    }

    void StateMachine_Next(UIEvent eid, UIActionBase action)
    {
        m_Module.StateMachine_Next();
    }

    void StateMachine_Pause(UIEvent eid, UIActionBase action)
    {
        m_Module.StateMachine_Pause();
    }

    void StateMachine_Resume(UIEvent eid, UIActionBase action)
    {
        m_Module.StateMachine_Resume();
    }

    void SpawnAirdrop(UIEvent eid, UIActionBase action)
    {
        m_Module.SpawnAirdrop();
    }
#endif
}
#endif
