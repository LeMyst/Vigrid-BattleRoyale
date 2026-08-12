#ifndef SERVER
/**
 *  Battle Royale - the death screen.
 *
 *  A real UIScriptedMenu rather than UIManager.ScreenFadeIn. The engine fade is a proto native call
 *  taking one string and two colours - it has no widget tree at all, so it cannot host the two
 *  buttons. This menu therefore draws its own dark-red full-screen backdrop and replaces the fade
 *  entirely on this path.
 *
 *  FOCUS ACCOUNTING - the easiest thing here to get wrong. Input focus is an ADDITIVE counter:
 *
 *    SimulateDeath        -> DayZPlayerImplement.LockControls(true)   +1 per device, cursor on
 *    this menu OnShow     -> super -> UIScriptedMenu.LockControls()   +1 per device, cursor on
 *    this menu OnHide     -> super -> UIScriptedMenu.UnlockControls() -1 per device
 *    EnterSpectate        -> one explicit release                     -1 per device, cursor off
 *                                                                     ------------------------
 *                                                                     net 0
 *
 *  So this class must add NO focus calls of its own - super handles its own half exactly. Note the
 *  mod's LeaderboardMenu does add ChangeGameFocus(1) / ResetGameFocus() on top of super; do not copy
 *  that here. ResetGameFocus zeroes the counter rather than decrementing it, which would eat the
 *  acquire that SimulateDeath still owns.
 */
class DeathScreenMenu extends UIScriptedMenu
{
    /**
     *  Self-reference, so the per-frame Tick() is reachable without going through
     *  UIManager.FindMenu().
     *
     *  FindMenu walks GetMenu()/GetParentMenu(), so it returns NULL the moment anything nulls the
     *  engine's current-menu pointer - which vanilla's inventory teardown used to do on every death
     *  (see the modded InventoryMenu). That made the countdown and the cursor re-assert silently
     *  stop even though the screen was still on the workspace and visibly open. The clobber itself
     *  is fixed at source; this makes the death screen not depend on that staying fixed.
     */
    private static DeathScreenMenu s_Instance;

    static DeathScreenMenu GetInstance()
    {
        return s_Instance;
    }

    private TextWidget m_PlacementText;
    private TextWidget m_FlavourText;
    private TextWidget m_StatusText;
    private ButtonWidget m_SpectateButton;
    private ButtonWidget m_QuitButton;

    //--- Admin only. Respawns into a non-participant body and opens the admin camera, in one press.
    //--- Shown purely on BattleRoyaleRPC.is_admin - the server re-checks admins_steamid64 when the
    //--- RPC lands, so a client that forces this button visible achieves nothing but a rejection.
    private ButtonWidget m_AdminButton;

    //--- Edge tracking for the admin button, exactly like m_SpectateShown below and for the same
    //--- reason: calling Show() every frame eats the click.
    private int m_AdminShown;

    private bool m_Requested;

    //--- Last value pushed to the button, so Show() is only called on a CHANGE. Calling Show() every
    //--- frame re-lays-out the widget between the mouse going down and coming back up, which eats
    //--- the click - that is why the buttons felt unresponsive and the cursor appeared to flicker.
    private int m_SpectateShown;

    //--- Local countdown to the server's automatic entry, so the player can see it coming rather
    //--- than being teleported into a camera mid-decision. Cosmetic: the server owns the real
    //--- deadline, this just mirrors BR_SPECTATE_ENTRY_DELAY_MS from when the offer arrived.
    private int m_AutoEnterAtMs;

    //--- SetText is only called when the whole second changes, for the same reason as m_SpectateShown.
    private int m_LastSecondShown;

    //--- The deadline is armed once and never moved again.
    private bool m_AutoEnterArmed;

    //--- The OTHER deadline: quit to the main menu on the player's behalf, running only while NO
    //--- spectate offer stands. Unlike m_AutoEnterAtMs this one is re-armed on every transition into
    //--- that state, so a player who watches an offer get withdrawn still gets the full window.
    private int m_QuitAtMs;
    private bool m_QuitArmed;

    void DeathScreenMenu()
    {
        m_Requested = false;
        m_SpectateShown = -1;
        m_AdminShown = -1;
        m_AutoEnterAtMs = 0;
        m_LastSecondShown = -1;
        m_AutoEnterArmed = false;
        m_QuitAtMs = 0;
        m_QuitArmed = false;
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/death_screen.layout");

        //--- Guard BEFORE the FindAnyWidget calls below, not after them.
        if( !layoutRoot )
        {
            BattleRoyaleUtils.Warn("[Spectate] death screen: CreateWidgets returned NULL");
            return NULL;
        }

        m_PlacementText = TextWidget.Cast( layoutRoot.FindAnyWidget("PlacementText") );
        m_FlavourText = TextWidget.Cast( layoutRoot.FindAnyWidget("FlavourText") );
        m_StatusText = TextWidget.Cast( layoutRoot.FindAnyWidget("StatusText") );
        m_SpectateButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("SpectateButton") );
        m_QuitButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("QuitButton") );
        m_AdminButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("AdminButton") );

        //--- Hidden until Tick decides otherwise, so an ordinary player never sees it flash on the
        //--- frame the menu opens.
        if( m_AdminButton )
            m_AdminButton.Show( false );

        //--- Which widgets resolved. Kept because a layout that loads but renames a widget fails
        //--- silently and invisibly - the screen just quietly stops doing one of its jobs.
        BattleRoyaleUtils.Trace(string.Format("[Spectate] death screen Init: placement=%1 flavour=%2 status=%3 spectate=%4 quit=%5",
            m_PlacementText != NULL, m_FlavourText != NULL, m_StatusText != NULL, m_SpectateButton != NULL, m_QuitButton != NULL));

        ApplyText();

        s_Instance = this;

        return layoutRoot;
    }

    /**
     *  MANDATORY override.
     *
     *  The default is true, which makes UIScriptedMenu.OnShow subscribe to the local player's
     *  GetOnDeathStart invoker and Close() this menu when it fires. This menu opens DURING the death
     *  sequence, so leaving it true can close the menu on the same frame it opens.
     */
    override bool IsHandlingPlayerDeathEvent()
    {
        return false;
    }

    /**
     *  The two death lines come from BattleRoyaleRPC, written by DayZPlayerImplement.ShowDeadScreen
     *  just before it opens this menu.
     *
     *  Not passed in by a method call, because ShowDeadScreen compiles in 4_World and this class is
     *  5_Mission - an earlier stage cannot name a later one. BattleRoyaleRPC is 3_Game, so both
     *  sides can reach it. (They are plain local strings there, never sent over the wire.)
     */
    protected void ApplyText()
    {
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc )
            return;

        if( m_PlacementText )
            m_PlacementText.SetText( br_rpc.dead_placement );

        if( m_FlavourText )
            m_FlavourText.SetText( br_rpc.dead_flavour );
    }

    override void OnShow()
    {
        //--- super does the whole focus acquire. Nothing else here - see the class comment.
        super.OnShow();

        SetFocus( layoutRoot );

        //--- Force the cursor on. super.OnShow() -> LockControls() already asks for it, but under
        //--- FEATURE_CURSOR (defined in this build) that early-returns when the menu was created
        //--- hidden, and the death sequence is still running around us - SimulateDeath's own
        //--- LockControls, HideInventory and the OnCommandDeathStart invoker all fire in the same
        //--- few frames and any of them can leave the cursor off.
        //--- Safe to call unconditionally: ShowUICursor is NOT the additive ChangeGameFocus counter,
        //--- so this cannot unbalance the focus accounting described on the class.
        GetGame().GetUIManager().ShowUICursor( true );

        BattleRoyaleUtils.Trace(string.Format("[Spectate] death screen OnShow: hidden=%1 cursor=%2",
            IsCreatedHidden(), GetGame().GetUIManager().IsCursorVisible()));
    }

    override void OnHide()
    {
        //--- super does the whole focus release.
        super.OnHide();

        if( s_Instance == this )
            s_Instance = NULL;
    }

    /**
     *  Per-frame work. Driven by BattleRoyaleClient.Update(), NOT by UIScriptedMenu.Update().
     *
     *  The engine calls UIScriptedMenu.Update() exactly ONCE for this menu - proven by logging the
     *  first five frames and only ever seeing frame 1. That is why the countdown froze and why the
     *  cursor, once something took it back, was never re-asserted. BattleRoyaleClient.Update() runs
     *  unconditionally every frame from MissionGameplay.OnUpdate, outside vanilla's
     *  m_LifeState == ALIVE gate, which is exactly what a dead player's screen needs.
     */
    void Tick()
    {
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc )
            return;

        //--- Spectating started (either because the player pressed the button, or because the server
        //--- timed out and started it anyway). The screen has done its job.
        if( br_rpc.spectate_active )
        {
            Close();
            return;
        }

        //--- A LIVING player has no business looking at the death screen. This is the admin respawn
        //--- landing: the server made a fresh body and selected it, and if entering the camera then
        //--- failed for any reason, nothing else here would close the screen - the quit countdown
        //--- would run and drop a perfectly alive admin back to the main menu.
        //---
        //--- Stated as an invariant rather than keyed to the respawn path, so it holds for any
        //--- future route back to a body without needing to know about this one. Note GetPlayer()
        //--- does NOT go NULL while spectating - it hands back the corpse - so IsAlive is the test,
        //--- not existence.
        PlayerBase local_player = PlayerBase.Cast( GetGame().GetPlayer() );
        if( local_player && local_player.IsAlive() )
        {
            BattleRoyaleUtils.Trace("[Spectate] death screen: player is alive again, closing");
            Close();
            return;
        }

        //--- Only the SPECTATE button is conditional - it appears once the server has said this
        //--- player is dead and eligible, and hides again once they have asked. Quit is always
        //--- available: with spectate_enabled false the server never sends an offer at all, and
        //--- hiding the whole row would leave the player on a dead screen with no way out.
        //---
        //--- `offered` is the server's standing offer, `offer` is whether to still show the button.
        //--- They part company once the player has pressed it - and, separately, the offer can be
        //--- WITHDRAWN under us: EndSpectate clears it when the match ends, which is what stops a
        //--- player who died moments before the last survivor being shown a button nobody will
        //--- honour.
        bool offered = br_rpc.spectate_offered;
        bool offer = offered && !m_Requested;

        int offer_state = 0;
        if( offer )
            offer_state = 1;

        //--- Re-assert the cursor if something took it back. The rest of the death sequence runs in
        //--- the frames AFTER this menu opens - HideInventory and the OnCommandDeathStart invoker
        //--- both fire later - so a one-shot ShowUICursor in OnShow is not enough on its own.
        //--- Guarded on IsCursorVisible so this is a no-op in the normal case rather than a
        //--- per-frame engine call.
        if( !GetGame().GetUIManager().IsCursorVisible() )
            GetGame().GetUIManager().ShowUICursor( true );

        //--- Edge-triggered. See m_SpectateShown.
        if( offer_state != m_SpectateShown )
        {
            m_SpectateShown = offer_state;

            if( m_SpectateButton )
                m_SpectateButton.Show( offer );
        }

        //--- The admin route out of the death screen: respawn into a non-participant body and open
        //--- the admin camera. Shown independently of the spectate offer, because the two are
        //--- separately configured - admin_spectate_enabled and spectate_enabled - and an admin on a
        //--- server where players cannot spectate should still get their own tools. Same edge
        //--- tracking as above, for the same click-eating reason.
        int admin_state = 0;
        if( br_rpc.is_admin && !m_Requested )
            admin_state = 1;

        if( admin_state != m_AdminShown )
        {
            m_AdminShown = admin_state;

            if( m_AdminButton )
                m_AdminButton.Show( admin_state == 1 );
        }

        //--- The deadline is armed EXACTLY ONCE and never recomputed. Deriving it inside the edge
        //--- block meant any re-fire of that edge pushed it forward again, which pins the countdown
        //--- at its starting value forever - the reported "countdown didn't decrease".
        if( offer && !m_AutoEnterArmed )
        {
            m_AutoEnterArmed = true;
            m_AutoEnterAtMs = GetGame().GetTime() + BR_SPECTATE_ENTRY_DELAY_MS;
        }

        //--- With no offer standing there is nothing to wait for and one button left, so count down
        //--- to pressing it. Without this the screen is a dead end wherever spectating is not on
        //--- offer - which includes the DEFAULT configuration, spectate_enabled false - where the
        //--- pre-spectate death path used an unconditional 15 s CallLater(LeaveServer).
        //--- Re-armed rather than armed-once, so an offer that is withdrawn mid-countdown starts a
        //--- fresh window instead of quitting on whatever was left of the old one.
        //--- "Is there anything on this screen still worth waiting for." The spectate offer is one
        //--- such thing; a standing ADMIN button is another, and forgetting the second one meant an
        //--- admin reading the screen for fifteen seconds got quit to the main menu out from under
        //--- a button they were about to press. Seen live 2026-08-11 with spectate_enabled off,
        //--- which is the configuration where the quit countdown is the only one running.
        bool pending_action = offered || (admin_state == 1);

        if( pending_action && m_QuitArmed )
        {
            m_QuitArmed = false;
            m_LastSecondShown = -1;
        }
        else if( !pending_action && !m_QuitArmed )
        {
            m_QuitArmed = true;
            m_QuitAtMs = GetGame().GetTime() + BR_DEAD_AUTO_QUIT_MS;
            m_LastSecondShown = -1;
        }

        UpdateStatus( offered );

        if( m_QuitArmed && GetGame().GetTime() >= m_QuitAtMs )
        {
            BattleRoyaleUtils.Trace("[Spectate] death screen: no offer, quitting to menu");
            QuitToMenu();
        }
    }

    /**
     *  The line under the flavour text. Which countdown it runs depends on whether an offer stands:
     *
     *    offer standing, not pressed  ->  "Spectating in N..."          (the SERVER enters at its own
     *                                                                    deadline; we only mirror it)
     *    offer standing, pressed      ->  "Entering spectator mode..."  (set once, in OnClick)
     *    no offer                     ->  "Returning to menu in N..."   (THIS screen quits at m_QuitAtMs)
     */
    protected void UpdateStatus(bool offered)
    {
        if( !m_StatusText )
            return;

        //--- Neither countdown is running: no spectate offer, and the quit clock is held off because
        //--- an admin button is standing. Blank rather than fall through - m_QuitAtMs is stale in
        //--- that state and would render a frozen or negative "Returning to menu in N...".
        if( !offered && !m_QuitArmed )
        {
            m_StatusText.SetText( "" );
            m_LastSecondShown = -1;
            return;
        }

        int deadline_ms = m_QuitAtMs;
        string status_key = "#STR_BR_DEAD_QUIT_IN";
        string status_label = "quit";

        if( offered )
        {
            //--- Already pressed: OnClick set the text once and there is nothing left to count -
            //--- the server acts the moment it receives the request.
            //--- Deliberately below the `offered` test rather than above it, so a WITHDRAWN offer
            //--- falls through to the quit countdown and replaces "Entering spectator mode...".
            //--- Left on screen, that line is a promise nothing is going to keep.
            if( m_Requested )
                return;

            deadline_ms = m_AutoEnterAtMs;
            status_key = "#STR_BR_DEAD_AUTO_IN";
            status_label = "spectate";
        }

        int remaining_ms = deadline_ms - GetGame().GetTime();
        if( remaining_ms < 0 )
            remaining_ms = 0;

        int seconds = remaining_ms / 1000;
        if( seconds == m_LastSecondShown )
            return;

        m_LastSecondShown = seconds;

        string countdown = Widget.TranslateString( status_key );
        m_StatusText.SetText( string.Format(countdown, seconds) );

        //--- Once per second, whichever countdown is running. Removing this in an earlier cleanup
        //--- was a mistake: it is the only thing that distinguishes "Tick is not running" from "Tick
        //--- runs but something else is wrong", and both look identical on screen.
        BattleRoyaleUtils.Trace("[Spectate] death screen tick: " + status_label + " " + seconds + "s, cursor=" + GetGame().GetUIManager().IsCursorVisible());
    }

    //! Close and leave. Shared by the Quit button and by the no-offer deadline above.
    protected void QuitToMenu()
    {
        Close();
        GetGame().GetMission().AbortMission();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        //--- Logged BEFORE the widget comparison, deliberately. This separates three failures that
        //--- are indistinguishable from the player's chair: the click never reaching the menu at all
        //--- (no line), reaching it but landing on the wrong widget (line with an unexpected name),
        //--- and reaching the right widget but the handler misbehaving (line then nothing after).
        string clicked = "<null>";
        if( w )
            clicked = w.GetName();

        BattleRoyaleUtils.Trace("[Spectate] death screen OnClick: " + clicked);

        if( w == m_SpectateButton )
        {
            m_Requested = true;

            if( m_SpectateButton )
                m_SpectateButton.Show( false );

            if( m_StatusText )
                m_StatusText.SetText( "#STR_BR_DEAD_ENTERING" );

            //--- No payload: the server resolves the actor from the RPC sender, so there is nothing
            //--- here a client could aim at somebody else.
            GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestSpectate", NULL, true );
            return true;
        }

        if( w == m_AdminButton )
        {
            //--- Same message F3 sends. The server reads the press against this player's actual
            //--- situation - dead, so it respawns them and opens the camera in one step - which is
            //--- why there is no separate "respawn" RPC for the button to send.
            //---
            //--- m_Requested latches so the button hides and the status line stops offering. The
            //--- screen itself is closed by Tick() the moment spectate_active arrives; if the server
            //--- refuses, the quit countdown takes over as it would for any withdrawn offer.
            m_Requested = true;

            if( m_AdminButton )
                m_AdminButton.Show( false );
            if( m_SpectateButton )
                m_SpectateButton.Show( false );

            if( m_StatusText )
                m_StatusText.SetText( "#STR_BR_DEAD_ENTERING" );

            GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "AdminSpectateToggle", NULL, true );
            return true;
        }

        if( w == m_QuitButton )
        {
            BattleRoyaleUtils.Trace("[Spectate] death screen: quit to menu");
            QuitToMenu();
            return true;
        }

        return super.OnClick(w, x, y, button);
    }
}
#endif
