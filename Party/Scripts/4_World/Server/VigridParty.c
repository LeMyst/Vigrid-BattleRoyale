#ifdef SERVER
/**
 *  One party. Members are identified by PlayerIdentity.GetPlainId() (SteamID64) throughout the
 *  addon - never GetPlayerId(), which is a session index the engine reuses after a disconnect and
 *  would silently reassign a party to a different person.
 *
 *  member_uids is ordered by join time and that order is the member's stable slot: it drives the
 *  spawn-marker colour and the roster indices the client renders against, so never reorder it for
 *  cosmetic reasons.
 */
class VigridParty
{
    string id;
    string leader_uid;
    ref array<string> member_uids;
    int created_at;      //!< hours since epoch, UTC
    int last_seen_hours; //!< refreshed whenever a member connects; drives TTL pruning

    /**
     *  Stamped from a process-wide monotonic counter every time membership or leadership changes,
     *  so a value is never reused - not even across two different parties.
     *
     *  This is what makes roster changes atomic without a handshake: VP_TeamState carries the
     *  version its parallel arrays were built against, and a client whose roster version differs
     *  discards the message instead of indexing into a roster that no longer matches.
     */
    int roster_version;

    /**
     *  World markers placed by members, in placement order - which is what makes the per-owner
     *  eviction below FIFO.
     *
     *  Hung off the party rather than kept in a second manager-side registry: there is no extra
     *  index to hold in lockstep with m_Parties, and deleting a party takes its markers with it.
     *  Session-scoped, so the store DTO does not carry the field - the runtime type is explicitly
     *  allowed to have members the file format does not (VigridPartyStoreDTO.c:5-6).
     */
    ref array<ref VigridPartyPing> pings;

    void VigridParty()
    {
        member_uids = new array<string>();
        pings = new array<ref VigridPartyPing>();
    }

    bool Contains(string uid)
    {
        return member_uids.Find(uid) != -1;
    }

    bool IsLeader(string uid)
    {
        return leader_uid == uid;
    }

    int IndexOf(string uid)
    {
        return member_uids.Find(uid);
    }

    int Count()
    {
        return member_uids.Count();
    }

    bool Add(string uid)
    {
        if (uid == "")
            return false;
        if (Contains(uid))
            return false;

        member_uids.Insert(uid);

        //--- First member in becomes the leader, so a party is never leaderless.
        if (leader_uid == "")
            leader_uid = uid;

        return true;
    }

    /**
     *  Remove a member. When the leader leaves, the longest-tenured remaining member is promoted
     *  immediately rather than leaving the party headless - member_uids is join-ordered, so
     *  element 0 is exactly that. Returns false when the uid was not a member.
     */
    bool Remove(string uid)
    {
        int index = member_uids.Find(uid);
        if (index == -1)
            return false;

        member_uids.Remove(index);

        if (leader_uid != uid)
            return true;

        leader_uid = "";
        if (member_uids.Count() > 0)
            leader_uid = member_uids.Get(0);

        return true;
    }

    bool SetLeader(string uid)
    {
        if (!Contains(uid))
            return false;

        leader_uid = uid;
        return true;
    }

    // ---------------------------------------------------------------- pings

    /**
     *  Append a marker, evicting this owner's oldest first once they are at the cap.
     *
     *  Evicting rather than refusing the placement is deliberate: a marker quietly disappearing
     *  explains itself, whereas a refusal needs a message the player then has to read mid-fight.
     *  Only the owner's own markers are ever evicted, so one player cannot push out a teammate's.
     */
    void AddPing(VigridPartyPing ping, int max_per_owner)
    {
        if (!ping)
            return;

        while (CountPingsOf(ping.owner_uid) >= max_per_owner)
        {
            int oldest = FirstPingIndexOf(ping.owner_uid);
            if (oldest == -1)
                break;

            //--- RemoveOrdered, never Remove: vanilla's Remove() fills the hole with the *last*
            //--- element ("do not retain order", EnScript.c), which silently destroys the placement
            //--- order this whole scheme is built on - the next eviction would then drop an
            //--- arbitrary marker instead of the oldest, and the #n labels would scramble with it.
            pings.RemoveOrdered(oldest);
        }

        pings.Insert(ping);
    }

    int CountPingsOf(string uid)
    {
        int total = 0;
        int count = pings.Count();
        for (int i = 0; i < count; i++)
        {
            VigridPartyPing ping = pings.Get(i);
            if (ping && ping.owner_uid == uid)
                total = total + 1;
        }

        return total;
    }

    //! Drop every marker owned by `uid`. Returns true when something was actually removed.
    bool RemovePingsOf(string uid)
    {
        bool removed = false;

        //--- Backwards so a removal does not shift an index not yet visited.
        for (int i = pings.Count() - 1; i >= 0; i--)
        {
            VigridPartyPing ping = pings.Get(i);
            if (ping && ping.owner_uid != uid)
                continue;

            //--- Ordered for the same reason as AddPing: the surviving markers keep their relative
            //--- placement order, so a teammate's #1/#2 do not swap when somebody else clears.
            pings.RemoveOrdered(i);
            removed = true;
        }

        return removed;
    }

    private int FirstPingIndexOf(string uid)
    {
        int count = pings.Count();
        for (int i = 0; i < count; i++)
        {
            VigridPartyPing ping = pings.Get(i);
            if (ping && ping.owner_uid == uid)
                return i;
        }

        return -1;
    }

    string Repr()
    {
        //--- Not named `out`: that is a reserved parameter-direction keyword in EnfusionScript
        //--- and using it as an identifier fails to parse.
        string text = id + "[leader=" + leader_uid + "]{";
        int count = member_uids.Count();
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
                text = text + ",";
            text = text + member_uids.Get(i);
        }
        return text + "}";
    }
}
#endif
