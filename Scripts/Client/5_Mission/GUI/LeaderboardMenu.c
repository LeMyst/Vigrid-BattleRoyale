#ifndef SERVER
/**
 *  Battle Royale - the leaderboard screen.
 *
 *  Two independent ladders behind a tab: Solo and Group. They are never merged, because a win as
 *  one of a four-stack and a win alone against the same field are not the same achievement - see
 *  BattleRoyaleLeaderboardData for the full scoring model. A third tab shows the previous match.
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
 *
 *  Everything sized to its text (tabs, badges, chips, the hint row) is laid out by Relayout() for a
 *  few frames after a change: GetTextSize lags SetText. Script works in real pixels, the layout in
 *  1920 reference units, so every length is multiplied by Scale().
 */
class LeaderboardMenu extends UIScriptedMenu
{
    //--- Only used while a ladder has never arrived. Once cached, nothing is ever requested again.
    private static const int REQUEST_RETRY_MS = 1000;

    //--- Inner width of the dialog in layout units, and how many frames a relayout runs for.
    private static const float BR_LB_DIALOG_W = 1348.0;
    private static const int BR_LB_LAYOUT_FRAMES = 4;

    private static const int BR_LB_DIM = 0xFF80868C;
    private static const int BR_LB_TEXT = 0xFFECEDEE;
    private static const int BR_LB_MUTED = 0xFF9BA1A6;
    private static const int BR_LB_GOLD = 0xFFF0B84A;
    private static const int BR_LB_SILVER = 0xFFC9CDD1;
    private static const int BR_LB_BRONZE = 0xFFC9895B;
    private static const int BR_LB_ROW_SELF = 0x14D99E29;
    private static const int BR_LB_ROW_STRIPE = 0x05FFFFFF;
    private static const int BR_LB_ROW_CLEAR = 0x00FFFFFF;

    private Widget m_Dialog;
    private Widget m_Rows;
    private ScrollWidget m_Scroll;
    private TextWidget m_SeasonText;
    private TextWidget m_EmptyText;
    private ButtonWidget m_CloseButton;

    private ButtonWidget m_SoloTabButton;
    private ButtonWidget m_GroupTabButton;
    private ButtonWidget m_LastMatchTabButton;
    private TextWidget m_SoloTabLabel;
    private TextWidget m_GroupTabLabel;
    private TextWidget m_LastMatchTabLabel;
    private Widget m_SoloTabAccent;
    private Widget m_GroupTabAccent;
    private Widget m_LastMatchTabAccent;
    private Widget m_TabKeyQ;
    private Widget m_TabKeyE;
    private TextWidget m_TabHint;

    //--- The pinned "you" row under the ladder.
    private TextWidget m_SelfRank;
    private TextWidget m_SelfName;
    private Widget m_SelfBadge;
    private TextWidget m_SelfBadgeText;
    private TextWidget m_SelfMatches;
    private TextWidget m_SelfWins;
    private TextWidget m_SelfKills;
    private TextWidget m_SelfPoints;
    private TextWidget m_SelfText;

    //--- The two tab bodies, swapped by Show().
    private Widget m_LadderPanel;
    private Widget m_LastMatchPanel;

    private Widget m_LastMatchRows;
    private ScrollWidget m_LastMatchScroll;
    private TextWidget m_LastMatchEmpty;
    private Widget m_Standings;

    private Widget m_CardSelf;
    private TextWidget m_AsideNote;
    private TextWidget m_PlaceNumber;
    private TextWidget m_PlaceOf;
    private TextWidget m_CardKillsValue;
    private TextWidget m_CardDamageValue;
    private TextWidget m_CardHitsValue;
    private TextWidget m_CardTimeValue;
    private TextWidget m_RecapLine;
    private TextWidget m_RecapDetail;
    private Widget m_WeaponChip;
    private TextWidget m_WeaponChipText;
    private Widget m_DistanceChip;
    private TextWidget m_DistanceChipText;

    private Widget m_CardSquad;
    private TextWidget m_SquadHeader;
    private TextWidget m_SquadKillsLabel;
    private TextWidget m_CardSquadKillsValue;
    private TextWidget m_SquadDamageLabel;
    private TextWidget m_CardSquadDamageValue;
    private TextWidget m_SquadBestLabel;
    private TextWidget m_CardSquadBestValue;

    //--- The hint row under the dialog.
    private Widget m_HintSquadSwatch;
    private TextWidget m_HintSquadText;
    private Widget m_HintWheelKey;
    private TextWidget m_HintWheelKeyText;
    private TextWidget m_HintWheelText;
    private Widget m_HintF4Key;
    private TextWidget m_HintF4KeyText;
    private Widget m_HintEscKey;
    private TextWidget m_HintEscKeyText;
    private TextWidget m_HintCloseText;

    private ref array<Widget> m_RowPool;

    private int m_Board;
    private int m_LastSeq;
    private int m_RequestDueMs;
    private int m_LayoutFrames;

    //--- Pool index of the ladder row that is the viewing player's, or -1.
    private int m_SelfRowIndex;
    private bool m_SquadShown;
    private bool m_AnySquadMark;
    private bool m_QHeld;
    private bool m_EHeld;
    private bool m_RestoreHud;

    private ref BattleRoyaleMenuInputLock m_InputLock;

    void LeaderboardMenu()
    {
        m_InputLock = new BattleRoyaleMenuInputLock();
        m_RowPool = new array<Widget>();
        m_Board = BR_LEADERBOARD_BOARD_SOLO;
        m_LastSeq = -1;
        m_RequestDueMs = 0;
        m_LayoutFrames = 0;
        m_SelfRowIndex = -1;
        m_SquadShown = false;
        m_AnySquadMark = false;
        m_QHeld = false;
        m_EHeld = false;
        m_RestoreHud = false;
    }

    private TextWidget FindText(string name)
    {
        return TextWidget.Cast(layoutRoot.FindAnyWidget(name));
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/leaderboard.layout");

        m_Dialog = layoutRoot.FindAnyWidget("Dialog");
        m_Rows = layoutRoot.FindAnyWidget("LeaderboardRows");
        m_Scroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("LeaderboardScroll"));
        m_SeasonText = FindText("SeasonText");
        m_EmptyText = FindText("EmptyText");
        m_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));

        m_SoloTabButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SoloTabButton"));
        m_GroupTabButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("GroupTabButton"));
        m_LastMatchTabButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("LastMatchTabButton"));
        m_SoloTabLabel = FindText("SoloTabLabel");
        m_GroupTabLabel = FindText("GroupTabLabel");
        m_LastMatchTabLabel = FindText("LastMatchTabLabel");
        m_SoloTabAccent = layoutRoot.FindAnyWidget("SoloTabAccent");
        m_GroupTabAccent = layoutRoot.FindAnyWidget("GroupTabAccent");
        m_LastMatchTabAccent = layoutRoot.FindAnyWidget("LastMatchTabAccent");
        m_TabKeyQ = layoutRoot.FindAnyWidget("TabKeyQ");
        m_TabKeyE = layoutRoot.FindAnyWidget("TabKeyE");
        m_TabHint = FindText("TabHint");

        m_SelfRank = FindText("SelfRank");
        m_SelfName = FindText("SelfName");
        m_SelfBadge = layoutRoot.FindAnyWidget("SelfBadge");
        m_SelfBadgeText = FindText("SelfBadgeText");
        m_SelfMatches = FindText("SelfMatches");
        m_SelfWins = FindText("SelfWins");
        m_SelfKills = FindText("SelfKills");
        m_SelfPoints = FindText("SelfPoints");
        m_SelfText = FindText("SelfText");

        m_LadderPanel = layoutRoot.FindAnyWidget("LadderPanel");
        m_LastMatchPanel = layoutRoot.FindAnyWidget("LastMatchPanel");
        m_LastMatchRows = layoutRoot.FindAnyWidget("LastMatchRows");
        m_LastMatchScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("LastMatchScroll"));
        m_LastMatchEmpty = FindText("LastMatchEmpty");
        m_Standings = layoutRoot.FindAnyWidget("Standings");

        m_CardSelf = layoutRoot.FindAnyWidget("CardSelf");
        m_AsideNote = FindText("AsideNote");
        m_PlaceNumber = FindText("PlaceNumber");
        m_PlaceOf = FindText("PlaceOf");
        m_CardKillsValue = FindText("CardKillsValue");
        m_CardDamageValue = FindText("CardDamageValue");
        m_CardHitsValue = FindText("CardHitsValue");
        m_CardTimeValue = FindText("CardTimeValue");
        m_RecapLine = FindText("RecapLine");
        m_RecapDetail = FindText("RecapDetail");
        m_WeaponChip = layoutRoot.FindAnyWidget("WeaponChip");
        m_WeaponChipText = FindText("WeaponChipText");
        m_DistanceChip = layoutRoot.FindAnyWidget("DistanceChip");
        m_DistanceChipText = FindText("DistanceChipText");

        m_CardSquad = layoutRoot.FindAnyWidget("CardSquad");
        m_SquadHeader = FindText("SquadHeader");
        m_SquadKillsLabel = FindText("SquadKillsLabel");
        m_CardSquadKillsValue = FindText("CardSquadKillsValue");
        m_SquadDamageLabel = FindText("SquadDamageLabel");
        m_CardSquadDamageValue = FindText("CardSquadDamageValue");
        m_SquadBestLabel = FindText("SquadBestLabel");
        m_CardSquadBestValue = FindText("CardSquadBestValue");

        m_HintSquadSwatch = layoutRoot.FindAnyWidget("HintSquadSwatch");
        m_HintSquadText = FindText("HintSquadText");
        m_HintWheelKey = layoutRoot.FindAnyWidget("HintWheelKey");
        m_HintWheelKeyText = FindText("HintWheelKeyText");
        m_HintWheelText = FindText("HintWheelText");
        m_HintF4Key = layoutRoot.FindAnyWidget("HintF4Key");
        m_HintF4KeyText = FindText("HintF4KeyText");
        m_HintEscKey = layoutRoot.FindAnyWidget("HintEscKey");
        m_HintEscKeyText = FindText("HintEscKeyText");
        m_HintCloseText = FindText("HintCloseText");

        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();

        //--- No ChangeGameFocus(1) here, and none of the matching -1 in OnHide. super.OnShow ->
        //--- LockControls already takes focus PER DEVICE and shows the cursor, as every vanilla menu
        //--- does. The extra ALL-DEVICE call froze the whole input layer while the board was up,
        //--- UAUIBack included: the release of F4 during the open went unseen, the action stayed
        //--- "held", and neither F4 nor Esc could close the board. Vanilla uses the all-device form
        //--- only in debug tools. Same fix as VigridPartyMenu, measured there with a per-frame trace.
        SetFocus(layoutRoot);

        //--- Focus alone leaves mouse look and movement live; this freezes both and keeps
        //--- push-to-talk. See BattleRoyaleMenuInputLock.
        m_InputLock.Take();

        //--- The hint row sits where the vanilla hotbar is drawn, and draws under it.
        m_RestoreHud = false;
        MissionGameplay gameplay = MissionGameplay.Cast(GetGame().GetMission());
        if (gameplay && gameplay.IsVanillaHudVisible())
        {
            gameplay.SetVanillaHudVisible(false);
            m_RestoreHud = true;
        }

        //--- Repaint from whatever is cached, immediately. -1 forces it even though the sequence
        //--- number has not moved while the menu was closed.
        m_LastSeq = -1;
        m_RequestDueMs = 0;
        m_LayoutFrames = BR_LB_LAYOUT_FRAMES;

        //--- The keys that opened the menu must not also switch a tab on the first frame.
        m_QHeld = true;
        m_EHeld = true;

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

        //--- super.OnHide -> UnlockControls releases exactly what LockControls took and hides the
        //--- cursor - the shape DeathScreenMenu already uses. NEVER add a ResetGameFocus() here:
        //--- input focus is an ADDITIVE counter every holder shares, and the reset SETS IT TO ZERO
        //--- across all devices (input.c:22-27). That was once reachable in three keypresses - die
        //--- (SimulateDeath +1, death screen +1), press F4 twice - and dropped the counter to 0 with
        //--- the death screen still up; the Spectate button's releases then drove it NEGATIVE.
        m_InputLock.Release();

        //--- Only what this menu hid, and never onto a corpse: dying with the board open leaves the
        //--- HUD to the death screen and spectate.
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        MissionGameplay gameplay = MissionGameplay.Cast(GetGame().GetMission());
        if (m_RestoreHud && gameplay && player && player.IsAlive())
            gameplay.SetVanillaHudVisible(true);

        m_RestoreHud = false;
    }

    private BattleRoyaleClient GetClient()
    {
        return BattleRoyaleClient.Cast( GetBR() );
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);

        PollTabKeys();

        if (m_LayoutFrames > 0)
        {
            m_LayoutFrames = m_LayoutFrames - 1;
            Relayout();
        }

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

    //! Q and E cycle the tabs. Read raw: vanilla binds no keyboard key to UAUITabLeft.
    private void PollTabKeys()
    {
        bool q_down = KeyState(KeyCode.KC_Q) != 0;
        bool e_down = KeyState(KeyCode.KC_E) != 0;

        if (q_down && !m_QHeld)
            CycleBoard(-1);
        if (e_down && !m_EHeld)
            CycleBoard(1);

        m_QHeld = q_down;
        m_EHeld = e_down;
    }

    private void CycleBoard(int step)
    {
        int next = (m_Board + step + 3) % 3;
        SwitchBoard(next);
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

        if (m_SelfRowIndex >= m_RowPool.Count())
            m_SelfRowIndex = -1;
    }

    //! Stripe every other row; the viewing player's own row is tinted instead.
    private int RowColor(int index, bool is_self)
    {
        if (is_self)
            return BR_LB_ROW_SELF;
        if (index % 2 == 1)
            return BR_LB_ROW_STRIPE;
        return BR_LB_ROW_CLEAR;
    }

    //------------------------------------------------------------------------------------------
    //--- The ladders
    //------------------------------------------------------------------------------------------

    private void RefreshRows(BattleRoyaleRPC rpc, BattleRoyaleLeaderboardBoard active)
    {
        RefreshTabs();

        int shown = 0;
        int count = active.Count();
        int self_index = active.self_rank - 1;
        m_SelfRowIndex = -1;

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
            Widget badge = row.FindAnyWidget("RowBadge");

            int display_rank = i + 1;
            rank_widget.SetText(display_rank.ToString());
            rank_widget.SetColor(MedalColor(i));
            name_widget.SetText(active.names.Get(i));
            matches_widget.SetText(active.matches.Get(i).ToString());
            wins_widget.SetText(active.wins.Get(i).ToString());
            kills_widget.SetText(active.kills.Get(i).ToString());
            points_widget.SetText(active.points.Get(i).ToString());

            bool is_self = (i == self_index);
            row.SetColor(RowColor(i, is_self));
            badge.Show(is_self);
            if (is_self)
                m_SelfRowIndex = shown;

            shown = shown + 1;
        }

        TrimPool(shown);

        //--- Scrolling is entirely declarative: LeaderboardRows is a WrapSpacerWidget carrying
        //--- "Size To Content V", so it grows with the rows and the ScrollWidget scrolls it. Still
        //--- traced, because a non-scrolling list looks exactly like a short one: if the reported
        //--- height stays at the viewport height while rows climb, this has regressed.
        if (m_Scroll)
            BattleRoyaleUtils.Trace(string.Format("Leaderboard rows=%1, scroll reports %2, scrollbar visible %3", shown, m_Scroll.GetContentHeight(), m_Scroll.IsScrollbarVisible()));

        //--- "Nothing here yet" and "not fetched yet" look identical on screen but mean opposite
        //--- things, so an unfetched ladder says loading instead of claiming to be empty.
        if (active.valid)
            m_EmptyText.SetText("#STR_BR_LEADERBOARD_EMPTY");
        else
            m_EmptyText.SetText("#STR_BR_LEADERBOARD_LOADING");

        m_EmptyText.Show(shown == 0);

        RefreshSelf(active);
        m_LayoutFrames = BR_LB_LAYOUT_FRAMES;
    }

    private int MedalColor(int index)
    {
        if (index == 0)
            return BR_LB_GOLD;
        if (index == 1)
            return BR_LB_SILVER;
        if (index == 2)
            return BR_LB_BRONZE;
        return BR_LB_DIM;
    }

    private void ShowSelfCells(bool show)
    {
        m_SelfRank.Show(show);
        m_SelfName.Show(show);
        m_SelfBadge.Show(show);
        m_SelfMatches.Show(show);
        m_SelfWins.Show(show);
        m_SelfKills.Show(show);
        m_SelfPoints.Show(show);
        m_SelfText.Show(!show);
    }

    //! The pinned row: the viewing player's standing, whether or not they made the visible rows.
    private void RefreshSelf(BattleRoyaleLeaderboardBoard active)
    {
        if (!active.valid)
        {
            ShowSelfCells(false);
            m_SelfText.SetText("");
            return;
        }

        if (active.self_rank <= 0)
        {
            ShowSelfCells(false);
            m_SelfText.SetText("#STR_BR_LEADERBOARD_YOU_UNRANKED");
            return;
        }

        ShowSelfCells(true);

        //--- The server sends no name for the requester; the ladder row has it when they are on it.
        string name = LocalPlayerName();
        int self_index = active.self_rank - 1;
        if (self_index < active.Count())
            name = active.names.Get(self_index);

        m_SelfRank.SetText(active.self_rank.ToString());
        m_SelfName.SetText(name);
        m_SelfMatches.SetText(active.self_matches.ToString());
        m_SelfWins.SetText(active.self_wins.ToString());
        m_SelfKills.SetText(active.self_kills.ToString());
        m_SelfPoints.SetText(active.self_points.ToString());
    }

    private string LocalPlayerName()
    {
        Man player = GetGame().GetPlayer();
        if (!player)
            return "";

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return "";

        return identity.GetName();
    }

    /**
     *  Show which board is on screen.
     *
     *  Deliberately does NOT disable the active tab. Disabling it was the first attempt and it made
     *  the button stop responding to the mouse - including its hover state - which read as the UI
     *  being broken rather than as a selection. All buttons stay live; the accent bar and the label
     *  colour carry the state instead.
     */
    private void RefreshTabs()
    {
        bool solo_active = m_Board == BR_LEADERBOARD_BOARD_SOLO;
        bool group_active = m_Board == BR_LEADERBOARD_BOARD_GROUP;
        bool last_active = m_Board == BR_LEADERBOARD_BOARD_LASTMATCH;

        PaintTab(m_SoloTabLabel, m_SoloTabAccent, solo_active);
        PaintTab(m_GroupTabLabel, m_GroupTabAccent, group_active);
        PaintTab(m_LastMatchTabLabel, m_LastMatchTabAccent, last_active);

        //--- Two grouping frames, swapped by Show() and nothing else.
        m_LadderPanel.Show(!last_active);
        m_LastMatchPanel.Show(last_active);

        //--- Under the title: the season for a ladder, the winner for the previous match.
        BattleRoyaleRPC rpc = BattleRoyaleRPC.GetInstance();
        if (last_active)
        {
            m_SeasonText.SetText(WinnerLine(rpc.last_match));
        }
        else
        {
            StringLocaliser season_label = new StringLocaliser("STR_BR_LEADERBOARD_SEASON", rpc.lb_season.ToString());
            m_SeasonText.SetText(season_label.Format());
        }

        m_LayoutFrames = BR_LB_LAYOUT_FRAMES;
    }

    private void PaintTab(TextWidget label, Widget accent, bool active)
    {
        accent.Show(active);
        if (active)
            label.SetColor(BR_LB_TEXT);
        else
            label.SetColor(BR_LB_MUTED);
    }

    //! "Last match won by X with N kills." Every first-place row is named, so a squad win reads as one.
    private string WinnerLine(BattleRoyaleLastMatch data)
    {
        if (!data.valid)
            return "";

        string names = "";
        int kills = 0;

        for (int i = 0; i < data.Count(); i++)
        {
            if (data.places.Get(i) != 1)
                continue;

            if (names != "")
                names = names + ", ";

            names = names + data.names.Get(i);
            kills = kills + data.kills.Get(i);
        }

        if (names == "")
            return "";

        StringLocaliser line = new StringLocaliser("STR_BR_LASTMATCH_WINNER_ANNOUNCE", names, kills.ToString());
        return line.Format();
    }

    //------------------------------------------------------------------------------------------
    //--- The previous match
    //------------------------------------------------------------------------------------------

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

        int self = data.self_index;
        int self_group = -1;
        if (self >= 0 && self < data.Count())
            self_group = data.groups.Get(self);

        //--- Squadmates are marked only when parties were really in play.
        bool mark_squad = data.IsGrouped() && self_group >= 0;
        m_AnySquadMark = false;

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
            Widget squad_mark = row.FindAnyWidget("RowSquad");

            int place = data.places.Get(i);
            bool is_self = (i == self);

            place_widget.SetText(place.ToString());
            name_widget.SetText(data.names.Get(i));
            kills_widget.SetText(data.kills.Get(i).ToString());
            damage_widget.SetText(data.damage.Get(i).ToString());
            survived_widget.SetText(BattleRoyaleRecapText.FormatDuration(data.survived.Get(i)));

            if (place == 1 || is_self)
                place_widget.SetColor(BR_LB_GOLD);
            else
                place_widget.SetColor(BR_LB_DIM);

            bool squadmate = mark_squad && data.groups.Get(i) == self_group;
            squad_mark.Show(squadmate);
            if (squadmate)
                m_AnySquadMark = true;

            row.SetColor(RowColor(i, is_self));

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
            BattleRoyaleUtils.Trace(string.Format("LastMatch rows=%1, scroll reports %2, scrollbar visible %3", shown, m_LastMatchScroll.GetContentHeight(), m_LastMatchScroll.IsScrollbarVisible()));

        m_LayoutFrames = BR_LB_LAYOUT_FRAMES;
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

        m_CardSelf.Show(has_card);

        //--- Nothing is said until the answer has arrived - before that the table says "loading".
        m_AsideNote.Show(!has_card && data.valid && data.Count() > 0);

        RefreshSquad(data, self, has_card);

        if (!has_card)
            return;

        //--- The number and its "of N" are separate widgets, so no key can put them in the wrong
        //--- order. "of N squads" only when parties were really in play: with the party manager
        //--- disabled the field size IS the player count (issue #158).
        string of_key = "STR_BR_LASTMATCH_OF_PLAYERS";
        if (data.IsGrouped())
            of_key = "STR_BR_LASTMATCH_OF_SQUADS";

        StringLocaliser of_label = new StringLocaliser(of_key, data.field_size.ToString());
        m_PlaceNumber.SetText(data.places.Get(self).ToString());
        m_PlaceOf.SetText(of_label.Format());

        m_CardKillsValue.SetText(data.kills.Get(self).ToString());
        m_CardDamageValue.SetText(data.damage.Get(self).ToString());
        m_CardHitsValue.SetText(rpc.recap_hits.ToString());
        m_CardTimeValue.SetText(BattleRoyaleRecapText.FormatDuration(data.survived.Get(self)));

        //--- Same formatter and same split as the death screen, so the two cannot disagree: a
        //--- named killer gets "Eliminated by X" and the weapon and range as chips; anything else
        //--- (zone, environment, infected, a win) gets the one-line cause.
        string killer = rpc.recap_killer_name;
        string weapon = "";
        bool has_distance = false;

        if (killer != "")
        {
            m_RecapLine.SetText(Widget.TranslateString("#STR_BR_DEAD_ELIMINATED_BY") + " " + killer);
            weapon = BattleRoyaleRecapText.WeaponDisplayName(rpc.recap_weapon_type);
            has_distance = rpc.recap_distance_m >= 0;
        }
        else
        {
            m_RecapLine.SetText(BattleRoyaleRecapText.BuildRecapLine(rpc.recap_cause, killer, rpc.recap_weapon_type, rpc.recap_distance_m));
        }

        m_WeaponChipText.SetText(weapon);
        m_WeaponChip.Show(weapon != "");
        m_DistanceChipText.SetText(rpc.recap_distance_m.ToString() + " m");
        m_DistanceChip.Show(has_distance);

        string detail = "";
        if (killer != "")
            detail = BattleRoyaleRecapText.BuildRecapDetail(rpc.recap_killer_health_pct, rpc.recap_damage_to_killer);

        m_RecapDetail.SetText(detail);
    }

    /**
     *  Squad totals, summed from the standings table.
     *
     *  Hidden when parties were not in play (every player is their own group, so the strip would
     *  just repeat the card beside it), and hidden when the table was TRUNCATED - a squadmate cut
     *  from the payload produces a smaller, WRONG total with nothing on screen to signal it, and no
     *  figure beats a wrong figure.
     */
    private void RefreshSquad(BattleRoyaleLastMatch data, int self, bool has_card)
    {
        int self_group = -1;
        if (has_card)
            self_group = data.groups.Get(self);

        m_SquadShown = has_card && data.IsGrouped() && !data.IsTruncated() && self_group >= 0;
        m_CardSquad.Show(m_SquadShown);

        if (!m_SquadShown)
            return;

        int squad_kills = 0;
        int squad_damage = 0;
        int squad_best = 0;

        for (int i = 0; i < data.Count(); i++)
        {
            if (data.groups.Get(i) != self_group)
                continue;

            squad_kills = squad_kills + data.kills.Get(i);
            squad_damage = squad_damage + data.damage.Get(i);

            if (squad_best == 0 || data.places.Get(i) < squad_best)
                squad_best = data.places.Get(i);
        }

        m_CardSquadKillsValue.SetText(squad_kills.ToString());
        m_CardSquadDamageValue.SetText(squad_damage.ToString());
        m_CardSquadBestValue.SetText(squad_best.ToString());
    }

    //------------------------------------------------------------------------------------------
    //--- Layout. Script works in real pixels, the layout in 1920 reference units.
    //------------------------------------------------------------------------------------------

    private float Scale()
    {
        if (!m_Dialog)
            return 0;

        float w;
        float h;
        m_Dialog.GetScreenSize(w, h);
        return w / BR_LB_DIALOG_W;
    }

    private float TextWidth(TextWidget t)
    {
        int w = 0;
        int h = 0;
        if (t)
            t.GetTextSize(w, h);
        return w;
    }

    private void Relayout()
    {
        float s = Scale();
        if (s <= 0)
            return;

        LayoutTabs(s);
        LayoutHints(s);

        if (m_Board == BR_LEADERBOARD_BOARD_LASTMATCH)
        {
            LayoutLastMatch(s);
            return;
        }

        if (m_SelfRowIndex >= 0 && m_SelfRowIndex < m_RowPool.Count())
        {
            Widget row = m_RowPool.Get(m_SelfRowIndex);
            PlaceBadge(TextWidget.Cast(row.FindAnyWidget("RowName")), row.FindAnyWidget("RowBadge"), TextWidget.Cast(row.FindAnyWidget("RowBadgeText")), 16, s);
        }

        PlaceBadge(m_SelfName, m_SelfBadge, m_SelfBadgeText, 22, s);
    }

    //! Tabs sized to their labels from the left; [Q][E] and the hint right-aligned.
    private void LayoutTabs(float s)
    {
        float x = 30 * s;
        x = PlaceTab(m_SoloTabButton, m_SoloTabLabel, m_SoloTabAccent, x, s);
        x = PlaceTab(m_GroupTabButton, m_GroupTabLabel, m_GroupTabAccent, x, s);
        PlaceTab(m_LastMatchTabButton, m_LastMatchTabLabel, m_LastMatchTabAccent, x, s);

        float hint_w = TextWidth(m_TabHint);
        float right = (BR_LB_DIALOG_W - 30) * s;
        m_TabHint.SetPos(right - hint_w, 21 * s);
        m_TabHint.SetSize(hint_w, 30 * s);

        right = right - hint_w - 12 * s;
        m_TabKeyE.SetPos(right - 36 * s, 21 * s);
        m_TabKeyE.SetSize(36 * s, 30 * s);
        right = right - 42 * s;
        m_TabKeyQ.SetPos(right - 36 * s, 21 * s);
        m_TabKeyQ.SetSize(36 * s, 30 * s);
    }

    //! Returns the next tab's left edge.
    private float PlaceTab(Widget button, TextWidget label, Widget accent, float x, float s)
    {
        float w = TextWidth(label) + 48 * s;
        if (w < 120 * s)
            w = 120 * s;

        button.SetPos(x, 0);
        button.SetSize(w, 72 * s);
        label.SetSize(w, 69 * s);
        accent.SetPos(0, 69 * s);
        accent.SetSize(w, 3 * s);

        return x + w + 6 * s;
    }

    //! The "YOU" badge just after the name, the name stopping short of it when it is long.
    private void PlaceBadge(TextWidget name, Widget badge, TextWidget badge_text, float y, float s)
    {
        if (!name || !badge || !badge_text)
            return;

        float badge_w = TextWidth(badge_text) + 18 * s;
        float name_w = TextWidth(name);
        float max_w = 618 * s - badge_w - 15 * s;
        if (name_w > max_w)
            name_w = max_w;

        badge.SetPos(138 * s + name_w + 15 * s, y * s);
        badge.SetSize(badge_w, 28 * s);
        badge_text.SetSize(badge_w, 28 * s);
    }

    //! The row under the dialog, centred: [squad legend]  [Wheel] Scroll  [F4][Esc] Close.
    private void LayoutHints(float s)
    {
        bool squad = m_Board == BR_LEADERBOARD_BOARD_LASTMATCH && m_AnySquadMark;
        m_HintSquadSwatch.Show(squad);
        m_HintSquadText.Show(squad);

        float total = PlaceHints(0, s, false);
        float x0 = ((BR_LB_DIALOG_W + 2) * s - total) * 0.5;
        PlaceHints(x0, s, true);
    }

    //! Lays the hint row out from x when `place`; always returns its width.
    private float PlaceHints(float x0, float s, bool place)
    {
        float x = x0;
        float h = 33 * s;
        float group_gap = 36 * s;
        float gap = 12 * s;

        if (m_HintSquadSwatch.IsVisible())
        {
            if (place)
            {
                m_HintSquadSwatch.SetPos(x, 10.5 * s);
                m_HintSquadSwatch.SetSize(12 * s, 12 * s);
            }
            x = x + 12 * s + gap;
            x = PlaceHintText(m_HintSquadText, x, h, place) + group_gap;
        }

        x = PlaceKeycap(m_HintWheelKey, m_HintWheelKeyText, x, h, s, place) + gap;
        x = PlaceHintText(m_HintWheelText, x, h, place) + group_gap;

        x = PlaceKeycap(m_HintF4Key, m_HintF4KeyText, x, h, s, place) + 6 * s;
        x = PlaceKeycap(m_HintEscKey, m_HintEscKeyText, x, h, s, place) + gap;
        x = PlaceHintText(m_HintCloseText, x, h, place);

        return x - x0;
    }

    private float PlaceHintText(TextWidget t, float x, float h, bool place)
    {
        float w = TextWidth(t);
        if (place)
        {
            t.SetPos(x, 0);
            t.SetSize(w, h);
        }
        return x + w;
    }

    private float PlaceKeycap(Widget cap, TextWidget label, float x, float h, float s, bool place)
    {
        float w = TextWidth(label) + 24 * s;
        if (w < h)
            w = h;

        if (place)
        {
            cap.SetPos(x, 0);
            cap.SetSize(w, h);
        }
        return x + w;
    }

    private void LayoutLastMatch(float s)
    {
        //--- "of N" just after the big number.
        float of_x = 42 * s + TextWidth(m_PlaceNumber) + 15 * s;
        m_PlaceOf.SetPos(of_x, 108 * s);
        m_PlaceOf.SetSize((480 - 36) * s - of_x, 42 * s);

        //--- Chips left to right under the recap line; the detail text below them, or in their place.
        float x = 21 * s;
        bool chips = false;
        if (m_WeaponChip.IsVisible())
        {
            x = PlaceChip(m_WeaponChip, m_WeaponChipText, x, s) + 9 * s;
            chips = true;
        }
        if (m_DistanceChip.IsVisible())
        {
            PlaceChip(m_DistanceChip, m_DistanceChipText, x, s);
            chips = true;
        }

        float detail_y = 90;
        if (chips)
            detail_y = 138;
        m_RecapDetail.SetPos(21 * s, detail_y * s);

        //--- The squad strip, and the standings moved up into its place when it is hidden.
        if (m_SquadShown)
        {
            float sx = 42 * s;
            sx = PlaceStripText(m_SquadHeader, sx, s) + 36 * s;
            sx = PlaceStripText(m_SquadKillsLabel, sx, s) + 9 * s;
            sx = PlaceStripText(m_CardSquadKillsValue, sx, s) + 36 * s;
            sx = PlaceStripText(m_SquadDamageLabel, sx, s) + 9 * s;
            sx = PlaceStripText(m_CardSquadDamageValue, sx, s) + 36 * s;
            sx = PlaceStripText(m_SquadBestLabel, sx, s) + 9 * s;
            PlaceStripText(m_CardSquadBestValue, sx, s);
        }

        float top = 0;
        if (m_SquadShown)
            top = 74;

        m_Standings.SetPos(482 * s, top * s);
        m_Standings.SetSize(866 * s, (696 - top) * s);
        m_LastMatchScroll.SetSize(866 * s, (696 - top - 56) * s);
    }

    private float PlaceChip(Widget chip, TextWidget label, float x, float s)
    {
        float w = TextWidth(label) + 30 * s;
        chip.SetPos(x, 90 * s);
        chip.SetSize(w, 33 * s);
        label.SetSize(w, 33 * s);
        return x + w;
    }

    private float PlaceStripText(TextWidget t, float x, float s)
    {
        float w = TextWidth(t);
        t.SetPos(x, 0);
        t.SetSize(w, 72 * s);
        return x + w;
    }

    //------------------------------------------------------------------------------------------
    //--- Input
    //------------------------------------------------------------------------------------------

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
        //--- empty table rather than an error.
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
     *  Scroll the visible list on the mouse wheel.
     *
     *  A ScrollWidget does not do this by itself: nothing in the engine turns a wheel event into a
     *  scroll, and no vanilla script calls VScrollStep anywhere. Vanilla's own ScrollBarContainer
     *  implements the wheel by hand (scrollbarcontainer.c:220), which is the precedent here. Only one
     *  of the two lists is ever visible, so any wheel over the menu is meant for it. Sign follows
     *  vanilla's convention - a positive wheel scrolls UP.
     */
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
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
        //--- Logged BEFORE the widget comparisons: it separates the click never reaching the menu
        //--- (no line), landing on the wrong widget (a line naming it) and the handler misbehaving
        //--- (a line and then nothing). A grouping panel once ate every tab click in silence.
        string clicked = "<null>";
        if (w)
            clicked = w.GetName();

        BattleRoyaleUtils.Trace("[Leaderboard] OnClick: " + clicked + " board=" + m_Board);

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
