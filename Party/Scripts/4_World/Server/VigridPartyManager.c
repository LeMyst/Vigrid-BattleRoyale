#ifdef SERVER
/**
 *  Vigrid Party - the server-side authority. Owns the party registry, the invite list, the
 *  persistence trigger and both RPC directions.
 *
 *  Two invariants everything else relies on:
 *
 *    1. m_MemberIndex (uid -> party_id) is kept in lockstep with m_Parties. Every mutation goes
 *       through IndexMember()/DropMemberIndex() so that "which party is this player in" is O(1) - the Carim code
 *       this replaces rebuilt every group and linear-searched it, twice per spawn selection.
 *    2. A uid appears in at most one party. Load-time pruning enforces it and the invite
 *       validation preserves it.
 *
 *  Security: every client -> server handler resolves the actor from the engine-supplied `sender`
 *  identity and ignores `target` entirely, which is client-chosen. Same convention as
 *  BattleRoyaleServer.PlayerReadyUp.
 */
class VigridPartyManager
{
    private static ref VigridPartyManager m_Instance;

    private ref array<ref VigridParty> m_Parties;
    private ref map<string, string> m_MemberIndex;
    private ref array<ref VigridPartyInvite> m_Invites;
    private ref map<string, int> m_LastInviteMs;
    private ref map<string, int> m_LastPlayerListMs;

    /**
     *  PlayerIdentity.GetId() (hashed) -> GetPlainId() (SteamID64).
     *
     *  MissionServer.PlayerDisconnected is handed identity.GetId(), and on the delayed-logout path
     *  (missionserver.c:275-290) the PlayerIdentity itself can be null by the time it fires. Party
     *  keys everything on GetPlainId(), so this table - filled while the player is definitely
     *  connected - is the only reliable way back.
     */
    private ref map<string, string> m_HashedToPlain;

    private VigridPartyData m_Settings;

    private bool m_FormationLocked;
    private bool m_Dirty;
    private int m_FlushDueMs;
    private int m_StatePushDueMs;
    private int m_SweepDueMs;
    private int m_IdCounter;
    private int m_RosterVersionCounter;

    void VigridPartyManager()
    {
        m_Parties = new array<ref VigridParty>();
        m_MemberIndex = new map<string, string>();
        m_Invites = new array<ref VigridPartyInvite>();
        m_LastInviteMs = new map<string, int>();
        m_LastPlayerListMs = new map<string, int>();
        m_HashedToPlain = new map<string, string>();

        m_FormationLocked = false;
        m_Dirty = false;
        m_FlushDueMs = 0;
        m_StatePushDueMs = 0;
        m_SweepDueMs = 0;
        m_IdCounter = 0;
        m_RosterVersionCounter = 0;

        m_Settings = VigridPartyConfig.GetConfig().GetSettings();

        //--- The registered name and the method name must match exactly: CF dispatches by calling
        //--- the method whose name equals the registered string.
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_CREATE, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_INVITE, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_INVITE_RESPOND, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_KICK, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_LEAVE, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_DISBAND, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_TRANSFER_LEADER, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_REQUEST_PLAYERLIST, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_REQUEST_SYNC, this);

        m_Parties = VigridPartyStore.Load(m_Settings);
        RebuildIndex();

        VigridPartyLog.Info("Manager ready with " + m_Parties.Count() + " parties");
    }

    static VigridPartyManager GetInstance()
    {
        return m_Instance;
    }

    static void CreateInstance()
    {
        if (m_Instance)
            return;

        m_Instance = new VigridPartyManager();
    }

    bool IsEnabled()
    {
        return m_Settings.enabled;
    }

    int GetMaxPartySize()
    {
        return m_Settings.max_party_size;
    }

    bool IsFormationLocked()
    {
        return m_FormationLocked;
    }

    // ---------------------------------------------------------------- registry

    private void RebuildIndex()
    {
        m_MemberIndex.Clear();

        int party_count = m_Parties.Count();
        for (int i = 0; i < party_count; i++)
        {
            VigridParty party = m_Parties.Get(i);
            if (!party)
                continue;

            party.roster_version = NextRosterVersion();

            int member_count = party.member_uids.Count();
            for (int j = 0; j < member_count; j++)
            {
                m_MemberIndex.Set(party.member_uids.Get(j), party.id);
            }
        }
    }

    private int NextRosterVersion()
    {
        m_RosterVersionCounter = m_RosterVersionCounter + 1;
        return m_RosterVersionCounter;
    }

    private string NewPartyId()
    {
        m_IdCounter = m_IdCounter + 1;
        return "p" + m_IdCounter.ToString() + "-" + Math.RandomInt(0x1000, 0xFFFF).ToString();
    }

    VigridParty GetPartyByUid(string uid)
    {
        if (uid == "")
            return null;
        if (!m_MemberIndex.Contains(uid))
            return null;

        return GetPartyById(m_MemberIndex.Get(uid));
    }

    VigridParty GetPartyById(string party_id)
    {
        int count = m_Parties.Count();
        for (int i = 0; i < count; i++)
        {
            VigridParty party = m_Parties.Get(i);
            if (party && party.id == party_id)
                return party;
        }

        return null;
    }

    /**
     *  Resolve a uid to a connected player. Returns null when they are offline - which is a normal
     *  state here, not an error: parties outlive sessions.
     */
    PlayerBase GetPlayerByUid(string uid)
    {
        if (uid == "")
            return null;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        int count = players.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = PlayerBase.Cast(players.Get(i));
            if (!candidate)
                continue;
            if (!candidate.GetIdentity())
                continue;
            if (candidate.GetIdentity().GetPlainId() == uid)
                return candidate;
        }

        return null;
    }

    static string UidOf(PlayerBase player)
    {
        if (!player)
            return "";
        if (!player.GetIdentity())
            return "";

        return player.GetIdentity().GetPlainId();
    }

    static string NameOf(PlayerBase player)
    {
        if (!player)
            return "";
        if (!player.GetIdentity())
            return "";

        return player.GetIdentity().GetPlainName();
    }

    //--- Not named Link()/Unlink(): `Link` is a global template class in the vanilla script
    //--- library (1_core/proto/proto.c), and an unqualified call from inside this class resolves
    //--- to its one-argument constructor rather than to a member.
    private void IndexMember(VigridParty party, string uid)
    {
        m_MemberIndex.Set(uid, party.id);
    }

    private void DropMemberIndex(string uid)
    {
        if (m_MemberIndex.Contains(uid))
            m_MemberIndex.Remove(uid);
    }

    // ---------------------------------------------------------------- queries used by the API

    bool AreTeammates(string uid_a, string uid_b)
    {
        if (uid_a == "")
            return false;
        if (uid_b == "")
            return false;
        if (uid_a == uid_b)
            return false;
        if (!m_MemberIndex.Contains(uid_a))
            return false;
        if (!m_MemberIndex.Contains(uid_b))
            return false;

        return m_MemberIndex.Get(uid_a) == m_MemberIndex.Get(uid_b);
    }

    int GetMemberIndexOf(string uid)
    {
        VigridParty party = GetPartyByUid(uid);
        if (!party)
            return -1;

        return party.IndexOf(uid);
    }

    string GetPartyIdOf(string uid)
    {
        if (!m_MemberIndex.Contains(uid))
            return "";

        return m_MemberIndex.Get(uid);
    }

    /**
     *  Partition `population` into groups. Every player appears in exactly one group and solo
     *  players form a group of one, so GetGroups().Count() is always the group count - there is no
     *  second code path that could disagree with it.
     */
    array<ref array<PlayerBase>> BuildGroups(array<PlayerBase> population)
    {
        array<ref array<PlayerBase>> groups = new array<ref array<PlayerBase>>();
        if (!population)
            return groups;

        //--- party_id -> index into `groups`, so members are appended to a group already started.
        map<string, int> group_of_party = new map<string, int>();

        int count = population.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase player = population.Get(i);
            if (!player)
                continue;

            string uid = UidOf(player);
            string party_id = "";
            if (uid != "" && m_MemberIndex.Contains(uid))
                party_id = m_MemberIndex.Get(uid);

            if (party_id == "")
            {
                ref array<PlayerBase> solo = new array<PlayerBase>();
                solo.Insert(player);
                groups.Insert(solo);
                continue;
            }

            if (group_of_party.Contains(party_id))
            {
                groups.Get(group_of_party.Get(party_id)).Insert(player);
                continue;
            }

            ref array<PlayerBase> group = new array<PlayerBase>();
            group.Insert(player);
            group_of_party.Set(party_id, groups.Count());
            groups.Insert(group);
        }

        return groups;
    }

    array<PlayerBase> GetTeammatesIn(PlayerBase player, array<PlayerBase> population)
    {
        array<PlayerBase> mates = new array<PlayerBase>();
        if (!player)
            return mates;
        if (!population)
            return mates;

        string uid = UidOf(player);
        if (uid == "")
            return mates;
        if (!m_MemberIndex.Contains(uid))
            return mates;

        string party_id = m_MemberIndex.Get(uid);

        int count = population.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase other = population.Get(i);
            if (!other)
                continue;
            if (other == player)
                continue;

            string other_uid = UidOf(other);
            if (other_uid == "")
                continue;
            if (!m_MemberIndex.Contains(other_uid))
                continue;
            if (m_MemberIndex.Get(other_uid) != party_id)
                continue;

            mates.Insert(other);
        }

        return mates;
    }

    /**
     *  The leader of `player`'s party, if they are present in `population`.
     *
     *  Null for a solo player, and null when the leader is offline or otherwise absent from the
     *  population - the caller decides what to do about that rather than being handed a substitute.
     */
    PlayerBase GetLeaderIn(PlayerBase player, array<PlayerBase> population)
    {
        if (!player)
            return NULL;
        if (!population)
            return NULL;

        string uid = UidOf(player);
        if (uid == "")
            return NULL;

        VigridParty party = GetPartyByUid(uid);
        if (!party)
            return NULL;
        if (party.leader_uid == "")
            return NULL;

        int count = population.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase candidate = population.Get(i);
            if (!candidate)
                continue;
            if (UidOf(candidate) == party.leader_uid)
                return candidate;
        }

        return NULL;
    }

    // ---------------------------------------------------------------- mutations

    void SetFormationLocked(bool locked)
    {
        if (m_FormationLocked == locked)
            return;

        m_FormationLocked = locked;
        VigridPartyLog.Info("Formation locked = " + locked);

        //--- Locking invalidates every pending invite: accepting one mid-match would create a
        //--- group the match state machine has already counted.
        if (locked)
            CancelAllInvites();

        BroadcastLocked();
    }

    private void MarkDirty(bool flush_now)
    {
        m_Dirty = true;

        if (!flush_now)
            return;

        Flush();
    }

    private void Flush()
    {
        if (!m_Dirty)
            return;

        VigridPartyStore.Save(m_Parties);
        m_Dirty = false;
        m_FlushDueMs = VigridPartyTime.NowMs() + VIGRID_PARTY_FLUSH_DEBOUNCE_MS;
    }

    private VigridParty CreatePartyFor(string uid)
    {
        ref VigridParty party = new VigridParty();
        party.id = NewPartyId();
        party.created_at = VigridPartyTime.NowHours();
        party.last_seen_hours = party.created_at;
        party.Add(uid);
        party.roster_version = NextRosterVersion();

        m_Parties.Insert(party);
        IndexMember(party, uid);

        VigridPartyLog.Debug("Created party " + party.Repr());
        return party;
    }

    private void DeleteParty(VigridParty party)
    {
        int members = party.member_uids.Count();
        for (int i = 0; i < members; i++)
        {
            DropMemberIndex(party.member_uids.Get(i));
        }

        int index = m_Parties.Find(party);
        if (index != -1)
            m_Parties.Remove(index);

        VigridPartyLog.Debug("Deleted party " + party.id);
    }

    /**
     *  Remove one member and settle the consequences: promote a new leader when the leader left,
     *  and dissolve a party that has decayed to a single member. Sends the updated roster to
     *  whoever is left and an empty roster to the departing player.
     */
    private void RemoveMemberInternal(VigridParty party, string uid, string notify_key)
    {
        if (!party.Remove(uid))
            return;

        DropMemberIndex(uid);
        party.roster_version = NextRosterVersion();

        PlayerBase leaver = GetPlayerByUid(uid);
        if (leaver && leaver.GetIdentity())
            SendEmptyRoster(leaver.GetIdentity());

        if (party.Count() < 2)
        {
            //--- A one-member party is just a player. Tell the survivor and drop it.
            if (party.Count() == 1)
            {
                PlayerBase last = GetPlayerByUid(party.member_uids.Get(0));
                if (last && last.GetIdentity())
                {
                    SendEmptyRoster(last.GetIdentity());
                    SendNotify(last.GetIdentity(), "STR_PARTY_DISBANDED", "", "");
                }
            }

            DeleteParty(party);
            MarkDirty(true);
            return;
        }

        if (notify_key != "")
            NotifyParty(party, notify_key, NameOfUid(uid), "");

        BroadcastRoster(party);
        MarkDirty(true);
    }

    /**
     *  Best-effort display name for a uid that may be offline. Falls back to the uid so a message
     *  is never rendered with an empty name.
     */
    private string NameOfUid(string uid)
    {
        PlayerBase player = GetPlayerByUid(uid);
        string name = NameOf(player);
        if (name != "")
            return name;

        return uid;
    }

    // ---------------------------------------------------------------- invites

    private void CancelAllInvites()
    {
        int count = m_Invites.Count();
        for (int i = 0; i < count; i++)
        {
            VigridPartyInvite invite = m_Invites.Get(i);
            if (!invite)
                continue;

            PlayerBase invitee = GetPlayerByUid(invite.invitee_uid);
            if (invitee && invitee.GetIdentity())
                SendInviteCancelled(invitee.GetIdentity(), invite.invite_id);
        }

        m_Invites.Clear();
    }

    private VigridPartyInvite FindInvite(string invite_id)
    {
        int count = m_Invites.Count();
        for (int i = 0; i < count; i++)
        {
            VigridPartyInvite invite = m_Invites.Get(i);
            if (invite && invite.invite_id == invite_id)
                return invite;
        }

        return null;
    }

    private void DropInvite(VigridPartyInvite invite)
    {
        int index = m_Invites.Find(invite);
        if (index != -1)
            m_Invites.Remove(index);
    }

    private int CountInvitesFor(string invitee_uid)
    {
        int total = 0;
        int count = m_Invites.Count();
        for (int i = 0; i < count; i++)
        {
            VigridPartyInvite invite = m_Invites.Get(i);
            if (invite && invite.invitee_uid == invitee_uid)
                total = total + 1;
        }

        return total;
    }

    private bool HasInviteBetween(string inviter_uid, string invitee_uid)
    {
        int count = m_Invites.Count();
        for (int i = 0; i < count; i++)
        {
            VigridPartyInvite invite = m_Invites.Get(i);
            if (!invite)
                continue;
            if (invite.inviter_uid != inviter_uid)
                continue;
            if (invite.invitee_uid != invitee_uid)
                continue;

            return true;
        }

        return false;
    }

    private void SweepInvites()
    {
        int now_ms = VigridPartyTime.NowMs();

        //--- Backwards so removal does not shift an index we have not visited yet.
        for (int i = m_Invites.Count() - 1; i >= 0; i--)
        {
            VigridPartyInvite invite = m_Invites.Get(i);
            if (!invite)
            {
                m_Invites.Remove(i);
                continue;
            }
            if (!invite.IsExpired(now_ms))
                continue;

            PlayerBase invitee = GetPlayerByUid(invite.invitee_uid);
            if (invitee && invitee.GetIdentity())
                SendInviteCancelled(invitee.GetIdentity(), invite.invite_id);

            PlayerBase inviter = GetPlayerByUid(invite.inviter_uid);
            if (inviter && inviter.GetIdentity())
                SendNotify(inviter.GetIdentity(), "STR_PARTY_INVITE_EXPIRED", NameOfUid(invite.invitee_uid), "");

            m_Invites.Remove(i);
        }
    }

    // ---------------------------------------------------------------- server -> client

    void SendSettings(PlayerIdentity identity)
    {
        if (!identity)
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_SETTINGS,
            new Param5<bool, int, int, float, float>(
                m_Settings.enabled,
                m_Settings.max_party_size,
                m_Settings.invite_ttl_seconds,
                m_Settings.nametag_max_distance,
                m_Settings.nametag_min_alpha),
            true, identity);

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_LOCKED,
            new Param1<bool>(m_FormationLocked), true, identity);
    }

    private void BroadcastLocked()
    {
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        int count = players.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (!player)
                continue;
            if (!player.GetIdentity())
                continue;

            GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_LOCKED,
                new Param1<bool>(m_FormationLocked), true, player.GetIdentity());
        }
    }

    /**
     *  Send the roster to every online member. self_index is resolved per recipient so the client
     *  never has to work out which slot is its own.
     */
    void BroadcastRoster(VigridParty party)
    {
        if (!party)
            return;

        array<string> uids = new array<string>();
        array<string> names = new array<string>();

        int member_count = party.member_uids.Count();
        for (int i = 0; i < member_count; i++)
        {
            string uid = party.member_uids.Get(i);
            uids.Insert(uid);
            names.Insert(NameOfUid(uid));
        }

        int leader_index = party.IndexOf(party.leader_uid);

        for (int j = 0; j < member_count; j++)
        {
            PlayerBase member = GetPlayerByUid(party.member_uids.Get(j));
            if (!member)
                continue;
            if (!member.GetIdentity())
                continue;

            GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_ROSTER,
                new Param7<int, string, int, int, array<string>, array<string>, bool>(
                    party.roster_version, party.id, j, leader_index, uids, names, m_FormationLocked),
                true, member.GetIdentity());
        }
    }

    void SendRosterTo(VigridParty party, PlayerIdentity identity, string uid)
    {
        if (!party)
            return;
        if (!identity)
            return;

        array<string> uids = new array<string>();
        array<string> names = new array<string>();

        int member_count = party.member_uids.Count();
        for (int i = 0; i < member_count; i++)
        {
            string member_uid = party.member_uids.Get(i);
            uids.Insert(member_uid);
            names.Insert(NameOfUid(member_uid));
        }

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_ROSTER,
            new Param7<int, string, int, int, array<string>, array<string>, bool>(
                party.roster_version, party.id, party.IndexOf(uid), party.IndexOf(party.leader_uid), uids, names, m_FormationLocked),
            true, identity);
    }

    /**
     *  An empty roster is how "you are no longer in a party" is expressed - version 0 and an empty
     *  uid list, which the client treats as "hide everything".
     */
    void SendEmptyRoster(PlayerIdentity identity)
    {
        if (!identity)
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_ROSTER,
            new Param7<int, string, int, int, array<string>, array<string>, bool>(
                0, "", -1, -1, new array<string>(), new array<string>(), m_FormationLocked),
            true, identity);
    }

    void SendNotify(PlayerIdentity identity, string key, string arg1, string arg2)
    {
        if (!identity)
            return;

        //--- The bare stringtable key travels, without a '#'. The client localises it, which is
        //--- the same split the Battle Royale notification RPC uses.
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_NOTIFY,
            new Param3<string, string, string>(key, arg1, arg2), true, identity);
    }

    private void NotifyParty(VigridParty party, string key, string arg1, string arg2)
    {
        int count = party.member_uids.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase member = GetPlayerByUid(party.member_uids.Get(i));
            if (!member)
                continue;
            if (!member.GetIdentity())
                continue;

            SendNotify(member.GetIdentity(), key, arg1, arg2);
        }
    }

    private void SendInviteCancelled(PlayerIdentity identity, string invite_id)
    {
        if (!identity)
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_INVITE_CANCELLED,
            new Param1<string>(invite_id), true, identity);
    }

    /**
     *  Push position and health for every member of every party with at least two members.
     *
     *  This channel exists because a client only receives entities inside its network bubble: a
     *  teammate 800 m away has no entity client-side, so neither their position nor their health
     *  can be read locally. Sent unguaranteed - a dropped packet costs one interval of staleness,
     *  which is cheaper than retransmission.
     */
    private void PushTeamState()
    {
        int party_count = m_Parties.Count();
        for (int i = 0; i < party_count; i++)
        {
            VigridParty party = m_Parties.Get(i);
            if (!party)
                continue;
            if (party.Count() < 2)
                continue; //!< nothing to show a solo player

            array<vector> positions = new array<vector>();
            array<int> health = new array<int>();
            array<int> blood = new array<int>();
            array<int> flags = new array<int>();

            int member_count = party.member_uids.Count();
            int online = 0;

            for (int j = 0; j < member_count; j++)
            {
                PlayerBase member = GetPlayerByUid(party.member_uids.Get(j));

                if (!member)
                {
                    positions.Insert("0 0 0");
                    health.Insert(0);
                    blood.Insert(0);
                    flags.Insert(0);
                    continue;
                }

                online = online + 1;

                int member_flags = VIGRID_PARTY_FLAG_ONLINE;
                if (member.IsAlive())
                    member_flags = member_flags | VIGRID_PARTY_FLAG_ALIVE;
                if (member.IsUnconscious())
                    member_flags = member_flags | VIGRID_PARTY_FLAG_UNCONSCIOUS;

                //--- GetHealth01 is already normalised to 0..1, which sidesteps having to guard the
                //--- max value against zero.
                positions.Insert(member.GetPosition());
                health.Insert(Percent(member.GetHealth01("", "Health")));
                blood.Insert(Percent(member.GetHealth01("", "Blood")));
                flags.Insert(member_flags);
            }

            if (online == 0)
                continue;

            for (int k = 0; k < member_count; k++)
            {
                PlayerBase recipient = GetPlayerByUid(party.member_uids.Get(k));
                if (!recipient)
                    continue;
                if (!recipient.GetIdentity())
                    continue;

                GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_TEAMSTATE,
                    new Param5<int, array<vector>, array<int>, array<int>, array<int>>(
                        party.roster_version, positions, health, blood, flags),
                    false, recipient.GetIdentity());
            }
        }
    }

    //! Normalised 0..1 reading to a clamped 0..100 integer, so the whole channel stays int-sized.
    private int Percent(float normalised)
    {
        int percent = Math.Round(normalised * 100);
        if (percent < 0)
            return 0;
        if (percent > 100)
            return 100;

        return percent;
    }

    // ---------------------------------------------------------------- tick

    void Update()
    {
        int now_ms = VigridPartyTime.NowMs();

        if (now_ms >= m_SweepDueMs)
        {
            m_SweepDueMs = now_ms + 1000;
            SweepInvites();
        }

        if (now_ms >= m_StatePushDueMs)
        {
            m_StatePushDueMs = now_ms + m_Settings.state_push_interval_ms;
            PushTeamState();
        }

        if (m_Dirty && now_ms >= m_FlushDueMs)
            Flush();
    }

    // ---------------------------------------------------------------- session hooks

    void OnPlayerConnected(PlayerIdentity identity)
    {
        if (!identity)
            return;

        SendSettings(identity);

        string uid = identity.GetPlainId();
        m_HashedToPlain.Set(identity.GetId(), uid);
        VigridParty party = GetPartyByUid(uid);

        if (!party)
        {
            SendEmptyRoster(identity);
            return;
        }

        //--- Touching last_seen keeps an actively used party from being pruned by the TTL.
        party.last_seen_hours = VigridPartyTime.NowHours();
        MarkDirty(false);

        //--- Names come from live identities, so a reconnect refreshes them for everyone.
        BroadcastRoster(party);
    }

    /**
     *  `hashed_uid` is what MissionServer.PlayerDisconnected receives (identity.GetId()), and
     *  `identity` may legitimately be null on the delayed-logout path - so resolve the SteamID64
     *  from whichever of the two is actually available.
     */
    void OnPlayerDisconnected(PlayerIdentity identity, string hashed_uid)
    {
        string uid = "";

        if (identity)
            uid = identity.GetPlainId();
        else if (m_HashedToPlain.Contains(hashed_uid))
            uid = m_HashedToPlain.Get(hashed_uid);

        if (hashed_uid != "" && m_HashedToPlain.Contains(hashed_uid))
            m_HashedToPlain.Remove(hashed_uid);

        if (uid == "")
        {
            VigridPartyLog.Debug("Disconnect for an unresolvable uid, nothing to do");
            return;
        }

        //--- Drop any invite this player was part of; neither side can act on it now.
        for (int i = m_Invites.Count() - 1; i >= 0; i--)
        {
            VigridPartyInvite invite = m_Invites.Get(i);
            if (!invite)
                continue;
            if (invite.inviter_uid != uid && invite.invitee_uid != uid)
                continue;

            PlayerBase invitee = GetPlayerByUid(invite.invitee_uid);
            if (invitee && invitee.GetIdentity())
                SendInviteCancelled(invitee.GetIdentity(), invite.invite_id);

            m_Invites.Remove(i);
        }

        VigridParty party = GetPartyByUid(uid);
        if (!party)
            return;

        //--- Membership deliberately survives a disconnect: the party is meant to outlive the
        //--- session. Only leadership moves, and only when configured to.
        if (!m_Settings.leader_transfer_on_disconnect)
            return;
        if (!party.IsLeader(uid))
            return;

        string new_leader = FirstOnlineOther(party, uid);
        if (new_leader == "")
            return;

        party.SetLeader(new_leader);
        party.roster_version = NextRosterVersion();
        NotifyParty(party, "STR_PARTY_NEW_LEADER", NameOfUid(new_leader), "");
        BroadcastRoster(party);
        MarkDirty(true);
    }

    private string FirstOnlineOther(VigridParty party, string exclude_uid)
    {
        int count = party.member_uids.Count();
        for (int i = 0; i < count; i++)
        {
            string uid = party.member_uids.Get(i);
            if (uid == exclude_uid)
                continue;

            PlayerBase player = GetPlayerByUid(uid);
            if (!player)
                continue;
            if (!player.GetIdentity())
                continue;

            return uid;
        }

        return "";
    }

    // ---------------------------------------------------------------- client -> server handlers
    //
    // `target` is deliberately ignored in every handler below: it is chosen by the client and
    // could name any other player. The actor is always resolved from `sender`.

    private bool RejectIfUnavailable(PlayerIdentity sender)
    {
        if (!m_Settings.enabled)
        {
            SendNotify(sender, "STR_PARTY_DISABLED", "", "");
            return true;
        }

        if (m_FormationLocked)
        {
            SendNotify(sender, "STR_PARTY_LOCKED", "", "");
            return true;
        }

        return false;
    }

    void VP_Create(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (RejectIfUnavailable(sender))
            return;

        string uid = sender.GetPlainId();
        if (GetPartyByUid(uid))
        {
            SendNotify(sender, "STR_PARTY_ALREADY_IN_PARTY", "", "");
            return;
        }

        VigridParty party = CreatePartyFor(uid);
        SendRosterTo(party, sender, uid);

        //--- Not persisted yet on purpose: a party of one is dropped by the store anyway. It
        //--- becomes durable the moment a second member accepts.
        VigridPartyLog.Debug("VP_Create by " + uid);
    }

    void VP_Invite(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (RejectIfUnavailable(sender))
            return;

        string inviter_uid = sender.GetPlainId();
        string invitee_uid = data.param1;

        if (invitee_uid == "")
            return;
        if (invitee_uid == inviter_uid)
            return;

        int now_ms = VigridPartyTime.NowMs();
        if (m_LastInviteMs.Contains(inviter_uid) && (now_ms - m_LastInviteMs.Get(inviter_uid)) < VIGRID_PARTY_INVITE_COOLDOWN_MS)
        {
            SendNotify(sender, "STR_PARTY_INVITE_COOLDOWN", "", "");
            return;
        }

        PlayerBase invitee = GetPlayerByUid(invitee_uid);
        if (!invitee)
        {
            SendNotify(sender, "STR_PARTY_PLAYER_OFFLINE", invitee_uid, "");
            return;
        }

        if (GetPartyByUid(invitee_uid))
        {
            SendNotify(sender, "STR_PARTY_TARGET_IN_PARTY", NameOf(invitee), "");
            return;
        }

        //--- A player with no party may invite directly; the party is created for them here so the
        //--- common case is one click rather than "create, then invite".
        VigridParty party = GetPartyByUid(inviter_uid);
        if (!party)
            party = CreatePartyFor(inviter_uid);

        if (!party.IsLeader(inviter_uid))
        {
            SendNotify(sender, "STR_PARTY_NOT_LEADER", "", "");
            return;
        }

        if (party.Count() >= m_Settings.max_party_size)
        {
            SendNotify(sender, "STR_PARTY_FULL", "", "");
            return;
        }

        if (HasInviteBetween(inviter_uid, invitee_uid))
            return; //!< already pending, silently ignore the double click

        if (CountInvitesFor(invitee_uid) >= VIGRID_PARTY_MAX_PENDING_INVITES)
            return; //!< protects the invitee from being spammed by a crowd

        m_LastInviteMs.Set(inviter_uid, now_ms);

        string invite_id = "i" + now_ms.ToString() + "-" + Math.RandomInt(0x1000, 0xFFFF).ToString();
        ref VigridPartyInvite invite = new VigridPartyInvite(
            invite_id, inviter_uid, invitee_uid, party.id,
            now_ms + m_Settings.invite_ttl_seconds * 1000);

        m_Invites.Insert(invite);

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_INVITE_RECEIVED,
            new Param4<string, string, string, int>(
                invite_id, inviter_uid, NameOfUid(inviter_uid), m_Settings.invite_ttl_seconds),
            true, invitee.GetIdentity());

        SendNotify(sender, "STR_PARTY_INVITE_SENT", NameOf(invitee), "");
    }

    void VP_InviteRespond(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param2<string, bool> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        string responder_uid = sender.GetPlainId();
        VigridPartyInvite invite = FindInvite(data.param1);

        if (!invite)
            return;

        //--- The invite must name the sender as its invitee: without this check a client could
        //--- accept an invitation addressed to somebody else.
        if (invite.invitee_uid != responder_uid)
        {
            VigridPartyLog.Warn("Rejected VP_InviteRespond from " + responder_uid + " for an invite addressed to " + invite.invitee_uid);
            return;
        }

        DropInvite(invite);

        if (!data.param2)
        {
            PlayerBase decliner_target = GetPlayerByUid(invite.inviter_uid);
            if (decliner_target && decliner_target.GetIdentity())
                SendNotify(decliner_target.GetIdentity(), "STR_PARTY_INVITE_DECLINED", NameOfUid(responder_uid), "");
            return;
        }

        if (RejectIfUnavailable(sender))
            return;

        //--- Everything is re-validated at accept time: the party may have filled, been disbanded,
        //--- or the invitee may have joined another one while the invite was in flight.
        VigridParty party = GetPartyById(invite.party_id);
        if (!party)
        {
            SendNotify(sender, "STR_PARTY_DISBANDED", "", "");
            return;
        }

        if (GetPartyByUid(responder_uid))
        {
            SendNotify(sender, "STR_PARTY_ALREADY_IN_PARTY", "", "");
            return;
        }

        if (party.Count() >= m_Settings.max_party_size)
        {
            SendNotify(sender, "STR_PARTY_FULL", "", "");
            return;
        }

        if (!party.Add(responder_uid))
            return;

        IndexMember(party, responder_uid);
        party.roster_version = NextRosterVersion();
        party.last_seen_hours = VigridPartyTime.NowHours();

        NotifyParty(party, "STR_PARTY_JOINED", NameOfUid(responder_uid), "");
        BroadcastRoster(party);

        //--- Flush immediately: this is the mutation a crash must not lose.
        MarkDirty(true);
    }

    void VP_Kick(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (RejectIfUnavailable(sender))
            return;

        string leader_uid = sender.GetPlainId();
        string victim_uid = data.param1;

        if (victim_uid == "")
            return;
        if (victim_uid == leader_uid)
            return; //!< use Leave, not Kick

        VigridParty party = GetPartyByUid(leader_uid);
        if (!party)
            return;
        if (!party.IsLeader(leader_uid))
        {
            SendNotify(sender, "STR_PARTY_NOT_LEADER", "", "");
            return;
        }
        if (!party.Contains(victim_uid))
            return;

        PlayerBase victim = GetPlayerByUid(victim_uid);
        if (victim && victim.GetIdentity())
            SendNotify(victim.GetIdentity(), "STR_PARTY_YOU_KICKED", "", "");

        RemoveMemberInternal(party, victim_uid, "STR_PARTY_KICKED");
    }

    void VP_Leave(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (RejectIfUnavailable(sender))
            return;

        string uid = sender.GetPlainId();
        VigridParty party = GetPartyByUid(uid);
        if (!party)
            return;

        RemoveMemberInternal(party, uid, "STR_PARTY_LEFT");
    }

    void VP_Disband(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (RejectIfUnavailable(sender))
            return;

        string uid = sender.GetPlainId();
        VigridParty party = GetPartyByUid(uid);
        if (!party)
            return;
        if (!party.IsLeader(uid))
        {
            SendNotify(sender, "STR_PARTY_NOT_LEADER", "", "");
            return;
        }

        int member_count = party.member_uids.Count();
        for (int i = 0; i < member_count; i++)
        {
            PlayerBase member = GetPlayerByUid(party.member_uids.Get(i));
            if (!member)
                continue;
            if (!member.GetIdentity())
                continue;

            SendEmptyRoster(member.GetIdentity());
            SendNotify(member.GetIdentity(), "STR_PARTY_DISBANDED", "", "");
        }

        DeleteParty(party);
        MarkDirty(true);
    }

    void VP_TransferLeader(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<string> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (RejectIfUnavailable(sender))
            return;

        string leader_uid = sender.GetPlainId();
        string target_uid = data.param1;

        if (target_uid == "")
            return;
        if (target_uid == leader_uid)
            return;

        VigridParty party = GetPartyByUid(leader_uid);
        if (!party)
            return;
        if (!party.IsLeader(leader_uid))
        {
            SendNotify(sender, "STR_PARTY_NOT_LEADER", "", "");
            return;
        }
        if (!party.Contains(target_uid))
            return;

        PlayerBase new_leader = GetPlayerByUid(target_uid);
        if (!new_leader)
        {
            SendNotify(sender, "STR_PARTY_PLAYER_OFFLINE", NameOfUid(target_uid), "");
            return;
        }

        party.SetLeader(target_uid);
        party.roster_version = NextRosterVersion();

        NotifyParty(party, "STR_PARTY_NEW_LEADER", NameOf(new_leader), "");
        BroadcastRoster(party);
        MarkDirty(true);
    }

    void VP_RequestPlayerList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        string requester_uid = sender.GetPlainId();
        int now_ms = VigridPartyTime.NowMs();

        if (m_LastPlayerListMs.Contains(requester_uid) && (now_ms - m_LastPlayerListMs.Get(requester_uid)) < VIGRID_PARTY_PLAYERLIST_COOLDOWN_MS)
            return;

        m_LastPlayerListMs.Set(requester_uid, now_ms);

        array<string> uids = new array<string>();
        array<string> names = new array<string>();
        array<int> flags = new array<int>();

        array<PlayerIdentity> identities = new array<PlayerIdentity>();
        GetGame().GetPlayerIndentities(identities);

        int count = identities.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerIdentity identity = identities.Get(i);
            if (!identity)
                continue;

            string uid = identity.GetPlainId();
            if (uid == requester_uid)
                continue;

            uids.Insert(uid);
            names.Insert(identity.GetPlainName());

            //--- bit0: already in a party, so the client can grey the invite button rather than
            //--- letting the player fire an invite the server will only reject.
            int entry_flags = 0;
            if (m_MemberIndex.Contains(uid))
                entry_flags = entry_flags | 1;

            flags.Insert(entry_flags);
        }

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PLAYERLIST,
            new Param3<array<string>, array<string>, array<int>>(uids, names, flags), true, sender);
    }

    void VP_RequestSync(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        SendSettings(sender);

        string uid = sender.GetPlainId();
        VigridParty party = GetPartyByUid(uid);

        if (!party)
        {
            SendEmptyRoster(sender);
            return;
        }

        SendRosterTo(party, sender, uid);
    }
}
#endif
