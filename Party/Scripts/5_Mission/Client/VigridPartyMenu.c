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
    private static const int ROW_HEIGHT = 44;

    private Widget m_OnlineRows;
    private Widget m_MemberRows;
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

        GetGame().GetInput().ResetGameFocus();
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
        row.SetPos(0, index * ROW_HEIGHT);
        row.Show(true);
        return row;
    }

    private void HideFrom(array<Widget> pool, int first)
    {
        for (int i = first; i < pool.Count(); i++)
        {
            pool.Get(i).Show(false);
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
            ButtonWidget invite_button = ButtonWidget.Cast(row.FindAnyWidget("PlayerRowInviteButton"));

            name_widget.SetText(rpc.list_names.Get(i));

            //--- bit0 means the server already has them in a party.
            bool already_in_party = (rpc.list_flags.Get(i) & 1) != 0;
            invite_button.Show(can_invite && !already_in_party);

            m_InviteTargets.Set(invite_button, rpc.list_uids.Get(i));

            shown = shown + 1;
        }

        HideFrom(m_OnlineRowPool, shown);
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
            ButtonWidget promote_button = ButtonWidget.Cast(row.FindAnyWidget("MemberRowPromoteButton"));
            ButtonWidget kick_button = ButtonWidget.Cast(row.FindAnyWidget("MemberRowKickButton"));

            string display_name = rpc.roster_names.Get(i);
            if (i == rpc.leader_index)
                display_name = display_name + " *";

            name_widget.SetText(display_name);

            //--- Never offer to promote or kick yourself; leaving is what that button is for.
            bool actionable = is_leader && !rpc.locked && i != rpc.self_index;
            promote_button.Show(actionable);
            kick_button.Show(actionable);

            m_PromoteTargets.Set(promote_button, rpc.roster_uids.Get(i));
            m_KickTargets.Set(kick_button, rpc.roster_uids.Get(i));

            shown = shown + 1;
        }

        HideFrom(m_MemberRowPool, shown);
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
