#ifdef JM_COT
class BRMasterControlsForm: JMFormBase
{
    private UIActionScroller m_sclr_MainActions;
    private Widget m_ActionsWrapper;
    private BRMasterControlsModule m_Module;

#ifndef SERVER
    //--- Status readout rows, repainted from the last snapshot the server answered.
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

    //--- GetGame().GetTime() of the next status poll, and the sequence number of the snapshot
    //--- currently painted. Polling and painting are separate clocks on purpose: the poll is a
    //--- network cost paid on a timer, the repaint is only worth doing when something arrived.
    private int m_NextPollMs;
    private int m_PaintedSeq;
#endif

    protected override bool SetModule( JMRenderableModuleBase mdl )
    {
        return Class.CastTo( m_Module, mdl );
    }

#ifndef SERVER
    override void OnInit()
    {
        m_sclr_MainActions = UIActionManager.CreateScroller( layoutRoot.FindAnyWidget( "panel" ) );
        m_ActionsWrapper = m_sclr_MainActions.GetContentWidget();

        //--- MATCH STATUS -------------------------------------------------------------------------
        //--- The panel was write-only until this block existed: an admin pressed "Next State" and
        //--- nothing on screen said whether it had done anything, what state the match was in, or
        //--- how many players were left.
        Widget wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 5, 1 );
            m_txt_State      = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_STATE", "-" );
            m_txt_Population = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_PLAYERS", "-" );
            m_txt_Countdown  = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_COUNTDOWN", "-" );
            m_txt_Circle     = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_CIRCLE", "-" );
            m_txt_Seed       = UIActionManager.CreateText( wrapper, "#STR_BR_COT_STATUS_SEED", "-" );

        //--- LOBBY -------------------------------------------------------------------------------
        //--- The two gate lines are the same facts BR_DiagLogGate() prints into the server log, which
        //--- is behind DIAG_DEVELOPER and therefore unreachable on exactly the live servers where
        //--- somebody asks why the match has not started.
        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 1 );
            UIActionManager.CreateText( wrapper, "#STR_BR_COT_LOBBY" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 2, 1 );
            m_txt_LobbyGateA = UIActionManager.CreateText( wrapper, "#STR_BR_COT_LOBBY_GATE", "-" );
            m_txt_LobbyGateB = UIActionManager.CreateText( wrapper, " ", "-" );

        wrapper = UIActionManager.CreateGridSpacer( m_ActionsWrapper, 1, 2 );
            m_chk_HoldLobby = UIActionManager.CreateCheckbox( wrapper, "#STR_BR_COT_LOBBY_HOLD", this, "HoldLobby_Changed", false );
            m_btn_StartNow  = UIActionManager.CreateButton( wrapper, "#STR_BR_COT_LOBBY_STARTNOW", this, "StartNow_Clicked" );

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
        m_NextPollMs = 0;
        m_PaintedSeq = -1;
    }

    override void OnHide()
    {
    }

    /**
     *  Driven per frame by JMWindowBase.Update. Two independent clocks:
     *
     *  - the POLL is a network cost, so it is thottled to BR_ADMIN_STATUS_INTERVAL_MS and only runs
     *    while the panel is actually on screen;
     *  - the REPAINT is edge-triggered on the module's sequence number, so a frame in which nothing
     *    arrived costs nothing. Writing the same string into a UIActionText every frame still walks
     *    the widget.
     */
    override void Update()
    {
        super.Update();

        if ( !m_Module )
            return;

        if ( !IsVisible() )
            return;

        int now = GetGame().GetTime();
        if ( now >= m_NextPollMs )
        {
            m_NextPollMs = now + BR_ADMIN_STATUS_INTERVAL_MS;
            m_Module.RequestStatus();
        }

        int seq = m_Module.GetStatusSeq();
        if ( seq == m_PaintedSeq )
            return;

        m_PaintedSeq = seq;
        Repaint( m_Module.GetStatus() );
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

        //--- State, as position-in-list rather than an index: the list is built dynamically (spawn
        //--- selection is conditional, the rounds are a loop over num_zones), so a bare number means
        //--- nothing without the total.
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
