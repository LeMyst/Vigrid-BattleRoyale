#ifndef SERVER
/**
 *  Battle Royale - the leaderboard screen.
 *
 *  Two independent ladders behind a tab: Solo and Group. They are never merged, because a win as
 *  one of a four-stack and a win alone against the same field are not the same achievement - see
 *  BattleRoyaleLeaderboardData for the full scoring model.
 *
 *  Each ladder is fetched from the server at most once and then answered from the client-side cache
 *  on BattleRoyaleRPC. That makes tab switching instant and, more importantly, correct: the server
 *  rate-limits leaderboard requests per player, so re-fetching on every click meant a quick
 *  solo->group->solo sequence got silently refused and the list drew empty. A ladder only changes
 *  when a match ends, and the server restarts its process between matches, so the cache never
 *  outlives the data it holds.
 *
 *  Rows are pooled and positioned manually, and surplus rows are hidden rather than destroyed.
 */
class LeaderboardMenu extends UIScriptedMenu
{
    //--- Only used to sanity-check the scroll content height in the trace below. The rows are
    //--- positioned by the WrapSpacer, not by this.
    private static const int ROW_HEIGHT = 26;

    //--- Only used while a ladder has never arrived. Once cached, nothing is ever requested again.
    private static const int REQUEST_RETRY_MS = 1000;

    private Widget m_Rows;
    private ScrollWidget m_Scroll;
    private TextWidget m_TitleText;
    private TextWidget m_SeasonText;
    private TextWidget m_SelfText;
    private TextWidget m_EmptyText;
    private ButtonWidget m_SoloTabButton;
    private ButtonWidget m_GroupTabButton;
    private Widget m_SoloTabAccent;
    private Widget m_GroupTabAccent;
    private ButtonWidget m_CloseButton;

    private ref array<Widget> m_RowPool;

    private int m_Board;
    private int m_LastSeq;
    private int m_RequestDueMs;

    void LeaderboardMenu()
    {
        m_RowPool = new array<Widget>();
        m_Board = BR_LEADERBOARD_BOARD_SOLO;
        m_LastSeq = -1;
        m_RequestDueMs = 0;
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/leaderboard.layout");

        m_Rows = layoutRoot.FindAnyWidget("LeaderboardRows");
        m_Scroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("LeaderboardScroll"));
        m_TitleText = TextWidget.Cast(layoutRoot.FindAnyWidget("Title"));
        m_SeasonText = TextWidget.Cast(layoutRoot.FindAnyWidget("SeasonText"));
        m_SelfText = TextWidget.Cast(layoutRoot.FindAnyWidget("SelfText"));
        m_EmptyText = TextWidget.Cast(layoutRoot.FindAnyWidget("EmptyText"));
        m_SoloTabButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SoloTabButton"));
        m_GroupTabButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("GroupTabButton"));
        m_SoloTabAccent = layoutRoot.FindAnyWidget("SoloTabAccent");
        m_GroupTabAccent = layoutRoot.FindAnyWidget("GroupTabAccent");
        m_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));

        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();

        SetFocus(layoutRoot);
        GetGame().GetInput().ChangeGameFocus(1);
        GetGame().GetUIManager().ShowUICursor(true);

        //--- Repaint from whatever is cached, immediately. -1 forces it even though the sequence
        //--- number has not moved while the menu was closed.
        m_LastSeq = -1;
        m_RequestDueMs = 0;
    }

    override void OnHide()
    {
        super.OnHide();

        //--- Both of these are mandatory. Skipping ResetGameFocus leaves the player unable to move.
        GetGame().GetInput().ResetGameFocus();
        GetGame().GetUIManager().ShowUICursor(false);
    }

    private BattleRoyaleClient GetClient()
    {
        return BattleRoyaleClient.Cast( GetBR() );
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);

        BattleRoyaleRPC rpc = BattleRoyaleRPC.GetInstance();
        BattleRoyaleLeaderboardBoard active = rpc.GetLeaderboardBoard(m_Board);
        int now_ms = GetGame().GetTime();

        //--- Ask only until this ladder has arrived once. The retry exists purely to ride out the
        //--- server's per-player cooldown, which silently drops a request that comes too soon after
        //--- the previous one.
        if (!active.valid && now_ms >= m_RequestDueMs)
        {
            m_RequestDueMs = now_ms + REQUEST_RETRY_MS;

            BattleRoyaleClient client = GetClient();
            if (client)
                client.RequestLeaderboard(m_Board);
        }

        if (rpc.leaderboard_seq == m_LastSeq)
            return;

        m_LastSeq = rpc.leaderboard_seq;
        RefreshRows(rpc, active);
    }

    private Widget EnsureRow(int index)
    {
        while (m_RowPool.Count() <= index)
        {
            Widget created = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/leaderboard_row.layout", m_Rows);
            if (!created)
                return null;

            m_RowPool.Insert(created);
        }

        //--- No SetPos here on purpose. LeaderboardRows is a WrapSpacerWidget, which lays its own
        //--- children out; positioning them by hand fights it and was why the list would not scroll.
        Widget row = m_RowPool.Get(index);
        row.Show(true);
        return row;
    }

    private void HideFrom(int first)
    {
        for (int i = first; i < m_RowPool.Count(); i++)
        {
            m_RowPool.Get(i).Show(false);
        }
    }

    private void RefreshRows(BattleRoyaleRPC rpc, BattleRoyaleLeaderboardBoard active)
    {
        RefreshTabs();

        StringLocaliser season_label = new StringLocaliser("STR_BR_LEADERBOARD_SEASON", rpc.lb_season.ToString());
        m_SeasonText.SetText(season_label.Format());

        int shown = 0;
        int count = active.Count();

        for (int i = 0; i < count; i++)
        {
            Widget row = EnsureRow(shown);
            if (!row)
                break;

            TextWidget rank_widget = TextWidget.Cast(row.FindAnyWidget("RowRank"));
            TextWidget name_widget = TextWidget.Cast(row.FindAnyWidget("RowName"));
            TextWidget matches_widget = TextWidget.Cast(row.FindAnyWidget("RowMatches"));
            TextWidget wins_widget = TextWidget.Cast(row.FindAnyWidget("RowWins"));
            TextWidget kills_widget = TextWidget.Cast(row.FindAnyWidget("RowKills"));
            TextWidget points_widget = TextWidget.Cast(row.FindAnyWidget("RowPoints"));

            int display_rank = i + 1;
            rank_widget.SetText(display_rank.ToString());
            name_widget.SetText(active.names.Get(i));
            matches_widget.SetText(active.matches.Get(i).ToString());
            wins_widget.SetText(active.wins.Get(i).ToString());
            kills_widget.SetText(active.kills.Get(i).ToString());
            points_widget.SetText(active.points.Get(i).ToString());

            shown = shown + 1;
        }

        HideFrom(shown);

        //--- Scrolling is entirely declarative: LeaderboardRows is a WrapSpacerWidget carrying
        //--- "Size To Content V", so it grows with the rows and the ScrollWidget scrolls it. An
        //--- earlier attempt used a plain PanelWidget and SetSize() from here, which silently did
        //--- nothing - the scroll widget kept reporting the viewport height and never scrolled.
        //--- Still traced, because a non-scrolling list looks exactly like a short one: if the
        //--- reported height stays at the viewport height while rows climb, this has regressed.
        if (m_Scroll)
            BattleRoyaleUtils.Trace(string.Format("Leaderboard rows=%1 (expect content height ~%2), scroll reports %3, scrollbar visible %4", shown, shown * ROW_HEIGHT, m_Scroll.GetContentHeight(), m_Scroll.IsScrollbarVisible()));

        //--- "Nothing here yet" and "not fetched yet" look identical on screen but mean opposite
        //--- things, so an unfetched ladder says loading instead of claiming to be empty.
        if (active.valid)
            m_EmptyText.SetText("#STR_BR_LEADERBOARD_EMPTY");
        else
            m_EmptyText.SetText("#STR_BR_LEADERBOARD_LOADING");

        m_EmptyText.Show(shown == 0);

        RefreshSelf(active);
    }

    /**
     *  Show which ladder is on screen.
     *
     *  Deliberately does NOT disable the active tab. Disabling it was the first attempt and it made
     *  the button stop responding to the mouse - including its hover state - which read as the UI
     *  being broken rather than as a selection. Both buttons stay live; the accent bar and the title
     *  carry the state instead.
     */
    private void RefreshTabs()
    {
        bool solo_active = m_Board == BR_LEADERBOARD_BOARD_SOLO;

        m_SoloTabAccent.Show(solo_active);
        m_GroupTabAccent.Show(!solo_active);

        if (solo_active)
            m_TitleText.SetText("#STR_BR_LEADERBOARD_TITLE_SOLO");
        else
            m_TitleText.SetText("#STR_BR_LEADERBOARD_TITLE_GROUP");
    }

    private void RefreshSelf(BattleRoyaleLeaderboardBoard active)
    {
        if (!active.valid)
        {
            m_SelfText.SetText("");
            return;
        }

        if (active.self_rank <= 0)
        {
            m_SelfText.SetText("#STR_BR_LEADERBOARD_YOU_UNRANKED");
            return;
        }

        StringLocaliser self_label = new StringLocaliser("STR_BR_LEADERBOARD_YOU", active.self_rank.ToString(), active.self_wins.ToString(), active.self_points.ToString());
        m_SelfText.SetText(self_label.Format());
    }

    private void SwitchBoard(int board)
    {
        if (m_Board == board)
            return;

        m_Board = board;

        //--- Let the next Update fetch straight away if this ladder has never been loaded.
        m_RequestDueMs = 0;

        //--- Repaint from cache now rather than next frame, so a cached ladder appears on the click
        //--- with no flicker through an empty list.
        BattleRoyaleRPC rpc = BattleRoyaleRPC.GetInstance();
        m_LastSeq = rpc.leaderboard_seq;
        RefreshRows(rpc, rpc.GetLeaderboardBoard(m_Board));
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == m_CloseButton)
        {
            Close();
            return true;
        }

        if (w == m_SoloTabButton)
        {
            SwitchBoard(BR_LEADERBOARD_BOARD_SOLO);
            return true;
        }

        if (w == m_GroupTabButton)
        {
            SwitchBoard(BR_LEADERBOARD_BOARD_GROUP);
            return true;
        }

        return super.OnClick(w, x, y, button);
    }
}
#endif
