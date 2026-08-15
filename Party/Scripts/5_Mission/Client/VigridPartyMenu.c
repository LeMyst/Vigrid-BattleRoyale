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

    private void RefreshOnline(VigridPartyRPC rpc)
    {
        m_InviteTargets.Clear();

        bool can_invite = !rpc.locked;
        if (rpc.roster_uids.Count() > 0 && rpc.self_index != rpc.leader_index)
            can_invite = false; //!< only the leader may grow an existing party
        if (rpc.roster_uids.Count() >= rpc.max_party_size && rpc.roster_uids.Count() > 0)
            can_invite = false;

        int shown = 0;
        int count = rpc.list_uids.Count();

        for (int i = 0; i < count; i++)
        {
            Widget row = EnsureRow(m_OnlineRowPool, m_OnlineRows, "party_menu_player_row.layout", shown);
            if (!row)
                break;

            TextWidget name_widget = TextWidget.Cast(row.FindAnyWidget("PlayerRowName"));
            TextWidget status_widget = TextWidget.Cast(row.FindAnyWidget("PlayerRowStatus"));
            Widget accent_widget = row.FindAnyWidget("PlayerRowAccent");
            ButtonWidget invite_button = ButtonWidget.Cast(row.FindAnyWidget("PlayerRowInviteButton"));

            name_widget.SetText(rpc.list_names.Get(i));

            //--- bit0 means the server already has them in a party.
            bool already_in_party = (rpc.list_flags.Get(i) & 1) != 0;

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

            m_InviteTargets.Set(invite_button, rpc.list_uids.Get(i));

            shown = shown + 1;
        }

        TrimPool(m_OnlineRowPool, shown);
    }

    private void RefreshMembers(VigridPartyRPC rpc)
    {
        m_KickTargets.Clear();
        m_PromoteTargets.Clear();

        bool is_leader = rpc.self_index >= 0 && rpc.self_index == rpc.leader_index;

        int shown = 0;
        int count = rpc.roster_uids.Count();

        for (int i = 0; i < count; i++)
        {
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
            name_widget.SetText(VigridPartyAPI.GetMemberName(i));

            //--- The member's party slot colour - the same palette entry their nametag, HUD row, map
            //--- marker and pings use, so identity is one glance in every surface.
            accent_widget.SetColor(VigridPartyAPI.GetMemberColour(i, 1.0));

            //--- Leader and Offline now live on their own line rather than being appended to the
            //--- name. Appending had to happen after GetMemberName had already localised the name,
            //--- could not be styled apart from it, and lengthened the tightest cell in the row.
            //--- Offline wins when a member is both: it is the more actionable of the two.
            bool online = VigridPartyAPI.IsMemberOnline(i);
            bool is_row_leader = (i == rpc.leader_index);

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
            bool actionable = is_leader && !rpc.locked && i != rpc.self_index;
            promote_button.Show(actionable);
            kick_button.Show(actionable);

            m_PromoteTargets.Set(promote_button, rpc.roster_uids.Get(i));
            m_KickTargets.Set(kick_button, rpc.roster_uids.Get(i));

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
