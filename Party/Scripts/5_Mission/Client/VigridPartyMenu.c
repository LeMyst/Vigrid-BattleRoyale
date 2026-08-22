#ifndef SERVER
/**
 *  Vigrid Party - the party management screen.
 *
 *  Left column: players connected to the server, each with an Invite button. Right column: the
 *  current party, with Promote/Kick on each member when you are the leader.
 *
 *  The online list is server-authoritative rather than read from ClientData.m_PlayerList: vanilla's
 *  own preprocessing skips entries whose identity has not synchronised yet, and it has no idea who
 *  is already in a party. Asking the server gets both right, and it is the server that will have
 *  to validate the invite anyway.
 *
 *  Rows are pooled and positioned manually; surplus rows are hidden rather than destroyed.
 *
 *  BOTH COLUMNS ARE SORTED BY DISPLAYED NAME - see BuildOnlineOrder / BuildMemberOrder. The sort is
 *  a DISPLAY ORDER ONLY: nothing here reorders rpc.list_* or rpc.roster_*, because a roster slot
 *  index is the member's palette colour everywhere else in the mod (nametag, HUD, map marker, ping)
 *  and slot 0 is what "the leader is the first member" means on the server. What the sort produces
 *  is an array of DATA indices, walked in order while the pooled rows keep their own creation
 *  order - which is also why no row has to be relinked: the WrapSpacer's children never move, only
 *  the content written into them does.
 */
class VigridPartyMenu extends UIScriptedMenu
{
    //--- Row name colours. The grey matches VigridPartyHud.COLOR_INACTIVE, so a member reads the
    //--- same way in the panel and in this list; the white is the layout's own default, restated
    //--- here because rows are pooled and a recycled row keeps whatever colour it was last given.
    private static const int COLOR_MEMBER = 0xFFFFFFFF;
    private static const int COLOR_MEMBER_OFFLINE = 0xFF787878;

    //--- Left-column accent bar. Availability only - these players are not in YOUR party, so they
    //--- have no slot colour to show. Members get theirs from VigridPartyPalette instead.
    private static const int COLOR_ACCENT_FREE = 0xFF8CA3B0;
    private static const int COLOR_ACCENT_TAKEN = 0xFF4A4A4A;

    //--- There is no ROW_HEIGHT any more. The WrapSpacer in party_menu.layout derives the pitch from
    //--- the rows' own declared height, which is what keeps it correct at every resolution - see
    //--- EnsureRow.

    private Widget m_OnlineRows;
    private Widget m_MemberRows;
    private ScrollWidget m_OnlineScroll;
    private ScrollWidget m_PartyScroll;
    private Widget m_InviteBanner;
    private TextWidget m_InviteText;
    private ButtonWidget m_InviteAccept;
    private ButtonWidget m_InviteDecline;
    private ButtonWidget m_CreateButton;
    private ButtonWidget m_LeaveButton;
    private ButtonWidget m_DisbandButton;
    private ButtonWidget m_CloseButton;

    private ref array<Widget> m_OnlineRowPool;
    private ref array<Widget> m_MemberRowPool;

    //--- Button widget -> the uid that button acts on.
    private ref map<Widget, string> m_InviteTargets;
    private ref map<Widget, string> m_KickTargets;
    private ref map<Widget, string> m_PromoteTargets;

    private int m_LastListSeq;
    private int m_LastRosterSeq;
    private int m_LastInviteSeq;
    private int m_RefreshDueMs;

    void VigridPartyMenu()
    {
        m_OnlineRowPool = new array<Widget>();
        m_MemberRowPool = new array<Widget>();
        m_InviteTargets = new map<Widget, string>();
        m_KickTargets = new map<Widget, string>();
        m_PromoteTargets = new map<Widget, string>();

        m_LastListSeq = -1;
        m_LastRosterSeq = -1;
        m_LastInviteSeq = -1;
        m_RefreshDueMs = 0;
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/party_menu.layout");

        m_OnlineRows = layoutRoot.FindAnyWidget("OnlineRows");
        m_MemberRows = layoutRoot.FindAnyWidget("MemberRows");
        m_OnlineScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("OnlineScroll"));
        m_PartyScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("PartyScroll"));
        m_InviteBanner = layoutRoot.FindAnyWidget("InviteBanner");
        m_InviteText = TextWidget.Cast(layoutRoot.FindAnyWidget("InviteText"));
        m_InviteAccept = ButtonWidget.Cast(layoutRoot.FindAnyWidget("InviteAcceptButton"));
        m_InviteDecline = ButtonWidget.Cast(layoutRoot.FindAnyWidget("InviteDeclineButton"));
        m_CreateButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CreateButton"));
        m_LeaveButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("LeaveButton"));
        m_DisbandButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("DisbandButton"));
        m_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));

        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();

        SetFocus(layoutRoot);
        GetGame().GetInput().ChangeGameFocus(1);
        GetGame().GetUIManager().ShowUICursor(true);

        //--- Force a full repaint: sequence numbers may not have moved while the menu was closed.
        m_LastListSeq = -1;
        m_LastRosterSeq = -1;
        m_LastInviteSeq = -1;
        m_RefreshDueMs = 0;
    }

    override void OnHide()
    {
        super.OnHide();

        //--- ChangeGameFocus(-1), not ResetGameFocus(): the reset SETS the shared additive input
        //--- focus counter to zero across all devices (vanilla input.c:22-27), which releases every
        //--- other holder's acquire and not just ours. Latent here - this menu only opens when no
        //--- other menu is up - but it is the same shape that was live elsewhere, and the pair is
        //--- balanced against the ChangeGameFocus(1) in OnShow either way.
        GetGame().GetInput().ChangeGameFocus(-1);
        GetGame().GetUIManager().ShowUICursor(false);
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

        //--- The roster changes which players are invitable, so it repaints both columns.
        if (rpc.roster_seq != m_LastRosterSeq)
        {
            m_LastRosterSeq = rpc.roster_seq;
            m_LastListSeq = -1;
            RefreshMembers(rpc);
            RefreshButtons(rpc);
        }

        if (rpc.list_seq != m_LastListSeq)
        {
            m_LastListSeq = rpc.list_seq;
            RefreshOnline(rpc);
        }
    }

    private void RefreshInviteBanner(VigridPartyRPC rpc)
    {
        if (!rpc.HasInvite())
        {
            m_InviteBanner.Show(false);
            return;
        }

        StringLocaliser prompt = new StringLocaliser("STR_PARTY_INVITE_PROMPT", rpc.invite_inviter_name);
        m_InviteText.SetText(prompt.Format());
        m_InviteBanner.Show(true);
    }

    private void RefreshButtons(VigridPartyRPC rpc)
    {
        bool in_party = rpc.roster_uids.Count() > 0;
        bool is_leader = rpc.self_index >= 0 && rpc.self_index == rpc.leader_index;

        //--- Everything is refused server-side while a match is running; greying the buttons just
        //--- avoids inviting a rejection the player cannot act on.
        m_CreateButton.Show(!in_party && !rpc.locked);
        m_LeaveButton.Show(in_party && !rpc.locked);
        m_DisbandButton.Show(in_party && is_leader && !rpc.locked);
    }

    /**
     *  The `index`-th row, created if the pool is not that long yet.
     *
     *  DELIBERATELY DOES NOT SetPos THE ROW. The rows hang off a WrapSpacer, which lays its own
     *  children out in creation order; positioning them by hand fights it. The previous version did
     *  `SetPos(0, index * ROW_HEIGHT)`, and that was wrong twice over - it fought a spacer that was
     *  not there yet, and it mixed unit systems. SetPos takes REAL SCREEN PIXELS while a layout's
     *  declared geometry is scaled by viewport/1920, so on any client that was not 1920 wide the
     *  44 px pitch and the ~30 px rendered row height disagreed and the list spread out or
     *  overlapped. That is the "bad scaling": the spacing was right only at one resolution.
     */
    private Widget EnsureRow(array<Widget> pool, Widget parent, string layout_name, int index)
    {
        while (pool.Count() <= index)
        {
            Widget created = GetGame().GetWorkspace().CreateWidgets(VIGRID_PARTY_PREFIX + "GUI/layouts/" + layout_name, parent);
            if (!created)
                return null;

            pool.Insert(created);
        }

        Widget row = pool.Get(index);
        row.Show(true);
        return row;
    }

    /**
     *  Destroy every pooled row from `first` on.
     *
     *  Unlink, not Show(false), which is what this used to do: a spacer lays out the children it
     *  HAS, so a hidden row still reserves its slot - leaving a gap in the middle of the list and a
     *  scroll range longer than the content. Unlink() destroys the widget and all its children.
     *
     *  Walked backwards so each removal is of the last element and cannot renumber an index still to
     *  be visited. RemoveOrdered rather than Remove for the same reason it is used everywhere else
     *  here: vanilla's Remove() fills the hole with the LAST element.
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
     *  comparison at all - no Compare, no relational operator worth depending on. The one native
     *  sort there is (array<string>.Sort, "strings alphabetically") sorts the array IN PLACE, which
     *  is exactly what must not happen here: the whole point is to leave the source arrays alone and
     *  produce an order over them. Packing "name + index" into a throwaway array to borrow that sort
     *  would need a separator no player name can contain, and there is no such character.
     *
     *  Bytes, not characters. Masked to 0..255 so a UTF-8 lead byte compares as unsigned and a
     *  non-ASCII name sorts after an ASCII one instead of before it. That is byte order rather than
     *  collation order - it will not put E after E for a French reader - but it is deterministic and
     *  identical on every client, which is the property this sort is actually for.
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
     *  with enable_steam_name_lookup off - the engine only disambiguates the second one, and the
     *  third onwards share the name outright. A merely stable sort would leave them in arrival
     *  order, i.e. exactly the churn this issue exists to remove, and only for the players hardest to
     *  tell apart. Sorting them by uid pins them.
     *
     *  Insertion sort: n is a party (single digits) or the connected player list (capped at the
     *  server's slot count), and this runs once per repaint edge, not per frame.
     *
     *  Every array element is read into a local before it is passed anywhere, per the container
     *  aliasing rule - an array read sharing an expression with a call has been measured to return
     *  another array's contents.
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
     *  Display order for the left column: every connected player, by name.
     *
     *  Sorted on rpc.list_names, which is what the row actually shows - so a player wearing a
     *  BattleRoyaleNameService override sorts under the name on screen rather than under the
     *  `Survivor` they connected as. That falls out for free because the correction is applied
     *  server-side, before this list is built; there is nothing to resolve on this side.
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
            order.Insert(i);
        }

        SortOrder(order, keys, rpc.list_uids);
    }

    /**
     *  Display order for the right column: THE LEADER FIRST, then the rest by name.
     *
     *  The open question in #283 was whether to pin the leader, and this is the answer. A party is
     *  a handful of people, so "findable" is not really what the member column needs; what it needs
     *  is the one row that is not interchangeable with the others. The leader is who every Invite
     *  has to come from, who Promote and Kick belong to, and the only member whose identity changes
     *  what the buttons on YOUR row do. Pinned at the top it is answered by position; sorted into
     *  the middle it has to be found by reading the tag on each row.
     *
     *  Sorted on GetMemberName, never rpc.roster_names: an offline member's name can arrive as a
     *  stringtable key and only that accessor resolves it. Sorting the raw value would file every
     *  such member under `#`.
     *
     *  Names are lowered once, into a key array parallel to the ROSTER (so it is indexed by roster
     *  slot, including the leader's, and stays aligned with roster_uids), rather than lowered inside
     *  the comparison - which would be O(n log n) proto calls for no reason.
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

    private void RefreshOnline(VigridPartyRPC rpc)
    {
        m_InviteTargets.Clear();

        bool can_invite = !rpc.locked;
        if (rpc.roster_uids.Count() > 0 && rpc.self_index != rpc.leader_index)
            can_invite = false; //!< only the leader may grow an existing party
        if (rpc.roster_uids.Count() >= rpc.max_party_size && rpc.roster_uids.Count() > 0)
            can_invite = false;

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
            Widget accent_widget = row.FindAnyWidget("PlayerRowAccent");
            ButtonWidget invite_button = ButtonWidget.Cast(row.FindAnyWidget("PlayerRowInviteButton"));

            string row_name = rpc.list_names.Get(index);
            name_widget.SetText(row_name);

            //--- bit0 means the server already has them in a party.
            bool already_in_party = (rpc.list_flags.Get(index) & 1) != 0;

            //--- The button and the status label share the right-hand cell and are mutually
            //--- exclusive. Saying WHY there is no button is the point: a row that simply lost its
            //--- button reads as a rendering fault, not as "this player already has a party".
            invite_button.Show(can_invite && !already_in_party);
            status_widget.Show(already_in_party);

            if (already_in_party)
            {
                status_widget.SetText("#STR_PARTY_IN_PARTY");
                accent_widget.SetColor(COLOR_ACCENT_TAKEN);
            }
            else
            {
                accent_widget.SetColor(COLOR_ACCENT_FREE);
            }

            //--- Set on both branches, like every other per-row property here: rows are pooled, so
            //--- one left tinted by an earlier refresh keeps that tint otherwise.
            name_widget.SetColor(COLOR_MEMBER);
            if (already_in_party)
                name_widget.SetColor(COLOR_MEMBER_OFFLINE);

            string row_uid = rpc.list_uids.Get(index);
            m_InviteTargets.Set(invite_button, row_uid);

            shown = shown + 1;
        }

        TrimPool(m_OnlineRowPool, shown);
    }

    private void RefreshMembers(VigridPartyRPC rpc)
    {
        m_KickTargets.Clear();
        m_PromoteTargets.Clear();

        bool is_leader = rpc.self_index >= 0 && rpc.self_index == rpc.leader_index;

        //--- Display order only - `index` is the ROSTER SLOT, `shown` is the row it is painted into.
        //--- Every comparison below is against the slot: rpc.leader_index and rpc.self_index are
        //--- slots too, and testing them against the loop counter would mark the wrong row.
        array<int> order = new array<int>();
        BuildMemberOrder(rpc, order);

        int shown = 0;
        int count = order.Count();

        for (int i = 0; i < count; i++)
        {
            int index = order.Get(i);

            Widget row = EnsureRow(m_MemberRowPool, m_MemberRows, "party_menu_member_row.layout", shown);
            if (!row)
                break;

            TextWidget name_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowName"));
            TextWidget status_widget = TextWidget.Cast(row.FindAnyWidget("MemberRowStatus"));
            Widget accent_widget = row.FindAnyWidget("MemberRowAccent");
            ButtonWidget promote_button = ButtonWidget.Cast(row.FindAnyWidget("MemberRowPromoteButton"));
            ButtonWidget kick_button = ButtonWidget.Cast(row.FindAnyWidget("MemberRowKickButton"));

            //--- Through the API, not rpc.roster_names: an offline member's name can arrive as a
            //--- stringtable key and only GetMemberName resolves it.
            name_widget.SetText(VigridPartyAPI.GetMemberName(index));

            //--- The member's party slot colour - the same palette entry their nametag, HUD row, map
            //--- marker and pings use, so identity is one glance in every surface. Asked for by SLOT,
            //--- which is why the sort had to stay a display order: reordering the roster itself
            //--- would repaint every teammate a different colour in five other surfaces at once.
            accent_widget.SetColor(VigridPartyAPI.GetMemberColour(index, 1.0));

            //--- Leader and Offline now live on their own line rather than being appended to the
            //--- name. Appending had to happen after GetMemberName had already localised the name,
            //--- could not be styled apart from it, and lengthened the tightest cell in the row.
            //--- Offline wins when a member is both: it is the more actionable of the two.
            bool online = VigridPartyAPI.IsMemberOnline(index);
            bool is_row_leader = (index == rpc.leader_index);

            status_widget.Show(!online || is_row_leader);

            if (!online)
                status_widget.SetText("#STR_PARTY_HUD_OFFLINE");
            else if (is_row_leader)
                status_widget.SetText("#STR_PARTY_LEADER_TAG");

            //--- The colour is set on BOTH branches on purpose: rows are pooled, so a row left grey
            //--- by an earlier refresh stays grey otherwise.
            if (online)
                name_widget.SetColor(COLOR_MEMBER);
            else
                name_widget.SetColor(COLOR_MEMBER_OFFLINE);

            //--- Never offer to promote or kick yourself; leaving is what that button is for.
            bool actionable = is_leader && !rpc.locked && index != rpc.self_index;
            promote_button.Show(actionable);
            kick_button.Show(actionable);

            string row_uid = rpc.roster_uids.Get(index);
            m_PromoteTargets.Set(promote_button, row_uid);
            m_KickTargets.Set(kick_button, row_uid);

            shown = shown + 1;
        }

        TrimPool(m_MemberRowPool, shown);
    }

    /**
     *  Scroll whichever column the pointer is over.
     *
     *  A ScrollWidget does NOT scroll itself on the mouse wheel - nothing in the engine converts the
     *  wheel into a scroll, and no vanilla script calls VScrollStep at all. Vanilla's own
     *  ScrollBarContainer implements the wheel by hand (scrollbarcontainer.c:220) and that is the
     *  precedent being followed here.
     *
     *  This is a SEPARATE defect from the one that made the list clip, and fixing that one is what
     *  exposed it: with a correct WrapSpacer the content really is taller than the viewport and the
     *  scrollbar really does appear, so the list looks scrollable and simply is not. Both halves are
     *  needed. `GetContentHeight()` and `IsScrollbarVisible()` cannot see this - they were already
     *  reporting 556 px against a 360 px viewport with the bar shown.
     *
     *  Which column is decided from the pointer's screen rect rather than from `w`, because `w` is
     *  whatever child happens to be under the cursor - a row, a text cell, a button, or the empty
     *  spacer below the last row - and walking its parents is one more thing to get wrong.
     *
     *  Sign follows vanilla's convention in the same method: a positive wheel scrolls UP.
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

        if (w == m_CreateButton)
        {
            client.CreateParty();
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
            client.Invite(m_InviteTargets.Get(w));
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
