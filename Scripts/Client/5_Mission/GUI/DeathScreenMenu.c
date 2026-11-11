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
 *  So this class must add NO focus calls of its own - super handles its own half exactly.
 *
 *  ResetGameFocus() is the specific thing never to reach for: it SETS the counter to zero across
 *  every device rather than decrementing it, so it releases every other holder's acquire and not
 *  just the caller's. LeaderboardMenu used to call it and that was reachable from this screen -
 *  F4 over the death screen, twice, zeroed the counter while this menu was still up, and the
 *  Spectate button then drove it negative. Every menu in the mod now takes the line this class
 *  does and adds no focus call at all: LeaderboardMenu, VigridPartyMenu and SpawnSelectionMenu
 *  used to add an extra ALL-DEVICE ChangeGameFocus(1), which froze the whole input layer while
 *  they were up - Esc and their own toggle keys included.
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
    private TextWidget m_FieldText;
    private TextWidget m_FlavourText;
    private TextWidget m_StatusText;

    //--- The killer card. Hidden until the server's recap arrives, which is structurally after the
    //--- menu opens (see ApplyRecap).
    private Widget m_Column;
    private Widget m_KillerCard;
    private Widget m_KillerCardInner;
    private TextWidget m_KillerLabel;
    private TextWidget m_KillerName;
    private Widget m_WeaponChip;
    private TextWidget m_WeaponText;
    private Widget m_DistanceChip;
    private TextWidget m_DistanceText;
    private TextWidget m_DetailLeft;
    private TextWidget m_DetailRight;
    private Widget m_BarBack;
    private Widget m_BarHealth;
    private Widget m_BarDealt;
    private Widget m_StatsGrid;
    private Widget m_ButtonRow;
    private Widget m_SpectateBg;
    private Widget m_SpectateProgress;
    private Widget m_QuitBg;
    private Widget m_AdminBg;

    //--- Card state the per-frame layout reads: does the card carry the detail line and bar.
    private bool m_CardHasDetail;
    private bool m_CardHasChips;
    private int m_BarHealthPct;
    private int m_BarDealtPct;

    //--- Last stats sequence painted, same edge rule as m_RecapSeq.
    private int m_StatsSeq;

    //--- Set whenever something that moves widgets changed; Relayout() runs on the next Tick.
    private bool m_LayoutDirty;
    private int m_LayoutFrames;

    //--- Last recap sequence painted. -1 so the first arrival always counts as a change.
    private int m_RecapSeq;
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

    //--- Focus alone leaves mouse look live, so the corpse camera turned behind the menu.
    private ref BattleRoyaleMenuInputLock m_InputLock;

    void DeathScreenMenu()
    {
        m_InputLock = new BattleRoyaleMenuInputLock();
        m_Requested = false;
        m_SpectateShown = -1;
        m_AdminShown = -1;
        m_AutoEnterAtMs = 0;
        m_LastSecondShown = -1;
        m_AutoEnterArmed = false;
        m_QuitAtMs = 0;
        m_QuitArmed = false;
        m_RecapSeq = -1;
        m_StatsSeq = -1;
        m_LayoutDirty = true;
        m_CardHasDetail = true;
        m_CardHasChips = false;
        m_BarHealthPct = 0;
        m_BarDealtPct = 0;
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
        m_FieldText = TextWidget.Cast( layoutRoot.FindAnyWidget("FieldText") );
        m_FlavourText = TextWidget.Cast( layoutRoot.FindAnyWidget("FlavourText") );
        m_StatusText = TextWidget.Cast( layoutRoot.FindAnyWidget("StatusText") );
        m_SpectateButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("SpectateButton") );
        m_QuitButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("QuitButton") );
        m_AdminButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("AdminButton") );

        m_Column = layoutRoot.FindAnyWidget("Column");
        m_KillerCard = layoutRoot.FindAnyWidget("KillerCard");
        m_KillerCardInner = layoutRoot.FindAnyWidget("KillerCardInner");
        m_KillerLabel = TextWidget.Cast( layoutRoot.FindAnyWidget("KillerLabel") );
        m_KillerName = TextWidget.Cast( layoutRoot.FindAnyWidget("KillerName") );
        m_WeaponChip = layoutRoot.FindAnyWidget("WeaponChip");
        m_WeaponText = TextWidget.Cast( layoutRoot.FindAnyWidget("WeaponChipText") );
        m_DistanceChip = layoutRoot.FindAnyWidget("DistanceChip");
        m_DistanceText = TextWidget.Cast( layoutRoot.FindAnyWidget("DistanceChipText") );
        m_DetailLeft = TextWidget.Cast( layoutRoot.FindAnyWidget("DetailLeft") );
        m_DetailRight = TextWidget.Cast( layoutRoot.FindAnyWidget("DetailRight") );
        m_BarBack = layoutRoot.FindAnyWidget("BarBack");
        m_BarHealth = layoutRoot.FindAnyWidget("BarHealth");
        m_BarDealt = layoutRoot.FindAnyWidget("BarDealt");
        m_StatsGrid = layoutRoot.FindAnyWidget("StatsGrid");
        m_ButtonRow = layoutRoot.FindAnyWidget("ButtonRow");
        m_SpectateBg = layoutRoot.FindAnyWidget("SpectateBg");
        m_SpectateProgress = layoutRoot.FindAnyWidget("SpectateProgress");
        m_QuitBg = layoutRoot.FindAnyWidget("QuitButtonBg");
        m_AdminBg = layoutRoot.FindAnyWidget("AdminButtonBg");

        //--- Hidden until Tick decides otherwise, so an ordinary player never sees it flash on the
        //--- frame the menu opens.
        if( m_AdminButton )
            m_AdminButton.Show( false );

        //--- Which widgets resolved. Kept because a layout that loads but renames a widget fails
        //--- silently and invisibly - the screen just quietly stops doing one of its jobs.
        BattleRoyaleUtils.Trace(string.Format("[Spectate] death screen Init: placement=%1 flavour=%2 status=%3 spectate=%4 quit=%5 card=%6 stats=%7 row=%8",
            m_PlacementText != NULL, m_FlavourText != NULL, m_StatusText != NULL, m_SpectateButton != NULL, m_QuitButton != NULL, m_KillerCardInner != NULL, m_StatsGrid != NULL, m_ButtonRow != NULL));

        ApplyText();
        ApplyStats();

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

        //--- The big number. Never "#7": a TextWidget swallows every '#'.
        string place = "";
        if( br_rpc.dead_place > 0 )
            place = br_rpc.dead_place.ToString();

        if( m_PlacementText )
            m_PlacementText.SetText( place );

        if( m_FlavourText )
            m_FlavourText.SetText( br_rpc.dead_flavour );

        ApplyField();
    }

    //! "of 24" / "of 24 squads" beside the number. Needs the server's field size, so it is re-run
    //! when SetDeathStats lands.
    protected void ApplyField()
    {
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc || !m_FieldText )
            return;

        string field = "";
        if( br_rpc.dead_place > 0 && br_rpc.death_field_size > 0 )
        {
            string field_key = "#STR_BR_DEAD_OF_PLAYERS";
            if( br_rpc.death_grouped )
                field_key = "#STR_BR_DEAD_OF_SQUADS";

            field = string.Format( Widget.TranslateString( field_key ), br_rpc.death_field_size );
        }

        m_FieldText.SetText( field );
        m_LayoutDirty = true;
    }

    //! The stat strip. "-" until the server's SetDeathStats arrives.
    protected void ApplyStats()
    {
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc )
            return;
        if( br_rpc.death_stats_seq == m_StatsSeq )
            return;

        m_StatsSeq = br_rpc.death_stats_seq;

        string kills = "-";
        string damage = "-";
        string alive = "-";
        if( br_rpc.death_stats_valid )
        {
            kills = br_rpc.death_kills.ToString();
            damage = br_rpc.death_damage.ToString();
            alive = BattleRoyaleRecapText.FormatDuration( br_rpc.death_survived_s );
        }

        SetStat( 0, kills );
        SetStat( 1, damage );
        SetStat( 2, alive );

        ApplyField();
    }

    protected void SetStat(int index, string value)
    {
        TextWidget w = TextWidget.Cast( layoutRoot.FindAnyWidget("StatValue" + index) );
        if( w )
            w.SetText( value );
    }

    /**
     *  The death recap - who killed you, with what, at what range.
     *
     *  Called from Tick() rather than Init(), and that is structural rather than defensive: the two
     *  lines above are written LOCALLY by ShowDeadScreen on the same frame this menu opens, while the
     *  recap is pushed by the SERVER from OnPlayerKilled. Two clocks with a round trip between them,
     *  so the menu always opens first.
     *
     *  Edge-triggered on the sequence counter, matching m_SpectateShown and m_LastSecondShown - and
     *  for the same reason: re-laying-out a widget every frame is what eats clicks on this screen.
     */
    protected void ApplyRecap()
    {
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if( !br_rpc )
            return;
        if( !m_KillerCard || !m_KillerName || !m_KillerLabel )
            return;
        if( !br_rpc.recap_valid )
            return;
        if( br_rpc.recap_seq == m_RecapSeq )
            return;

        m_RecapSeq = br_rpc.recap_seq;

        int cause = br_rpc.recap_cause;
        string killer = br_rpc.recap_killer_name;
        string weapon = BattleRoyaleRecapText.WeaponDisplayName( br_rpc.recap_weapon_type );

        //--- A named killer: "Eliminated by" over the name, weapon and range as chips. Anything else
        //--- (zone, environment, infected, unknown): the cause over the recap sentence, no chips.
        if( killer != "" )
        {
            m_KillerLabel.SetText( "#STR_BR_DEAD_ELIMINATED_BY" );
            m_KillerName.SetText( killer );
        }
        else
        {
            m_KillerLabel.SetText( BattleRoyaleRecapText.CauseLabel( cause ) );
            m_KillerName.SetText( BattleRoyaleRecapText.BuildRecapLine( cause, killer, br_rpc.recap_weapon_type, br_rpc.recap_distance_m ) );
            weapon = "";
        }

        bool has_weapon = ( weapon != "" );
        bool has_distance = ( killer != "" && br_rpc.recap_distance_m >= 0 );

        if( m_WeaponText )
            m_WeaponText.SetText( weapon );
        if( m_DistanceText )
            m_DistanceText.SetText( br_rpc.recap_distance_m.ToString() + " m" );
        if( m_WeaponChip )
            m_WeaponChip.Show( has_weapon );
        if( m_DistanceChip )
            m_DistanceChip.Show( has_distance );

        m_CardHasChips = has_weapon || has_distance;

        //--- Detail line and bar: only for a killer whose state is known.
        int health = br_rpc.recap_killer_health_pct;
        int dealt = br_rpc.recap_damage_to_killer;
        m_CardHasDetail = ( killer != "" && (health >= 0 || dealt > 0) );

        string left = "";
        string right = "";
        if( health >= 0 )
            left = string.Format( Widget.TranslateString("#STR_BR_DEAD_RECAP_HEALTH"), health );
        if( dealt > 0 )
            right = string.Format( Widget.TranslateString("#STR_BR_DEAD_RECAP_DEALT"), dealt );

        if( m_DetailLeft )
            m_DetailLeft.SetText( left );
        if( m_DetailRight )
            m_DetailRight.SetText( right );

        //--- The bar: what they had left, then what you had taken off them, as shares of one
        //--- full player (100). Dealt is a match total and can exceed what remains, so it is clamped.
        m_BarHealthPct = health;
        if( m_BarHealthPct < 0 )
            m_BarHealthPct = 0;
        if( m_BarHealthPct > 100 )
            m_BarHealthPct = 100;

        m_BarDealtPct = dealt;
        if( m_BarDealtPct < 0 )
            m_BarDealtPct = 0;
        if( m_BarDealtPct > 100 - m_BarHealthPct )
            m_BarDealtPct = 100 - m_BarHealthPct;

        m_KillerCard.Show( true );
        m_LayoutDirty = true;

        string line = "[Spectate] death screen recap: cause=" + cause;
        line = line + " killer=" + killer;
        line = line + " weapon=" + weapon;
        line = line + " detail=" + m_CardHasDetail;
        BattleRoyaleUtils.Trace( line );
    }

    //------------------------------------------------------------------------------------------
    //--- Layout. Script works in real pixels (SetPos/SetSize), the layout in 1920 reference
    //--- units, so every design length is multiplied by the column's measured scale.
    //------------------------------------------------------------------------------------------

    protected float ColumnScale()
    {
        if( !m_Column )
            return 1.0;

        float w;
        float h;
        m_Column.GetScreenSize( w, h );
        if( w <= 0 )
            return 1.0;

        return w / 780.0;
    }

    //! Everything that moves: the card's height, the blocks below it, the chips, the bar, the button
    //! row and the "of N" beside the number. Cheap, but only run when something changed.
    protected void Relayout()
    {
        float s = ColumnScale();

        //--- The card is 183 units with its detail line and bar, 114 without; the blocks below
        //--- follow it up. Reserved at full height until the recap arrives.
        float card_h = 183;
        if( m_KillerCard && m_KillerCard.IsVisible() && !m_CardHasDetail )
            card_h = 114;

        if( m_KillerCard )
            m_KillerCard.SetSize( 780 * s, card_h * s );
        if( m_KillerCardInner )
            m_KillerCardInner.SetSize( 778 * s, (card_h - 2) * s );

        bool show_detail = m_CardHasDetail;
        if( m_DetailLeft )
            m_DetailLeft.Show( show_detail );
        if( m_DetailRight )
            m_DetailRight.Show( show_detail );
        if( m_BarBack )
            m_BarBack.Show( show_detail );
        if( m_BarHealth )
        {
            m_BarHealth.Show( show_detail );
            m_BarHealth.SetPos( 29 * s, 143 * s );
            m_BarHealth.SetSize( 7.2 * m_BarHealthPct * s, 12 * s );
        }
        if( m_BarDealt )
        {
            m_BarDealt.Show( show_detail );
            m_BarDealt.SetPos( (29 + 7.2 * m_BarHealthPct) * s, 143 * s );
            m_BarDealt.SetSize( 7.2 * m_BarDealtPct * s, 12 * s );
        }

        float below = 306 + card_h + 42;
        if( m_StatsGrid )
            m_StatsGrid.SetPos( 0, below * s );
        if( m_ButtonRow )
            m_ButtonRow.SetPos( 0, (below + 104 + 42) * s );
        if( m_StatusText )
            m_StatusText.SetPos( 0, (below + 104 + 42 + 78 + 15) * s );

        LayoutChips( s );
        LayoutButtons( s );
        LayoutField( s );
    }

    //! Chips sized to their text, right-aligned in the card; the name stops short of them.
    protected void LayoutChips(float s)
    {
        float right = 749 * s;
        float pad = 15 * s;
        float gap = 9 * s;
        float chip_h = 33 * s;
        float chip_y = (26 + 30) * s - chip_h * 0.5;

        if( m_DistanceChip && m_DistanceChip.IsVisible() )
            right = PlaceChip( m_DistanceChip, m_DistanceText, right, chip_y, chip_h, pad ) - gap;

        if( m_WeaponChip && m_WeaponChip.IsVisible() )
            right = PlaceChip( m_WeaponChip, m_WeaponText, right, chip_y, chip_h, pad ) - gap;

        if( m_KillerName )
        {
            float name_w = right - 107 * s;
            if( !m_CardHasChips )
                name_w = 640 * s;

            m_KillerName.SetSize( name_w, 36 * s );
        }
    }

    //! Returns the chip's left edge.
    protected float PlaceChip(Widget chip, TextWidget label, float right, float y, float h, float pad)
    {
        int tw = 0;
        int th = 0;
        if( label )
            label.GetTextSize( tw, th );

        float w = tw + pad * 2;
        float x = right - w;

        chip.SetPos( x, y );
        chip.SetSize( w, h );
        if( label )
        {
            label.SetPos( 0, 0 );
            label.SetSize( w, h );
        }

        return x;
    }

    //! Spectate takes the width the others leave; Admin and Quit are fixed. Hidden ones take none.
    protected void LayoutButtons(float s)
    {
        float gap = 15 * s;
        float fixed_w = 240 * s;
        float row_w = 780 * s;
        float x = 0;

        int fixed_count = 1;
        if( m_AdminButton && m_AdminButton.IsVisible() )
            fixed_count++;

        if( m_SpectateButton && m_SpectateButton.IsVisible() )
        {
            float spectate_w = row_w - fixed_count * (fixed_w + gap);
            m_SpectateButton.SetPos( x, 0 );
            m_SpectateButton.SetSize( spectate_w, 78 * s );
            if( m_SpectateBg )
                m_SpectateBg.SetSize( spectate_w, 78 * s );

            TextWidget spectate_label = TextWidget.Cast( layoutRoot.FindAnyWidget("SpectateLabel") );
            if( spectate_label )
                spectate_label.SetSize( spectate_w, 78 * s );

            x = x + spectate_w + gap;
        }

        if( m_AdminButton && m_AdminButton.IsVisible() )
        {
            m_AdminButton.SetPos( x, 0 );
            x = x + fixed_w + gap;
        }

        if( m_QuitButton )
            m_QuitButton.SetPos( x, 0 );
    }

    //! "of 24" just after the big number, from the number's measured width.
    protected void LayoutField(float s)
    {
        if( !m_PlacementText || !m_FieldText )
            return;

        int tw = 0;
        int th = 0;
        m_PlacementText.GetTextSize( tw, th );

        m_FieldText.SetPos( tw + 21 * s, 126 * s );
    }

    //! The strip at the foot of the Spectate button: what is left of the automatic entry countdown.
    protected void UpdateProgress()
    {
        if( !m_SpectateProgress || !m_SpectateButton )
            return;

        float frac = 0;
        if( m_AutoEnterArmed && !m_Requested )
        {
            frac = (m_AutoEnterAtMs - GetGame().GetTime()) / (float)BR_SPECTATE_ENTRY_DELAY_MS;
            frac = Math.Clamp( frac, 0, 1 );
        }

        float bw;
        float bh;
        m_SpectateButton.GetScreenSize( bw, bh );
        float strip_h = 6 * ColumnScale();

        m_SpectateProgress.SetPos( 0, bh - strip_h );
        m_SpectateProgress.SetSize( bw * frac, strip_h );
    }

    //! Hover: the primary brightens, the outlined ones gain a faint fill.
    override bool OnMouseEnter(Widget w, int x, int y)
    {
        SetHover( w, true );
        return super.OnMouseEnter( w, x, y );
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        SetHover( w, false );
        return super.OnMouseLeave( w, enterW, x, y );
    }

    protected void SetHover(Widget w, bool on)
    {
        if( w == m_SpectateButton && m_SpectateBg )
        {
            if( on )
                m_SpectateBg.SetColor( ARGB(255, 240, 184, 74) );
            else
                m_SpectateBg.SetColor( ARGB(255, 217, 158, 41) );
            return;
        }

        int fill = 0;
        if( on )
            fill = 15;

        if( w == m_QuitButton && m_QuitBg )
            m_QuitBg.SetColor( ARGB(fill, 255, 255, 255) );

        if( w == m_AdminButton && m_AdminBg )
            m_AdminBg.SetColor( ARGB(fill, 255, 255, 255) );
    }

    override void OnShow()
    {
        //--- super does the whole focus acquire. Nothing else here - see the class comment.
        super.OnShow();
        m_InputLock.Take();

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
        m_InputLock.Release();

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
        //--- The one exemption: the diag menu's "Open Death Screen", which exists so the layout can
        //--- be looked at offline - where SERVER is undefined, nothing can kill you, and this guard
        //--- would otherwise shut the screen on its first tick and read as a failed layout.
        bool alive_close = true;
#ifdef DIAG_DEVELOPER
        alive_close = !BattleRoyaleDiag.suppress_alive_close;
#endif

        PlayerBase local_player = PlayerBase.Cast( GetGame().GetPlayer() );
        if( alive_close && local_player && local_player.IsAlive() )
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
        //--- Above the button work, so a recap that lands while the player is reading gets painted
        //--- even on a frame that changes nothing else.
        ApplyRecap();
        ApplyStats();

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

            m_LayoutDirty = true;
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

            m_LayoutDirty = true;
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
        //--- And a third: the next match. With the in-place reset keeping dead players
        //--- (in_place_reset_keep_dead_players) the server hands this player a lobby body when the
        //--- match ends, so the screen waits for it rather than quitting. Quit stays available.
        bool pending_action = offered || (admin_state == 1) || br_rpc.wait_next_match;

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

        //--- Relayout only after a change, then for a few frames more: GetTextSize lags a SetText,
        //--- and moving the buttons every frame would eat clicks like Show() does.
        if( m_LayoutDirty )
        {
            m_LayoutDirty = false;
            m_LayoutFrames = 4;
        }
        if( m_LayoutFrames > 0 )
        {
            m_LayoutFrames--;
            Relayout();
        }

        UpdateProgress();

        //--- The diag open also suppresses the quit countdown: with no spectate offer standing it
        //--- would abort the mission fifteen seconds into looking at the layout.
        if( !alive_close )
            return;

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
            //--- Held off for the next match (in-place reset) rather than for an admin button: say so.
            BattleRoyaleRPC wait_rpc = BattleRoyaleRPC.GetInstance();
            if( wait_rpc && wait_rpc.wait_next_match )
                m_StatusText.SetText( "#STR_BR_DEAD_WAIT_NEXT_MATCH" );
            else
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
            m_LayoutDirty = true;

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
            m_LayoutDirty = true;

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
