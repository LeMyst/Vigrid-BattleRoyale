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
    private ref VigridPartyPings m_Pings;

    private int m_HudDueMs;
    private int m_LastInviteSeq;
    private bool m_RequestedSync;
    private int m_PingCooldownDueMs;

    //--- Repeat suppression for Announce. See its header for why chat needed none of this.
    private string m_LastAnnounceKey;
    private int m_AnnounceRepeatDueMs;

#ifdef DIAG_DEVELOPER
    //! Last frame's latch state, so Update can spot the falling edge and re-sync.
    private bool m_WasFakeSession;
#endif

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

        VigridPartyLog.Debug("VigridPartyClient ctor: creating pings");
        m_Pings = new VigridPartyPings();

        m_HudDueMs = 0;
        m_LastInviteSeq = 0;
        m_RequestedSync = false;
        m_PingCooldownDueMs = 0;
        m_LastAnnounceKey = "";
        m_AnnounceRepeatDueMs = 0;

#ifdef DIAG_DEVELOPER
        m_WasFakeSession = false;
#endif

        VigridPartyLog.Debug("VigridPartyClient ctor: done");
    }

    void ~VigridPartyClient()
    {
        m_Hud = NULL;
        m_Nametags = NULL;
        m_Pings = NULL;
    }

    void Update(float timeslice)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

#ifdef DIAG_DEVELOPER
        //--- Re-sync the moment a fabricated session is cleared. Every push was being discarded
        //--- while the latch was down, including the roster, so without this the party stays empty
        //--- until the server next happens to broadcast one - which on a live server it only does
        //--- when the composition changes, i.e. possibly never.
        bool faking_now = IsFakeSession();
        if (m_WasFakeSession && !faking_now)
            m_RequestedSync = false;

        m_WasFakeSession = faking_now;
#endif

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

        //--- After the name tags on purpose. The two are independent workspace-level roots, so
        //--- SetSort inside one says nothing about the other and creation order is what decides
        //--- which layer wins: pings are the transient, deliberate signal and belong on top.
        m_Pings.Update();

        //--- The panel is fed by a 2 Hz push, so refreshing it faster than 5 Hz buys nothing.
        int now_ms = GetGame().GetTime();
        if (now_ms < m_HudDueMs)
            return;

        m_HudDueMs = now_ms + 200;
        m_Hud.Update();
    }

    /**
     *  Surface everything the server has queued for this player.
     *
     *  Drains the WHOLE queue in one pass, deliberately. NotificationSystem shows five at a time and
     *  defers the rest, releasing them LIFO at about one per second - so a burst of more than five
     *  would arrive reordered and late. Nothing this addon sends can produce such a burst: the
     *  server emits one message per event per recipient (SendNotify / NotifyParty), and the most
     *  that has ever landed in one frame is the two of a leader disconnecting - DISBANDED beside
     *  NEW_LEADER. Pacing the drain would buy nothing against that and would cost a due-time field,
     *  a partial-drain cursor and a second place for a message to be delayed. The one repeatable
     *  source in the addon is the client-local Announce path, rate-limited at the source instead.
     */
    private void DrainNotifications(VigridPartyRPC rpc)
    {
        int count = rpc.pending_notifications.Count();
        if (count == 0)
            return;

        for (int i = 0; i < count; i++)
        {
            //--- Read the element into a local before the call - the container-aliasing shape
            //--- recorded in CLAUDE.md, and this line is being touched anyway.
            string message = rpc.pending_notifications.Get(i);
            Notify(message, VIGRID_PARTY_NOTIFY_SECONDS);
        }

        rpc.pending_notifications.Clear();
    }

    /**
     *  Surface an incoming invitation. The menu may well be closed, so this draws over whatever is
     *  on screen, along with the key that opens it - resolved live rather than hard-coded, since
     *  the player may have rebound it.
     *
     *  ONE notification, not two: the prompt is the title and the keybind hint is the detail line,
     *  which is exactly the shape notification_element.layout is built for - a headline over a
     *  subordinate line. Three reasons it must not be two:
     *
     *    - Only five notifications are visible at once and the overflow is released LIFO, so of two
     *      it is the HINT that gets deferred. A player would be told they were invited and not told
     *      which key answers it - the one combination worse than either message alone.
     *    - Two elements carry two independent timers and fade apart; the prompt and the key that
     *      answers it have to disappear together.
     *    - It halves this addon's peak claim on those five slots.
     *
     *  The invitation is also the one message that does not wear the generic party title: the
     *  prompt itself is the headline, which is right for the message that must not be missed.
     */
    private void AnnounceInvite(VigridPartyRPC rpc)
    {
        if (rpc.invite_seq == m_LastInviteSeq)
            return;

        m_LastInviteSeq = rpc.invite_seq;

        if (!rpc.HasInvite())
            return;

        StringLocaliser prompt = new StringLocaliser("STR_PARTY_INVITE_PROMPT", rpc.invite_inviter_name);
        StringLocaliser hint = new StringLocaliser("STR_PARTY_TOAST_HINT", InputUtils.GetButtonNameFromInput(VIGRID_PARTY_INPUT_MENU, EInputDeviceType.MOUSE_AND_KEYBOARD));

        NotifyExtended(prompt.Format(), hint.Format(), VIGRID_PARTY_NOTIFY_INVITE_SECONDS);
    }

    // ---------------------------------------------------------------- client -> server

    /**
     *  DIAG ONLY - is the diag menu currently driving a fabricated party?
     *
     *  Every command below asks this first. The menu targets uids the server has never heard of
     *  while a fake party is up, so each one is applied to the fabrication here instead of going on
     *  the wire; that is what makes Invite, Kick, Promote, Leave, Disband and the invite banner
     *  reachable from a single client. Diverting them in this class rather than in VigridPartyMenu
     *  keeps "the single place client -> server commands are sent from" true, and leaves the menu
     *  itself completely unaware that any of this exists.
     *
     *  Compiles to a constant false in a release build, so nothing below changes shape.
     */
    private bool IsFakeSession()
    {
#ifdef DIAG_DEVELOPER
        if (VigridPartyAPI.IsDebugFakeSession())
            return true;
#endif

        return false;
    }

    void RequestSync()
    {
        //--- Nothing would come back that is not discarded, so the poll is simply not sent.
        if (IsFakeSession())
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_REQUEST_SYNC, NULL, true);
    }

    void RequestPlayerList()
    {
        if (IsFakeSession())
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_REQUEST_PLAYERLIST, NULL, true);
    }

    void CreateParty()
    {
#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugCreateParty();
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_CREATE, NULL, true);
    }

    void Invite(string uid)
    {
#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugInvite(uid);
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_INVITE, new Param1<string>(uid), true);
    }

    void RespondToInvite(bool accept)
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();
        if (!rpc.HasInvite())
            return;

#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            //--- DebugRespondToInvite clears the invite and bumps the sequence itself; the tracker
            //--- still has to follow, or AnnounceInvite re-announces a banner nobody can act on.
            VigridPartyAPI.DebugRespondToInvite(accept);
            m_LastInviteSeq = rpc.invite_seq;
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_INVITE_RESPOND, new Param2<string, bool>(rpc.invite_id, accept), true);

        //--- Clear locally so the banner disappears immediately; the server confirms either way.
        rpc.ClearInvite();
        rpc.invite_seq = rpc.invite_seq + 1;
        m_LastInviteSeq = rpc.invite_seq;
    }

    void Kick(string uid)
    {
#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugKick(uid);
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_KICK, new Param1<string>(uid), true);
    }

    void TransferLeader(string uid)
    {
#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugPromote(uid);
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_TRANSFER_LEADER, new Param1<string>(uid), true);
    }

    void Leave()
    {
#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugLeaveParty();
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_LEAVE, NULL, true);
    }

    void Disband()
    {
#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugDisbandParty();
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_DISBAND, NULL, true);
    }

    /**
     *  Place a world marker where the player is looking.
     *
     *  Every rejection is decided here, so a press that cannot succeed generates no traffic at all -
     *  including the cooldown, which is the difference between a held-down key costing nothing and
     *  it costing a packet per frame. The server re-checks all of it regardless: none of this binds
     *  a modified client.
     *
     *  The ray starts a metre ahead of the camera and reaches 8 km, matching Carim, which is far
     *  enough to mark a ridgeline across the map and near enough not to catch the player's own body.
     */
    void PlacePing()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        //--- Silent, unlike the ping switch below: with the whole manager off there is no party
        //--- system on this server at all, and telling the player that *pings* are disabled points
        //--- them at the wrong setting. It also costs one of NotificationSystem's five slots.
        if (!rpc.enabled)
            return;

        if (!rpc.ping_enabled)
        {
            Announce("STR_PARTY_PING_DISABLED");
            return;
        }

        if (!rpc.HasParty())
        {
            Announce("STR_PARTY_PING_NO_PARTY");
            return;
        }

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        //--- Silent, unlike the cases above: the player already knows they are dead.
        if (!player.IsAlive())
            return;
        if (player.IsUnconscious())
            return;

        int now_ms = GetGame().GetTime();
        if (now_ms < m_PingCooldownDueMs)
            return;

        vector begin = GetGame().GetCurrentCameraPosition() + GetGame().GetCurrentCameraDirection();
        vector end = begin + GetGame().GetCurrentCameraDirection() * VIGRID_PARTY_PING_RAY_LENGTH;

        vector contact_pos;
        vector contact_dir;
        int contact_component;

        if (!DayZPhysics.RaycastRV(begin, end, contact_pos, contact_dir, contact_component))
        {
            Announce("STR_PARTY_PING_NO_TARGET");
            return;
        }

        //--- vector.Zero is treated as a miss on both sides of the wire; the server refuses it too.
        if (contact_pos == vector.Zero)
        {
            Announce("STR_PARTY_PING_NO_TARGET");
            return;
        }

        m_PingCooldownDueMs = now_ms + rpc.ping_cooldown_ms;

#ifdef DIAG_DEVELOPER
        //--- A fabricated roster makes HasParty() true above, so without this the key would send a
        //--- real RPC whose answer the latch discards - a marker that never appears.
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugAddPing(contact_pos, 0);
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_PING_ADD, new Param1<vector>(contact_pos), true);
    }

    //! Clear every marker this player owns. Teammates' markers are untouched, here and server-side.
    void ClearPings()
    {
        VigridPartyRPC rpc = VigridPartyRPC.GetInstance();

        if (!rpc.enabled || !rpc.ping_enabled)
            return;
        if (!rpc.HasParty())
            return;

#ifdef DIAG_DEVELOPER
        if (IsFakeSession())
        {
            VigridPartyAPI.DebugClearPings();
            Announce("STR_PARTY_PING_CLEARED");
            return;
        }
#endif

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_PING_CLEAR, NULL, true);
        Announce("STR_PARTY_PING_CLEARED");
    }

    /**
     *  Say something about a ping locally.
     *
     *  Deliberately not routed through VP_Notify: every condition that produces one of these is
     *  detectable before the RPC is sent, so the server never has to say anything on this path.
     *
     *  This is the ONLY message in the addon a player can repeat at will, and moving off chat is
     *  what makes that matter. PlacePing answers PING_DISABLED and PING_NO_PARTY BEFORE it reaches
     *  its cooldown check and PING_NO_TARGET before the cooldown is set, and ClearPings has no
     *  cooldown at all - so a held or mashed key produces one message per press. A chat line costs
     *  nothing; a notification takes one of five slots, and the sixth onwards is deferred and
     *  released LIFO at about one per second, so the same mash would leave a stale, out-of-order
     *  tail crawling down the screen long after the player stopped pressing. Repeats of the same
     *  key inside the window are therefore dropped - keyed on the KEY rather than on time alone, so
     *  a different message still gets through immediately.
     */
    private void Announce(string key)
    {
        int now_ms = GetGame().GetTime();
        if (key == m_LastAnnounceKey && now_ms < m_AnnounceRepeatDueMs)
            return;

        m_LastAnnounceKey = key;
        m_AnnounceRepeatDueMs = now_ms + VIGRID_PARTY_NOTIFY_REPEAT_MS;

        StringLocaliser message = new StringLocaliser(key);
        Notify(message.Format(), VIGRID_PARTY_NOTIFY_SECONDS);
    }

    /**
     *  Put one line on screen.
     *
     *  NOT GetGame().Chat, and that is the whole of issue #275: chat is a channel a player can turn
     *  off, and this addon has things to say that have to be read. Vanilla's NotificationSystem
     *  draws top-centre over an open menu, fades itself out, and is declared in 3_Game as part of
     *  the base game - so this costs config.cpp nothing and its three requiredAddons stand.
     *
     *  AddNotificationExtended rather than AddNotification: the enum form needs a NotificationType
     *  that only a modded enum plus an entry in VANILLA'S OWN scripts/data/notifications.json can
     *  supply, and an unregistered type renders the literal string "please_add_a_title".
     *
     *  The title is a constant and the message goes in the detail line, because Detail is the
     *  widget carrying `wrap 1` - anything of variable length belongs there.
     */
    private void Notify(string message, float seconds)
    {
        NotifyExtended(VIGRID_PARTY_NOTIFY_TITLE, message, seconds);
    }

    /**
     *  Two-line form: a headline and a subordinate line that are halves of one event.
     *
     *  The guard is not decoration. AddNotificationExtended dereferences its static m_Instance with
     *  no check of its own, and a null there unwinds the stack - which here would take the rest of
     *  VigridPartyClient.Update with it, every frame, silently killing the name tags, the pings and
     *  the HUD panel. In practice it cannot be null: DayZGame.OnInitialize calls
     *  NotificationSystem.InitInstance() long before any mission exists, and only ~DayZGame clears
     *  it. If it ever is, one log line beats a dead HUD. The fallback must not be chat.
     */
    private void NotifyExtended(string title, string detail, float seconds)
    {
        if (!NotificationSystem.GetInstance())
        {
            VigridPartyLog.Warn("NotificationSystem unavailable; dropped notification: " + title);
            return;
        }

        NotificationSystem.AddNotificationExtended(seconds, title, detail, VIGRID_PARTY_NOTIFY_ICON);
    }
}
#endif
