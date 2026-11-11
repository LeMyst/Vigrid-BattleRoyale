#ifndef SERVER
/**
 *  Vigrid Party - the party management screen.
 *
 *  Left column: your party as cards - the members, the invitations you have out, then the open
 *  slots - with Promote/Kick on each member when you are the leader. Right column: players connected
 *  to the server, filtered by the search box, each with an Invite button or the reason there is none.
 *  An incoming invitation is a banner across the top with its own countdown.
 *
 *  The online list is server-authoritative rather than read from ClientData.m_PlayerList: vanilla's
 *  own preprocessing skips entries whose identity has not synchronised yet, and it has no idea who
 *  is already in a party. Asking the server gets both right, and it is the server that will have
 *  to validate the invite anyway.
 *
 *  BOTH COLUMNS ARE SORTED BY DISPLAYED NAME - see BuildOnlineOrder / BuildMemberOrder. The sort is
 *  a DISPLAY ORDER ONLY: nothing here reorders rpc.list_* or rpc.roster_*, because a roster slot
 *  index is the member's palette colour everywhere else in the mod (nametag, HUD, map marker, ping)
 *  and slot 0 is what "the leader is the first member" means on the server. What the sort produces
 *  is an array of DATA indices, walked in order while the pooled rows keep their own creation
 *  order - which is also why no row has to be relinked: the WrapSpacer's children never move, only
 *  the content written into them does.
 *
 *  Everything sized to its text (buttons, badges, key caps) is laid out by Relayout() for a few
 *  frames after a change, because GetTextSize lags SetText. Script works in real pixels and the
 *  layout in 1920 reference units, so every length is multiplied by Scale().
 */
class VigridPartyMenu extends UIScriptedMenu
{
    //--- Inner width of the dialog in layout units, and how many frames a relayout runs for.
    private static const float LAYOUT_DIALOG_W = 1438.0;
    private static const int LAYOUT_FRAMES = 4;

    //--- Layout lengths the script moves things by, in layout units (see gen_party_menu.py).
    private static const float BODY_TOP = 116.0;
    private static const float BODY_TOP_BANNER = 236.0;
    private static const float FOOTER_TOP = 782.0;
    private static const float CARD_W = 492.0;

    private static const int COLOR_TEXT = 0xFFECEDEE;
    private static const int COLOR_MUTED = 0xFF9BA1A6;
    private static const int COLOR_DIM = 0xFF80868C;
    private static const int COLOR_WINDOW = 0xFF111417;
    private static const int COLOR_CARD = 0xFF171B1E;
    private static const int COLOR_CARD_EDGE = 0x14FFFFFF;
    private static const int COLOR_PENDING_EDGE = 0x663DB9A6;
    private static const int COLOR_OPEN_EDGE = 0x1AFFFFFF;
    private static const int COLOR_AVATAR_NEUTRAL = 0xFF22272C;
    private static const int COLOR_AVATAR_OPEN = 0x0FFFFFFF;

    private Widget m_Dialog;
    private Widget m_Body;
    private Widget m_OnlineColumn;
    private Widget m_ColumnLine;
    private Widget m_OnlineRows;
    private Widget m_MemberRows;
    private ScrollWidget m_OnlineScroll;
    private ScrollWidget m_PartyScroll;
    private TextWidget m_Subtitle;
    private TextWidget m_OnlineLabel;
    private TextWidget m_OnlineEmpty;

    private Widget m_InviteBanner;
    private TextWidget m_InviteText;
    private TextWidget m_InviteTimer;
    private ButtonWidget m_InviteAccept;
    private ButtonWidget m_InviteDecline;

    private EditBoxWidget m_SearchEdit;
    private TextWidget m_SearchPlaceholder;

    private ButtonWidget m_CloseButton;
    private Widget m_CloseKey;
    private TextWidget m_CloseKeyText;
    private ButtonWidget m_LeaveButton;
    private TextWidget m_LeaveLabel;
    private ButtonWidget m_DisbandButton;
    private TextWidget m_DisbandLabel;
    private Widget m_PingKey;
    private TextWidget m_PingKeyText;
    private TextWidget m_PingHint;
    private Widget m_PingClearKey;
    private TextWidget m_PingClearKeyText;
    private TextWidget m_PingClearHint;

    private ref array<Widget> m_OnlineRowPool;
    private ref array<Widget> m_MemberRowPool;

    //--- Button widget -> the uid that button acts on.
    private ref map<Widget, string> m_InviteTargets;
    private ref map<Widget, string> m_KickTargets;
    private ref map<Widget, string> m_PromoteTargets;
    private ref map<Widget, string> m_CancelTargets;

    //--- Invites sent from this menu, shown as pending at once rather than after the next list reply.
    //--- Cleared whenever a reply lands: from then on the server's own flag is the truth.
    private ref map<string, bool> m_SentInvites;

    //--- Invites withdrawn from this menu, no longer shown as pending even before the list says so.
    private ref map<string, bool> m_CancelledInvites;

    //--- The search, lowered. Empty shows everyone.
    private string m_Filter;

    private int m_LastListSeq;
    private int m_LastRosterSeq;
    private int m_LastInviteSeq;
    private int m_RefreshDueMs;
    private int m_LastTimerSecond;
    private int m_LayoutFrames;
    private bool m_RestoreHud;

    private ref VigridPartyMenuInputLock m_InputLock;

    void VigridPartyMenu()
    {
        m_InputLock = new VigridPartyMenuInputLock();
        m_OnlineRowPool = new array<Widget>();
        m_MemberRowPool = new array<Widget>();
        m_InviteTargets = new map<Widget, string>();
        m_KickTargets = new map<Widget, string>();
        m_PromoteTargets = new map<Widget, string>();
        m_SentInvites = new map<string, bool>();
        m_CancelledInvites = new map<string, bool>();
        m_CancelTargets = new map<Widget, string>();
        m_Filter = "";

        m_LastListSeq = -1;
        m_LastRosterSeq = -1;
        m_LastInviteSeq = -1;
        m_RefreshDueMs = 0;
        m_LastTimerSecond = -1;
        m_LayoutFrames = 0;
    }

    private TextWidget FindText(string name)
    {
        return TextWidget.Cast(layoutRoot.FindAnyWidget(name));
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_menu.layout");

        m_Dialog = layoutRoot.FindAnyWidget("Dialog");
        m_Body = layoutRoot.FindAnyWidget("Body");
        m_OnlineColumn = layoutRoot.FindAnyWidget("OnlineColumn");
        m_ColumnLine = layoutRoot.FindAnyWidget("ColumnLine");
        m_OnlineRows = layoutRoot.FindAnyWidget("OnlineRows");
        m_MemberRows = layoutRoot.FindAnyWidget("MemberRows");
        m_OnlineScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("OnlineScroll"));
        m_PartyScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("PartyScroll"));
        m_Subtitle = FindText("Subtitle");
        m_OnlineLabel = FindText("OnlineLabel");
        m_OnlineEmpty = FindText("OnlineEmpty");

        m_InviteBanner = layoutRoot.FindAnyWidget("InviteBanner");
        m_InviteText = FindText("InviteText");
        m_InviteTimer = FindText("InviteTimer");
        m_InviteAccept = ButtonWidget.Cast(layoutRoot.FindAnyWidget("InviteAcceptButton"));
        m_InviteDecline = ButtonWidget.Cast(layoutRoot.FindAnyWidget("InviteDeclineButton"));

        m_SearchEdit = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("SearchEdit"));
        m_SearchPlaceholder = FindText("SearchPlaceholder");

        m_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));
        m_CloseKey = layoutRoot.FindAnyWidget("CloseKey");
        m_CloseKeyText = FindText("CloseKeyText");
        m_LeaveButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("LeaveButton"));
        m_LeaveLabel = FindText("LeaveButtonLabel");
        m_DisbandButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("DisbandButton"));
        m_DisbandLabel = FindText("DisbandButtonLabel");
        m_PingKey = layoutRoot.FindAnyWidget("PingKey");
        m_PingKeyText = FindText("PingKeyText");
        m_PingHint = FindText("PingHint");
        m_PingClearKey = layoutRoot.FindAnyWidget("PingClearKey");
        m_PingClearKeyText = FindText("PingClearKeyText");
        m_PingClearHint = FindText("PingClearHint");

        //--- The keys as the player has them bound, not as shipped.
        m_CloseKeyText.SetText(KeyName(VIGRID_PARTY_INPUT_MENU));
        m_PingKeyText.SetText(KeyName(VIGRID_PARTY_INPUT_PING));
        m_PingClearKeyText.SetText(KeyName(VIGRID_PARTY_INPUT_PING_CLEAR));

        return layoutRoot;
    }

    private static string KeyName(string input_name)
    {
        if (!GetUApi().GetInputByName(input_name))
            return "";

        return InputUtils.GetButtonNameFromInput(input_name, EInputDeviceType.MOUSE_AND_KEYBOARD);
    }

    override void OnShow()
    {
        super.OnShow();

        //--- No ChangeGameFocus(1) here, and none of the matching -1 in OnHide. super.OnShow ->
        //--- LockControls already takes focus PER DEVICE (mouse, keyboard, gamepad) and shows the
        //--- cursor, exactly as every vanilla menu does. The extra ALL-DEVICE call that used to sit
        //--- here froze the whole input layer while the menu was up - not just gameplay binds but
        //--- UAUIBack too: a release of P during the open went unseen, so the action stayed "held",
        //--- the second press raised no edge, and neither P nor Esc could close the menu. Vanilla
        //--- uses the all-device form only in debug tools. Measured with a per-frame trace.
        SetFocus(layoutRoot);

        //--- Focus alone leaves mouse look and movement live; this freezes both and keeps
        //--- push-to-talk, so a party can still talk while one of them manages it. See
        //--- VigridPartyMenuInputLock.
        m_InputLock.Take();

        //--- The vanilla hotbar draws over the footer.
        m_RestoreHud = false;
        MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
        if (mission)
            m_RestoreHud = mission.VigridParty_SetHudPanelVisible(false);

        //--- Force a full repaint: sequence numbers may not have moved while the menu was closed.
        m_LastListSeq = -1;
        m_LastRosterSeq = -1;
        m_LastInviteSeq = -1;
        m_RefreshDueMs = 0;
        m_LayoutFrames = LAYOUT_FRAMES;
    }

    override void OnHide()
    {
        super.OnHide();

        //--- super.OnHide -> UnlockControls releases exactly what LockControls took and hides the
        //--- cursor; the exclude groups are ours to give back.
        m_InputLock.Release();

        //--- Only what this menu hid, and never onto a corpse.
        Man player = GetGame().GetPlayer();
        MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
        if (m_RestoreHud && mission && player && player.IsAlive())
            mission.VigridParty_SetHudPanelVisible(true);

        m_RestoreHud = false;
    }

    //! True while the search box has the keyboard: the menu key must type, not close the menu.
    bool IsTyping()
    {
        return m_SearchEdit && GetFocus() == m_SearchEdit;
    }

    private VigridPartyClient GetClient()
    {
        MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
        if (!mission)
            return null;

        return mission.GetVigridParty();
    }

    override void Update(float timeslice)
    {
        super.Update(timeslice);

        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        int now_ms = GetGame().GetTime();

        //--- Poll the server while the menu is open so players joining or leaving show up.
        if (now_ms >= m_RefreshDueMs)
        {
            m_RefreshDueMs = now_ms + 3000;

            VigridPartyClient client = GetClient();
            if (client)
                client.RequestPlayerList();
        }

        if (rpc.invite_seq != m_LastInviteSeq)
        {
            m_LastInviteSeq = rpc.invite_seq;
            RefreshInviteBanner(rpc);
        }

        UpdateInviteTimer(rpc, now_ms);

        //--- A fresh list replaces the invites this menu was remembering: the server's flag says.
        if (rpc.list_seq != m_LastListSeq)
        {
            m_SentInvites.Clear();
            m_CancelledInvites.Clear();
        }

        //--- The roster decides who is invitable and the list decides which invites are pending, so
        //--- either one repaints both columns.
        if (rpc.roster_seq != m_LastRosterSeq || rpc.list_seq != m_LastListSeq)
        {
            m_LastRosterSeq = rpc.roster_seq;
            m_LastListSeq = rpc.list_seq;
            RefreshAll(rpc);
        }

        if (m_LayoutFrames > 0)
        {
            m_LayoutFrames = m_LayoutFrames - 1;
            Relayout();
        }
    }

    private void RefreshAll(VigridPartyRPC rpc)
    {
        RefreshMembers(rpc);
        RefreshOnline(rpc);
        RefreshButtons(rpc);
        RefreshSubtitle(rpc);
        m_LayoutFrames = LAYOUT_FRAMES;
    }

    private void RefreshSubtitle(VigridPartyRPC rpc)
    {
        int members = rpc.roster_uids.Count();

        if (rpc.locked)
        {
            m_Subtitle.SetText("#STR_PARTY_LOCKED");
            return;
        }

        if (members == 0)
        {
            m_Subtitle.SetText("#STR_PARTY_NO_PARTY_YET");
            return;
        }

        StringLocaliser count = new StringLocaliser("STR_PARTY_MEMBER_COUNT", members.ToString(), rpc.max_party_size.ToString());
        m_Subtitle.SetText(count.Format());
    }

    private void RefreshInviteBanner(VigridPartyRPC rpc)
    {
        bool shown = rpc.HasInvite();
        m_InviteBanner.Show(shown);
        m_LastTimerSecond = -1;
        m_LayoutFrames = LAYOUT_FRAMES;

        if (!shown)
            return;

        StringLocaliser prompt = new StringLocaliser("STR_PARTY_INVITE_PROMPT", rpc.invite_inviter_name);
        m_InviteText.SetText(prompt.Format());
    }

    //! The invitation's time left, repainted once a second.
    private void UpdateInviteTimer(VigridPartyRPC rpc, int now_ms)
    {
        if (!m_InviteBanner.IsVisible())
            return;

        int left_s = (rpc.invite_expires_ms - now_ms + 999) / 1000;
        if (left_s < 0)
            left_s = 0;
        if (left_s == m_LastTimerSecond)
            return;

        m_LastTimerSecond = left_s;

        int minutes = left_s / 60;
        int seconds = left_s % 60;
        string seconds_text = seconds.ToString();
        if (seconds < 10)
            seconds_text = "0" + seconds_text;

        m_InviteTimer.SetText(minutes.ToString() + ":" + seconds_text);
    }

    private void RefreshButtons(VigridPartyRPC rpc)
    {
        bool in_party = rpc.roster_uids.Count() > 0;
        bool is_leader = rpc.self_index >= 0 && rpc.self_index == rpc.leader_index;

        //--- Everything is refused server-side while a match is running; hiding the buttons just
        //--- avoids inviting a rejection the player cannot act on.
        m_LeaveButton.Show(in_party && !rpc.locked);
        m_DisbandButton.Show(in_party && is_leader && !rpc.locked);
    }

    /**
     *  The `index`-th row, created if the pool is not that long yet.
     *
     *  DELIBERATELY DOES NOT SetPos THE ROW. The rows hang off a WrapSpacer, which lays its own
     *  children out in creation order; positioning them by hand fights it, and SetPos takes REAL
     *  SCREEN PIXELS while a layout's declared geometry is scaled by viewport/1920, so the pitch
     *  would only be right at one resolution.
     */
    private Widget EnsureRow(array<Widget> pool, Widget parent, string layout_name, int index)
    {
        while (pool.Count() <= index)
        {
            Widget created = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/" + layout_name, parent);
            if (!created)
                return null;

            //--- The invite label carries a "+", which the stringtable entry does not.
            TextWidget invite_label = TextWidget.Cast(created.FindAnyWidget("PlayerRowInviteButtonLabel"));
            if (invite_label)
                invite_label.SetText("+ " + Widget.TranslateString("#STR_PARTY_INVITE"));

            pool.Insert(created);
        }

        Widget row = pool.Get(index);
        row.Show(true);
        return row;
    }

    /**
     *  Destroy every pooled row from `first` on.
     *
     *  Unlink, not Show(false): a spacer lays out the children it HAS, so a hidden row still reserves
     *  its slot - leaving a gap in the middle of the list and a scroll range longer than the content.
     *
     *  Walked backwards so each removal is of the last element and cannot renumber an index still to
     *  be visited. RemoveOrdered rather than Remove: vanilla's Remove() fills the hole with the LAST
     *  element.
     */
    private void TrimPool(array<Widget> pool, int first)
    {
        for (int i = pool.Count() - 1; i >= first; i--)
        {
            pool.Get(i).Unlink();
            pool.RemoveOrdered(i);
        }
    }

    /**
     *  Byte-wise lexical comparison of two strings. <0, 0 or >0, like every other compare.
     *
     *  Written out rather than using an operator because EnfusionScript's `string` exposes no
     *  comparison at all. The one native sort there is (array<string>.Sort) sorts the array IN
     *  PLACE, which is exactly what must not happen here: the point is to leave the source arrays
     *  alone and produce an order over them.
     *
     *  Bytes, not characters. Masked to 0..255 so a UTF-8 lead byte compares as unsigned and a
     *  non-ASCII name sorts after an ASCII one instead of before it. That is byte order rather than
     *  collation order, but it is deterministic and identical on every client, which is the property
     *  this sort is actually for.
     */
    private static int CompareStrings(string a, string b)
    {
        int len_a = a.Length();
        int len_b = b.Length();

        int shared = len_a;
        if (len_b < shared)
            shared = len_b;

        for (int i = 0; i < shared; i++)
        {
            string char_a = a.Get(i);
            string char_b = b.Get(i);

            int code_a = char_a.ToAscii() & 0xFF;
            int code_b = char_b.ToAscii() & 0xFF;

            if (code_a != code_b)
                return code_a - code_b;
        }

        return len_a - len_b;
    }

    /**
     *  Order `order` (a list of data indices) by `keys`, tie-broken on `uids`.
     *
     *  The tie-break is not decoration. Two players called `Survivor` is the NORMAL case on a server
     *  with enable_steam_name_lookup off, and a merely stable sort would leave them in arrival order -
     *  churning, and only for the players hardest to tell apart. Sorting them by uid pins them.
     *
     *  Insertion sort: n is a party or the connected player list, and this runs once per repaint
     *  edge, not per frame. Every array element is read into a local before it is passed anywhere,
     *  per the container aliasing rule.
     */
    private static void SortOrder(array<int> order, array<string> keys, array<string> uids)
    {
        int count = order.Count();

        for (int i = 1; i < count; i++)
        {
            int moving = order.Get(i);
            string moving_key = keys.Get(moving);
            string moving_uid = uids.Get(moving);

            int j = i - 1;
            while (j >= 0)
            {
                int settled = order.Get(j);
                string settled_key = keys.Get(settled);
                string settled_uid = uids.Get(settled);

                int cmp = CompareStrings(settled_key, moving_key);
                if (cmp == 0)
                    cmp = CompareStrings(settled_uid, moving_uid);

                if (cmp <= 0)
                    break;

                order.Set(j + 1, settled);
                j = j - 1;
            }

            order.Set(j + 1, moving);
        }
    }

    /**
     *  Display order for the online column: every connected player matching the search, by name.
     *
     *  Sorted on rpc.list_names, which is what the row actually shows - the BattleRoyaleNameService
     *  correction is applied server-side, before this list is built.
     */
    private void BuildOnlineOrder(VigridPartyRPC rpc, array<int> order)
    {
        order.Clear();

        array<string> keys = new array<string>();
        int count = rpc.list_uids.Count();

        for (int i = 0; i < count; i++)
        {
            string lowered = rpc.list_names.Get(i);
            lowered.ToLower();

            keys.Insert(lowered);

            if (m_Filter == "" || lowered.IndexOf(m_Filter) >= 0)
                order.Insert(i);
        }

        SortOrder(order, keys, rpc.list_uids);
    }

    /**
     *  Display order for the party column: THE LEADER FIRST, then the rest by name.
     *
     *  A party is a handful of people, so what that column needs is not findability but the one row
     *  that is not interchangeable: the leader is who every Invite has to come from, who Promote and
     *  Kick belong to, and the only member whose identity changes what the buttons do.
     *
     *  Sorted on GetMemberName, never rpc.roster_names: an offline member's name can arrive as a
     *  stringtable key and only that accessor resolves it.
     */
    private void BuildMemberOrder(VigridPartyRPC rpc, array<int> order)
    {
        order.Clear();

        array<string> keys = new array<string>();
        int count = rpc.roster_uids.Count();

        for (int i = 0; i < count; i++)
        {
            string lowered = VigridPartyAPI.GetMemberName(i);
            lowered.ToLower();

            keys.Insert(lowered);

            if (i != rpc.leader_index)
                order.Insert(i);
        }

        SortOrder(order, keys, rpc.roster_uids);

        if (rpc.leader_index >= 0 && rpc.leader_index < count)
            order.InsertAt(rpc.leader_index, 0);
    }

    //! The first character of a name, capitalised, for the avatar square. UTF-8 aware.
    private static string Initial(string name)
    {
        if (name == "")
            return "";

        string first = name.SubstringUtf8(0, 1);
        first.ToUpper();
        return first;
    }

    private bool IsInvitePending(VigridPartyRPC rpc, int list_index)
    {
        string uid = rpc.list_uids.Get(list_index);
        if (m_CancelledInvites.Contains(uid))
            return false;

        if ((rpc.list_flags.Get(list_index) & VIGRID_PARTY_LIST_INVITED) != 0)
            return true;

        return m_SentInvites.Contains(uid);
    }

    //! Invitations out that are still waiting, to players not in a party yet.
    private int CountPendingInvites(VigridPartyRPC rpc)
    {
        int pending = 0;
        int count = rpc.list_uids.Count();
        for (int i = 0; i < count; i++)
        {
            if ((rpc.list_flags.Get(i) & VIGRID_PARTY_LIST_IN_PARTY) != 0)
                continue;
            if (IsInvitePending(rpc, i))
                pending = pending + 1;
        }

        return pending;
    }

    private void RefreshOnline(VigridPartyRPC rpc)
    {
        m_InviteTargets.Clear();

        //--- An invitation out holds a seat, as it does on the server: inviting past the size would
        //--- only get the late acceptors refused. With no party yet, you are the first seat.
        int seats = rpc.roster_uids.Count();
        int pending_count = CountPendingInvites(rpc);
        if (seats == 0 && pending_count > 0)
            seats = 1;
        seats = seats + pending_count;
        bool full = seats > 0 && seats >= rpc.max_party_size;

        bool can_invite = !rpc.locked && !full;
        if (rpc.roster_uids.Count() > 0 && rpc.self_index != rpc.leader_index)
            can_invite = false; //!< only the leader may grow an existing party

        int online_count = rpc.list_uids.Count();
        string heading = Widget.TranslateString("#STR_PARTY_ONLINE_PLAYERS");
        m_OnlineLabel.SetText(heading + " · " + online_count.ToString());

        //--- Display order only - `index` is the data index, `shown` is the row slot it is painted
        //--- into. Everything below reads rpc.list_* with `index` and never with the loop counter.
        array<int> order = new array<int>();
        BuildOnlineOrder(rpc, order);

        int shown = 0;
        int count = order.Count();

        for (int i = 0; i < count; i++)
        {
            int index = order.Get(i);

            Widget row = EnsureRow(m_OnlineRowPool, m_OnlineRows, "party_menu_player_row.layout", shown);
            if (!row)
                break;

            TextWidget name_widget = TextWidget.Cast(row.FindAnyWidget("PlayerRowName"));
            TextWidget status_widget = TextWidget.Cast(row.FindAnyWidget("PlayerRowStatus"));
            TextWidget initial_widget = TextWidget.Cast(row.FindAnyWidget("PlayerRowInitial"));
            ButtonWidget invite_button = ButtonWidget.Cast(row.FindAnyWidget("PlayerRowInviteButton"));
            Widget sent_widget = row.FindAnyWidget("PlayerRowSent");

            //--- A broken layout skips the row rather than throwing out of the whole refresh; the
            //--- slot is reused by the next index and trimmed at the end.
            if (!name_widget || !status_widget || !initial_widget || !invite_button || !sent_widget)
            {
                VigridPartyLog.Warn("Party menu: player row is missing a widget, skipped");
                continue;
            }

            string row_name = rpc.list_names.Get(index);
            int flags = rpc.list_flags.Get(index);
            bool in_party = (flags & VIGRID_PARTY_LIST_IN_PARTY) != 0;
            int their_size = (flags >> VIGRID_PARTY_LIST_SIZE_SHIFT) & 0xFF;
            bool pending = IsInvitePending(rpc, index);

            name_widget.SetText(row_name);
            initial_widget.SetText(Initial(row_name));

            //--- Saying WHY there is no button is the point: a row that simply lost its button reads
            //--- as a rendering fault, not as "this player already has a party".
            if (in_party && their_size > 0)
            {
                StringLocaliser sized = new StringLocaliser("STR_PARTY_IN_PARTY_COUNT", their_size.ToString());
                status_widget.SetText(sized.Format());
            }
            else if (in_party)
            {
                status_widget.SetText("#STR_PARTY_IN_PARTY");
            }
            else if (full && !pending)
            {
                status_widget.SetText("#STR_PARTY_FULL_STATUS");
            }
            else
            {
                status_widget.SetText("#STR_PARTY_SOLO");
            }

            if (in_party)
                status_widget.SetColor(COLOR_MUTED);
            else
                status_widget.SetColor(COLOR_DIM);

            invite_button.Show(can_invite && !in_party && !pending);
            sent_widget.Show(pending && !in_party);

            string row_uid = rpc.list_uids.Get(index);
            m_InviteTargets.Set(invite_button, row_uid);

            shown = shown + 1;
        }

        TrimPool(m_OnlineRowPool, shown);

        //--- Only a search can empty the list on purpose; an empty server says nothing.
        bool no_match = shown == 0 && m_Filter != "";
        m_OnlineEmpty.Show(no_match);
        if (no_match)
        {
            StringLocaliser none = new StringLocaliser("STR_PARTY_NO_MATCH", m_Filter);
            m_OnlineEmpty.SetText(none.Format());
        }
    }

    //! Card colours and which parts of the card show, for the three modes.
    private void PaintCard(Widget row, int edge, int fill, int avatar, int initial_colour, bool named)
    {
        row.FindAnyWidget("MemberRowEdge").SetColor(edge);
        row.FindAnyWidget("MemberRowFill").SetColor(fill);
        row.FindAnyWidget("MemberRowAvatar").SetColor(avatar);
        row.FindAnyWidget("MemberRowInitial").SetColor(initial_colour);
        row.FindAnyWidget("MemberRowName").Show(named);
        row.FindAnyWidget("MemberRowStatus").Show(named);
        row.FindAnyWidget("MemberRowOpen").Show(!named);
    }

    private void RefreshMembers(VigridPartyRPC rpc)
    {
        m_KickTargets.Clear();
        m_PromoteTargets.Clear();
        m_CancelTargets.Clear();

        bool is_leader = rpc.self_index >= 0 && rpc.self_index == rpc.leader_index;

        //--- Display order only - `index` is the ROSTER SLOT, `shown` is the row it is painted into.
        //--- Every comparison below is against the slot: rpc.leader_index and rpc.self_index are
        //--- slots too, and testing them against the loop counter would mark the wrong row.
        array<int> order = new array<int>();
        BuildMemberOrder(rpc, order);

        int shown = 0;
        int count = order.Count();
        int i = 0;
        Widget row = null;
        TextWidget name_widget = null;
        TextWidget status_widget = null;
        TextWidget initial_widget = null;

        for (i = 0; i < count; i++)
        {
            int index = order.Get(i);

            row = EnsureRow(m_MemberRowPool, m_MemberRows, "party_menu_member_row.layout", shown);
            if (!row)
                break;

            name_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowName"));
            status_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowStatus"));
            initial_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowInitial"));
            ButtonWidget promote_button = ButtonWidget.Cast(row.FindAnyWidget("MemberRowPromoteButton"));
            ButtonWidget kick_button = ButtonWidget.Cast(row.FindAnyWidget("MemberRowKickButton"));
            row.FindAnyWidget("MemberRowCancelButton").Show(false);

            if (!name_widget || !status_widget || !initial_widget || !promote_button || !kick_button)
            {
                VigridPartyLog.Warn("Party menu: member row is missing a widget, skipped");
                continue;
            }

            //--- The avatar takes the member's party slot colour - the same palette entry their
            //--- nametag, HUD row, map marker and pings use. Asked for by SLOT, which is why the
            //--- sort had to stay a display order.
            PaintCard(row, COLOR_CARD_EDGE, COLOR_CARD, VigridPartyAPI.GetMemberColour(index, 0.18), VigridPartyAPI.GetMemberColour(index, 1.0), true);

            //--- Through the API, not rpc.roster_names: an offline member's name can arrive as a
            //--- stringtable key and only GetMemberName resolves it.
            string member_name = VigridPartyAPI.GetMemberName(index);
            name_widget.SetText(member_name);
            initial_widget.SetText(Initial(member_name));

            bool online = VigridPartyAPI.IsMemberOnline(index);
            row.FindAnyWidget("MemberRowBadge").Show(index == rpc.leader_index);

            if (index == rpc.self_index)
                status_widget.SetText("#STR_PARTY_STATUS_YOU");
            else if (!online)
                status_widget.SetText("#STR_PARTY_HUD_OFFLINE");
            else
                status_widget.SetText("#STR_PARTY_STATUS_ONLINE");

            //--- Both branches, on purpose: rows are pooled and keep their last colour otherwise.
            if (online)
                name_widget.SetColor(COLOR_TEXT);
            else
                name_widget.SetColor(COLOR_MUTED);

            //--- Never offer to promote or kick yourself; leaving is what that button is for.
            bool actionable = is_leader && !rpc.locked && index != rpc.self_index;
            promote_button.Show(actionable);
            kick_button.Show(actionable);

            string row_uid = rpc.roster_uids.Get(index);
            m_PromoteTargets.Set(promote_button, row_uid);
            m_KickTargets.Set(kick_button, row_uid);

            shown = shown + 1;
        }

        //--- Invitations on their way take the next cards, then open slots fill the party's size -
        //--- but only for someone who can fill them: not while the formation is locked, and not for
        //--- a member who is not the leader, since only the leader may invite.
        bool can_grow = !rpc.locked && (rpc.roster_uids.Count() == 0 || is_leader);
        int capacity = rpc.max_party_size;
        if (!can_grow)
            capacity = shown;

        int list_count = rpc.list_uids.Count();
        for (i = 0; i < list_count && shown < capacity; i++)
        {
            if (!IsInvitePending(rpc, i))
                continue;
            if ((rpc.list_flags.Get(i) & VIGRID_PARTY_LIST_IN_PARTY) != 0)
                continue;

            row = EnsureRow(m_MemberRowPool, m_MemberRows, "party_menu_member_row.layout", shown);
            if (!row)
                break;

            string pending_name = rpc.list_names.Get(i);
            PaintCard(row, COLOR_PENDING_EDGE, COLOR_WINDOW, COLOR_AVATAR_NEUTRAL, COLOR_MUTED, true);
            name_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowName"));
            status_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowStatus"));
            initial_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowInitial"));
            name_widget.SetText(pending_name);
            name_widget.SetColor(COLOR_TEXT);
            status_widget.SetText("#STR_PARTY_STATUS_PENDING");
            initial_widget.SetText(Initial(pending_name));
            HideCardExtras(row);

            //--- Only the inviter sees their invitations, so the button is always theirs to press.
            ButtonWidget cancel_button = ButtonWidget.Cast(row.FindAnyWidget("MemberRowCancelButton"));
            cancel_button.Show(true);
            string pending_uid = rpc.list_uids.Get(i);
            m_CancelTargets.Set(cancel_button, pending_uid);

            shown = shown + 1;
        }

        while (shown < capacity)
        {
            row = EnsureRow(m_MemberRowPool, m_MemberRows, "party_menu_member_row.layout", shown);
            if (!row)
                break;

            PaintCard(row, COLOR_OPEN_EDGE, COLOR_WINDOW, COLOR_AVATAR_OPEN, COLOR_DIM, false);
            initial_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowInitial"));
            initial_widget.SetText("+");
            HideCardExtras(row);

            shown = shown + 1;
        }

        TrimPool(m_MemberRowPool, shown);
    }

    private void HideCardExtras(Widget row)
    {
        row.FindAnyWidget("MemberRowBadge").Show(false);
        row.FindAnyWidget("MemberRowPromoteButton").Show(false);
        row.FindAnyWidget("MemberRowKickButton").Show(false);
        row.FindAnyWidget("MemberRowCancelButton").Show(false);
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
        return w / LAYOUT_DIALOG_W;
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

        //--- The close key cap fits whatever the menu key is bound to.
        float cap_w = TextWidth(m_CloseKeyText) + 24 * s;
        if (cap_w < 54 * s)
            cap_w = 54 * s;
        m_CloseKey.SetSize(cap_w, 30 * s);

        LayoutBody(s);
        LayoutFooter(s);
        LayoutCards(s);
    }

    //! The columns start under the invitation banner while there is one.
    private void LayoutBody(float s)
    {
        float top = BODY_TOP;
        if (m_InviteBanner.IsVisible())
            top = BODY_TOP_BANNER;

        float h = FOOTER_TOP - top;
        m_Body.SetPos(0, top * s);
        m_Body.SetSize(LAYOUT_DIALOG_W * s, h * s);
        m_ColumnLine.SetSize(2 * s, h * s);
        m_OnlineColumn.SetSize(866 * s, h * s);
        m_PartyScroll.SetSize((CARD_W + 18) * s, (h - 66 - 24) * s);
        m_OnlineScroll.SetSize(806 * s, (h - 86 - 24) * s);
    }

    //! Buttons sized to their labels from the left; the ping hints right-aligned.
    private void LayoutFooter(float s)
    {
        float x = 42 * s;
        x = PlaceFooterButton(m_LeaveButton, m_LeaveLabel, x, s);
        PlaceFooterButton(m_DisbandButton, m_DisbandLabel, x, s);

        float right = (LAYOUT_DIALOG_W - 42) * s;
        right = PlaceHint(m_PingClearKey, m_PingClearKeyText, m_PingClearHint, right, s) - 30 * s;
        PlaceHint(m_PingKey, m_PingKeyText, m_PingHint, right, s);
    }

    private float PlaceFooterButton(ButtonWidget button, TextWidget label, float x, float s)
    {
        if (!button.IsVisible())
            return x;

        float w = TextWidth(label) + 48 * s;
        if (w < 150 * s)
            w = 150 * s;

        button.SetPos(x, 21 * s);
        button.SetSize(w, 60 * s);
        return x + w + 15 * s;
    }

    //! [key] hint, ending at `right`. Returns its left edge.
    private float PlaceHint(Widget cap, TextWidget cap_text, TextWidget hint, float right, float s)
    {
        float hint_w = TextWidth(hint);
        hint.SetPos(right - hint_w, 36 * s);
        hint.SetSize(hint_w, 30 * s);

        float cap_w = TextWidth(cap_text) + 18 * s;
        if (cap_w < 36 * s)
            cap_w = 36 * s;

        float cap_x = right - hint_w - 12 * s - cap_w;
        cap.SetPos(cap_x, 36 * s);
        cap.SetSize(cap_w, 30 * s);
        return cap_x;
    }

    //! The leader badge just after the name, and the name stopping short of the buttons.
    private void LayoutCards(float s)
    {
        int count = m_MemberRowPool.Count();
        for (int i = 0; i < count; i++)
        {
            Widget row = m_MemberRowPool.Get(i);
            TextWidget name_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowName"));
            Widget badge = row.FindAnyWidget("MemberRowBadge");
            TextWidget badge_text = TextWidget.Cast(row.FindAnyWidget("MemberRowBadgeText"));
            Widget promote = row.FindAnyWidget("MemberRowPromoteButton");
            Widget cancel = row.FindAnyWidget("MemberRowCancelButton");

            float name_max = CARD_W - 99;
            if (promote.IsVisible())
                name_max = CARD_W - 84 - 15 - 249 - 12;
            else if (cancel.IsVisible())
                name_max = CARD_W - 84 - 15 - 120 - 12;

            name_widget.SetSize(name_max * s, 36 * s);

            if (!badge.IsVisible())
                continue;

            float badge_w = TextWidth(badge_text) + 18 * s;
            float name_w = TextWidth(name_widget);
            if (name_w > name_max * s - badge_w - 12 * s)
                name_w = name_max * s - badge_w - 12 * s;

            badge.SetPos(84 * s + name_w + 12 * s, 18 * s);
            badge.SetSize(badge_w, 27 * s);
        }
    }

    //------------------------------------------------------------------------------------------
    //--- Input
    //------------------------------------------------------------------------------------------

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w != m_SearchEdit)
            return super.OnChange(w, x, y, finished);

        string filter = m_SearchEdit.GetText();
        filter.TrimInPlace();
        filter.ToLower();

        m_SearchPlaceholder.Show(m_SearchEdit.GetText() == "");

        if (filter == m_Filter)
            return true;

        m_Filter = filter;
        RefreshOnline(VigridPartyRPC.GetInstance());
        return true;
    }

    /**
     *  Scroll whichever column the pointer is over.
     *
     *  A ScrollWidget does NOT scroll itself on the mouse wheel - nothing in the engine converts the
     *  wheel into a scroll. Vanilla's own ScrollBarContainer implements it by hand
     *  (scrollbarcontainer.c:220) and that is the precedent being followed here.
     *
     *  Which column is decided from the pointer's screen rect rather than from `w`, because `w` is
     *  whatever child happens to be under the cursor. Sign follows vanilla: a positive wheel scrolls UP.
     */
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        ScrollWidget target = ScrollUnderPointer(x, y);
        if (!target)
            return super.OnMouseWheel(w, x, y, wheel);

        target.VScrollStep(-wheel);
        return true;
    }

    private ScrollWidget ScrollUnderPointer(int x, int y)
    {
        if (IsPointerOver(m_OnlineScroll, x, y))
            return m_OnlineScroll;

        if (IsPointerOver(m_PartyScroll, x, y))
            return m_PartyScroll;

        return null;
    }

    //! Screen-space hit test. GetScreenPos/GetScreenSize report real pixels, which is the same space
    //! the wheel event's x/y arrive in.
    private bool IsPointerOver(Widget target, int x, int y)
    {
        if (!target)
            return false;

        float px;
        float py;
        target.GetScreenPos(px, py);

        float sw;
        float sh;
        target.GetScreenSize(sw, sh);

        if (x < px)
            return false;
        if (y < py)
            return false;
        if (x > px + sw)
            return false;
        if (y > py + sh)
            return false;

        return true;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        VigridPartyClient client = GetClient();
        if (!client)
            return super.OnClick(w, x, y, button);

        if (w == m_CloseButton)
        {
            Close();
            return true;
        }

        if (w == m_LeaveButton)
        {
            client.Leave();
            return true;
        }

        if (w == m_DisbandButton)
        {
            client.Disband();
            return true;
        }

        if (w == m_InviteAccept)
        {
            client.RespondToInvite(true);
            RefreshInviteBanner(VigridPartyRPC.GetInstance());
            return true;
        }

        if (w == m_InviteDecline)
        {
            client.RespondToInvite(false);
            RefreshInviteBanner(VigridPartyRPC.GetInstance());
            return true;
        }

        if (m_InviteTargets.Contains(w))
        {
            string invited_uid = m_InviteTargets.Get(w);
            client.Invite(invited_uid);

            //--- Pending at once; the next list reply confirms it or takes it back.
            m_SentInvites.Set(invited_uid, true);
            RefreshAll(VigridPartyRPC.GetInstance());
            return true;
        }

        if (m_CancelTargets.Contains(w))
        {
            string cancelled_uid = m_CancelTargets.Get(w);
            client.CancelInvite(cancelled_uid);

            //--- Gone at once; the next list reply confirms it.
            m_SentInvites.Remove(cancelled_uid);
            m_CancelledInvites.Set(cancelled_uid, true);
            RefreshAll(VigridPartyRPC.GetInstance());
            return true;
        }

        if (m_PromoteTargets.Contains(w))
        {
            client.TransferLeader(m_PromoteTargets.Get(w));
            return true;
        }

        if (m_KickTargets.Contains(w))
        {
            client.Kick(m_KickTargets.Get(w));
            return true;
        }

        return super.OnClick(w, x, y, button);
    }
}
#endif
