#ifndef SERVER
/**
 *  Toasts - this mod's own notification widget, and what replaced ExpansionNotification.
 *
 *  Newest on top, stacked downward from BR_TOAST_TOP_PX, horizontally centred. The widget pool is
 *  fixed at BR_TOAST_MAX_ROWS and never grows or reorders; a new toast shifts the model list and the
 *  rows are re-bound from it, so a burst of notifications causes no widget churn.
 *
 *  Rows are re-laid-out only when the model changed, when a row is mid-fade, or when the viewport
 *  moved. Measuring wrapped text forces a reflow and a stack spends most of its life either empty or
 *  static.
 *
 *  THE STACK IS PLACED IN REAL SCREEN PIXELS, MEASURED OFF THE ROOT. A widget's DECLARED position and
 *  size are scaled by viewport/1920 while SetPos and SetSize are not, so the two must never be mixed
 *  - the root is the full-screen `size 1 1` / `hexactsize 0` frame whose GetScreenSize() reports true
 *  pixels, and everything under it is positioned from here. Mixing them shifts the whole group while
 *  its internal spacing stays perfect, which does not read as a coordinate bug at all.
 */
class BattleRoyaleToasts
{
    private Widget m_Root;
    private bool m_RootFailed;

    //--- Newest first. Never longer than BR_TOAST_MAX_ROWS.
    private ref array<ref BattleRoyaleToastRow> m_Model;

    private ref array<Widget> m_RowWidgets;
    private ref array<MultilineTextWidget> m_RowTexts;
    private ref array<Widget> m_RowBackdrops;
    private ref array<Widget> m_RowAccents;

    //--- Last viewport the stack was laid out against. A resolution change moves every row and
    //--- nothing in the model would otherwise notice.
    private float m_LastScreenW;
    private float m_LastScreenH;

    //--- Frames of geometry logging still owed, armed whenever a row is bound to a new toast.
    //--- A toast is on screen for seconds and repaints every frame while it fades, so logging
    //--- unconditionally would bury the log; logging only at bind time would miss a row that is
    //--- laid out correctly on frame 1 and wrong on frame 2. A short burst catches both.
    private int m_DiagFrames;

    void BattleRoyaleToasts()
    {
        m_Model = new array<ref BattleRoyaleToastRow>();

        m_RowWidgets = new array<Widget>();
        m_RowTexts = new array<MultilineTextWidget>();
        m_RowBackdrops = new array<Widget>();
        m_RowAccents = new array<Widget>();
    }

    void ~BattleRoyaleToasts()
    {
        //--- The root is parented to the WORKSPACE, which outlives the mission, so nothing else will
        //--- ever take it down. Same reasoning as BattleRoyaleClient's win screen.
        if ( m_Root )
        {
            m_Root.Unlink();
            m_Root = NULL;
        }
    }

    /**
     *  Raise a toast directly, without going near the wire.
     *
     *  Used by BattleRoyaleClient.NotifyLocal for the handful of notifications this client raises for
     *  itself. Everything the SERVER decides arrives through the RPC queue instead - see Drain().
     */
    void Push( string message, float seconds )
    {
        if ( message == "" )
            return;

        Add( new BattleRoyaleToast( message, seconds ) );
    }

    void Update( float timeslice )
    {
        if ( !EnsureRoot() )
            return;

        bool dirty = Drain();

        if ( Expire() )
            dirty = true;

        //--- ViewportMoved() is called unconditionally and FIRST, because it also refreshes the probe
        //--- it tests against. Short-circuited behind the other two, a frame that repainted for some
        //--- other reason would leave the probe stale, this would latch true, and the whole stack
        //--- would silently re-measure at frame rate - the trap the map's transform probe documents.
        bool moved = ViewportMoved();

        if ( dirty || moved || IsFading() )
            Refresh();
    }

    //! Drop every live toast. Used on mission teardown and when the match hands over.
    void Clear()
    {
        m_Model.Clear();

        if ( m_Root )
            Refresh();
    }

    /**
     *  Widgets are created on the first Update rather than in the constructor.
     *
     *  The constructor runs inside MissionGameplay.OnInit, and the only thing proven to work at that
     *  point is what the host mod does - it builds its HUD after super.OnInit() has returned.
     *  Deferring to the first frame means the mission is fully up before any layout is parsed, and it
     *  costs one boolean test per call.
     */
    private bool EnsureRoot()
    {
        if ( m_Root )
            return true;
        if ( m_RootFailed )
            return false;

        BattleRoyaleUtils.Debug( "[Toasts] Creating toast layout" );
        m_Root = GetGame().GetWorkspace().CreateWidgets( "Vigrid-BattleRoyale/GUI/layouts/hud/toasts.layout" );

        if ( !m_Root )
        {
            //--- Latch the failure: retrying every frame would spam the log for the whole session.
            //--- Warn rather than Error - BattleRoyaleUtils.Error raises a VM exception and unwinds,
            //--- and a missing notification widget must not take the client's Update loop with it.
            m_RootFailed = true;
            BattleRoyaleUtils.Warn( "[Toasts] Could not create toasts.layout - notifications disabled" );
            return false;
        }

        BuildRows();
        m_Root.Show( false );
        BattleRoyaleUtils.Debug( "[Toasts] Toast layout ready" );
        return true;
    }

    private void BuildRows()
    {
        for ( int i = 0; i < BR_TOAST_MAX_ROWS; i++ )
        {
            Widget row = GetGame().GetWorkspace().CreateWidgets( "Vigrid-BattleRoyale/GUI/layouts/hud/toast_row.layout", m_Root );
            if ( !row )
            {
                BattleRoyaleUtils.Warn( "[Toasts] Could not create toast_row.layout" );
                return;
            }

            row.Show( false );

            MultilineTextWidget text = MultilineTextWidget.Cast( row.FindAnyWidget( "ToastText" ) );
            Widget backdrop = row.FindAnyWidget( "ToastBackdrop" );
            Widget accent = row.FindAnyWidget( "ToastAccent" );

            //--- Say so LOUDLY when a child does not resolve. Every consumer below is null-guarded,
            //--- so a renamed widget degrades to a toast that is silently blank or unpainted rather
            //--- than to an error - and "the layout loaded fine" is exactly the wrong thing to
            //--- conclude from a clean log in that case. The cast is separate from the find because
            //--- a widget that exists under the wrong CLASS fails the cast and nothing else.
            //--- Built in steps rather than as one expression: a single concatenation of about ten
            //--- terms is rejected outright with "Formula too complex", and it is a hard COMPILE
            //--- error that packing does not catch - it only surfaces when the module loads.
            if ( !text || !backdrop || !accent )
            {
                string missing = "";
                if ( !text )
                    missing = missing + " ToastText";
                if ( !backdrop )
                    missing = missing + " ToastBackdrop";
                if ( !accent )
                    missing = missing + " ToastAccent";

                BattleRoyaleUtils.Warn( "[Toasts] toast_row.layout row " + i.ToString() + " missing:" + missing );
            }

            m_RowWidgets.Insert( row );
            m_RowTexts.Insert( text );
            m_RowBackdrops.Insert( backdrop );
            m_RowAccents.Insert( accent );
        }
    }

    //! Move everything the RPC layer has received into the model. Returns true if anything moved.
    private bool Drain()
    {
        BattleRoyaleRPC rpc = BattleRoyaleRPC.GetInstance();
        if ( !rpc )
            return false;

        int count = rpc.pending_toasts.Count();
        if ( count == 0 )
            return false;

        for ( int i = 0; i < count; i++ )
        {
            //--- Read the element into a local before it is passed anywhere. An array read sharing an
            //--- expression with a call has been measured in this codebase to return an entry from a
            //--- DIFFERENT array, silently.
            BattleRoyaleToast queued = rpc.pending_toasts.Get( i );
            Add( queued );
        }

        rpc.pending_toasts.Clear();
        return true;
    }

    private void Add( BattleRoyaleToast toast )
    {
        if ( !toast )
            return;

        m_Model.InsertAt( new BattleRoyaleToastRow( toast ), 0 );

        //--- Oldest toasts fall off the bottom of the stack.
        while ( m_Model.Count() > BR_TOAST_MAX_ROWS )
            m_Model.Remove( m_Model.Count() - 1 );
    }

    //! Drop toasts whose time is up. Returns true if anything was dropped.
    private bool Expire()
    {
        bool changed = false;
        int now = GetGame().GetTime();

        for ( int i = m_Model.Count() - 1; i >= 0; i-- )
        {
            BattleRoyaleToastRow row = m_Model.Get( i );
            if ( row && row.expires_at > now )
                continue;

            m_Model.Remove( i );
            changed = true;
        }

        return changed;
    }

    //! Is any live toast currently inside one of its fades, and therefore repainting every frame?
    private bool IsFading()
    {
        int now = GetGame().GetTime();
        int count = m_Model.Count();

        for ( int i = 0; i < count; i++ )
        {
            BattleRoyaleToastRow row = m_Model.Get( i );
            if ( row && row.IsFading( now ) )
                return true;
        }

        return false;
    }

    //! Has the viewport moved since the last pass? Also refreshes the probe it tests against, so it
    //! must be called every frame - see the note in Update().
    private bool ViewportMoved()
    {
        float w;
        float h;
        m_Root.GetScreenSize( w, h );

        bool moved = ( w != m_LastScreenW ) || ( h != m_LastScreenH );

        m_LastScreenW = w;
        m_LastScreenH = h;

        return moved;
    }

    private void Refresh()
    {
        int model_count = m_Model.Count();
        int row_count = m_RowWidgets.Count();
        int now = GetGame().GetTime();

        float screen_w;
        float screen_h;
        m_Root.GetScreenSize( screen_w, screen_h );

        float left = ( screen_w - BR_TOAST_WIDTH_PX ) / 2;

        //--- Stack downward from the top inset, each row starting below the one above it. The step is
        //--- accumulated rather than `index * height` because a wrapped message is taller than a
        //--- short one and no two rows are guaranteed the same height.
        int y = BR_TOAST_TOP_PX;

        for ( int i = 0; i < row_count; i++ )
        {
            Widget row = m_RowWidgets.Get( i );
            if ( !row )
                continue;

            if ( i >= model_count )
            {
                row.Show( false );
                continue;
            }

            //--- Show before binding: GetScreenSize measures a laid-out widget, and one that is still
            //--- hidden measures as zero. Vanilla's own measure sites do the same (sizetochild.c:35,
            //--- actiontargetscursor.c:1148-1151).
            row.Show( true );

            BattleRoyaleToastRow model = m_Model.Get( i );

            int height = Bind( i, model );

            float alpha = model.AlphaAt( now );
            row.SetPos( left, y );
            row.SetAlpha( alpha );

            if ( m_DiagFrames > 0 )
                DiagRow( i, row, alpha );

            y = y + height + BR_TOAST_GAP_PX;
        }

        m_Root.Show( model_count > 0 );

        if ( m_DiagFrames > 0 )
        {
            m_DiagFrames = m_DiagFrames - 1;
            BattleRoyaleUtils.Debug( "[Toasts] root shown=" + ( model_count > 0 ).ToString() + " screen=" + screen_w.ToString() + "x" + screen_h.ToString() + " left=" + left.ToString() );
        }
    }

    /**
     *  Dump one row's real geometry.
     *
     *  Everything here is read back with GetScreenPos / GetScreenSize rather than echoing what was
     *  set, because the whole class of bug this is chasing is "the value was set and the widget did
     *  something else with it". Echoing the input would confirm only that the code ran.
     */
    private void DiagRow( int index, Widget row, float alpha )
    {
        float rx;
        float ry;
        float rw;
        float rh;
        row.GetScreenPos( rx, ry );
        row.GetScreenSize( rw, rh );

        string line = "[Toasts] row " + index.ToString();
        line = line + " pos " + rx.ToString() + "," + ry.ToString();
        line = line + " size " + rw.ToString() + "x" + rh.ToString();
        line = line + " alpha " + alpha.ToString();
        BattleRoyaleUtils.Debug( line );

        MultilineTextWidget text = m_RowTexts.Get( index );
        if ( !text )
            return;

        float tx;
        float ty;
        float tw;
        float th;
        text.GetScreenPos( tx, ty );
        text.GetScreenSize( tw, th );

        string tline = "[Toasts]   text pos " + tx.ToString() + "," + ty.ToString();
        tline = tline + " size " + tw.ToString() + "x" + th.ToString();
        tline = tline + " vis " + text.IsVisible().ToString();
        tline = tline + " alpha " + text.GetAlpha().ToString();
        BattleRoyaleUtils.Debug( tline );
    }

    /**
     *  Height of `message` in pixels, when nothing can be measured.
     *
     *  Only reached if GetTextSize answers zero. Estimates the unwrapped run from the character
     *  count and a mean glyph advance, divides by the wrap width for a line count, and multiplies
     *  by a nominal line height. Crude for a proportional face, but it only has to land within a
     *  line, and rounding up costs padding where rounding down would clip the message.
     */
    private int FallbackHeight( string message, int wrap_w )
    {
        float run_w = message.Length() * BR_TOAST_AVG_CHAR_PX;

        float usable = wrap_w * BR_TOAST_WRAP_PACKING;
        if ( usable < 1 )
            usable = 1;

        int lines = Math.Ceil( run_w / usable );
        if ( lines < 1 )
            lines = 1;

        return lines * BR_TOAST_LINE_H_PX;
    }

    /**
     *  Bind one row and size it to its own text. Returns the row's height in real screen pixels.
     *
     *  The text is only re-set when the row is bound to a toast it is not already showing. SetText on
     *  a wrapping widget forces a reflow, and a stack repainting purely to advance a fade must not
     *  pay for that on every frame of it.
     */
    private int Bind( int index, BattleRoyaleToastRow model )
    {
        Widget row = m_RowWidgets.Get( index );
        MultilineTextWidget text = m_RowTexts.Get( index );

        if ( !row || !text || !model || !model.toast )
            return BR_TOAST_MIN_HEIGHT_PX;

        int text_w = BR_TOAST_WIDTH_PX - ( 2 * BR_TOAST_PAD_X );

        if ( !model.bound )
        {
            //--- Width BEFORE text: the wrap is what decides the height, so a height read back before
            //--- the widget knows how wide it may be is a height for the wrong line count.
            text.SetSize( text_w, BR_TOAST_MIN_HEIGHT_PX );
            text.SetText( model.toast.text );

            model.bound = true;
            m_DiagFrames = BR_TOAST_DIAG_FRAMES;

            BattleRoyaleUtils.Debug( "[Toasts] bind " + index.ToString() + " w=" + text_w.ToString() + " text=" + model.toast.text );
        }

        //--- Update() before the read-back, every time. A widget that was only just shown has a stale
        //--- layout and measures as zero; TabberUI.c:126 and sizetochild.c:35 both do this.
        text.Update();

        //--- GetTextSize ON A WRAPPING WIDGET ALREADY RETURNS THE WRAPPED SIZE - width of the longest
        //--- line, and the FULL height of the wrapped block. Measured: STR_BR_UNSTUCK_INFORMATION
        //--- reports 301x66 at a 528px wrap width, and 66 is exactly three 22px lines. So the widget
        //--- knows its own wrapped layout perfectly; what it will not do is SIZE ITSELF to it, which
        //--- is the whole reason "size to text v" had to go.
        //---
        //--- This replaced a line-counting pass that divided the measured width by the wrap width.
        //--- That was redundant, and worse, only correct by accident: it computed lines=1 and then
        //--- multiplied by a "line height" that was already the full block height. The moment a
        //--- message was long enough to compute lines=2 - the French unstuck text is the obvious
        //--- candidate - it would have doubled the plate to 194px.
        int measured_w;
        int measured_h;
        text.GetTextSize( measured_w, measured_h );

        int content_h = measured_h;
        if ( content_h <= 0 )
            content_h = FallbackHeight( model.toast.text, text_w );

        //--- Cap it. A message long enough to fill the screen is a bug somewhere upstream, and a
        //--- toast is not the place to find that out.
        int max_h = BR_TOAST_MAX_LINES * BR_TOAST_LINE_H_PX;
        if ( content_h > max_h )
            content_h = max_h;

        int height = content_h + ( 2 * BR_TOAST_PAD_Y );
        if ( height < BR_TOAST_MIN_HEIGHT_PX )
            height = BR_TOAST_MIN_HEIGHT_PX;

        if ( m_DiagFrames > 0 )
        {
            string mline = "[Toasts] measure " + index.ToString();
            mline = mline + " textsize " + measured_w.ToString() + "x" + measured_h.ToString();
            mline = mline + " content " + content_h.ToString();
            mline = mline + " height " + height.ToString();
            BattleRoyaleUtils.Debug( mline );
        }

        text.SetPos( BR_TOAST_PAD_X, BR_TOAST_PAD_Y );
        text.SetSize( text_w, height - ( 2 * BR_TOAST_PAD_Y ) );

        row.SetSize( BR_TOAST_WIDTH_PX, height );

        Widget backdrop = m_RowBackdrops.Get( index );
        if ( backdrop )
        {
            backdrop.SetPos( 0, 0 );
            backdrop.SetSize( BR_TOAST_WIDTH_PX, height );
        }

        Widget accent = m_RowAccents.Get( index );
        if ( accent )
        {
            accent.SetPos( 0, 0 );
            accent.SetSize( BR_TOAST_ACCENT_PX, height );
        }

        return height;
    }
}
#endif
