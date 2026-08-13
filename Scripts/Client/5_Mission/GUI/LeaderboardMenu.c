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
 *  Rows are pooled, laid out by the WrapSpacer they hang off rather than positioned by hand, and
 *  surplus rows are destroyed rather than hidden - see TrimPool for why the distinction matters.
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
    private ButtonWidget m_LastMatchTabButton;
    private Widget m_SoloTabAccent;
    private Widget m_GroupTabAccent;
    private Widget m_LastMatchTabAccent;
    private ButtonWidget m_CloseButton;

    //--- The two tab bodies. Swapped by Show() alone - see the layout for why nothing here ever
    //--- calls SetPos.
    private Widget m_LadderPanel;
    private Widget m_LastMatchPanel;

    private Widget m_LastMatchRows;
    private ScrollWidget m_LastMatchScroll;
    private TextWidget m_LastMatchEmpty;

    private Widget m_CardStats;
    private Widget m_CardRecap;
    private Widget m_CardSquad;
    private TextWidget m_CardRecapText;
    private TextWidget m_CardPlacementValue;
    private TextWidget m_CardKillsValue;
    private TextWidget m_CardDamageValue;
    private TextWidget m_CardHitsValue;
    private TextWidget m_CardTimeValue;
    private TextWidget m_CardSquadKillsValue;
    private TextWidget m_CardSquadDamageValue;
    private TextWidget m_CardSquadBestValue;
    private TextWidget m_CardSquadMembers;

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

        m_LastMatchTabButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("LastMatchTabButton"));
        m_LastMatchTabAccent = layoutRoot.FindAnyWidget("LastMatchTabAccent");
        m_LadderPanel = layoutRoot.FindAnyWidget("LadderPanel");
        m_LastMatchPanel = layoutRoot.FindAnyWidget("LastMatchPanel");
        m_LastMatchRows = layoutRoot.FindAnyWidget("LastMatchRows");
        m_LastMatchScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("LastMatchScroll"));
        m_LastMatchEmpty = TextWidget.Cast(layoutRoot.FindAnyWidget("LastMatchEmpty"));

        m_CardStats = layoutRoot.FindAnyWidget("CardStats");
        m_CardRecap = layoutRoot.FindAnyWidget("CardRecap");
        m_CardSquad = layoutRoot.FindAnyWidget("CardSquad");
        m_CardRecapText = TextWidget.Cast(layoutRoot.FindAnyWidget("CardRecapText"));
        m_CardPlacementValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardPlacementValue"));
        m_CardKillsValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardKillsValue"));
        m_CardDamageValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardDamageValue"));
        m_CardHitsValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardHitsValue"));
        m_CardTimeValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardTimeValue"));
        m_CardSquadKillsValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardSquadKillsValue"));
        m_CardSquadDamageValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardSquadDamageValue"));
        m_CardSquadBestValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CardSquadBestValue"));
        m_CardSquadMembers = TextWidget.Cast(layoutRoot.FindAnyWidget("CardSquadMembers"));

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

        //--- Open on the previous match when this player actually played it. Someone who has just
        //--- reconnected almost certainly wants their own match rather than the all-time ladder, and
        //--- self_index is already the exact test for "did they play it" - it is -1 for everyone who
        //--- joined after the restart, who correctly land on Solo as before.
        BattleRoyaleRPC rpc = BattleRoyaleRPC.GetInstance();
        if (rpc.last_match.valid && rpc.last_match.self_index >= 0)
            m_Board = BR_LEADERBOARD_BOARD_LASTMATCH;
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
        int now_ms = GetGame().GetTime();
        BattleRoyaleClient client = NULL;

        //--- The Last Match tab is not a ladder: different payload, different cache, different
        //--- request. Branching here rather than inside RefreshRows keeps the two apart entirely.
        if (m_Board == BR_LEADERBOARD_BOARD_LASTMATCH)
        {
            if (!rpc.last_match.valid && now_ms >= m_RequestDueMs)
            {
                m_RequestDueMs = now_ms + REQUEST_RETRY_MS;

                client = GetClient();
                if (client)
                    client.RequestLastMatch();
            }

            if (rpc.last_match_seq == m_LastSeq)
                return;

            m_LastSeq = rpc.last_match_seq;
            RefreshLastMatch(rpc);
            return;
        }

        BattleRoyaleLeaderboardBoard active = rpc.GetLeaderboardBoard(m_Board);

        //--- Ask only until this ladder has arrived once. The retry exists purely to ride out the
        //--- server's per-player cooldown, which silently drops a request that comes too soon after
        //--- the previous one.
        if (!active.valid && now_ms >= m_RequestDueMs)
        {
            m_RequestDueMs = now_ms + REQUEST_RETRY_MS;

            client = GetClient();
            //--- Never RequestLeaderboard(BR_LEADERBOARD_BOARD_LASTMATCH) - the branch above is what
            //--- keeps board 2 out of here. ServeRequest treats anything that is not GROUP as SOLO,
            //--- so it would answer with the solo ladder and burn this player's cooldown doing it.
            if (client)
                client.RequestLeaderboard(m_Board);
        }

        if (rpc.leaderboard_seq == m_LastSeq)
            return;

        m_LastSeq = rpc.leaderboard_seq;
        RefreshRows(rpc, active);
    }

    private Widget EnsureRow(int index, string layout_path, Widget parent)
    {
        while (m_RowPool.Count() <= index)
        {
            Widget created = GetGame().GetWorkspace().CreateWidgets(layout_path, parent);
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

    /**
     *  Destroy every pooled row from `first` on.
     *
     *  Unlink, not Show(false), which is what this used to do. A spacer lays out the children it
     *  HAS, and LeaderboardRows is a WrapSpacer - so a hidden row is still a child holding its slot,
     *  and the scroll range stays as long as the LONGEST ladder ever displayed. That is reachable
     *  here in one click: Group ships fewer entries than Solo, so switching to it left the shorter
     *  list followed by a screen of nothing to scroll through.
     *
     *  Walked backwards so each removal is of the last element and cannot renumber an index still to
     *  be visited. RemoveOrdered rather than Remove, because vanilla's Remove() fills the hole with
     *  the LAST element.
     */
    private void TrimPool(int first)
    {
        for (int i = m_RowPool.Count() - 1; i >= first; i--)
        {
            m_RowPool.Get(i).Unlink();
            m_RowPool.RemoveOrdered(i);
        }
    }

    /**
     *  Paint the previous match: the personal card, then the standings.
     *
     *  Everything on the card except the recap comes from the table row at self_index, so nothing is
     *  duplicated on the wire - and the squad block is summed from the table here rather than sent,
     *  which is only sound while the table is complete. See the TRUNCATED handling below.
     */
    private void RefreshLastMatch(BattleRoyaleRPC rpc)
    {
        RefreshTabs();

        BattleRoyaleLastMatch data = rpc.last_match;
        int shown = 0;
        int i = 0;

        for (i = 0; i < data.Count(); i++)
        {
            Widget row = EnsureRow(shown, "Vigrid-BattleRoyale/GUI/layouts/match_summary_row.layout", m_LastMatchRows);
            if (!row)
                break;

            TextWidget place_widget = TextWidget.Cast(row.FindAnyWidget("RowPlace"));
            TextWidget name_widget = TextWidget.Cast(row.FindAnyWidget("RowName"));
            TextWidget kills_widget = TextWidget.Cast(row.FindAnyWidget("RowKills"));
            TextWidget damage_widget = TextWidget.Cast(row.FindAnyWidget("RowDamage"));
            TextWidget survived_widget = TextWidget.Cast(row.FindAnyWidget("RowSurvived"));

            place_widget.SetText("#" + data.places.Get(i).ToString());
            name_widget.SetText(data.names.Get(i));
            kills_widget.SetText(data.kills.Get(i).ToString());
            damage_widget.SetText(data.damage.Get(i).ToString());
            survived_widget.SetText(BattleRoyaleRecapText.FormatDuration(data.survived.Get(i)));

            //--- The viewing player's own row, marked by raising the root's alpha rather than by a
            //--- second row layout that would have to be kept in step with this one.
            if (i == data.self_index)
                row.SetColor(ARGB(90, 255, 255, 255));
            else
                row.SetColor(ARGB(41, 255, 255, 255));

            shown = shown + 1;
        }

        TrimPool(shown);

        if (data.valid)
            m_LastMatchEmpty.SetText("#STR_BR_LASTMATCH_EMPTY");
        else
            m_LastMatchEmpty.SetText("#STR_BR_LASTMATCH_LOADING");

        m_LastMatchEmpty.Show(shown == 0);

        RefreshLastMatchCard(rpc, data);

        if (m_LastMatchScroll)
            BattleRoyaleUtils.Trace(string.Format("LastMatch rows=%1 (expect content height ~%2), scroll reports %3, scrollbar visible %4", shown, shown * ROW_HEIGHT, m_LastMatchScroll.GetContentHeight(), m_LastMatchScroll.IsScrollbarVisible()));
    }

    /**
     *  The personal card.
     *
     *  Hidden WHOLESALE when self_index is -1, which is the common case in a lobby rather than an
     *  edge case: most people there connected after the server restarted and never played the match
     *  being shown. Painting it zeroed would tell them they placed #0 with no kills.
     */
    private void RefreshLastMatchCard(BattleRoyaleRPC rpc, BattleRoyaleLastMatch data)
    {
        int self = data.self_index;
        bool has_card = data.valid && self >= 0 && self < data.Count();

        m_CardStats.Show(has_card);
        m_CardSquad.Show(has_card);

        if (!has_card)
        {
            //--- The recap panel stays up and says so, rather than borrowing the table's empty-state
            //--- text: that widget sits over the scroll area, so writing into it would print a line
            //--- across a table that is present and correct. Nothing is said at all until the answer
            //--- has actually arrived - before that the table is already showing "loading".
            m_CardRecap.Show(data.valid && data.Count() > 0);
            m_CardRecapText.SetText("#STR_BR_LASTMATCH_NOT_PLAYED");
            return;
        }

        m_CardRecap.Show(true);

        //--- "#4 of 12" versus "#4 of 12 squads" - the second is only true when parties were really
        //--- in play. With the party manager disabled the field size IS the player count, and calling
        //--- it a squad count is the mistake behind issue #158.
        string place_key = "STR_BR_LASTMATCH_PLACE_PLAYERS";
        if (data.IsGrouped())
            place_key = "STR_BR_LASTMATCH_PLACE_SQUADS";

        StringLocaliser place_label = new StringLocaliser(place_key, data.places.Get(self).ToString(), data.field_size.ToString());
        m_CardPlacementValue.SetText(place_label.Format());

        m_CardKillsValue.SetText(data.kills.Get(self).ToString());
        m_CardDamageValue.SetText(data.damage.Get(self).ToString());
        m_CardHitsValue.SetText(rpc.recap_hits.ToString());
        m_CardTimeValue.SetText(BattleRoyaleRecapText.FormatDuration(data.survived.Get(self)));

        //--- One formatter, shared with the death screen, so the line a player read as they died and
        //--- the line they read back in the lobby cannot disagree.
        string recap = BattleRoyaleRecapText.BuildRecapLine(rpc.recap_cause, rpc.recap_killer_name, rpc.recap_weapon_type, rpc.recap_distance_m);
        string detail = BattleRoyaleRecapText.BuildRecapDetail(rpc.recap_killer_health_pct, rpc.recap_damage_to_killer);

        if (detail != "")
        {
            if (recap != "")
                recap = recap + "\n";

            recap = recap + detail;
        }

        m_CardRecapText.SetText(recap);

        RefreshSquad(data, self);
    }

    /**
     *  Squad totals, summed from the standings table.
     *
     *  Hidden when parties were not in play (every player is their own group, so the block would
     *  just repeat the card beside it), and hidden when the table was TRUNCATED - a squadmate cut
     *  from the payload produces a smaller, WRONG total with nothing on screen to signal it, and no
     *  figure beats a wrong figure.
     */
    private void RefreshSquad(BattleRoyaleLastMatch data, int self)
    {
        int self_group = data.groups.Get(self);
        bool show = data.IsGrouped() && !data.IsTruncated() && self_group >= 0;

        m_CardSquad.Show(show);

        if (!show)
            return;

        int squad_kills = 0;
        int squad_damage = 0;
        int squad_best = 0;
        string members = "";
        int i = 0;

        for (i = 0; i < data.Count(); i++)
        {
            if (data.groups.Get(i) != self_group)
                continue;

            squad_kills = squad_kills + data.kills.Get(i);
            squad_damage = squad_damage + data.damage.Get(i);

            if (squad_best == 0 || data.places.Get(i) < squad_best)
                squad_best = data.places.Get(i);

            if (members != "")
                members = members + ", ";

            members = members + data.names.Get(i);
        }

        m_CardSquadKillsValue.SetText(squad_kills.ToString());
        m_CardSquadDamageValue.SetText(squad_damage.ToString());
        m_CardSquadBestValue.SetText("#" + squad_best.ToString());
        m_CardSquadMembers.SetText(members);
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
            Widget row = EnsureRow(shown, "Vigrid-BattleRoyale/GUI/layouts/leaderboard_row.layout", m_Rows);
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

        TrimPool(shown);

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
        bool group_active = m_Board == BR_LEADERBOARD_BOARD_GROUP;
        bool last_active = m_Board == BR_LEADERBOARD_BOARD_LASTMATCH;

        m_SoloTabAccent.Show(solo_active);
        m_GroupTabAccent.Show(group_active);
        m_LastMatchTabAccent.Show(last_active);

        //--- Two grouping panels, swapped by Show() and nothing else. No SetPos anywhere: declared
        //--- geometry is viewport-scaled while SetPos is real pixels, and mixing them moves a whole
        //--- group while its internal spacing stays right, which reads as anything but a coordinate
        //--- bug.
        m_LadderPanel.Show(!last_active);
        m_LastMatchPanel.Show(last_active);

        //--- The season line belongs to the ladders; the previous match has no season.
        m_SeasonText.Show(!last_active);

        if (last_active)
        {
            m_TitleText.SetText("#STR_BR_LASTMATCH_TITLE");
            return;
        }

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

        //--- Let the next Update fetch straight away if this board has never been loaded.
        m_RequestDueMs = 0;

        //--- ⚠ Destroy every pooled row before repainting. The ladder and the last-match table use
        //--- DIFFERENT row layouts, so a widget built for one and reused for the other answers NULL
        //--- to every FindAnyWidget the new tab makes - which paints nothing at all and looks like an
        //--- empty table rather than an error. The two-tab code this replaced had no such hazard,
        //--- because both its tabs shared one layout.
        TrimPool(0);

        //--- Repaint from cache now rather than next frame, so a cached board appears on the click
        //--- with no flicker through an empty list.
        BattleRoyaleRPC rpc = BattleRoyaleRPC.GetInstance();

        if (m_Board == BR_LEADERBOARD_BOARD_LASTMATCH)
        {
            m_LastSeq = rpc.last_match_seq;
            RefreshLastMatch(rpc);
            return;
        }

        m_LastSeq = rpc.leaderboard_seq;
        RefreshRows(rpc, rpc.GetLeaderboardBoard(m_Board));
    }

    /**
     *  Scroll the ladder on the mouse wheel.
     *
     *  A ScrollWidget does not do this by itself: nothing in the engine turns a wheel event into a
     *  scroll, and no vanilla script calls VScrollStep anywhere. Vanilla's own ScrollBarContainer
     *  implements the wheel by hand (scrollbarcontainer.c:220), which is the precedent here.
     *
     *  Note this is invisible to the trace in RefreshRows: content height and IsScrollbarVisible()
     *  were both already correct, which is exactly why it went unnoticed - the bar is drawn, so the
     *  list looks scrollable and simply is not.
     *
     *  No hit test, unlike VigridPartyMenu: this screen has one list, so any wheel over the menu is
     *  meant for it. Sign follows vanilla's convention - a positive wheel scrolls UP.
     */
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        //--- Whichever list is on screen. Still no hit test: only one of the two is ever visible, so
        //--- any wheel over the menu is meant for it.
        ScrollWidget target = m_Scroll;
        if (m_Board == BR_LEADERBOARD_BOARD_LASTMATCH)
            target = m_LastMatchScroll;

        if (!target)
            return super.OnMouseWheel(w, x, y, wheel);

        target.VScrollStep(-wheel);
        return true;
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

        if (w == m_LastMatchTabButton)
        {
            SwitchBoard(BR_LEADERBOARD_BOARD_LASTMATCH);
            return true;
        }

        return super.OnClick(w, x, y, button);
    }
}
#endif
