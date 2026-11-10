#ifndef SERVER
/**
 *  The win screen: placement, title, the winning squad, the winner's own figures and what happens
 *  next. Painted once from BattleRoyaleRPC's win_* fields; Tick() only runs the status countdown
 *  and places the two rules beside the placement line.
 *
 *  Parented to the WORKSPACE, which outlives the mission, so the owner must call Destroy().
 */
class BattleRoyaleWinScreen
{
    protected Widget m_Root;
    protected Widget m_Content;
    protected TextWidget m_PlaceText;
    protected Widget m_LineLeft;
    protected Widget m_LineRight;
    protected TextWidget m_SubtitleText;
    protected Widget m_StatsGrid;
    protected TextWidget m_StatusText;

    protected int m_LastSecondShown;

    //--- Last rule geometry applied, in real pixels, so SetPos/SetSize only run on a change.
    protected float m_LineX;
    protected float m_LineY;
    protected float m_LineW;

    void BattleRoyaleWinScreen()
    {
        m_LastSecondShown = -1;
        m_LineX = -1;
        m_LineY = -1;
        m_LineW = -1;
    }

    void ~BattleRoyaleWinScreen()
    {
        Destroy();
    }

    bool Create()
    {
        m_Root = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/win_screen.layout");
        if ( !m_Root )
        {
            BattleRoyaleUtils.Warn("[Win] could not create win_screen.layout");
            return false;
        }

        m_Content = m_Root.FindAnyWidget("Content");
        m_PlaceText = TextWidget.Cast( m_Root.FindAnyWidget("PlaceText") );
        m_LineLeft = m_Root.FindAnyWidget("PlaceLineLeft");
        m_LineRight = m_Root.FindAnyWidget("PlaceLineRight");
        m_SubtitleText = TextWidget.Cast( m_Root.FindAnyWidget("SubtitleText") );
        m_StatsGrid = m_Root.FindAnyWidget("StatsGrid");
        m_StatusText = TextWidget.Cast( m_Root.FindAnyWidget("StatusText") );

        //--- A renamed widget fails silently; this line is the only way to tell.
        bool has_lines = false;
        if ( m_LineLeft && m_LineRight )
            has_lines = true;

        BattleRoyaleUtils.Trace(string.Format("[Win] Init: content=%1 place=%2 lines=%3 subtitle=%4 stats=%5 status=%6",
            m_Content != NULL, m_PlaceText != NULL, has_lines, m_SubtitleText != NULL, m_StatsGrid != NULL, m_StatusText != NULL));

        Apply();
        m_Root.Show( true );
        return true;
    }

    void Destroy()
    {
        if ( m_Root )
            m_Root.Unlink();

        m_Root = NULL;
    }

    protected void Apply()
    {
        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        bool valid = false;
        if ( br_rpc )
            valid = br_rpc.win_valid;

        if ( m_StatsGrid )
            m_StatsGrid.Show( valid );

        if ( !valid )
        {
            if ( m_PlaceText )
                m_PlaceText.SetText( "" );
            if ( m_SubtitleText )
                m_SubtitleText.SetText( "" );
            return;
        }

        if ( m_PlaceText )
        {
            string place = "";
            if ( br_rpc.win_field_size > 0 )
            {
                string place_key = "#STR_BR_LASTMATCH_PLACE_PLAYERS";
                if ( br_rpc.win_grouped )
                    place_key = "#STR_BR_LASTMATCH_PLACE_SQUADS";

                place = string.Format( Widget.TranslateString( place_key ), 1, br_rpc.win_field_size );
            }

            m_PlaceText.SetText( place );
        }

        if ( m_SubtitleText )
        {
            string subtitle = "#STR_BR_WIN_LAST_SOLO";
            if ( br_rpc.win_grouped )
            {
                subtitle = "#STR_BR_WIN_LAST_SQUAD";
                if ( br_rpc.win_squad_names != "" )
                    subtitle = subtitle + " - " + br_rpc.win_squad_names;
            }

            m_SubtitleText.SetText( subtitle );
        }

        SetStat( 0, br_rpc.win_kills.ToString() );
        SetStat( 1, br_rpc.win_damage.ToString() );
        SetStat( 2, br_rpc.win_hits.ToString() );
        SetStat( 3, BattleRoyaleRecapText.FormatDuration( br_rpc.win_survived_s ) );
    }

    protected void SetStat(int index, string value)
    {
        TextWidget w = TextWidget.Cast( m_Root.FindAnyWidget("StatValue" + index) );
        if ( w )
            w.SetText( value );
    }

    void Tick()
    {
        if ( !m_Root )
            return;

        UpdateStatus();
        PlaceRules();
    }

    protected void UpdateStatus()
    {
        if ( !m_StatusText )
            return;

        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if ( !br_rpc )
            return;

        if ( br_rpc.win_next_match )
        {
            if ( m_LastSecondShown != -2 )
            {
                m_LastSecondShown = -2;
                m_StatusText.SetText( "#STR_BR_DEAD_WAIT_NEXT_MATCH" );
            }
            return;
        }

        if ( br_rpc.win_kick_deadline_ms <= 0 )
            return;

        int remaining_ms = br_rpc.win_kick_deadline_ms - GetGame().GetTime();
        if ( remaining_ms < 0 )
            remaining_ms = 0;

        //--- Rounded up, so the line reads 15 on arrival and 1 in its final second.
        int seconds = (remaining_ms + 999) / 1000;
        if ( seconds == m_LastSecondShown )
            return;

        m_LastSecondShown = seconds;
        m_StatusText.SetText( string.Format( Widget.TranslateString("#STR_BR_DEAD_QUIT_IN"), seconds ) );
    }

    //! Rules either side of the placement text, from its MEASURED width: localized strings differ
    //! too much for a fixed offset. Re-measured every frame because GetTextSize lags a SetText.
    protected void PlaceRules()
    {
        if ( !m_Content || !m_PlaceText || !m_LineLeft || !m_LineRight )
            return;

        int text_w = 0;
        int text_h = 0;
        m_PlaceText.GetTextSize( text_w, text_h );

        if ( text_w <= 0 )
        {
            m_LineLeft.Show( false );
            m_LineRight.Show( false );
            return;
        }

        float content_x;
        float content_y;
        float content_w;
        float content_h;
        float place_x;
        float place_y;
        float place_w;
        float place_h;
        m_Content.GetScreenPos( content_x, content_y );
        m_Content.GetScreenSize( content_w, content_h );
        m_PlaceText.GetScreenPos( place_x, place_y );
        m_PlaceText.GetScreenSize( place_w, place_h );

        //--- Design lengths in 1920 units, scaled the way declared geometry is.
        float scale = content_w / 1800.0;
        float line_w = 60.0 * scale;
        float line_h = Math.Max( 1.0, 2.0 * scale );
        float gap = 21.0 * scale;

        float left_x = (content_w * 0.5) - (text_w * 0.5) - gap - line_w;
        float line_y = (place_y - content_y) + (place_h * 0.5) - (line_h * 0.5);

        if ( left_x == m_LineX && line_y == m_LineY && line_w == m_LineW )
            return;

        m_LineX = left_x;
        m_LineY = line_y;
        m_LineW = line_w;

        float right_x = content_w - left_x - line_w;

        m_LineLeft.SetPos( left_x, line_y );
        m_LineLeft.SetSize( line_w, line_h );
        m_LineRight.SetPos( right_x, line_y );
        m_LineRight.SetSize( line_w, line_h );
        m_LineLeft.Show( true );
        m_LineRight.Show( true );
    }
}
#endif
