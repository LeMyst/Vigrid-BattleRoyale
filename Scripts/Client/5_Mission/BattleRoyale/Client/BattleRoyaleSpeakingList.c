#ifndef SERVER
/**
 *  Vigrid Battle Royale - "who is currently speaking" panel.
 *
 *  Direct voice in DayZ is anonymous: you hear someone nearby and have no idea who it is. In the
 *  lobby, where voice is deliberately left open, that makes abuse untraceable. This panel names
 *  whoever you can hear, for as long as you can hear them.
 *
 *  It is a pure renderer. The list arrives from the server via the SetSpeakingPlayers RPC and is
 *  parked on BattleRoyaleRPC; nothing is computed here.
 *
 *  That split is forced, not stylistic. DayZPlayer.IsPlayerSpeaking() is per-entity on the server,
 *  but on a client it returns the LOCAL microphone level whichever entity it is called on - while
 *  one player spoke, their own client reported the same amplitude for themselves and for a silent
 *  bystander, and the listening client reported zero for everyone. So a client cannot determine
 *  who is speaking, and the server has to tell it. Measured 2026-08-04; see BattleRoyaleConstants.c.
 *
 *  Rows are pooled and positioned manually, matching VigridPartyHud, so the panel behaves the same
 *  regardless of how many rows are live.
 */
class BattleRoyaleSpeakingList
{
    //--- These sit on a translucent dark backdrop over an arbitrary map, so both are kept at full
    //--- brightness. The self colour was 0xFF6ECF6E, a mid-tone green that read as dark against the
    //--- lighter parts of the spawn map.
    private static const int COLOR_OTHER = 0xFFFFFFFF;
    private static const int COLOR_SELF = 0xFF8CFF8C;

    private Widget m_Root;
    private Widget m_Rows;
    private bool m_RootFailed;

    private ref array<Widget> m_RowWidgets;
    private ref array<TextWidget> m_RowNames;

    //--- Last sequence rendered, so the widget tree is only touched when the server actually
    //--- pushed something new.
    private int m_RenderedSeq;

    void BattleRoyaleSpeakingList()
    {
        m_RowWidgets = new array<Widget>();
        m_RowNames = new array<TextWidget>();
        m_RenderedSeq = -1;
    }

    void ~BattleRoyaleSpeakingList()
    {
        if (m_Root)
            m_Root.Unlink();
    }

    /**
     *  Widgets are built on first use rather than in the constructor - the same reasoning as
     *  VigridPartyHud: the only widget creation proven to work in this mod happens after the
     *  mission is fully up. The failure latch stops a broken layout path spamming the log every
     *  frame for the whole session.
     */
    private bool EnsureRoot()
    {
        if (m_Root)
            return true;
        if (m_RootFailed)
            return false;

        m_Root = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/speaking_players.layout");
        if (!m_Root)
        {
            m_RootFailed = true;
            BattleRoyaleUtils.Warn("BattleRoyaleSpeakingList: could not create speaking_players.layout - panel disabled");
            return false;
        }

        m_Rows = m_Root.FindAnyWidget("SpeakingRows");
        m_Root.Show(false);
        BattleRoyaleUtils.Debug("BattleRoyaleSpeakingList: layout ready");
        return true;
    }

    private void EnsureCapacity(int wanted)
    {
        if (!m_Rows)
            return;

        while (m_RowWidgets.Count() < wanted)
        {
            Widget row = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/hud/speaking_players_row.layout", m_Rows);
            if (!row)
                return;

            row.SetPos(0, m_RowWidgets.Count() * BR_SPEAKING_ROW_HEIGHT);

            m_RowWidgets.Insert(row);
            m_RowNames.Insert(TextWidget.Cast(row.FindAnyWidget("SpeakingRowName")));
        }
    }

    /**
     *  Called every frame from BattleRoyaleClient.Update.
     */
    void Update(bool enabled)
    {
        if (!enabled)
        {
            if (m_Root)
                m_Root.Show(false);

            //--- Force a redraw when it comes back, rather than trusting the stale sequence.
            m_RenderedSeq = -1;
            return;
        }

        if (!EnsureRoot())
            return;
        if (!m_Rows)
            return;

        BattleRoyaleRPC br_rpc = BattleRoyaleRPC.GetInstance();
        if (!br_rpc)
            return;

        if (br_rpc.speaking_seq == m_RenderedSeq)
            return;

        m_RenderedSeq = br_rpc.speaking_seq;
        Render(br_rpc.speaking_names, br_rpc.speaking_self_index);
    }

    private void Render(array<string> names, int self_index)
    {
        int wanted = names.Count();
        if (wanted > BR_SPEAKING_MAX_ROWS)
            wanted = BR_SPEAKING_MAX_ROWS;

        EnsureCapacity(wanted);

        m_Root.Show(wanted > 0);

        //--- Edge-triggered: only runs when the server pushed a different set. Reports the row
        //--- count actually created, so a layout that failed to spawn rows is distinguishable from
        //--- a push that never arrived.
        BattleRoyaleUtils.Debug(string.Format("BattleRoyaleSpeakingList: rendering %1 row(s), pooled=%2, root=%3", wanted, m_RowWidgets.Count(), m_Root != NULL));

        for (int i = 0; i < wanted; i++)
        {
            if (i >= m_RowWidgets.Count())
                break;

            Widget row = m_RowWidgets.Get(i);
            row.Show(true);
            row.SetPos(0, i * BR_SPEAKING_ROW_HEIGHT);

            m_RowNames.Get(i).SetText(names.Get(i));

            //--- Your own row is coloured differently: it doubles as a hot-mic indicator.
            if (i == self_index)
                m_RowNames.Get(i).SetColor(COLOR_SELF);
            else
                m_RowNames.Get(i).SetColor(COLOR_OTHER);
        }

        for (int j = wanted; j < m_RowWidgets.Count(); j++)
        {
            m_RowWidgets.Get(j).Show(false);
        }
    }
}
#endif
