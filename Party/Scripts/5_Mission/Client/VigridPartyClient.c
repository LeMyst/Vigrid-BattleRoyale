#ifndef SERVER
/**
 *  Vigrid Party - client controller.
 *
 *  Owns the HUD panel and the name tag renderer, drains anything the RPC receiver has queued, and
 *  is the single place client -> server commands are sent from.
 *
 *  Commands never carry a target: the server resolves the actor from the RPC sender identity and
 *  re-validates everything. Passing one would be how a client asks the server to act on somebody
 *  else's behalf.
 */
class VigridPartyClient
{
    private ref VigridPartyHud m_Hud;
    private ref VigridPartyNametags m_Nametags;

    private int m_HudDueMs;
    private int m_LastInviteSeq;
    private bool m_RequestedSync;

    /**
     *  Traced step by step on purpose. This runs inside MissionGameplay.OnInit, which is early
     *  enough that a failure here stalls the client on the loading screen with nothing after the
     *  Battle Royale mod's own OnInit line in the script log - so each step announces itself and
     *  the log says exactly how far it got.
     */
    void VigridPartyClient()
    {
        VigridPartyLog.Debug("VigridPartyClient ctor: registering RPCs");
        VigridPartyRPC.GetInstance().Reset();

        VigridPartyLog.Debug("VigridPartyClient ctor: creating HUD");
        m_Hud = new VigridPartyHud();

        VigridPartyLog.Debug("VigridPartyClient ctor: creating nametags");
        m_Nametags = new VigridPartyNametags();

        m_HudDueMs = 0;
        m_LastInviteSeq = 0;
        m_RequestedSync = false;

        VigridPartyLog.Debug("VigridPartyClient ctor: done");
    }

    void ~VigridPartyClient()
    {
        m_Hud = NULL;
        m_Nametags = NULL;
    }

    void Update(float timeslice)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        //--- Ask once, on the first tick after the mission is live, for settings plus roster. The
        //--- server also pushes both on connect; this covers a client that loaded late.
        if (!m_RequestedSync)
        {
            m_RequestedSync = true;
            RequestSync();
        }

        DrainNotifications(rpc);
        AnnounceInvite(rpc);

        //--- Name tags follow the camera, so they must run every frame.
        m_Nametags.Update(timeslice);

        //--- The panel is fed by a 2 Hz push, so refreshing it faster than 5 Hz buys nothing.
        int now_ms = GetGame().GetTime();
        if (now_ms < m_HudDueMs)
            return;

        m_HudDueMs = now_ms + 200;
        m_Hud.Update();
    }

    private void DrainNotifications(VigridPartyRPC rpc)
    {
        int count = rpc.pending_notifications.Count();
        if (count == 0)
            return;

        for (int i = 0; i < count; i++)
        {
            GetGame().Chat(rpc.pending_notifications.Get(i), "colorFriendly");
        }

        rpc.pending_notifications.Clear();
    }

    /**
     *  Surface an incoming invitation. The menu may well be closed, so this goes to chat along with
     *  the key that opens it - resolved live rather than hard-coded, since the player may have
     *  rebound it.
     */
    private void AnnounceInvite(VigridPartyRPC rpc)
    {
        if (rpc.invite_seq == m_LastInviteSeq)
            return;

        m_LastInviteSeq = rpc.invite_seq;

        if (!rpc.HasInvite())
            return;

        StringLocaliser prompt = new StringLocaliser("STR_PARTY_INVITE_PROMPT", rpc.invite_inviter_name);
        GetGame().Chat(prompt.Format(), "colorImportant");

        StringLocaliser hint = new StringLocaliser("STR_PARTY_TOAST_HINT", InputUtils.GetButtonNameFromInput(VIGRID_PARTY_INPUT_MENU, EInputDeviceType.MOUSE_AND_KEYBOARD));
        GetGame().Chat(hint.Format(), "colorImportant");
    }

    // ---------------------------------------------------------------- client -> server

    void RequestSync()
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_REQUEST_SYNC, NULL, true);
    }

    void RequestPlayerList()
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_REQUEST_PLAYERLIST, NULL, true);
    }

    void CreateParty()
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_CREATE, NULL, true);
    }

    void Invite(string uid)
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_INVITE, new Param1<string>(uid), true);
    }

    void RespondToInvite(bool accept)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (!rpc.HasInvite())
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_INVITE_RESPOND, new Param2<string, bool>(rpc.invite_id, accept), true);

        //--- Clear locally so the banner disappears immediately; the server confirms either way.
        rpc.ClearInvite();
        rpc.invite_seq = rpc.invite_seq + 1;
        m_LastInviteSeq = rpc.invite_seq;
    }

    void Kick(string uid)
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_KICK, new Param1<string>(uid), true);
    }

    void TransferLeader(string uid)
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_TRANSFER_LEADER, new Param1<string>(uid), true);
    }

    void Leave()
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_LEAVE, NULL, true);
    }

    void Disband()
    {
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_DISBAND, NULL, true);
    }
}
#endif
