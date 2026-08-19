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
    private ref map<string, int> m_LastPingMs;

    /**
     *  PlayerIdentity.GetId() (hashed) -> GetPlainId() (SteamID64).
     *
     *  MissionServer.PlayerDisconnected is handed identity.GetId(), and on the delayed-logout path
     *  (missionserver.c:275-290) the PlayerIdentity itself can be null by the time it fires. Party
     *  keys everything on GetPlainId(), so this table - filled while the player is definitely
     *  connected - is the only reliable way back.
     */
    private ref map<string, string> m_HashedToPlain;

    /**
     *  GetPlainId() (SteamID64) -> the last display name seen for it, persisted alongside the
     *  parties.
     *
     *  A party outlives both the session and the process - a Battle Royale server restarts between
     *  every match - but a name can only be read off a live PlayerIdentity. Without this, every
     *  member who is not currently connected rendered as their raw 17-digit SteamID64 on every
     *  client, which is what the whole cache exists to stop.
     */
    private ref map<string, string> m_NameByUid;

    /**
     *  Members who are connected but must not be shown to their party, by GetPlainId().
     *
     *  Set by the host mod through VigridPartyAPI.SetMemberHidden. Party has no idea why anyone is
     *  on this list and deliberately does not ask - the case it was added for is a host whose player
     *  is temporarily somewhere that is not their real position, but "do not broadcast this member's
     *  state" is a complete description of the request either way.
     *
     *  SESSION-SCOPED and never persisted: parties survive a process restart, this does not, and a
     *  hidden flag surviving one would leave a member invisible with nothing left to clear it.
     */
    private ref array<string> m_HiddenUids;

    /**
     *  GetPlainId() (SteamID64) -> the connected player's entity, rebuilt at most once per
     *  millisecond. See GetPlayerByUid, which is the only reader and the only writer.
     *
     *  m_PlayerByUidMs is the millisecond the index was built on; -1 means "nothing indexed yet",
     *  which GetGame().GetTime() never returns, and is also what InvalidatePlayerIndex sets.
     */
    private ref map<string, PlayerBase> m_PlayerByUid;
    private int m_PlayerByUidMs;

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
        m_LastPingMs = new map<string, int>();
        m_HashedToPlain = new map<string, string>();
        m_NameByUid = new map<string, string>();
        m_HiddenUids = new array<string>();
        m_PlayerByUid = new map<string, PlayerBase>();

        m_PlayerByUidMs = -1;
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
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_PING_ADD, this);
        GetRPCManager().AddRPC(RPC_VIGRIDPARTY_SERVER_NAMESPACE, VP_RPC_PING_CLEAR, this);

        m_Parties = VigridPartyStore.Load(m_Settings, m_NameByUid);
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
     *
     *  MEMOIZED PER MILLISECOND, because the callers ask it in bursts and there are twenty of them.
     *  This was a fresh array<Man> allocation plus a full walk of the population per call, and the
     *  recurring paths each ask it once PER MEMBER: PushTeamState twice per member of every party on
     *  a 500 ms timer, BroadcastRoster twice per member (once here, once through NameOfUid) on every
     *  composition change, BroadcastPings and SweepInvites once per member at 1 Hz. So the cost was
     *  O(members x population) per push, and each walk materialises a fresh string from GetPlainId()
     *  for every candidate it rejects. AutoGroupPopulation is the one loop that already dodged this,
     *  by hand - see the comment above its own single pass.
     *
     *  Invalidation is the millisecond ALONE, plus the two session hooks. The client-side twin
     *  (VigridPartyAPI.FindLocalPlayer) also keys on the population count, but that term is free
     *  there and is not available here: nothing in the engine reports a server population count, and
     *  both GetPlayers and GetPlayerIndentities fill an out array - reading the count means doing the
     *  walk this exists to remove. InvalidatePlayerIndex covers what the count was a proxy for, and
     *  covers it exactly rather than by inference.
     *
     *  Entries are still null-checked on the way out: an entity deleted after the index was built
     *  reads as null through its reference, exactly as it did mid-walk before.
     */
    PlayerBase GetPlayerByUid(string uid)
    {
        if (uid == "")
            return null;

        int now_ms = VigridPartyTime.NowMs();

        if (now_ms != m_PlayerByUidMs || !m_PlayerByUid)
            RebuildPlayerIndex(now_ms);

        PlayerBase found = m_PlayerByUid.Get(uid);
        if (!found)
            return null;

        return found;
    }

    //! One walk of the population, keyed by uid. The only place the scan still happens.
    private void RebuildPlayerIndex(int now_ms)
    {
        if (!m_PlayerByUid)
            m_PlayerByUid = new map<string, PlayerBase>();
        else
            m_PlayerByUid.Clear();

        m_PlayerByUidMs = now_ms;

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

            //--- Read out before the call that consumes it, per the container-aliasing rule.
            string candidate_uid = candidate.GetIdentity().GetPlainId();

            //--- FIRST WINS, because the linear scan this replaces returned on its first match. Two
            //--- entities sharing a uid is not something we expect - but a corpse is never deleted
            //--- by the host mod and stays in the population, so it is not obviously impossible
            //--- either, and "same answer as before" is worth one Contains() per entry.
            if (m_PlayerByUid.Contains(candidate_uid))
                continue;

            m_PlayerByUid.Set(candidate_uid, candidate);
        }

        //--- Gated rather than handed straight to Trace(): the argument would be concatenated on
        //--- every rebuild regardless of level. This line is the acceptance measurement - run the
        //--- server at -party-trace and count it against the lookups a push issues.
        if (VigridPartyLog.CheckLogLevel(VigridPartyLog.TRACE))
            VigridPartyLog.Trace("Player index rebuilt at " + now_ms + " ms, " + m_PlayerByUid.Count() + " online of " + count);
    }

    /**
     *  Force the next GetPlayerByUid to walk the population again.
     *
     *  Called from both session hooks, and NOT optional - each has a caller that would otherwise
     *  read an index built earlier in the same millisecond and get a different answer than the scan
     *  would have given:
     *
     *    - OnPlayerConnected broadcasts a roster that resolves THE JOINER THEMSELVES. Update() can
     *      have run PushTeamState earlier in the same millisecond, before that entity existed, so
     *      the joiner would resolve null and miss their own roster push.
     *    - OnPlayerDisconnected runs FirstOnlineOther, which promotes a new leader. A stale entry
     *      there would hand the party to somebody who has already left.
     */
    private void InvalidatePlayerIndex()
    {
        m_PlayerByUidMs = -1;
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

        return NameOfIdentity(player.GetIdentity());
    }

    /**
     *  The display name for a connected player.
     *
     *  Vanilla's own server-side cache (playerbase.c:221) is seeded from the identity, so this
     *  normally answers exactly as GetPlainName() would. A host mod is free to overwrite it with a
     *  better name - which is how a player who never set one in the launcher stops being "Survivor"
     *  here - and Party picks that up without knowing anything about the mod that did it.
     *
     *  Takes an identity, not a PlayerBase, because two callers only have one: the invite browser's
     *  online list walks GetPlayerIndentities(), and OnConnect records a name before it has looked
     *  the player up. Both used to read GetPlainName() directly and so showed the launcher name
     *  while the roster beside them showed the corrected one.
     */
    static string NameOfIdentity(PlayerIdentity identity)
    {
        if (!identity)
            return "";

        PlayerBase player = PlayerBase.Cast(identity.GetPlayer());
        if (player && player.GetCachedName() != "")
            return player.GetCachedName();

        return identity.GetPlainName();
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
                array<PlayerBase> solo = new array<PlayerBase>();
                solo.Insert(player);
                groups.Insert(solo);
                continue;
            }

            if (group_of_party.Contains(party_id))
            {
                groups.Get(group_of_party.Get(party_id)).Insert(player);
                continue;
            }

            array<PlayerBase> group = new array<PlayerBase>();
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

        VigridPartyStore.Save(m_Parties, m_NameByUid);
        m_Dirty = false;
        m_FlushDueMs = VigridPartyTime.NowMs() + VIGRID_PARTY_FLUSH_DEBOUNCE_MS;
    }

    private VigridParty CreatePartyFor(string uid)
    {
        VigridParty party = new VigridParty();
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

        //--- Their markers leave with them: a marker outliving its owner's membership would keep
        //--- pointing somewhere on behalf of somebody no longer on the team.
        party.RemovePingsOf(uid);

        PlayerBase leaver = GetPlayerByUid(uid);
        if (leaver && leaver.GetIdentity())
        {
            SendEmptyRoster(leaver.GetIdentity());
            SendEmptyPings(leaver.GetIdentity());
        }

        if (party.Count() < 2)
        {
            //--- A one-member party is just a player. Tell the survivor and drop it.
            if (party.Count() == 1)
            {
                PlayerBase last = GetPlayerByUid(party.member_uids.Get(0));
                if (last && last.GetIdentity())
                {
                    SendEmptyRoster(last.GetIdentity());
                    SendEmptyPings(last.GetIdentity());
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
        BroadcastPings(party);
        MarkDirty(true);
    }

    // ---------------------------------------------------------------- auto grouping

    /**
     *  Fill `population` into parties of at least `min_size`. Backs VigridPartyAPI.AutoGroup, which
     *  is where the contract is written down.
     *
     *  Deliberately NOT gated on m_FormationLocked. That flag is a client-request gate - only
     *  RejectIfUnavailable consults it, and only the RPC handlers call that - so a host asking for
     *  this is not a player sneaking a composition change past the lock. The host is expected to
     *  call it just BEFORE locking anyway, so that nobody is told their party is frozen and then
     *  watches it change.
     *
     *  Returns the resulting group count, or -1 when it did nothing at all.
     */
    int AutoGroupPopulation(array<PlayerBase> population, int min_size, int min_groups, int remainder)
    {
        if (!m_Settings.enabled)
            return -1;
        if (min_size <= 1)
            return -1;
        if (!population)
            return -1;
        if (population.Count() == 0)
            return -1;

        //--- One pass over the population for everything below. GetPlayerByUid and NameOfUid each
        //--- walk GetGame().GetPlayers(), so reaching for them per member inside the loops that
        //--- follow would make this quadratic in a full lobby.
        array<PlayerBase> pool = new array<PlayerBase>();
        array<VigridParty> existing = new array<VigridParty>();
        array<int> existing_sizes = new array<int>();
        map<string, string> names = new map<string, string>();

        int population_count = population.Count();
        for (int i = 0; i < population_count; i++)
        {
            PlayerBase player = population.Get(i);
            string uid = UidOf(player);
            if (uid == "")
                continue;

            names.Set(uid, NameOf(player));

            VigridParty party = GetPartyByUid(uid);
            if (!party)
            {
                pool.Insert(player);
                continue;
            }

            //--- A party is sized by how many of its members are actually HERE, not by Count().
            //--- One whose third player never connected is a duo in this match, and topping that
            //--- duo up is precisely the job.
            int known = existing.Find(party);
            if (known == -1)
            {
                existing.Insert(party);
                existing_sizes.Insert(1);
                continue;
            }

            existing_sizes.Set(known, existing_sizes.Get(known) + 1);
        }

        //--- Fisher-Yates. This shuffle IS the "random leader" rule: VigridParty.Add makes the first
        //--- member in the leader and the plan fills each new party in pool order, so randomising
        //--- the pool randomises both who ends up with whom and who leads them.
        for (int s = pool.Count() - 1; s > 0; s--)
        {
            int j = Math.RandomInt(0, s + 1);
            PlayerBase swap = pool.Get(s);
            pool.Set(s, pool.Get(j));
            pool.Set(j, swap);
        }

        VigridPartyAutoGroupPlan plan = VigridPartyAutoGroup.Plan(pool.Count(), existing_sizes, min_size, m_Settings.max_party_size, min_groups, remainder);

        string summary = "AutoGroup min_size=" + min_size + " floor=" + min_groups;
        summary = summary + " pool=" + pool.Count() + " -> " + plan.Repr();
        VigridPartyLog.Info(summary);

        if (plan.overflow_count > 0)
            VigridPartyLog.Warn("AutoGroup placed " + plan.overflow_count + " player(s) past max_party_size " + m_Settings.max_party_size);

        if (plan.floor_hit)
            VigridPartyLog.Warn("AutoGroup stopped short of min_size to keep at least " + min_groups + " groups standing");

        if (!plan.ChangedAnything())
            return plan.groups_after;

        //--- Slot -> party. The leading slots are parties that already exist; the rest are created
        //--- lazily by the first member the plan assigns to them.
        array<VigridParty> slot_party = new array<VigridParty>();
        int slot_count = plan.slot_size.Count();
        for (int a = 0; a < slot_count; a++)
        {
            if (a < plan.existing_count)
                slot_party.Insert(existing.Get(a));
            else
                slot_party.Insert(NULL);
        }

        array<VigridParty> touched = new array<VigridParty>();

        int pool_count = pool.Count();
        for (int b = 0; b < pool_count; b++)
        {
            int slot = plan.slot_of_pool.Get(b);
            if (slot == -1)
                continue;

            string member_uid = UidOf(pool.Get(b));
            if (member_uid == "")
                continue;

            VigridParty target = slot_party.Get(slot);
            if (!target)
            {
                target = CreatePartyFor(member_uid);
                slot_party.Set(slot, target);
            }
            else
            {
                //--- The add contract, identical to the invite-accept path: Add THEN IndexMember,
                //--- or m_MemberIndex falls out of lockstep with m_Parties and every grouping query
                //--- silently answers from the stale one.
                if (!target.Add(member_uid))
                    continue;

                IndexMember(target, member_uid);
            }

            //--- Flags them for VigridPartyStore.Save to subtract, which is what stops a randomly
            //--- assigned teammate surviving into the next match.
            target.MarkAuto(member_uid);

            if (touched.Find(target) == -1)
                touched.Insert(target);
        }

        int touched_count = touched.Count();
        for (int c = 0; c < touched_count; c++)
        {
            VigridParty done = touched.Get(c);

            //--- One version bump per party, after its last add rather than per add. A client
            //--- discards a VP_TeamState whose version does not match the roster it holds, so
            //--- publishing the intermediate versions would only make it drop state it is about to
            //--- be sent again.
            done.roster_version = NextRosterVersion();
            done.last_seen_hours = VigridPartyTime.NowHours();

            BroadcastRoster(done);
            NotifyAutoGrouped(done, names);
        }

        //--- No MarkDirty on purpose. Nothing an auto-group does is persistable - Save() subtracts
        //--- every uid marked above - so a flush here would only rewrite the file with what it
        //--- already contains.

        return plan.groups_after;
    }

    /**
     *  Tell each member of a freshly grouped party who they are now playing with.
     *
     *  Per recipient rather than one NotifyParty broadcast, because the useful message names the
     *  OTHER members and that list differs for every one of them.
     */
    private void NotifyAutoGrouped(VigridParty party, map<string, string> names)
    {
        int count = party.member_uids.Count();
        for (int i = 0; i < count; i++)
        {
            PlayerBase member = GetPlayerByUid(party.member_uids.Get(i));
            if (!member)
                continue;
            if (!member.GetIdentity())
                continue;

            //--- Accumulated one name per statement. A single expression carrying this many
            //--- concatenated terms is rejected outright with "Formula too complex", and it is a
            //--- compile error the PBO packs straight past.
            string others = "";
            for (int j = 0; j < count; j++)
            {
                if (j == i)
                    continue;

                string other_uid = party.member_uids.Get(j);

                //--- The map covers everyone in the population; NameOfUid is the fallback for a
                //--- member of a topped-up party who is not in this match.
                string other_name = NameOfUid(other_uid);
                if (names.Contains(other_uid))
                    other_name = names.Get(other_uid);

                if (other_name == "")
                    continue;

                if (others != "")
                    others = others + ", ";

                others = others + other_name;
            }

            if (others == "")
                continue;

            SendNotify(member.GetIdentity(), "STR_PARTY_AUTO_GROUPED", others, "");
        }
    }

    /**
     *  Record a display name against a uid, so it survives the player's disconnect and the process.
     *
     *  Returns early when nothing changed: this runs from NameOfUid, which every roster broadcast
     *  calls once per member, and dirtying the store on each of those would defeat the write
     *  debounce entirely. MarkDirty(false) for the same reason - the flush is never synchronous, a
     *  name is not worth a disk write of its own.
     */
    private void RememberName(string uid, string name)
    {
        if (uid == "")
            return;
        if (name == "")
            return;
        if (m_NameByUid.Contains(uid) && m_NameByUid.Get(uid) == name)
            return;

        m_NameByUid.Set(uid, name);
        MarkDirty(false);
    }

    /**
     *  Display name for a uid that may be offline, in three steps: live identity, then the
     *  remembered name, then a stringtable key the client resolves.
     *
     *  The live branch also refreshes the cache, which is what picks up a Steam name change - and
     *  it is why every path that needs a name goes through here rather than reading NameOf()
     *  directly. The uid itself is never returned: a raw SteamID64 in the party menu is the bug
     *  this exists to fix.
     */
    private string NameOfUid(string uid)
    {
        PlayerBase player = GetPlayerByUid(uid);
        string name = NameOf(player);
        if (name != "")
        {
            RememberName(uid, name);
            return name;
        }

        if (m_NameByUid.Contains(uid))
            return m_NameByUid.Get(uid);

        return VIGRID_PARTY_UNKNOWN_NAME_KEY;
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

        //--- A separate message rather than two more members on VP_Settings. If the arities of that
        //--- Param5 ever disagreed between client and server, ctx.Read would fail *silently* and
        //--- every setting it carries would sit at its default with nothing in the log to say so;
        //--- widening it puts working settings inside that blast radius for no gain. VP_Locked was
        //--- split out for the same reason.
        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PING_SETTINGS,
            new Param2<bool, int>(m_Settings.ping_enabled, m_Settings.ping_cooldown_ms), true, identity);
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
     *  Re-push every party's roster because a display name changed underneath us.
     *
     *  Names are baked into the roster message at broadcast time, and every BroadcastRoster() call
     *  site is a *composition* change - join, leave, kick, new leader. A name that changes while the
     *  party's shape does not therefore never reaches the client, and the HUD row and name plate keep
     *  rendering whatever the player was called when they joined. Nothing inside Party can notice
     *  that, since the name comes from outside it, so this is the host mod's to call.
     *
     *  Cheap and rare by construction: a name resolves at most once per player per process.
     */
    void RefreshRosterNames()
    {
        for (int i = 0; i < m_Parties.Count(); i++)
        {
            VigridParty party = m_Parties.Get(i);
            if (!party)
                continue;

            BroadcastRoster(party);
        }
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

    /**
     *  Push the party's whole ping set to every online member.
     *
     *  The entire set travels rather than a delta. That makes the message idempotent, so a client
     *  that missed one is corrected by the next and no periodic resend is needed at all - Carim, in
     *  which the client owned the list, had to heartbeat it every 60 s.
     *
     *  Entries name their own owner, unlike VP_TeamState's roster-indexed arrays, so a roster change
     *  cannot mis-index them and there is no version guard to keep in step. Do not add one: it would
     *  put a ping rebroadcast next to every BroadcastRoster() call site.
     */
    void BroadcastPings(VigridParty party)
    {
        if (!party)
            return;

        array<string> owners = new array<string>();
        array<vector> positions = new array<vector>();
        array<int> remaining = new array<int>();
        BuildPingArrays(party, owners, positions, remaining);

        int member_count = party.member_uids.Count();
        for (int i = 0; i < member_count; i++)
        {
            PlayerBase member = GetPlayerByUid(party.member_uids.Get(i));
            if (!member)
                continue;
            if (!member.GetIdentity())
                continue;

            GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PING_SET,
                new Param3<array<string>, array<vector>, array<int>>(owners, positions, remaining),
                true, member.GetIdentity());
        }
    }

    void SendPingsTo(VigridParty party, PlayerIdentity identity)
    {
        if (!party)
            return;
        if (!identity)
            return;

        array<string> owners = new array<string>();
        array<vector> positions = new array<vector>();
        array<int> remaining = new array<int>();
        BuildPingArrays(party, owners, positions, remaining);

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PING_SET,
            new Param3<array<string>, array<vector>, array<int>>(owners, positions, remaining),
            true, identity);
    }

    //! An empty set is how "you have no markers any more" is expressed, mirroring SendEmptyRoster.
    void SendEmptyPings(PlayerIdentity identity)
    {
        if (!identity)
            return;

        GetRPCManager().SendRPC(RPC_VIGRIDPARTY_NAMESPACE, VP_RPC_PING_SET,
            new Param3<array<string>, array<vector>, array<int>>(
                new array<string>(), new array<vector>(), new array<int>()),
            true, identity);
    }

    /**
     *  Fill the three parallel wire arrays, skipping anything already expired.
     *
     *  Milliseconds *remaining* travel, not an absolute expiry: this process's GetTime() and the
     *  client's are unrelated. VP_InviteReceived ships a TTL for exactly the same reason.
     */
    private void BuildPingArrays(VigridParty party, array<string> owners, array<vector> positions, array<int> remaining)
    {
        int now_ms = VigridPartyTime.NowMs();

        int count = party.pings.Count();
        for (int i = 0; i < count; i++)
        {
            VigridPartyPing ping = party.pings.Get(i);
            if (!ping)
                continue;
            if (ping.IsExpired(now_ms))
                continue;

            owners.Insert(ping.owner_uid);
            positions.Insert(ping.position);
            remaining.Insert(ping.RemainingMs(now_ms));
        }
    }

    /**
     *  Drop expired markers, and re-broadcast only the parties that actually changed - a party whose
     *  markers are all permanent generates no traffic here at all.
     */
    private void SweepPings()
    {
        int now_ms = VigridPartyTime.NowMs();

        int party_count = m_Parties.Count();
        for (int i = 0; i < party_count; i++)
        {
            VigridParty party = m_Parties.Get(i);
            if (!party)
                continue;

            bool changed = false;

            //--- Backwards so a removal does not shift an index not yet visited.
            for (int j = party.pings.Count() - 1; j >= 0; j--)
            {
                VigridPartyPing ping = party.pings.Get(j);
                if (ping && !ping.IsExpired(now_ms))
                    continue;

                //--- Ordered: Remove() would fill the hole with the last element and scramble the
                //--- placement order the FIFO cap depends on. See VigridParty.AddPing.
                party.pings.RemoveOrdered(j);
                changed = true;
            }

            if (changed)
                BroadcastPings(party);
        }
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
     *  Hide or show one member in the state their party receives. See m_HiddenUids.
     *
     *  Idempotent both ways, so a caller can assert the state it wants without tracking edges.
     */
    void SetMemberHidden(string uid, bool hidden)
    {
        if (uid == "")
            return;

        int index = m_HiddenUids.Find(uid);

        if (hidden)
        {
            if (index == -1)
            {
                m_HiddenUids.Insert(uid);
                VigridPartyLog.Debug("SetMemberHidden " + uid + " hidden");
            }

            return;
        }

        if (index != -1)
        {
            m_HiddenUids.Remove(index);
            VigridPartyLog.Debug("SetMemberHidden " + uid + " visible");
        }
    }

    bool IsMemberHidden(string uid)
    {
        int index = m_HiddenUids.Find(uid);

        return index != -1;
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
            array<int> health_levels = new array<int>();
            array<int> blood_levels = new array<int>();
            array<int> flags = new array<int>();

            int member_count = party.member_uids.Count();
            int online = 0;

            for (int j = 0; j < member_count; j++)
            {
                //--- Read out before the call that consumes it, per the container-aliasing rule.
                string member_uid = party.member_uids.Get(j);

                PlayerBase member = GetPlayerByUid(member_uid);

                /**
                 *  A HIDDEN MEMBER IS PRESENTED EXACTLY AS AN OFFLINE ONE, and that is the whole
                 *  implementation rather than a new flag on the wire.
                 *
                 *  Every consumer already routes through IsMemberVisible / IsMemberOnline, both of
                 *  which key off the ONLINE bit - so zeroing the flags suppresses the world nametag,
                 *  the compass caret and the map's team layer in one move, with no client change and
                 *  no new state for a renderer to get wrong. The position is zeroed too: leaving a
                 *  real one in an array the client is told to ignore would put the thing being
                 *  hidden on the wire anyway.
                 */
                bool member_hidden = IsMemberHidden(member_uid);

                if (!member || member_hidden)
                {
                    //--- Level 0 reads as GREAT, but flags are empty here so the client takes its
                    //--- offline branch and never looks at the level at all.
                    positions.Insert("0 0 0");
                    health_levels.Insert(0);
                    blood_levels.Insert(0);
                    flags.Insert(0);
                    continue;
                }

                online = online + 1;

                int member_flags = VIGRID_PARTY_FLAG_ONLINE;
                if (member.IsAlive())
                    member_flags = member_flags | VIGRID_PARTY_FLAG_ALIVE;
                if (member.IsUnconscious())
                    member_flags = member_flags | VIGRID_PARTY_FLAG_UNCONSCIOUS;

                //--- Send the stat level (EStatLevels, 0..4) rather than a percentage, and let
                //--- vanilla decide it. A percentage had to be quantised to an int before the
                //--- client could bucket it, which moved every threshold by up to half a percent,
                //--- and it forced the client to hardcode blood's cutoffs as a fraction of a 5000
                //--- maximum. These are the very calls the player's own HUD badge reads, so the
                //--- teammate icon now cannot disagree with what that player sees.
                positions.Insert(member.GetPosition());
                health_levels.Insert(member.GetStatLevelHealth());
                blood_levels.Insert(member.GetStatLevelBlood());
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
                        party.roster_version, positions, health_levels, blood_levels, flags),
                    false, recipient.GetIdentity());
            }
        }
    }

    // ---------------------------------------------------------------- tick

    void Update()
    {
        int now_ms = VigridPartyTime.NowMs();

        if (now_ms >= m_SweepDueMs)
        {
            m_SweepDueMs = now_ms + 1000;
            SweepInvites();
            SweepPings();
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
        //--- First, before anything below can memoize a population this player is missing from.
        //--- BroadcastRoster at the bottom resolves the joiner themselves.
        InvalidatePlayerIndex();

        if (!identity)
            return;

        SendSettings(identity);

        string uid = identity.GetPlainId();
        m_HashedToPlain.Set(identity.GetId(), uid);

        //--- Before the partyless early return below, deliberately: this is the only moment the
        //--- name is readable, and a player with no party today may be invited into one tomorrow -
        //--- possibly while they are offline, from a roster their teammates are already looking at.
        RememberName(uid, NameOfIdentity(identity));

        VigridParty party = GetPartyByUid(uid);

        if (!party)
        {
            SendEmptyRoster(identity);
            SendEmptyPings(identity);
            return;
        }

        //--- Touching last_seen keeps an actively used party from being pruned by the TTL.
        party.last_seen_hours = VigridPartyTime.NowHours();
        MarkDirty(false);

        //--- Names come from live identities, so a reconnect refreshes them for everyone.
        BroadcastRoster(party);

        //--- Markers survive a disconnect, exactly as membership does, so someone rejoining
        //--- mid-match is handed whatever the team has standing right now.
        SendPingsTo(party, identity);
    }

    /**
     *  `hashed_uid` is what MissionServer.PlayerDisconnected receives (identity.GetId()), and
     *  `identity` may legitimately be null on the delayed-logout path - so resolve the SteamID64
     *  from whichever of the two is actually available.
     */
    void OnPlayerDisconnected(PlayerIdentity identity, string hashed_uid)
    {
        //--- First, for the same reason as OnPlayerConnected: FirstOnlineOther below picks the new
        //--- leader, and a stale index would offer it the player who has just left.
        InvalidatePlayerIndex();

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
        VigridPartyInvite invite = new VigridPartyInvite(
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
            SendEmptyPings(member.GetIdentity());
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
            names.Insert(NameOfIdentity(identity));

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
            SendEmptyPings(sender);
            return;
        }

        SendRosterTo(party, sender, uid);
        SendPingsTo(party, sender);
    }

    /**
     *  Place a world marker where the sender says they are looking.
     *
     *  Deliberately NOT gated on RejectIfUnavailable. That helper also refuses while the formation
     *  is locked, and the lock is switched on for the entire match - which is the only time markers
     *  are of any use, so gating on it would ship a feature that never works. Only the two enable
     *  switches are checked, exactly as VP_RequestSync does.
     *
     *  A single position is all that travels; owner, name and both timestamps are minted here from
     *  `sender`. The one value a modified client gets to choose is therefore range-checked below,
     *  and that is the whole of its influence - unlike Carim, where the client uploaded the entire
     *  set including its own cap and labels.
     */
    void VP_PingAdd(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<vector> data;
        if (!ctx.Read(data))
            return;
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (!m_Settings.enabled)
            return;
        if (!m_Settings.ping_enabled)
            return;

        string uid = sender.GetPlainId();

        //--- Re-checked here even though the client checks it too: a modified client is not bound
        //--- by the engine's input handling.
        PlayerBase player = GetPlayerByUid(uid);
        if (!player)
            return;
        if (!player.IsAlive())
            return;
        if (player.IsUnconscious())
            return;

        int now_ms = VigridPartyTime.NowMs();
        if (m_LastPingMs.Contains(uid) && (now_ms - m_LastPingMs.Get(uid)) < m_Settings.ping_cooldown_ms)
        {
            //--- Dropped silently. A notification per rejected press would itself become the spam
            //--- channel the cooldown exists to close.
            VigridPartyLog.Trace("Ping from " + uid + " dropped by cooldown");
            return;
        }

        vector position = data.param1;
        if (position == vector.Zero)
            return;

        //--- Warn, never Error: Error routes to Error2() and raises a VM exception, which would
        //--- turn one malformed packet into a dead server.
        if (vector.Distance(player.GetPosition(), position) > VIGRID_PARTY_PING_MAX_PLACE_DIST)
        {
            VigridPartyLog.Warn("Rejected out-of-range ping from " + uid);
            return;
        }

        VigridParty party = GetPartyByUid(uid);
        if (!party)
            return;

        m_LastPingMs.Set(uid, now_ms);

        int expires_at_ms = 0;
        if (m_Settings.ping_ttl_seconds > 0)
            expires_at_ms = now_ms + m_Settings.ping_ttl_seconds * 1000;

        //--- Held in a ref local before being handed over, exactly as an invite is: AddPing evicts
        //--- before it inserts, so the new marker has to survive a few statements first.
        VigridPartyPing ping = new VigridPartyPing(uid, position, now_ms, expires_at_ms);
        party.AddPing(ping, m_Settings.ping_max_per_player);

        BroadcastPings(party);

        VigridPartyLog.Debug("Ping by " + uid + " at " + position.ToString() + ", party now holds " + party.pings.Count());
    }

    //! Clear every marker the sender owns. A member can only ever clear their own.
    void VP_PingClear(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;
        if (!m_Settings.enabled)
            return;
        if (!m_Settings.ping_enabled)
            return;

        string uid = sender.GetPlainId();
        VigridParty party = GetPartyByUid(uid);
        if (!party)
            return;
        if (!party.RemovePingsOf(uid))
            return;

        BroadcastPings(party);
        VigridPartyLog.Debug("Pings cleared by " + uid);
    }
}
#endif
